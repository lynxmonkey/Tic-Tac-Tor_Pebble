/* AI: proposes moves only. The engine validates and applies them. */
#ifndef TTT_AI_H
#define TTT_AI_H
#include "ttt_game.h"

int ttt_ai_choose_move(const TttGame *g, TttRng *rng);                 /* by g->difficulty; -1 if none */
int ttt_ai_easy_move(const TttBoard *b, TttPlayer me, TttRng *rng);
int ttt_ai_perfect_move(const TttBoard *b, TttPlayer me, TttRng *rng);
/* Perfect-AI score per cell for `me` to move (higher = better; win sooner >
 * win later > 0 > lose later > lose sooner). Occupied cells = TTT_AI_ILLEGAL. */
#define TTT_AI_ILLEGAL (-127)
void ttt_ai_perfect_scores(const TttBoard *b, TttPlayer me, int8_t out[TTT_CELLS]);
/* Convenience: ask AI, then let the engine validate/apply. */
TttMoveResult ttt_ai_play(TttGame *g, TttRng *rng);

#endif
