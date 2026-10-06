#include "torus_geom.h"

/* sin of 0..90 deg in 64 steps, Q14, +1 guard entry */
static const int16_t QSIN[66] = {
    0, 402, 804, 1205, 1606, 2006, 2404, 2801, 3196, 3590, 3981,
    4370, 4756, 5139, 5520, 5897, 6270, 6639, 7005, 7366, 7723, 8076,
    8423, 8765, 9102, 9434, 9760, 10080, 10394, 10702, 11003, 11297, 11585,
    11866, 12140, 12406, 12665, 12916, 13160, 13395, 13623, 13842, 14053, 14256,
    14449, 14635, 14811, 14978, 15137, 15286, 15426, 15557, 15679, 15791, 15893,
    15986, 16069, 16143, 16207, 16261, 16305, 16340, 16364, 16379, 16384, 16384
};

int16_t torus_sin(uint16_t a) {
    unsigned q = a >> 14, r = a & 0x3FFF;
    if (q & 1) r = 0x4000 - r;
    unsigned idx = r >> 8, frac = r & 0xFF;
    int v = QSIN[idx] + (((QSIN[idx + 1] - QSIN[idx]) * (int)frac) >> 8);
    return (int16_t)((q & 2) ? -v : v);
}
int16_t torus_cos(uint16_t a) { return torus_sin((uint16_t)(a + 0x4000)); }

/* ---- static model: vertices, quad centres, normals (built once) ---- */
static bool g_ready = false;
static TorusVec g_vert[TORUS_U_SEGS][TORUS_V_SEGS];
static TorusVec g_qcenter[TORUS_QUADS];
static TorusVec g_qnormal[TORUS_QUADS];     /* Q14 */
static TorusVec g_ccenter[9];
static TorusVec g_cnormal[9];               /* Q14 */

static void surface(uint16_t u, uint16_t v, TorusVec *p, TorusVec *n) {
    int cu = torus_cos(u), su = torus_sin(u), cv = torus_cos(v), sv = torus_sin(v);
    int rad = TORUS_MAJOR_R + ((TORUS_MINOR_R * cv) >> 14);
    p->x = (int16_t)((rad * cu) >> 14); p->y = (int16_t)((rad * su) >> 14); p->z = (int16_t)((TORUS_MINOR_R * sv) >> 14);
    n->x = (int16_t)((cv * cu) >> 14); n->y = (int16_t)((cv * su) >> 14); n->z = (int16_t)sv;
}
static uint16_t ang(int num, int den) { return (uint16_t)(((uint32_t)num * 65536u) / (uint32_t)den); }

static void model_init(void) {
    if (g_ready) return;
    TorusVec dummy;
    for (int i = 0; i < TORUS_U_SEGS; i++) for (int j = 0; j < TORUS_V_SEGS; j++)
        surface(ang(i, TORUS_U_SEGS), ang(j, TORUS_V_SEGS), &g_vert[i][j], &dummy);
    for (int i = 0; i < TORUS_U_SEGS; i++) for (int j = 0; j < TORUS_V_SEGS; j++)
        surface(ang(2 * i + 1, 2 * TORUS_U_SEGS), ang(2 * j + 1, 2 * TORUS_V_SEGS),
                &g_qcenter[i * TORUS_V_SEGS + j], &g_qnormal[i * TORUS_V_SEGS + j]);
    for (int row = 0; row < 3; row++) for (int col = 0; col < 3; col++)
        surface(ang(2 * col + 1, 6), ang(2 * row + 1, 6), &g_ccenter[row * 3 + col], &g_cnormal[row * 3 + col]);
    g_ready = true;
}

/* ---- camera ---- */
TorusMat torus_camera_matrix(const TorusCamera *c) {
    int cs = torus_cos(c->spin), ss = torus_sin(c->spin);
    uint16_t pa = (uint16_t)c->pitch;
    int ca = torus_cos(pa), sa = torus_sin(pa);
    TorusMat M;
    M.m[0][0] = (int16_t)cs;               M.m[0][1] = (int16_t)(-ss);            M.m[0][2] = 0;
    M.m[1][0] = (int16_t)((ca * ss) >> 14); M.m[1][1] = (int16_t)((ca * cs) >> 14); M.m[1][2] = (int16_t)sa;
    M.m[2][0] = (int16_t)((-sa * ss) >> 14); M.m[2][1] = (int16_t)((-sa * cs) >> 14); M.m[2][2] = (int16_t)ca;
    return M;
}

static int deg(int d) { return (int)(((long)d * 65536) / 360); }

/* Row 0 (top/outer): near side from above.  Row 1 (inner wall): far side from above.
 * Row 2 (bottom/outer): near side from below. Near side = ring azimuth 270 deg. */
void torus_camera_target_for_cell(int cell, TorusCamera *t) {
    int row = cell / 3, col = cell % 3;
    uint16_t uc = (uint16_t)(ang(col, 3) + ang(1, 6));
    static const int pitch_deg[3] = { 50, 45, 130 };
    int az = (row == 1) ? 90 : 270;
    t->spin = (uint16_t)(deg(az) - uc);
    t->pitch = deg(pitch_deg[row]);
}

bool torus_camera_step(TorusCamera *cur, const TorusCamera *tgt) {
    int32_t ds = (int16_t)(uint16_t)(tgt->spin - cur->spin);
    int32_t dp = tgt->pitch - cur->pitch;
    const int32_t snap = 250;
    if (ds > -snap && ds < snap) cur->spin = tgt->spin; else cur->spin = (uint16_t)(cur->spin + ds / 3);
    if (dp > -snap && dp < snap) cur->pitch = tgt->pitch; else cur->pitch += dp / 3;
    return cur->spin != tgt->spin || cur->pitch != tgt->pitch;
}
void torus_camera_idle_step(TorusCamera *c) { c->spin = (uint16_t)(c->spin + TORUS_ROTATION_SPEED); }

/* ---- projection ---- */
void torus_projection_fit(TorusProjection *p, int x, int y, int w, int h) {
    int m = (w < h ? w : h);
    p->half = (int16_t)(m / 2 - 1);
    p->cx = (int16_t)(x + w / 2); p->cy = (int16_t)(y + h / 2);
}

static void rot(const TorusMat *M, const TorusVec *v, int *x, int *y, int *z) {
    *x = (M->m[0][0] * v->x + M->m[0][1] * v->y + M->m[0][2] * v->z) >> 14;
    *y = (M->m[1][0] * v->x + M->m[1][1] * v->y + M->m[1][2] * v->z) >> 14;
    *z = (M->m[2][0] * v->x + M->m[2][1] * v->y + M->m[2][2] * v->z) >> 14;
}
static TorusPt proj(const TorusProjection *P, int x, int y) {
    TorusPt r; r.x = (int16_t)(P->cx + (x * P->half) / TORUS_EXTENT); r.y = (int16_t)(P->cy - (y * P->half) / TORUS_EXTENT); return r;
}

/* Light from upper-left-front, view space, Q14 */
#define LX (-4915)
#define LY 8192
#define LZ 13271

void torus_build_frame(const TorusCamera *cam, const TorusProjection *P, TorusFrame *f) {
    model_init();
    TorusMat M = torus_camera_matrix(cam);
    int x, y, z;
    for (int i = 0; i < TORUS_U_SEGS; i++) for (int j = 0; j < TORUS_V_SEGS; j++) {
        rot(&M, &g_vert[i][j], &x, &y, &z); f->vert[i][j] = proj(P, x, y);
    }
    int16_t depth[TORUS_QUADS]; uint8_t order[TORUS_QUADS]; int n = 0;
    for (int i = 0; i < TORUS_U_SEGS; i++) for (int j = 0; j < TORUS_V_SEGS; j++) {
        int q = i * TORUS_V_SEGS + j, nx, ny, nz;
        rot(&M, &g_qnormal[q], &nx, &ny, &nz);
        if (nz <= 0) continue;                       /* back face: always hidden on a closed convex-ish surface */
        int dx, dy, dz; rot(&M, &g_qcenter[q], &dx, &dy, &dz);
        int dot = (nx * LX + ny * LY + nz * LZ) >> 14;
        if (dot < 0) dot = 0;
        TorusQuad *Q = &f->quads[n];
        Q->i = (uint8_t)i; Q->j = (uint8_t)j; Q->cell = (uint8_t)((j / TORUS_SEGS_PER_CELL) * 3 + (i / TORUS_SEGS_PER_CELL));
        Q->light = (uint8_t)(90 + ((dot * 165) >> 14));
        depth[n] = (int16_t)dz; order[n] = (uint8_t)n; n++;
    }
    /* painter's algorithm: insertion sort far -> near */
    for (int a = 1; a < n; a++) { uint8_t k = order[a]; int b = a - 1; while (b >= 0 && depth[order[b]] > depth[k]) { order[b + 1] = order[b]; b--; } order[b + 1] = k; }
    static TorusQuad tmp[TORUS_QUADS];
    for (int a = 0; a < n; a++) tmp[a] = f->quads[order[a]];
    for (int a = 0; a < n; a++) f->quads[a] = tmp[a];
    f->num_quads = n;
    for (int c = 0; c < 9; c++) {
        rot(&M, &g_ccenter[c], &x, &y, &z); f->cell_center[c] = proj(P, x, y);
        int nx, ny, nz; rot(&M, &g_cnormal[c], &nx, &ny, &nz); f->cell_facing[c] = (int16_t)nz;
    }
}
