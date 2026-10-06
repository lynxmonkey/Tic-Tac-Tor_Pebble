#ifndef TORUS_RENDER_H
#define TORUS_RENDER_H
#include <pebble.h>
#include "../core/ttt_game.h"
#include "torus_geom.h"
void torus_render_init(void);
void torus_render_deinit(void);
/* Draws the torus scaled to `area` using the given camera. Does not modify game state. */
void torus_render_draw(GContext *ctx, GRect area, const TttGame *g, int selected, const TorusCamera *cam);
#endif
