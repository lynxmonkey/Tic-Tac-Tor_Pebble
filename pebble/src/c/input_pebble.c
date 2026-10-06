#include "input_pebble.h"

static InputActionHandler s_action; static InputLongSelectHandler s_long; static void *s_ctx;

static void up_cb(ClickRecognizerRef r, void *c) { (void)r; (void)c; if (s_action) s_action(TTT_ACTION_PREVIOUS, s_ctx); }
static void down_cb(ClickRecognizerRef r, void *c) { (void)r; (void)c; if (s_action) s_action(TTT_ACTION_NEXT, s_ctx); }
static void sel_cb(ClickRecognizerRef r, void *c) { (void)r; (void)c; if (s_action) s_action(TTT_ACTION_SELECT, s_ctx); }
static void back_cb(ClickRecognizerRef r, void *c) { (void)r; (void)c; if (s_action) s_action(TTT_ACTION_BACK, s_ctx); }
static void long_cb(ClickRecognizerRef r, void *c) { (void)r; (void)c; if (s_long) s_long(s_ctx); }

static void provider(void *context) {
    (void)context;
    window_single_repeating_click_subscribe(BUTTON_ID_UP, 150, up_cb);
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 150, down_cb);
    window_single_click_subscribe(BUTTON_ID_SELECT, sel_cb);
    window_long_click_subscribe(BUTTON_ID_SELECT, 600, long_cb, NULL);
    window_single_click_subscribe(BUTTON_ID_BACK, back_cb);
}

void input_attach(Window *w, InputActionHandler a, InputLongSelectHandler l, void *ctx) {
    s_action = a; s_long = l; s_ctx = ctx;
    window_set_click_config_provider(w, provider);
}
