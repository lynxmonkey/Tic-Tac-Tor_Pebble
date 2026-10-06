/* Toroidal Tic-Tac-Toe - Pebble entry point. */
#include <pebble.h>
#include "app.h"
#include "platform_storage.h"
#include "ui/menu_window.h"

TttApp g_app;

void app_refresh_active(void) { g_app.has_active = ttt_has_active_game(&g_app.storage); }

static void init(void) {
    platform_storage_init(&g_app.storage);
    ttt_load_settings(&g_app.storage, &g_app.settings);   /* defaults on missing/corrupt/unknown version */
    ttt_load_stats(&g_app.storage, &g_app.stats);
    ttt_rng_seed(&g_app.rng, (uint32_t)time(NULL) * 2654435761u ^ (uint32_t)time_ms(NULL, NULL) * 40503u);
    g_app.banner[0] = '\0';
    app_refresh_active();
    menu_window_push();
}
static void deinit(void) { /* all state is saved eagerly; nothing to flush */ }

int main(void) { init(); app_event_loop(); deinit(); return 0; }
