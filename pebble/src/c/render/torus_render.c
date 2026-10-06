#include "torus_render.h"
#include "colors.h"

static GPath *s_path;
static GPoint s_pts[4];
static TorusFrame s_frame;

void torus_render_init(void) {
    GPathInfo info = { .num_points = 4, .points = s_pts };
    s_path = gpath_create(&info);
}
void torus_render_deinit(void) { if (s_path) { gpath_destroy(s_path); s_path = NULL; } }

static GColor mix(GColor a, GColor b) {      /* 50/50 */
    return GColorFromRGB((a.r + b.r) * 85 / 2, (a.g + b.g) * 85 / 2, (a.b + b.b) * 85 / 2);
}
static GColor shade(GColor c, int light) {   /* light 0..255; GColor channels are 0..3 */
    return GColorFromRGB(c.r * 85 * light / 255, c.g * 85 * light / 255, c.b * 85 * light / 255);
}

void torus_render_draw(GContext *ctx, GRect area, const TttGame *g, int selected, const TorusCamera *cam) {
    if (!s_path) return;
    TorusProjection proj;
    torus_projection_fit(&proj, area.origin.x, area.origin.y, area.size.w, area.size.h);
    torus_build_frame(cam, &proj, &s_frame);

    GColor fills[9];
    for (int i = 0; i < 9; i++) {
        GColor base = g->board.cell[i] == TTT_PLAYER_1 ? PLAYER_1_COLOR :
                      g->board.cell[i] == TTT_PLAYER_2 ? PLAYER_2_COLOR : EMPTY_CELL_COLOR;
        if (i == selected) base = (g->board.cell[i] == TTT_EMPTY) ? HIGHLIGHT_COLOR : mix(base, HIGHLIGHT_COLOR);
        fills[i] = base;
    }

    for (int n = 0; n < s_frame.num_quads; n++) {
        const TorusQuad *q = &s_frame.quads[n];
        int i0 = q->i, i1 = (q->i + 1) % TORUS_U_SEGS, j0 = q->j, j1 = (q->j + 1) % TORUS_V_SEGS;
        TorusPt a = s_frame.vert[i0][j0], b = s_frame.vert[i1][j0], c = s_frame.vert[i1][j1], d = s_frame.vert[i0][j1];
        s_path->points[0] = GPoint(a.x, a.y); s_path->points[1] = GPoint(b.x, b.y);
        s_path->points[2] = GPoint(c.x, c.y); s_path->points[3] = GPoint(d.x, d.y);
        GColor col = shade(fills[q->cell], q->light);
        graphics_context_set_fill_color(ctx, col);
        graphics_context_set_stroke_color(ctx, col);      /* same-colour outline closes hairline gaps */
        graphics_context_set_stroke_width(ctx, 1);
        gpath_draw_filled(ctx, s_path);
        gpath_draw_outline(ctx, s_path);
        /* cell boundaries: U boundary every 4 segments in i, V boundary every 4 in j */
        graphics_context_set_stroke_color(ctx, FOREGROUND_COLOR);
        graphics_context_set_stroke_width(ctx, 2);
        if (i0 % TORUS_SEGS_PER_CELL == 0) graphics_draw_line(ctx, GPoint(a.x, a.y), GPoint(d.x, d.y));
        if (j0 % TORUS_SEGS_PER_CELL == 0) graphics_draw_line(ctx, GPoint(a.x, a.y), GPoint(b.x, b.y));
    }

    GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
    for (int i = 0; i < 9; i++) {
        int who = g->board.cell[i];
        if (who == TTT_EMPTY || s_frame.cell_facing[i] < 2500) continue;     /* hide marks on the far side */
        char s[2] = { ttt_game_glyph(g, (TttPlayer)who), 0 };
        int x = s_frame.cell_center[i].x, y = s_frame.cell_center[i].y;
        graphics_context_set_text_color(ctx, GColorBlack);
        graphics_draw_text(ctx, s, font, GRect(x - 13, y - 18, 28, 30), GTextOverflowModeFill, GTextAlignmentCenter, NULL);
        graphics_context_set_text_color(ctx, GColorWhite);
        graphics_draw_text(ctx, s, font, GRect(x - 14, y - 19, 28, 30), GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    }
}
