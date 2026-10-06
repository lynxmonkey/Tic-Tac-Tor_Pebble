/* Persistent defaults for new games. SELECT cycles the highlighted value; BACK leaves. */
#include <pebble.h>
#include "settings_window.h"
#include "../app.h"
#include "../render/colors.h"

enum { ROW_MODE, ROW_HUMAN, ROW_GLYPHS, ROW_FIRST, ROW_DIFF, ROW_VIEW, ROW_COUNT };
static Window *s_win; static MenuLayer *s_menu;

static bool is_hvc(void) { return g_app.settings.mode == TTT_MODE_HVC; }
static TttPlayer human_role(void) { return g_app.settings.participant[0] == TTT_HUMAN ? TTT_PLAYER_1 : TTT_PLAYER_2; }

static const char *row_title(int r) {
    static const char *t[ROW_COUNT] = { "Game mode", "Human plays", "Glyphs", "First player", "AI difficulty", "Board view" };
    return t[r];
}
static void row_value(int r, char *out, size_t n) {
    const TttSettings *s = &g_app.settings;
    switch (r) {
    case ROW_MODE:   snprintf(out, n, "%s", s->mode == TTT_MODE_HVH ? "Human vs Human" : "Human vs Computer"); break;
    case ROW_HUMAN:  snprintf(out, n, "%s", !is_hvc() ? "n/a" : (human_role() == TTT_PLAYER_1 ? "Player 1" : "Player 2")); break;
    case ROW_GLYPHS: snprintf(out, n, "P1 = %c   P2 = %c", s->glyph[0], s->glyph[1]); break;
    case ROW_FIRST:  snprintf(out, n, "%s", s->first_player_setting == TTT_FIRST_P1 ? "Player 1" : s->first_player_setting == TTT_FIRST_P2 ? "Player 2" : "Random"); break;
    case ROW_DIFF:   snprintf(out, n, "%s", s->difficulty == TTT_DIFF_EASY ? "Easy" : "Perfect"); break;
    case ROW_VIEW:   snprintf(out, n, "%s", s->board_view == TTT_VIEW_TORUS ? "Torus" : "Flat"); break;
    }
}

static uint16_t num_rows(MenuLayer *m, uint16_t s, void *c) { (void)m; (void)s; (void)c; return ROW_COUNT; }
static int16_t header_h(MenuLayer *m, uint16_t s, void *c) { (void)m; (void)s; (void)c; return MENU_CELL_BASIC_HEADER_HEIGHT; }
static void draw_header(GContext *ctx, const Layer *cl, uint16_t s, void *c) { (void)s; (void)c; menu_cell_basic_header_draw(ctx, cl, "Settings"); }
static void draw_row(GContext *ctx, const Layer *cl, MenuIndex *idx, void *c) {
    (void)c; char v[32]; row_value(idx->row, v, sizeof v);
    menu_cell_basic_draw(ctx, cl, row_title(idx->row), v, NULL);
}

static void select_click(MenuLayer *m, MenuIndex *idx, void *c) {
    (void)c; TttSettings *s = &g_app.settings;
    switch (idx->row) {
    case ROW_MODE: ttt_settings_set_mode(s, s->mode == TTT_MODE_HVH ? TTT_MODE_HVC : TTT_MODE_HVH, human_role()); break;
    case ROW_HUMAN: if (is_hvc()) ttt_settings_set_mode(s, TTT_MODE_HVC, human_role() == TTT_PLAYER_1 ? TTT_PLAYER_2 : TTT_PLAYER_1); break;
    case ROW_GLYPHS: { uint8_t t = s->glyph[0]; s->glyph[0] = s->glyph[1]; s->glyph[1] = t; } break;
    case ROW_FIRST: s->first_player_setting = (uint8_t)((s->first_player_setting + 1) % 3); break;
    case ROW_DIFF:  s->difficulty = (uint8_t)(1 - s->difficulty); break;
    case ROW_VIEW:  s->board_view = (uint8_t)(1 - s->board_view); break;
    }
    ttt_save_settings(&g_app.storage, s);          /* defaults for subsequent New Game only */
    menu_layer_reload_data(m);
}

static void win_load(Window *w) {
    Layer *root = window_get_root_layer(w);
    s_menu = menu_layer_create(layer_get_bounds(root));
    menu_layer_set_normal_colors(s_menu, BACKGROUND_COLOR, FOREGROUND_COLOR);
    menu_layer_set_highlight_colors(s_menu, HIGHLIGHT_COLOR, BACKGROUND_COLOR);
    menu_layer_set_callbacks(s_menu, NULL, (MenuLayerCallbacks){
        .get_num_rows = num_rows, .get_header_height = header_h, .draw_header = draw_header,
        .draw_row = draw_row, .select_click = select_click });
    menu_layer_set_click_config_onto_window(s_menu, w);
    layer_add_child(root, menu_layer_get_layer(s_menu));
}
static void win_unload(Window *w) { (void)w; menu_layer_destroy(s_menu); window_destroy(s_win); s_win = NULL; }

void settings_window_push(void) {
    s_win = window_create();
    window_set_background_color(s_win, BACKGROUND_COLOR);
    window_set_window_handlers(s_win, (WindowHandlers){ .load = win_load, .unload = win_unload });
    window_stack_push(s_win, true);
}
