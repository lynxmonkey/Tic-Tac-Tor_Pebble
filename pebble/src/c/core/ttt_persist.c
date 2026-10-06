#include "ttt_persist.h"
#include <string.h>

static uint8_t checksum(const uint8_t *p, size_t n) { uint8_t c = 0x5A; while (n--) c = (uint8_t)((c << 1 | c >> 7) ^ *p++); return c; }
static void put32(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }
static uint32_t get32(const uint8_t *p) { return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }

/* Migration hook: add cases here when TTT_STORAGE_VERSION increases. */
static TttLoadStatus check_header(const uint8_t *buf, size_t len, size_t expect) {
    if (!buf || len < 1) return TTT_LOAD_CORRUPT;
    if (buf[0] != TTT_STORAGE_VERSION) return TTT_LOAD_VERSION;
    if (len != expect) return TTT_LOAD_CORRUPT;
    if (checksum(buf, len - 1) != buf[len - 1]) return TTT_LOAD_CORRUPT;
    return TTT_LOAD_OK;
}

void ttt_stats_default(TttStats *s) { s->wins = s->losses = s->draws = 0; }
void ttt_stats_record(TttStats *s, TttResult r) {
    if (r == TTT_RESULT_WIN) s->wins++; else if (r == TTT_RESULT_LOSS) s->losses++; else if (r == TTT_RESULT_DRAW) s->draws++;
}

size_t ttt_game_serialize(const TttGame *g, uint8_t o[TTT_GAME_BLOB_SIZE]) {
    o[0] = TTT_STORAGE_VERSION;
    memcpy(&o[1], g->board.cell, 9);
    o[10] = g->current_player; o[11] = g->status; o[12] = g->winner; o[13] = g->mode;
    o[14] = g->participant[0]; o[15] = g->participant[1]; o[16] = g->glyph[0]; o[17] = g->glyph[1];
    o[18] = g->difficulty; o[19] = g->first_player; o[20] = g->board_view; o[21] = g->move_count;
    o[22] = checksum(o, 22);
    return TTT_GAME_BLOB_SIZE;
}

TttLoadStatus ttt_game_deserialize(TttGame *out, const uint8_t *b, size_t len) {
    TttLoadStatus st = check_header(b, len, TTT_GAME_BLOB_SIZE);
    if (st != TTT_LOAD_OK) return st;
    TttGame g; memset(&g, 0, sizeof g);
    memcpy(g.board.cell, &b[1], 9);
    for (int i = 0; i < 9; i++) if (g.board.cell[i] > 2) return TTT_LOAD_CORRUPT;
    g.current_player = b[10]; g.status = b[11]; g.winner = b[12]; g.mode = b[13];
    g.participant[0] = b[14]; g.participant[1] = b[15]; g.glyph[0] = b[16]; g.glyph[1] = b[17];
    g.difficulty = b[18]; g.first_player = b[19]; g.board_view = b[20]; g.move_count = b[21];
    if (g.current_player < 1 || g.current_player > 2 || g.first_player < 1 || g.first_player > 2) return TTT_LOAD_CORRUPT;
    if (g.participant[0] > 1 || g.participant[1] > 1) return TTT_LOAD_CORRUPT;
    if (g.mode != ttt_mode_from_participants(g.participant[0], g.participant[1])) return TTT_LOAD_CORRUPT;
    for (int i = 0; i < 2; i++) if (g.glyph[i] < 0x21 || g.glyph[i] > 0x7E) return TTT_LOAD_CORRUPT;
    if (g.glyph[0] == g.glyph[1] || g.difficulty > 1 || g.board_view > 1) return TTT_LOAD_CORRUPT;
    /* Only in-progress games are resumable. */
    if (g.status != TTT_STATUS_IN_PROGRESS || g.winner != 0) return TTT_LOAD_CORRUPT;
    int n = 9 - ttt_board_count_empty(&g.board);
    if (g.move_count != n || n >= 9) return TTT_LOAD_CORRUPT;
    if (ttt_board_winner(&g.board, NULL) != TTT_EMPTY) return TTT_LOAD_CORRUPT;
    /* Turn consistency: first player has ceil(n/2) marks, and it is their turn iff n is even. */
    int first_marks = 0; for (int i = 0; i < 9; i++) if (g.board.cell[i] == g.first_player) first_marks++;
    if (first_marks != (n + 1) / 2) return TTT_LOAD_CORRUPT;
    uint8_t expect = (n % 2 == 0) ? g.first_player : (uint8_t)ttt_other_player((TttPlayer)g.first_player);
    if (g.current_player != expect) return TTT_LOAD_CORRUPT;
    g.win_line = -1;
    *out = g;
    return TTT_LOAD_OK;
}

size_t ttt_settings_serialize(const TttSettings *s, uint8_t o[TTT_SETTINGS_BLOB_SIZE]) {
    o[0] = TTT_STORAGE_VERSION; o[1] = s->mode; o[2] = s->participant[0]; o[3] = s->participant[1];
    o[4] = s->glyph[0]; o[5] = s->glyph[1]; o[6] = s->first_player_setting; o[7] = s->difficulty; o[8] = s->board_view;
    o[9] = checksum(o, 9);
    return TTT_SETTINGS_BLOB_SIZE;
}
TttLoadStatus ttt_settings_deserialize(TttSettings *out, const uint8_t *b, size_t len) {
    TttLoadStatus st = check_header(b, len, TTT_SETTINGS_BLOB_SIZE);
    if (st != TTT_LOAD_OK) return st;
    TttSettings s;
    s.mode = b[1]; s.participant[0] = b[2]; s.participant[1] = b[3]; s.glyph[0] = b[4]; s.glyph[1] = b[5];
    s.first_player_setting = b[6]; s.difficulty = b[7]; s.board_view = b[8];
    if (!ttt_settings_valid(&s)) return TTT_LOAD_CORRUPT;
    *out = s; return TTT_LOAD_OK;
}

size_t ttt_stats_serialize(const TttStats *s, uint8_t o[TTT_STATS_BLOB_SIZE]) {
    o[0] = TTT_STORAGE_VERSION; put32(&o[1], s->wins); put32(&o[5], s->losses); put32(&o[9], s->draws);
    o[13] = checksum(o, 13);
    return TTT_STATS_BLOB_SIZE;
}
TttLoadStatus ttt_stats_deserialize(TttStats *out, const uint8_t *b, size_t len) {
    TttLoadStatus st = check_header(b, len, TTT_STATS_BLOB_SIZE);
    if (st != TTT_LOAD_OK) return st;
    out->wins = get32(&b[1]); out->losses = get32(&b[5]); out->draws = get32(&b[9]);
    return TTT_LOAD_OK;
}

static TttLoadStatus read_blob(const TttStorage *st, uint32_t key, uint8_t *buf, size_t cap, size_t *len) {
    if (!st->exists(st->ctx, key)) return TTT_LOAD_NONE;
    int n = st->read(st->ctx, key, buf, cap);
    if (n <= 0) return TTT_LOAD_CORRUPT;
    *len = (size_t)n; return TTT_LOAD_OK;
}

bool ttt_save_game(const TttStorage *st, const TttGame *g) {
    if (ttt_game_is_over(g)) { ttt_clear_game(st); return true; }
    uint8_t buf[TTT_GAME_BLOB_SIZE]; size_t n = ttt_game_serialize(g, buf);
    return st->write(st->ctx, TTT_KEY_GAME, buf, n);
}
TttLoadStatus ttt_load_game(const TttStorage *st, TttGame *g) {
    uint8_t buf[TTT_GAME_BLOB_SIZE + 8]; size_t n = 0;
    TttLoadStatus r = read_blob(st, TTT_KEY_GAME, buf, sizeof buf, &n);
    if (r != TTT_LOAD_OK) return r;
    r = ttt_game_deserialize(g, buf, n);
    if (r != TTT_LOAD_OK) st->remove(st->ctx, TTT_KEY_GAME);   /* never offer an unusable Continue */
    return r;
}
void ttt_clear_game(const TttStorage *st) { st->remove(st->ctx, TTT_KEY_GAME); }
bool ttt_has_active_game(const TttStorage *st) { TttGame g; return ttt_load_game(st, &g) == TTT_LOAD_OK; }

bool ttt_save_settings(const TttStorage *st, const TttSettings *s) {
    uint8_t buf[TTT_SETTINGS_BLOB_SIZE]; size_t n = ttt_settings_serialize(s, buf);
    return st->write(st->ctx, TTT_KEY_SETTINGS, buf, n);
}
TttLoadStatus ttt_load_settings(const TttStorage *st, TttSettings *s) {
    uint8_t buf[TTT_SETTINGS_BLOB_SIZE + 8]; size_t n = 0;
    TttLoadStatus r = read_blob(st, TTT_KEY_SETTINGS, buf, sizeof buf, &n);
    if (r == TTT_LOAD_OK) r = ttt_settings_deserialize(s, buf, n);
    if (r != TTT_LOAD_OK) ttt_settings_default(s);
    return r;
}
bool ttt_save_stats(const TttStorage *st, const TttStats *s) {
    uint8_t buf[TTT_STATS_BLOB_SIZE]; size_t n = ttt_stats_serialize(s, buf);
    return st->write(st->ctx, TTT_KEY_STATS, buf, n);
}
TttLoadStatus ttt_load_stats(const TttStorage *st, TttStats *s) {
    uint8_t buf[TTT_STATS_BLOB_SIZE + 8]; size_t n = 0;
    TttLoadStatus r = read_blob(st, TTT_KEY_STATS, buf, sizeof buf, &n);
    if (r == TTT_LOAD_OK) r = ttt_stats_deserialize(s, buf, n);
    if (r != TTT_LOAD_OK) ttt_stats_default(s);
    return r;
}

TttResult ttt_complete_game(const TttStorage *st, const TttGame *g, TttStats *stats) {
    if (!ttt_game_is_over(g)) return TTT_RESULT_NONE;
    TttPlayer who = ttt_game_stats_player(g);
    TttResult r = TTT_RESULT_NONE;
    if (who != TTT_EMPTY) {
        r = ttt_game_result_for(g, who);
        ttt_stats_record(stats, r);
        ttt_save_stats(st, stats);
    }
    ttt_clear_game(st);
    return r;
}
