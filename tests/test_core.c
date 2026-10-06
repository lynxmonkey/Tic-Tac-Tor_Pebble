/* Host-side tests for the platform-independent game core. */
#include <stdio.h>
#include <string.h>
#include "ttt_board.h"
#include "ttt_game.h"
#include "ttt_ai.h"
#include "ttt_persist.h"

static int g_run = 0, g_fail = 0;
#define CHECK(c) do { g_run++; if (!(c)) { g_fail++; printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)
#define SECTION(n) printf("[%s]\n", n)

/* Independently written expectation of the 12 lines (A=0 .. I=8). */
static const int EXPECT[12][3] = {
    {0,1,2},{3,4,5},{6,7,8},        /* horizontal (rows) */
    {0,3,6},{1,4,7},{2,5,8},        /* vertical (cols) */
    {0,4,8},{1,5,6},{2,3,7},        /* positive-slope wrapped diagonals: AEI, BFG, CDH */
    {2,4,6},{0,5,7},{1,3,8}         /* negative-slope wrapped diagonals: CEG, AFH, BDI */
};
static unsigned mask3(int a, int b, int c) { return (1u << a) | (1u << b) | (1u << c); }
static bool is_line(int a, int b, int c) {
    for (int i = 0; i < 12; i++) if (mask3(a, b, c) == mask3(EXPECT[i][0], EXPECT[i][1], EXPECT[i][2])) return true;
    return false;
}

static void test_board(void) {
    SECTION("board");
    TttBoard b; ttt_board_init(&b);
    for (int i = 0; i < 9; i++) CHECK(ttt_board_get_index(&b, i) == TTT_EMPTY);
    CHECK(ttt_board_count_empty(&b) == 9 && !ttt_board_is_full(&b));
    CHECK(ttt_wrap(-1) == 2 && ttt_wrap(3) == 0 && ttt_wrap(-4) == 2 && ttt_wrap(7) == 1);
    CHECK(ttt_cell_index(0, -1) == 2 && ttt_cell_index(-1, 0) == 6 && ttt_cell_index(3, 3) == 0);
    CHECK(ttt_cell_row(7) == 2 && ttt_cell_col(7) == 1);
    CHECK(ttt_board_place(&b, 4, TTT_PLAYER_1));
    CHECK(ttt_board_get(&b, 1, 1) == TTT_PLAYER_1 && ttt_board_get(&b, 4, 4) == TTT_PLAYER_1);
    CHECK(!ttt_board_place(&b, 4, TTT_PLAYER_2));          /* occupied rejected */
    CHECK(b.cell[4] == TTT_PLAYER_1);
    CHECK(!ttt_board_is_valid_move(&b, -1) && !ttt_board_is_valid_move(&b, 9));
    CHECK(!ttt_board_place(&b, 0, TTT_EMPTY));
    for (int i = 0; i < 9; i++) if (i != 4) ttt_board_place(&b, i, (i & 1) ? TTT_PLAYER_1 : TTT_PLAYER_2);
    CHECK(ttt_board_is_full(&b));
    /* selection cycle */
    int i = 0; for (int k = 0; k < 9; k++) i = ttt_cell_next(i); CHECK(i == 0);
    CHECK(ttt_cell_prev(0) == 8 && ttt_cell_next(8) == 0);
}

static void test_lines(void) {
    SECTION("12 toroidal winning lines");
    const TttLine *L = ttt_lines();
    for (int i = 0; i < 12; i++) {
        CHECK(is_line(L[i].cell[0], L[i].cell[1], L[i].cell[2]));
        for (int j = i + 1; j < 12; j++)
            CHECK(mask3(L[i].cell[0], L[i].cell[1], L[i].cell[2]) != mask3(L[j].cell[0], L[j].cell[1], L[j].cell[2]));
    }
    for (int p = 1; p <= 2; p++) for (int i = 0; i < 12; i++) {
        TttBoard b; ttt_board_init(&b);
        for (int k = 0; k < 3; k++) b.cell[EXPECT[i][k]] = (uint8_t)p;
        int line = -1;
        CHECK(ttt_board_winner(&b, &line) == (TttPlayer)p);
        CHECK(line >= 0 && mask3(L[line].cell[0], L[line].cell[1], L[line].cell[2]) == mask3(EXPECT[i][0], EXPECT[i][1], EXPECT[i][2]));
    }
    /* Named examples from the spec: C->A->B and I->A->E */
    TttBoard b; ttt_board_init(&b); b.cell[2] = b.cell[0] = b.cell[1] = 1; CHECK(ttt_board_winner(&b, NULL) == 1);
    ttt_board_init(&b); b.cell[8] = b.cell[0] = b.cell[4] = 2; CHECK(ttt_board_winner(&b, NULL) == 2);
    /* Wrapping category spot checks */
    ttt_board_init(&b); b.cell[1] = b.cell[5] = b.cell[6] = 1; CHECK(ttt_board_winner(&b, NULL) == 1);  /* B-F-G both dims */
    ttt_board_init(&b); b.cell[2] = b.cell[3] = b.cell[7] = 1; CHECK(ttt_board_winner(&b, NULL) == 1);  /* C-D-H */
    ttt_board_init(&b); b.cell[1] = b.cell[3] = b.cell[8] = 2; CHECK(ttt_board_winner(&b, NULL) == 2);  /* B-D-I */
}

static void test_near_misses(void) {
    SECTION("near misses");
    int nonlines = 0;
    for (int a = 0; a < 9; a++) for (int b2 = a + 1; b2 < 9; b2++) for (int c = b2 + 1; c < 9; c++) {
        TttBoard b; ttt_board_init(&b);
        b.cell[a] = b.cell[b2] = b.cell[c] = 1;
        bool win = ttt_board_winner(&b, NULL) == 1;
        CHECK(win == is_line(a, b2, c));
        if (!is_line(a, b2, c)) nonlines++;
        /* every triple is a line or not; replace one by the other player => never a win */
        b.cell[c] = 2; CHECK(ttt_board_winner(&b, NULL) == TTT_EMPTY);
    }
    CHECK(nonlines == 84 - 12);
    /* Two in a row with the third empty / opponent: no win, correct winning move */
    TttBoard b; ttt_board_init(&b); b.cell[0] = b.cell[1] = 1;
    CHECK(ttt_board_winner(&b, NULL) == TTT_EMPTY && ttt_board_find_winning_move(&b, TTT_PLAYER_1) == 2);
    b.cell[2] = 2; CHECK(ttt_board_find_winning_move(&b, TTT_PLAYER_1) == -1);
    /* A,B,D and conventional-looking non-lines */
    ttt_board_init(&b); b.cell[0] = b.cell[1] = b.cell[3] = 1; CHECK(ttt_board_winner(&b, NULL) == TTT_EMPTY);
    ttt_board_init(&b); b.cell[0] = b.cell[4] = b.cell[5] = 1; CHECK(ttt_board_winner(&b, NULL) == TTT_EMPTY);
    /* any two cells lie on exactly one line (affine plane) */
    for (int a = 0; a < 9; a++) for (int c = a + 1; c < 9; c++) {
        int n = 0; for (int i = 0; i < 12; i++) {
            bool ha = false, hc = false;
            for (int k = 0; k < 3; k++) { if (EXPECT[i][k] == a) ha = true; if (EXPECT[i][k] == c) hc = true; }
            if (ha && hc) n++;
        }
        CHECK(n == 1);
    }
}

static TttGame new_default(TttRng *r) { TttSettings s; ttt_settings_default(&s); TttGame g; ttt_game_new(&g, &s, r); return g; }
static void play(TttGame *g, const int *mv, int n) { for (int i = 0; i < n; i++) CHECK(ttt_game_apply_move(g, mv[i]) == TTT_MOVE_OK); }

static void test_game(void) {
    SECTION("game state");
    TttRng r; ttt_rng_seed(&r, 1234);
    TttGame g = new_default(&r);
    CHECK(g.current_player == 1 && g.first_player == 1 && g.status == TTT_STATUS_IN_PROGRESS);
    CHECK(ttt_game_current_participant(&g) == TTT_HUMAN);
    CHECK(ttt_game_glyph(&g, TTT_PLAYER_1) == 'X' && ttt_game_glyph(&g, TTT_PLAYER_2) == 'O');
    CHECK(ttt_game_apply_move(&g, 4) == TTT_MOVE_OK && g.current_player == 2);
    CHECK(ttt_game_current_participant(&g) == TTT_COMPUTER);
    CHECK(ttt_game_apply_move(&g, 4) == TTT_MOVE_OCCUPIED && g.current_player == 2);   /* no turn lost */
    CHECK(ttt_game_apply_move(&g, 9) == TTT_MOVE_BAD_CELL && ttt_game_apply_move(&g, -1) == TTT_MOVE_BAD_CELL);
    CHECK(ttt_game_apply_move(&g, 0) == TTT_MOVE_OK && g.current_player == 1);

    /* player roles are independent of glyphs and participant types */
    TttSettings s; ttt_settings_default(&s);
    s.glyph[0] = 'O'; s.glyph[1] = 'X'; s.first_player_setting = TTT_FIRST_P2;
    ttt_settings_set_mode(&s, TTT_MODE_HVC, TTT_PLAYER_2);
    CHECK(ttt_settings_valid(&s));
    ttt_game_new(&g, &s, &r);
    CHECK(g.first_player == 2 && g.current_player == 2 && ttt_game_current_participant(&g) == TTT_HUMAN);
    CHECK(ttt_game_glyph(&g, TTT_PLAYER_1) == 'O' && g.participant[0] == TTT_COMPUTER);
    CHECK(ttt_game_stats_player(&g) == TTT_PLAYER_2);

    /* Human vs Human / Computer vs Computer modes */
    ttt_settings_set_mode(&s, TTT_MODE_HVH, TTT_PLAYER_1); ttt_game_new(&g, &s, &r);
    CHECK(g.mode == TTT_MODE_HVH && ttt_game_stats_player(&g) == TTT_EMPTY);
    ttt_settings_set_mode(&s, TTT_MODE_CVC, TTT_PLAYER_1); ttt_game_new(&g, &s, &r);
    CHECK(g.mode == TTT_MODE_CVC && ttt_game_current_participant(&g) == TTT_COMPUTER);

    /* Random first player: resolved at creation, both values reachable, never Random in state */
    ttt_settings_default(&s); s.first_player_setting = TTT_FIRST_RANDOM;
    int seen[3] = {0, 0, 0};
    for (int i = 0; i < 200; i++) { ttt_game_new(&g, &s, &r); seen[g.first_player]++; CHECK(g.current_player == g.first_player); }
    CHECK(seen[1] > 20 && seen[2] > 20 && seen[0] == 0);

    /* Win (X wins on a wrapped line C-A-B ordering irrelevant), and loss perspective */
    ttt_settings_default(&s); ttt_game_new(&g, &s, &r);
    int w[] = {2, 3, 0, 4, 1};
    play(&g, w, 5);
    CHECK(g.status == TTT_STATUS_WON && g.winner == 1 && g.win_line >= 0 && ttt_game_is_over(&g));
    CHECK(ttt_game_result_for(&g, TTT_PLAYER_1) == TTT_RESULT_WIN && ttt_game_result_for(&g, TTT_PLAYER_2) == TTT_RESULT_LOSS);
    CHECK(ttt_game_apply_move(&g, 8) == TTT_MOVE_GAME_OVER);
    CHECK(g.current_player == 1);   /* winner stays current, no turn flip after game end */

    /* Simultaneous lines: X B,C,D,G then A completes A-B-C and A-D-G; also board is full. */
    ttt_game_new(&g, &s, &r);
    int m[] = {1, 4, 2, 5, 3, 7, 6, 8};
    play(&g, m, 8);
    CHECK(g.status == TTT_STATUS_IN_PROGRESS);
    CHECK(ttt_game_apply_move(&g, 0) == TTT_MOVE_OK);
    CHECK(g.status == TTT_STATUS_WON && g.winner == 1 && ttt_board_count_lines_for(&g.board, TTT_PLAYER_1) >= 2);
    CHECK(ttt_board_is_full(&g.board));          /* win takes precedence over "full" */
    CHECK(ttt_game_result_for(&g, TTT_PLAYER_1) == TTT_RESULT_WIN);

    /* Draw branch of the status logic (see exhaustive test: unreachable by legal play) */
    TttGame d = new_default(&r);
    ttt_game_refresh_status(&d); CHECK(d.status == TTT_STATUS_IN_PROGRESS);
    CHECK(ttt_game_result_for(&d, TTT_PLAYER_1) == TTT_RESULT_NONE);
    d.status = TTT_STATUS_DRAW; CHECK(ttt_game_result_for(&d, TTT_PLAYER_1) == TTT_RESULT_DRAW);
}

/* Exhaustive enumeration of the reachable game tree. */
static unsigned char g_seen[19683];
static int g_states, g_term, g_draws;
static int key_of(const TttBoard *b) { int k = 0; for (int i = 0; i < 9; i++) k = k * 3 + b->cell[i]; return k; }
static void walk(TttBoard *b, TttPlayer p) {
    int k = key_of(b); if (g_seen[k]) return; g_seen[k] = 1; g_states++;
    if (ttt_board_winner(b, NULL) != TTT_EMPTY) { g_term++; return; }
    if (ttt_board_is_full(b)) { g_term++; g_draws++; return; }
    for (int i = 0; i < 9; i++) if (b->cell[i] == 0) { b->cell[i] = (uint8_t)p; walk(b, ttt_other_player(p)); b->cell[i] = 0; }
}
static void test_tree(void) {
    SECTION("exhaustive game tree");
    TttBoard b; ttt_board_init(&b); memset(g_seen, 0, sizeof g_seen);
    walk(&b, TTT_PLAYER_1);
    printf("  distinct reachable states=%d terminal=%d draws=%d\n", g_states, g_term, g_draws);
    CHECK(g_states == 5194 && g_term == 1314 && g_draws == 0);   /* NOTE: spec text says 6046/906 */
}

/* Independent reference minimax (win/draw/loss only, no depth). Returns +1/0/-1 for mover. */
static signed char ref_memo[19683]; static unsigned char ref_known[19683];
static int ref_value(TttBoard *b, TttPlayer mover) {
    int k = key_of(b); if (ref_known[k]) return ref_memo[k];
    int best = -2;
    for (int i = 0; i < 9; i++) if (b->cell[i] == 0) {
        b->cell[i] = (uint8_t)mover; int v;
        if (ttt_board_winner(b, NULL) == mover) v = 1;
        else if (ttt_board_is_full(b)) v = 0;
        else v = -ref_value(b, ttt_other_player(mover));
        b->cell[i] = 0; if (v > best) best = v;
    }
    ref_known[k] = 1; ref_memo[k] = (signed char)best; return best;
}
static int ref_move_class(TttBoard *b, TttPlayer me, int cell) {
    b->cell[cell] = (uint8_t)me; int v;
    if (ttt_board_winner(b, NULL) == me) v = 1; else if (ttt_board_is_full(b)) v = 0; else v = -ref_value(b, ttt_other_player(me));
    b->cell[cell] = 0; return v;
}

static int g_ai_checked, g_ai_bad;
static unsigned char g_seen2[19683];
static void check_all_positions(TttBoard *b, TttPlayer mover, TttRng *rng) {
    int k = key_of(b); if (g_seen2[k]) return; g_seen2[k] = 1;
    if (ttt_board_winner(b, NULL) != TTT_EMPTY || ttt_board_is_full(b)) return;
    int best = -2; for (int i = 0; i < 9; i++) if (b->cell[i] == 0) { int v = ref_move_class(b, mover, i); if (v > best) best = v; }
    for (int t = 0; t < 4; t++) {                 /* several random tie-breaks */
        int mv = ttt_ai_perfect_move(b, mover, rng);
        g_ai_checked++;
        if (mv < 0 || mv > 8 || b->cell[mv] != 0 || ref_move_class(b, mover, mv) != best) g_ai_bad++;
    }
    for (int i = 0; i < 9; i++) if (b->cell[i] == 0) { b->cell[i] = (uint8_t)mover; check_all_positions(b, ttt_other_player(mover), rng); b->cell[i] = 0; }
}

static void test_ai(void) {
    SECTION("AI");
    TttRng r; ttt_rng_seed(&r, 99);
    /* ---- Easy ---- */
    TttBoard b; ttt_board_init(&b);
    b.cell[0] = b.cell[1] = TTT_PLAYER_2; b.cell[3] = TTT_PLAYER_1; b.cell[4] = TTT_PLAYER_1;   /* P2 wins at C; P1 would win at F */
    for (int t = 0; t < 200; t++) CHECK(ttt_ai_easy_move(&b, TTT_PLAYER_2, &r) == 2);          /* takes win over block */
    ttt_board_init(&b); b.cell[0] = b.cell[1] = TTT_PLAYER_1; b.cell[4] = TTT_PLAYER_2;
    for (int t = 0; t < 200; t++) CHECK(ttt_ai_easy_move(&b, TTT_PLAYER_2, &r) == 2);          /* blocks */
    int hit[9] = {0}; ttt_board_init(&b); b.cell[4] = TTT_PLAYER_1;
    for (int t = 0; t < 2000; t++) { int mv = ttt_ai_easy_move(&b, TTT_PLAYER_2, &r); CHECK(mv >= 0 && mv < 9 && mv != 4); if (mv >= 0) hit[mv]++; }
    int distinct = 0; for (int i = 0; i < 9; i++) if (hit[i]) distinct++;
    CHECK(distinct > 1);                                 /* imperfect / varied */
    /* never selects occupied cells, across random games */
    for (int gme = 0; gme < 300; gme++) {
        TttBoard bb; ttt_board_init(&bb); TttPlayer p = TTT_PLAYER_1;
        while (!ttt_board_is_full(&bb) && ttt_board_winner(&bb, NULL) == TTT_EMPTY) {
            int mv = (gme & 1) ? ttt_ai_easy_move(&bb, p, &r) : ttt_ai_perfect_move(&bb, p, &r);
            CHECK(mv >= 0 && mv < 9 && bb.cell[mv] == 0); if (mv < 0) break;
            bb.cell[mv] = (uint8_t)p; p = ttt_other_player(p);
        }
    }
    CHECK(ttt_ai_easy_move(&b, TTT_PLAYER_1, &r) >= 0);
    TttBoard full; ttt_board_init(&full); for (int i = 0; i < 9; i++) full.cell[i] = (i % 2) + 1;
    CHECK(ttt_ai_easy_move(&full, TTT_PLAYER_1, &r) == -1 && ttt_ai_perfect_move(&full, TTT_PLAYER_1, &r) == -1);

    /* ---- Perfect ---- */
    ttt_board_init(&b); b.cell[0] = b.cell[1] = TTT_PLAYER_1; b.cell[3] = b.cell[4] = TTT_PLAYER_2;   /* forced win at C */
    CHECK(ttt_ai_perfect_move(&b, TTT_PLAYER_1, &r) == 2);
    ttt_board_init(&b); b.cell[0] = b.cell[1] = TTT_PLAYER_2; b.cell[4] = TTT_PLAYER_1;               /* must block C */
    for (int t = 0; t < 50; t++) CHECK(ttt_ai_perfect_move(&b, TTT_PLAYER_1, &r) == 2);
    /* Opening: all nine moves equal -> varies */
    ttt_board_init(&b); memset(hit, 0, sizeof hit);
    for (int t = 0; t < 500; t++) hit[ttt_ai_perfect_move(&b, TTT_PLAYER_1, &r)]++;
    distinct = 0; for (int i = 0; i < 9; i++) if (hit[i]) distinct++;
    CHECK(distinct == 9);
    /* Exhaustive: never an inferior move (vs an independent reference minimax) from every reachable position */
    memset(ref_known, 0, sizeof ref_known); memset(g_seen2, 0, sizeof g_seen2);
    ttt_board_init(&b); check_all_positions(&b, TTT_PLAYER_1, &r);
    printf("  perfect AI checked %d choices, %d inferior\n", g_ai_checked, g_ai_bad);
    CHECK(g_ai_checked > 10000 && g_ai_bad == 0);
    /* Perfect vs Perfect: first player wins on this board (no draws exist) */
    TttSettings s; ttt_settings_default(&s); s.difficulty = TTT_DIFF_PERFECT;
    ttt_settings_set_mode(&s, TTT_MODE_CVC, TTT_PLAYER_1);
    for (int gm = 0; gm < 20; gm++) {
        TttGame g; ttt_game_new(&g, &s, &r);
        while (!ttt_game_is_over(&g)) CHECK(ttt_ai_play(&g, &r) == TTT_MOVE_OK);
        CHECK(g.status == TTT_STATUS_WON && g.winner == 1);
        CHECK(ttt_ai_choose_move(&g, &r) == -1);
    }
    /* Perfect never loses to Easy as second player... (sanity) and always beats or ties */
    int perfect_losses = 0;
    for (int gm = 0; gm < 200; gm++) {
        TttSettings t; ttt_settings_default(&t); ttt_settings_set_mode(&t, TTT_MODE_CVC, TTT_PLAYER_1);
        TttGame g; ttt_game_new(&g, &t, &r);
        TttPlayer perfect = (gm & 1) ? TTT_PLAYER_2 : TTT_PLAYER_1;
        while (!ttt_game_is_over(&g)) {
            int mv = (g.current_player == perfect) ? ttt_ai_perfect_move(&g.board, perfect, &r) : ttt_ai_easy_move(&g.board, ttt_other_player(perfect), &r);
            CHECK(ttt_game_apply_move(&g, mv) == TTT_MOVE_OK);
        }
        if (perfect == TTT_PLAYER_1 && g.winner != 1) perfect_losses++;   /* first mover must always win */
    }
    CHECK(perfect_losses == 0);
}

/* ---- in-memory storage ---- */
typedef struct { uint8_t data[8][64]; int len[8]; } Mem;
static int m_read(void *c, uint32_t k, void *buf, size_t n) { Mem *m = c; if (k > 7 || m->len[k] <= 0) return -1; size_t l = (size_t)m->len[k] < n ? (size_t)m->len[k] : n; memcpy(buf, m->data[k], l); return (int)l; }
static bool m_write(void *c, uint32_t k, const void *buf, size_t n) { Mem *m = c; if (k > 7 || n > 64) return false; memcpy(m->data[k], buf, n); m->len[k] = (int)n; return true; }
static void m_remove(void *c, uint32_t k) { Mem *m = c; if (k <= 7) m->len[k] = 0; }
static bool m_exists(void *c, uint32_t k) { Mem *m = c; return k <= 7 && m->len[k] > 0; }

static void test_persistence(void) {
    SECTION("persistence");
    Mem mem; memset(&mem, 0, sizeof mem);
    TttStorage st = { &mem, m_read, m_write, m_remove, m_exists };
    TttRng r; ttt_rng_seed(&r, 5);
    TttSettings s; ttt_settings_default(&s);
    s.first_player_setting = TTT_FIRST_RANDOM; s.difficulty = TTT_DIFF_PERFECT; s.board_view = TTT_VIEW_FLAT; s.glyph[0] = 'O'; s.glyph[1] = 'X';
    TttSettings ls; CHECK(ttt_load_settings(&st, &ls) == TTT_LOAD_NONE && ls.difficulty == TTT_DIFF_EASY);   /* defaults */
    CHECK(ttt_save_settings(&st, &s) && ttt_load_settings(&st, &ls) == TTT_LOAD_OK && memcmp(&s, &ls, sizeof s) == 0);

    CHECK(!ttt_has_active_game(&st));
    TttGame g; ttt_game_new(&g, &s, &r);
    int first = g.first_player;
    ttt_game_apply_move(&g, 4); ttt_game_apply_move(&g, 0);
    CHECK(ttt_save_game(&st, &g));
    TttGame lg; TttRng r2; ttt_rng_seed(&r2, 777);
    uint32_t before = r2.s;
    CHECK(ttt_load_game(&st, &lg) == TTT_LOAD_OK);
    CHECK(r2.s == before);                                 /* loading never touches RNG */
    CHECK(lg.first_player == first && lg.current_player == g.current_player && lg.move_count == 2);
    CHECK(memcmp(lg.board.cell, g.board.cell, 9) == 0 && lg.glyph[0] == 'O' && lg.board_view == TTT_VIEW_FLAT && lg.difficulty == TTT_DIFF_PERFECT);
    CHECK(ttt_has_active_game(&st));

    /* Bad data: corruption, wrong version, wrong length are rejected and cleared */
    uint8_t blob[TTT_GAME_BLOB_SIZE]; ttt_game_serialize(&g, blob);
    uint8_t bad[TTT_GAME_BLOB_SIZE]; memcpy(bad, blob, sizeof bad); bad[3] ^= 1;
    CHECK(ttt_game_deserialize(&lg, bad, sizeof bad) == TTT_LOAD_CORRUPT);
    memcpy(bad, blob, sizeof bad); bad[0] = 99;
    CHECK(ttt_game_deserialize(&lg, bad, sizeof bad) == TTT_LOAD_VERSION);
    CHECK(ttt_game_deserialize(&lg, blob, 5) == TTT_LOAD_CORRUPT);
    m_write(&mem, TTT_KEY_GAME, bad, sizeof bad);
    CHECK(ttt_load_game(&st, &lg) == TTT_LOAD_VERSION && !m_exists(&mem, TTT_KEY_GAME));

    /* Completed games: record result, update stats, remove active game, never resumable */
    TttSettings hs; ttt_settings_default(&hs);
    TttGame wg; ttt_game_new(&wg, &hs, &r);
    ttt_save_game(&st, &wg);
    int win[] = {2, 3, 0, 4, 1}; for (int i = 0; i < 5; i++) ttt_game_apply_move(&wg, win[i]);
    CHECK(wg.status == TTT_STATUS_WON);
    TttStats stats; ttt_load_stats(&st, &stats); CHECK(stats.wins == 0);
    CHECK(ttt_complete_game(&st, &wg, &stats) == TTT_RESULT_WIN);
    CHECK(!ttt_has_active_game(&st));
    TttStats ls2; CHECK(ttt_load_stats(&st, &ls2) == TTT_LOAD_OK && ls2.wins == 1 && ls2.losses == 0);
    CHECK(ttt_save_game(&st, &wg) && !m_exists(&mem, TTT_KEY_GAME));   /* completed games are never stored */
    /* a completed-game blob is rejected on load */
    uint8_t cb[TTT_GAME_BLOB_SIZE]; ttt_game_serialize(&wg, cb); CHECK(ttt_game_deserialize(&lg, cb, sizeof cb) == TTT_LOAD_CORRUPT);
    /* loss + draw bookkeeping */
    TttGame lossg; ttt_game_new(&lossg, &hs, &r);
    int lo[] = {0, 2, 3, 5, 8, 1, 4}; /* P1 0,3,8,4 ; P2 2,5,1 -> P2 has 1,2,... check */
    for (int i = 0; i < 7 && !ttt_game_is_over(&lossg); i++) ttt_game_apply_move(&lossg, lo[i]);
    ttt_stats_record(&stats, TTT_RESULT_LOSS); ttt_stats_record(&stats, TTT_RESULT_DRAW);
    CHECK(stats.losses == 1 && stats.draws == 1);
    /* HvH: no stats change */
    TttSettings hh = hs; ttt_settings_set_mode(&hh, TTT_MODE_HVH, TTT_PLAYER_1);
    TttGame hg; ttt_game_new(&hg, &hh, &r); for (int i = 0; i < 5; i++) ttt_game_apply_move(&hg, win[i]);
    TttStats before_s = stats; CHECK(ttt_complete_game(&st, &hg, &stats) == TTT_RESULT_NONE && memcmp(&before_s, &stats, sizeof stats) == 0);
    /* Stats round trip */
    uint8_t sb[TTT_STATS_BLOB_SIZE]; ttt_stats_serialize(&stats, sb); TttStats rs;
    CHECK(ttt_stats_deserialize(&rs, sb, sizeof sb) == TTT_LOAD_OK && rs.wins == stats.wins && rs.losses == 1 && rs.draws == 1);
    /* Settings validity */
    TttSettings bs = s; bs.glyph[1] = bs.glyph[0]; CHECK(!ttt_settings_valid(&bs));
}

int main(void) {
    test_board(); test_lines(); test_near_misses(); test_game(); test_tree(); test_ai(); test_persistence();
    printf("\n%d checks, %d failed\n", g_run, g_fail);
    return g_fail ? 1 : 0;
}
