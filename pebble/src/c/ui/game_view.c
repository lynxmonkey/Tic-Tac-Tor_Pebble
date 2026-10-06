/* Game screen: presentation state (selection, camera, timers) + input handling.
 * Authoritative state lives in TttGame and is changed only through the engine. */
#include <pebble.h>
#include "game_view.h"
#include "../app.h"
#include "../core/ttt_ai.h"
#include "../input_pebble.h"
#include "../render/colors.h"
#include "../render/flat_render.h"
#include "../render/torus_render.h"

#define AI_DELAY_MS 450
#define STATUS_H 28

typedef struct {
    Window *window; Layer *layer;
    TttGame game;
    int selected;
    TorusCamera cam, target;
    bool animating, over;
    AppTimer *tick, *ai_timer;
} GameView;

static GameView s_gv;
static bool s_inited_once = false;

static void schedule_tick(void);

static void save_active(void) { ttt_save_game(&g_app.storage, &s_gv.game); }

static void on_game_over(void) {
    TttGame *g = &s_gv.game;
    s_gv.over = true;
    TttResult r = ttt_complete_game(&g_app.storage, g, &g_app.stats);   /* stats + remove active record */
    if (r == TTT_RESULT_WIN) snprintf(g_app.banner, sizeof g_app.banner, "WIN!");
    else if (r == TTT_RESULT_LOSS) snprintf(g_app.banner, sizeof g_app.banner, "LOSS");
    else if (r == TTT_RESULT_DRAW) snprintf(g_app.banner, sizeof g_app.banner, "DRAW");
    else if (g->status == TTT_STATUS_DRAW) snprintf(g_app.banner, sizeof g_app.banner, "DRAW");
    else snprintf(g_app.banner, sizeof g_app.banner, "%c WINS", ttt_game_glyph(g, (TttPlayer)g->winner));
    g_app.has_active = false;
    vibes_double_pulse();
}

static void after_move(void) {
    if (ttt_game_is_over(&s_gv.game)) on_game_over(); else save_active();
    if (s_gv.layer) layer_mark_dirty(s_gv.layer);
}

static void ai_cb(void *data) {
    (void)data; s_gv.ai_timer = NULL;
    if (s_gv.over || ttt_game_current_participant(&s_gv.game) != TTT_COMPUTER) return;
    if (ttt_ai_play(&s_gv.game, &g_app.rng) == TTT_MOVE_OK) after_move();
    if (!s_gv.over && ttt_game_current_participant(&s_gv.game) == TTT_COMPUTER)   /* Computer vs Computer */
        s_gv.ai_timer = app_timer_register(AI_DELAY_MS, ai_cb, NULL);
}
static void maybe_schedule_ai(void) {
    if (!s_gv.over && !s_gv.ai_timer && ttt_game_current_participant(&s_gv.game) == TTT_COMPUTER)
        s_gv.ai_timer = app_timer_register(AI_DELAY_MS, ai_cb, NULL);
}

static void tick_cb(void *data) {
    (void)data; s_gv.tick = NULL;
    if (s_gv.animating) s_gv.animating = torus_camera_step(&s_gv.cam, &s_gv.target);
    else torus_camera_idle_step(&s_gv.cam);               /* slow idle rotation, sparse updates */
    layer_mark_dirty(s_gv.layer);
    schedule_tick();
}
static void schedule_tick(void) {
    if (s_gv.tick) { app_timer_cancel(s_gv.tick); s_gv.tick = NULL; }
    if (s_gv.game.board_view != TTT_VIEW_TORUS) return;    /* flat view: no animation timer at all */
    s_gv.tick = app_timer_register(s_gv.animating ? TORUS_TRANSITION_INTERVAL_MS : TORUS_UPDATE_INTERVAL_MS, tick_cb, NULL);
}

static void select_cell(int cell) {
    s_gv.selected = cell;
    torus_camera_target_for_cell(cell, &s_gv.target);
    s_gv.animating = true;
    schedule_tick();
    layer_mark_dirty(s_gv.layer);
}

static void handle_action(TttAction a, void *ctx) {
    (void)ctx;
    switch (a) {
    case TTT_ACTION_NEXT:     select_cell(ttt_cell_next(s_gv.selected)); break;
    case TTT_ACTION_PREVIOUS: select_cell(ttt_cell_prev(s_gv.selected)); break;
    case TTT_ACTION_BACK:     window_stack_pop(true); break;
    case TTT_ACTION_SELECT:
        if (s_gv.over) { window_stack_pop(true); break; }
        if (ttt_game_current_participant(&s_gv.game) != TTT_HUMAN) break;      /* computer's turn */
        if (ttt_game_apply_move(&s_gv.game, s_gv.selected) == TTT_MOVE_OK) { after_move(); maybe_schedule_ai(); }
        else vibes_short_pulse();                                              /* occupied: subtle feedback */
        break;
    }
}

static void long_select(void *ctx) {   /* presentation-only: toggle Torus/Flat */
    (void)ctx;
    s_gv.game.board_view = (s_gv.game.board_view == TTT_VIEW_TORUS) ? TTT_VIEW_FLAT : TTT_VIEW_TORUS;
    if (!s_gv.over) save_active();
    schedule_tick();
    layer_mark_dirty(s_gv.layer);
}

static void update_proc(Layer *layer, GContext *ctx) {
    GRect b = layer_get_bounds(layer);
    graphics_context_set_fill_color(ctx, BACKGROUND_COLOR);
    graphics_fill_rect(ctx, b, 0, GCornerNone);

    char status[24];
    if (s_gv.over) snprintf(status, sizeof status, "%s", g_app.banner);
    else snprintf(status, sizeof status, "%c'S TURN  [%c]",
                  ttt_game_glyph(&s_gv.game, (TttPlayer)s_gv.game.current_player), 'A' + s_gv.selected);
    int top = PBL_IF_ROUND_ELSE(8, 0);
    graphics_context_set_text_color(ctx, s_gv.over ? HIGHLIGHT_COLOR : FOREGROUND_COLOR);
    graphics_draw_text(ctx, status, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                       GRect(0, top - 3, b.size.w, STATUS_H), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

    GRect area = GRect(0, top + STATUS_H, b.size.w, b.size.h - top - STATUS_H);
    if (s_gv.game.board_view == TTT_VIEW_TORUS) torus_render_draw(ctx, area, &s_gv.game, s_gv.selected, &s_gv.cam);
    else flat_render_draw(ctx, area, &s_gv.game, s_gv.selected);
}

static void window_load(Window *w) {
    Layer *root = window_get_root_layer(w);
    s_gv.layer = layer_create(layer_get_bounds(root));
    layer_set_update_proc(s_gv.layer, update_proc);
    layer_add_child(root, s_gv.layer);
    torus_render_init();
    input_attach(w, handle_action, long_select, NULL);
    schedule_tick();
    maybe_schedule_ai();
}
static void window_unload(Window *w) {
    (void)w;
    if (s_gv.tick) { app_timer_cancel(s_gv.tick); s_gv.tick = NULL; }
    if (s_gv.ai_timer) { app_timer_cancel(s_gv.ai_timer); s_gv.ai_timer = NULL; }
    torus_render_deinit();
    layer_destroy(s_gv.layer); s_gv.layer = NULL;
    window_destroy(s_gv.window); s_gv.window = NULL;
}

bool game_view_push(bool continue_game) {
    (void)s_inited_once;
    if (continue_game) {
        if (ttt_load_game(&g_app.storage, &s_gv.game) != TTT_LOAD_OK) { g_app.has_active = false; return false; }
    } else {
        ttt_game_new(&s_gv.game, &g_app.settings, &g_app.rng);   /* random first player resolved once, here */
        ttt_save_game(&g_app.storage, &s_gv.game);
        g_app.has_active = true;
    }
    g_app.banner[0] = '\0';
    s_gv.over = false; s_gv.tick = NULL; s_gv.ai_timer = NULL;
    s_gv.selected = 0; s_gv.animating = false;
    /* initial camera: already looking at the selected cell */
    torus_camera_target_for_cell(s_gv.selected, &s_gv.target);
    s_gv.cam = s_gv.target;
    s_gv.window = window_create();
    window_set_background_color(s_gv.window, BACKGROUND_COLOR);
    window_set_window_handlers(s_gv.window, (WindowHandlers){ .load = window_load, .unload = window_unload });
    window_stack_push(s_gv.window, true);
    return true;
}
