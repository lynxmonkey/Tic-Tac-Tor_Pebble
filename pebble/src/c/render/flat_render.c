#include "flat_render.h"
#include "colors.h"

void flat_render_draw(GContext *ctx, GRect area, const TttGame *g, int selected) {
    int side = (area.size.w < area.size.h ? area.size.w : area.size.h) - 8;
    int cell = side / 3; side = cell * 3;
    int x0 = area.origin.x + (area.size.w - side) / 2, y0 = area.origin.y + (area.size.h - side) / 2;
    GFont font = fonts_get_system_font(cell >= 44 ? FONT_KEY_GOTHIC_28_BOLD : FONT_KEY_GOTHIC_24_BOLD);
    int fh = cell >= 44 ? 28 : 24;

    for (int i = 0; i < TTT_CELLS; i++) {
        int r = ttt_cell_row(i), c = ttt_cell_col(i);
        GRect rc = GRect(x0 + c * cell + 1, y0 + r * cell + 1, cell - 2, cell - 2);
        if (i == selected) {
            graphics_context_set_fill_color(ctx, HIGHLIGHT_COLOR);
            graphics_fill_rect(ctx, rc, 0, GCornerNone);
            graphics_context_set_fill_color(ctx, BACKGROUND_COLOR);
            graphics_fill_rect(ctx, GRect(rc.origin.x + 3, rc.origin.y + 3, rc.size.w - 6, rc.size.h - 6), 0, GCornerNone);
        }
        int who = g->board.cell[i];
        if (who != TTT_EMPTY) {
            char s[2] = { ttt_game_glyph(g, (TttPlayer)who), 0 };
            graphics_context_set_text_color(ctx, who == TTT_PLAYER_1 ? PLAYER_1_COLOR : PLAYER_2_COLOR);
            graphics_draw_text(ctx, s, font, GRect(rc.origin.x, rc.origin.y + (rc.size.h - fh) / 2 - 4, rc.size.w, fh + 6),
                               GTextOverflowModeFill, GTextAlignmentCenter, NULL);
        }
    }
    graphics_context_set_stroke_color(ctx, FOREGROUND_COLOR);
    graphics_context_set_stroke_width(ctx, 2);
    for (int k = 1; k < 3; k++) {
        graphics_draw_line(ctx, GPoint(x0 + k * cell, y0), GPoint(x0 + k * cell, y0 + side));
        graphics_draw_line(ctx, GPoint(x0, y0 + k * cell), GPoint(x0 + side, y0 + k * cell));
    }
}
