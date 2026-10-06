#ifndef INPUT_PEBBLE_H
#define INPUT_PEBBLE_H
#include <pebble.h>
#include "core/ttt_game.h"
/* Pebble buttons -> abstract actions: Up=PREVIOUS, Down=NEXT, Select=SELECT, Back=BACK. */
typedef void (*InputActionHandler)(TttAction action, void *ctx);
typedef void (*InputLongSelectHandler)(void *ctx);
void input_attach(Window *window, InputActionHandler on_action, InputLongSelectHandler on_long_select, void *ctx);
#endif
