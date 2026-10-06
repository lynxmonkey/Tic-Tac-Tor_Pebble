#ifndef GAME_VIEW_H
#define GAME_VIEW_H
#include <stdbool.h>
/* Pushes the game window. continue_game=false: create a new game from settings.
 * continue_game=true: restore the saved active game. Returns false if nothing to continue. */
bool game_view_push(bool continue_game);
#endif
