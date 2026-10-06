#include "ttt_board.h"

static TttLine g_lines[TTT_NUM_LINES];
static int g_line_count = 0;

int ttt_wrap(int v) { return ((v % 3) + 3) % 3; }
int ttt_cell_index(int row, int col) { return ttt_wrap(row) * TTT_COLS + ttt_wrap(col); }
int ttt_cell_row(int index) { return index / TTT_COLS; }
int ttt_cell_col(int index) { return index % TTT_COLS; }
int ttt_cell_next(int index) { return (index + 1) % TTT_CELLS; }
int ttt_cell_prev(int index) { return (index + TTT_CELLS - 1) % TTT_CELLS; }
TttPlayer ttt_other_player(TttPlayer p) { return p == TTT_PLAYER_1 ? TTT_PLAYER_2 : TTT_PLAYER_1; }

void ttt_board_init(TttBoard *b) { for (int i = 0; i < TTT_CELLS; i++) b->cell[i] = TTT_EMPTY; }
TttPlayer ttt_board_get(const TttBoard *b, int row, int col) { return (TttPlayer)b->cell[ttt_cell_index(row, col)]; }
TttPlayer ttt_board_get_index(const TttBoard *b, int i) {
    return (i >= 0 && i < TTT_CELLS) ? (TttPlayer)b->cell[i] : TTT_EMPTY;
}
bool ttt_board_is_valid_move(const TttBoard *b, int i) {
    return i >= 0 && i < TTT_CELLS && b->cell[i] == TTT_EMPTY;
}
bool ttt_board_place(TttBoard *b, int i, TttPlayer p) {
    if (p != TTT_PLAYER_1 && p != TTT_PLAYER_2) return false;
    if (!ttt_board_is_valid_move(b, i)) return false;
    b->cell[i] = (uint8_t)p;
    return true;
}
int ttt_board_count_empty(const TttBoard *b) {
    int n = 0; for (int i = 0; i < TTT_CELLS; i++) if (b->cell[i] == TTT_EMPTY) n++; return n;
}
bool ttt_board_is_full(const TttBoard *b) { return ttt_board_count_empty(b) == 0; }

static void build_lines(void) {
    static const int dirs[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    uint16_t seen[TTT_NUM_LINES * 4]; int nseen = 0;
    g_line_count = 0;
    for (int r = 0; r < 3; r++) for (int c = 0; c < 3; c++) for (int d = 0; d < 4; d++) {
        TttLine ln; uint16_t mask = 0;
        for (int k = 0; k < 3; k++) {
            ln.cell[k] = (uint8_t)ttt_cell_index(r + k * dirs[d][0], c + k * dirs[d][1]);
            mask |= (uint16_t)(1u << ln.cell[k]);
        }
        bool dup = false;
        for (int s = 0; s < nseen; s++) if (seen[s] == mask) { dup = true; break; }
        if (!dup && g_line_count < TTT_NUM_LINES) { seen[nseen++] = mask; g_lines[g_line_count++] = ln; }
    }
}

const TttLine *ttt_lines(void) { if (g_line_count == 0) build_lines(); return g_lines; }

TttPlayer ttt_board_winner(const TttBoard *b, int *line_out) {
    const TttLine *L = ttt_lines();
    for (int i = 0; i < TTT_NUM_LINES; i++) {
        uint8_t a = b->cell[L[i].cell[0]];
        if (a != TTT_EMPTY && a == b->cell[L[i].cell[1]] && a == b->cell[L[i].cell[2]]) {
            if (line_out) *line_out = i;
            return (TttPlayer)a;
        }
    }
    if (line_out) *line_out = -1;
    return TTT_EMPTY;
}

int ttt_board_count_lines_for(const TttBoard *b, TttPlayer p) {
    const TttLine *L = ttt_lines(); int n = 0;
    for (int i = 0; i < TTT_NUM_LINES; i++)
        if (b->cell[L[i].cell[0]] == p && b->cell[L[i].cell[1]] == p && b->cell[L[i].cell[2]] == p) n++;
    return n;
}

int ttt_board_find_winning_move(const TttBoard *b, TttPlayer p) {
    const TttLine *L = ttt_lines();
    for (int i = 0; i < TTT_NUM_LINES; i++) {
        int mine = 0, empty = -1;
        for (int k = 0; k < 3; k++) {
            uint8_t v = b->cell[L[i].cell[k]];
            if (v == p) mine++; else if (v == TTT_EMPTY) empty = L[i].cell[k];
        }
        if (mine == 2 && empty >= 0) return empty;
    }
    return -1;
}
