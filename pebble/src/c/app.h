/* Shared application state (Pebble layer). */
#ifndef APP_H
#define APP_H
#include <pebble.h>
#include "core/ttt_persist.h"

typedef struct {
    TttStorage  storage;
    TttSettings settings;
    TttStats    stats;
    TttRng      rng;
    bool        has_active;       /* Continue available */
    char        banner[20];       /* last result shown on the menu, "" if none */
} TttApp;

extern TttApp g_app;
void app_refresh_active(void);
#endif
