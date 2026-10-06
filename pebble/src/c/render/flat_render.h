#ifndef FLAT_RENDER_H
#define FLAT_RENDER_H
#include <pebble.h>
#include "../core/ttt_game.h"
/* Flat 3x3 view of the same game state; draws into `area` (any size). */
void flat_render_draw(GContext *ctx, GRect area, const TttGame *g, int selected);
#endif
