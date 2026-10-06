/* Main menu: title, optional last result, New Game / Continue / Settings. */
#include <pebble.h>
#include "menu_window.h"
#include "game_view.h"
#include "settings_window.h"
#include "../app.h"
#include "../render/colors.h"

enum { ROW_NEW, ROW_CONTINUE, ROW_SETTINGS, ROW_COUNT };
static Window *s_win; static MenuLayer *s_menu;

static uint16_t num_rows(MenuLayer *m, uint16_t s, void *c) { (void)m; (void)s; (void)c; return ROW_COUNT; }
static int16_t header_h(MenuLayer *m, uint16_t s, void *c) { (void)m; (void)s; (void)c; return g_app.banner[0] ? 98 : 62; }

static void draw_header(GContext *ctx, const Layer *cl, uint16_t s, void *c) {
    (void)s; (void)c; GRect b = layer_get_bounds(cl);
    int y = PBL_IF_ROUND_ELSE(6, 0);
    graphics_context_set_text_color(ctx, FOREGROUND_COLOR);
    graphics_draw_text(ctx, "TOROIDAL", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(0, y, b.size.w, 28),
                       GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    graphics_draw_text(ctx, "TIC-TAC-TOE", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(0, y + 25, b.size.w, 28),
                       GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    if (g_app.banner[0]) {
        graphics_context_set_text_color(ctx, HIGHLIGHT_COLOR);
        graphics_draw_text(ctx, g_app.banner, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD), GRect(0, y + 58, b.size.w, 34),
                           GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    }
}

static void draw_row(GContext *ctx, const Layer *cl, MenuIndex *idx, void *c) {
    (void)c; GRect b = layer_get_bounds(cl);
    static const char *names[ROW_COUNT] = { "New Game", "Continue", "Settings" };
    bool hl = menu_cell_layer_is_highlighted(cl), disabled = (idx->row == ROW_CONTINUE && !g_app.has_active);
    graphics_context_set_text_color(ctx, disabled ? GColorDarkGray : (hl ? BACKGROUND_COLOR : FOREGROUND_COLOR));
    graphics_draw_text(ctx, names[idx->row], fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                       GRect(PBL_IF_ROUND_ELSE(0, 12), 4, b.size.w - 12, 30), GTextOverflowModeFill,
                       PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft), NULL);
}

static void select_click(MenuLayer *m, MenuIndex *idx, void *c) {
    (void)m; (void)c;
    switch (idx->row) {
    case ROW_NEW: game_view_push(false); break;
    case ROW_CONTINUE: if (g_app.has_active) game_view_push(true); break;   /* disabled when none */
    case ROW_SETTINGS: settings_window_push(); break;
    }
}

static void win_appear(Window *w) { (void)w; app_refresh_active(); menu_layer_reload_data(s_menu); }
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

void menu_window_push(void) {
    s_win = window_create();
    window_set_background_color(s_win, BACKGROUND_COLOR);
    window_set_window_handlers(s_win, (WindowHandlers){ .load = win_load, .appear = win_appear, .unload = win_unload });
    window_stack_push(s_win, true);
}
