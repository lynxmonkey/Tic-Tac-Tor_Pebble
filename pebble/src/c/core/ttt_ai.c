#include "ttt_ai.h"

#define MEMO_SIZE 19683   /* 3^9 */
#define UNKNOWN 127
static int8_t g_memo[MEMO_SIZE];

static int board_key(const TttBoard *b) {
    int k = 0; for (int i = 0; i < TTT_CELLS; i++) k = k * 3 + b->cell[i]; return k;
}
static bool has_win(const TttBoard *b, TttPlayer p) {
    return ttt_board_count_lines_for(b, p) > 0;
}

/* Value for `mover` (to move) in a non-terminal position. */
static int search(TttBoard *b, TttPlayer mover, int moves_made) {
    int key = board_key(b);
    if (g_memo[key] != UNKNOWN) return g_memo[key];
    int best = -100;
    for (int i = 0; i < TTT_CELLS; i++) {
        if (b->cell[i] != TTT_EMPTY) continue;
        b->cell[i] = (uint8_t)mover;
        int v;
        if (has_win(b, mover)) v = 10 - (moves_made + 1);
        else if (moves_made + 1 == TTT_CELLS) v = 0;
        else v = -search(b, ttt_other_player(mover), moves_made + 1);
        b->cell[i] = TTT_EMPTY;
        if (v > best) best = v;
    }
    g_memo[key] = (int8_t)best;
    return best;
}

void ttt_ai_perfect_scores(const TttBoard *board, TttPlayer me, int8_t out[TTT_CELLS]) {
    TttBoard b = *board;
    for (int i = 0; i < MEMO_SIZE; i++) g_memo[i] = UNKNOWN;
    int made = TTT_CELLS - ttt_board_count_empty(&b);
    for (int i = 0; i < TTT_CELLS; i++) {
        if (b.cell[i] != TTT_EMPTY) { out[i] = TTT_AI_ILLEGAL; continue; }
        b.cell[i] = (uint8_t)me;
        int v;
        if (has_win(&b, me)) v = 10 - (made + 1);
        else if (made + 1 == TTT_CELLS) v = 0;
        else v = -search(&b, ttt_other_player(me), made + 1);
        b.cell[i] = TTT_EMPTY;
        out[i] = (int8_t)v;
    }
}

int ttt_ai_perfect_move(const TttBoard *b, TttPlayer me, TttRng *rng) {
    int8_t sc[TTT_CELLS]; int best = -128, n = 0, pick[TTT_CELLS];
    ttt_ai_perfect_scores(b, me, sc);
    for (int i = 0; i < TTT_CELLS; i++) if (sc[i] != TTT_AI_ILLEGAL && sc[i] > best) best = sc[i];
    if (best == -128) return -1;
    for (int i = 0; i < TTT_CELLS; i++) if (sc[i] == best) pick[n++] = i;  /* only equally-optimal moves */
    return pick[ttt_rng_range(rng, (uint32_t)n)];
}

/* Easy: win > block > (usually) simple heuristic, sometimes random mistake. */
int ttt_ai_easy_move(const TttBoard *b, TttPlayer me, TttRng *rng) {
    int legal[TTT_CELLS], nl = 0;
    for (int i = 0; i < TTT_CELLS; i++) if (b->cell[i] == TTT_EMPTY) legal[nl++] = i;
    if (nl == 0) return -1;
    int m = ttt_board_find_winning_move(b, me);
    if (m >= 0) return m;
    m = ttt_board_find_winning_move(b, ttt_other_player(me));
    if (m >= 0) return m;
    if (ttt_rng_range(rng, 100) < 35) return legal[ttt_rng_range(rng, (uint32_t)nl)];  /* mistake */
    /* heuristic: prefer cells on many lines not yet blocked by the opponent, favouring own marks */
    const TttLine *L = ttt_lines();
    int best = -1, nb = 0, pick[TTT_CELLS];
    TttPlayer opp = ttt_other_player(me);
    for (int k = 0; k < nl; k++) {
        int c = legal[k], score = 0;
        for (int li = 0; li < TTT_NUM_LINES; li++) {
            bool on = false; int mine = 0, theirs = 0;
            for (int j = 0; j < 3; j++) {
                int cc = L[li].cell[j];
                if (cc == c) on = true;
                else if (b->cell[cc] == me) mine++;
                else if (b->cell[cc] == opp) theirs++;
            }
            if (on && theirs == 0) score += 1 + 2 * mine;
        }
        if (score > best) { best = score; nb = 0; }
        if (score == best) pick[nb++] = c;
    }
    return pick[ttt_rng_range(rng, (uint32_t)nb)];
}

int ttt_ai_choose_move(const TttGame *g, TttRng *rng) {
    if (g->status != TTT_STATUS_IN_PROGRESS) return -1;
    TttPlayer me = (TttPlayer)g->current_player;
    return g->difficulty == TTT_DIFF_PERFECT ? ttt_ai_perfect_move(&g->board, me, rng)
                                             : ttt_ai_easy_move(&g->board, me, rng);
}

TttMoveResult ttt_ai_play(TttGame *g, TttRng *rng) {
    return ttt_game_apply_move(g, ttt_ai_choose_move(g, rng));
}
