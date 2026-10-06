#include "ttt_game.h"

void ttt_rng_seed(TttRng *r, uint32_t seed) { r->s = seed ? seed : 0x9E3779B9u; }
uint32_t ttt_rng_next(TttRng *r) {
    uint32_t x = r->s ? r->s : 0x9E3779B9u;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    r->s = x; return x;
}
uint32_t ttt_rng_range(TttRng *r, uint32_t n) { return n ? (ttt_rng_next(r) >> 8) % n : 0; }

void ttt_settings_default(TttSettings *s) {
    s->mode = TTT_MODE_HVC;
    s->participant[0] = TTT_HUMAN; s->participant[1] = TTT_COMPUTER;
    s->glyph[0] = 'X'; s->glyph[1] = 'O';
    s->first_player_setting = TTT_FIRST_P1;
    s->difficulty = TTT_DIFF_EASY;
    s->board_view = TTT_VIEW_TORUS;
}

TttMode ttt_mode_from_participants(uint8_t p1, uint8_t p2) {
    if (p1 == TTT_HUMAN && p2 == TTT_HUMAN) return TTT_MODE_HVH;
    if (p1 == TTT_COMPUTER && p2 == TTT_COMPUTER) return TTT_MODE_CVC;
    return TTT_MODE_HVC;
}

bool ttt_settings_valid(const TttSettings *s) {
    if (s->participant[0] > 1 || s->participant[1] > 1) return false;
    if (s->mode > TTT_MODE_CVC) return false;
    if (s->mode != ttt_mode_from_participants(s->participant[0], s->participant[1])) return false;
    for (int i = 0; i < 2; i++) if (s->glyph[i] < 0x21 || s->glyph[i] > 0x7E) return false;
    if (s->glyph[0] == s->glyph[1]) return false;
    return s->first_player_setting <= TTT_FIRST_RANDOM && s->difficulty <= TTT_DIFF_PERFECT &&
           s->board_view <= TTT_VIEW_FLAT;
}

void ttt_settings_set_mode(TttSettings *s, TttMode mode, TttPlayer human) {
    s->mode = (uint8_t)mode;
    if (mode == TTT_MODE_HVH) { s->participant[0] = s->participant[1] = TTT_HUMAN; }
    else if (mode == TTT_MODE_CVC) { s->participant[0] = s->participant[1] = TTT_COMPUTER; }
    else {
        s->participant[0] = (human == TTT_PLAYER_2) ? TTT_COMPUTER : TTT_HUMAN;
        s->participant[1] = (human == TTT_PLAYER_2) ? TTT_HUMAN : TTT_COMPUTER;
    }
}

void ttt_game_new(TttGame *g, const TttSettings *s, TttRng *rng) {
    ttt_board_init(&g->board);
    g->mode = s->mode;
    g->participant[0] = s->participant[0]; g->participant[1] = s->participant[1];
    g->glyph[0] = s->glyph[0]; g->glyph[1] = s->glyph[1];
    g->difficulty = s->difficulty; g->board_view = s->board_view;
    if (s->first_player_setting == TTT_FIRST_RANDOM)
        g->first_player = (ttt_rng_next(rng) & 0x100) ? TTT_PLAYER_2 : TTT_PLAYER_1;
    else
        g->first_player = (s->first_player_setting == TTT_FIRST_P2) ? TTT_PLAYER_2 : TTT_PLAYER_1;
    g->current_player = g->first_player;
    g->status = TTT_STATUS_IN_PROGRESS; g->winner = TTT_EMPTY;
    g->move_count = 0; g->win_line = -1;
}

void ttt_game_refresh_status(TttGame *g) {
    int line; TttPlayer w = ttt_board_winner(&g->board, &line);
    g->move_count = (uint8_t)(TTT_CELLS - ttt_board_count_empty(&g->board));
    if (w != TTT_EMPTY) { g->status = TTT_STATUS_WON; g->winner = w; g->win_line = (int8_t)line; }
    else if (ttt_board_is_full(&g->board)) { g->status = TTT_STATUS_DRAW; g->winner = TTT_EMPTY; g->win_line = -1; }
    else { g->status = TTT_STATUS_IN_PROGRESS; g->winner = TTT_EMPTY; g->win_line = -1; }
}

TttMoveResult ttt_game_apply_move(TttGame *g, int cell) {
    if (g->status != TTT_STATUS_IN_PROGRESS) return TTT_MOVE_GAME_OVER;
    if (cell < 0 || cell >= TTT_CELLS) return TTT_MOVE_BAD_CELL;
    if (g->board.cell[cell] != TTT_EMPTY) return TTT_MOVE_OCCUPIED;
    ttt_board_place(&g->board, cell, (TttPlayer)g->current_player);
    ttt_game_refresh_status(g);   /* win takes precedence over draw; multiple lines = one win */
    if (g->status == TTT_STATUS_IN_PROGRESS) g->current_player = (uint8_t)ttt_other_player((TttPlayer)g->current_player);
    return TTT_MOVE_OK;
}

bool ttt_game_is_over(const TttGame *g) { return g->status != TTT_STATUS_IN_PROGRESS; }
TttParticipant ttt_game_current_participant(const TttGame *g) { return (TttParticipant)g->participant[g->current_player - 1]; }
char ttt_game_glyph(const TttGame *g, TttPlayer p) { return (p == TTT_PLAYER_1 || p == TTT_PLAYER_2) ? (char)g->glyph[p - 1] : ' '; }

TttResult ttt_game_result_for(const TttGame *g, TttPlayer p) {
    if (g->status == TTT_STATUS_IN_PROGRESS) return TTT_RESULT_NONE;
    if (g->status == TTT_STATUS_DRAW) return TTT_RESULT_DRAW;
    return g->winner == p ? TTT_RESULT_WIN : TTT_RESULT_LOSS;
}

TttPlayer ttt_game_stats_player(const TttGame *g) {
    if (ttt_mode_from_participants(g->participant[0], g->participant[1]) != TTT_MODE_HVC) return TTT_EMPTY;
    return g->participant[0] == TTT_HUMAN ? TTT_PLAYER_1 : TTT_PLAYER_2;
}
