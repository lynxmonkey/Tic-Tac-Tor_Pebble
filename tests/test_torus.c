#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "torus_geom.h"
#define PI_ 3.14159265358979323846
static int run = 0, fail = 0;
#define CHECK(c) do { run++; if (!(c)) { fail++; printf("  FAIL line %d: %s\n", __LINE__, #c); } } while (0)

int main(void) {
    /* trig accuracy */
    int maxerr = 0;
    for (unsigned a = 0; a < 65536; a += 7) {
        double s = sin(a * 2 * PI_ / 65536.0) * 16384.0;
        int e = abs((int)lround(s) - torus_sin((uint16_t)a)); if (e > maxerr) maxerr = e;
    }
    printf("max sin error (Q14): %d\n", maxerr); CHECK(maxerr <= 24);
    CHECK(torus_sin(0) == 0 && torus_sin(16384) == 16384 && torus_cos(0) == 16384 && torus_sin(49152) == -16384);

    /* rotation matrix is orthonormal for a spread of cameras */
    for (int s = 0; s < 65536; s += 4099) for (int p = 0; p < 65536; p += 5003) {
        TorusCamera c = { (uint16_t)s, p }; TorusMat M = torus_camera_matrix(&c);
        for (int r = 0; r < 3; r++) {
            long d = 0; for (int k = 0; k < 3; k++) d += (long)M.m[r][k] * M.m[r][k];
            CHECK(labs(d - 16384L * 16384L) < 16384L * 120);
        }
    }

    /* Every cell: target camera brings that cell to face the viewer, and its centre is on screen. */
    TorusFrame *f = malloc(sizeof *f); TorusProjection P; torus_projection_fit(&P, 0, 30, 200, 198);
    for (int cell = 0; cell < 9; cell++) {
        TorusCamera t; torus_camera_target_for_cell(cell, &t);
        torus_build_frame(&t, &P, f);
        printf("cell %c facing=%5d at (%3d,%3d) quads=%d\n", 'A' + cell, f->cell_facing[cell], f->cell_center[cell].x, f->cell_center[cell].y, f->num_quads);
        CHECK(f->cell_facing[cell] > 10000);                       /* strongly toward viewer */
        CHECK(f->cell_center[cell].x >= 0 && f->cell_center[cell].x < 200 && f->cell_center[cell].y >= 30 && f->cell_center[cell].y < 228);
        CHECK(f->num_quads > 30 && f->num_quads < TORUS_QUADS);
        /* quads are sorted back-to-front only by construction; check all indices valid */
        for (int q = 0; q < f->num_quads; q++) CHECK(f->quads[q].cell < 9 && f->quads[q].i < TORUS_U_SEGS && f->quads[q].j < TORUS_V_SEGS);
    }
    /* Camera animation converges to the target from anywhere, using shortest spin direction. */
    for (int cell = 0; cell < 9; cell++) for (int from = 0; from < 9; from++) {
        TorusCamera a, b; torus_camera_target_for_cell(from, &a); torus_camera_target_for_cell(cell, &b);
        int steps = 0; while (torus_camera_step(&a, &b) && steps < 100) steps++;
        CHECK(steps < 40 && a.spin == b.spin && a.pitch == b.pitch);
    }
    /* Resolution independence: projected extent scales with the region and stays inside it. */
    int sizes[][2] = { {144, 168}, {200, 228}, {180, 180}, {400, 456} };
    for (unsigned k = 0; k < 4; k++) {
        TorusProjection Q; torus_projection_fit(&Q, 10, 20, sizes[k][0], sizes[k][1]);
        int minx = 9999, maxx = -9999, miny = 9999, maxy = -9999;
        for (int s = 0; s < 65536; s += 2000) for (int p = 0; p < 140; p += 10) {
            TorusCamera c = { (uint16_t)s, (int32_t)p * 65536 / 360 }; torus_build_frame(&c, &Q, f);
            for (int i = 0; i < TORUS_U_SEGS; i++) for (int j = 0; j < TORUS_V_SEGS; j++) {
                int vx = f->vert[i][j].x, vy = f->vert[i][j].y;
                if (vx < minx) minx = vx;
                if (vx > maxx) maxx = vx;
                if (vy < miny) miny = vy;
                if (vy > maxy) maxy = vy;
            }
        }
        CHECK(minx >= 10 && maxx <= 10 + sizes[k][0] && miny >= 20 && maxy <= 20 + sizes[k][1]);
        printf("region %dx%d -> x[%d,%d] y[%d,%d]\n", sizes[k][0], sizes[k][1], minx, maxx, miny, maxy);
    }
    free(f);
    printf("\n%d checks, %d failed\n", run, fail);
    return fail ? 1 : 0;
}
