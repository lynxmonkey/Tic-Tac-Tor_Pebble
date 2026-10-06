/* Persistence abstraction + versioned serialisation. Platform independent:
 * a platform supplies a TttStorage (Pebble: persist_*). */
#ifndef TTT_PERSIST_H
#define TTT_PERSIST_H
#include <stddef.h>
#include "ttt_game.h"

#define TTT_STORAGE_VERSION 1
#define TTT_KEY_GAME     1
#define TTT_KEY_SETTINGS 2
#define TTT_KEY_STATS    3

#define TTT_GAME_BLOB_SIZE     23
#define TTT_SETTINGS_BLOB_SIZE 10
#define TTT_STATS_BLOB_SIZE    14

typedef struct { uint32_t wins, losses, draws; } TttStats;

typedef struct TttStorage {
    void *ctx;
    int  (*read)(void *ctx, uint32_t key, void *buf, size_t len);   /* bytes read, <0 if absent */
    bool (*write)(void *ctx, uint32_t key, const void *buf, size_t len);
    void (*remove)(void *ctx, uint32_t key);
    bool (*exists)(void *ctx, uint32_t key);
} TttStorage;

typedef enum { TTT_LOAD_OK = 0, TTT_LOAD_NONE, TTT_LOAD_CORRUPT, TTT_LOAD_VERSION } TttLoadStatus;

void ttt_stats_default(TttStats *s);
void ttt_stats_record(TttStats *s, TttResult r);

/* Raw (de)serialisation, exposed for tests. */
size_t ttt_game_serialize(const TttGame *g, uint8_t out[TTT_GAME_BLOB_SIZE]);
TttLoadStatus ttt_game_deserialize(TttGame *g, const uint8_t *buf, size_t len);
size_t ttt_settings_serialize(const TttSettings *s, uint8_t out[TTT_SETTINGS_BLOB_SIZE]);
TttLoadStatus ttt_settings_deserialize(TttSettings *s, const uint8_t *buf, size_t len);
size_t ttt_stats_serialize(const TttStats *s, uint8_t out[TTT_STATS_BLOB_SIZE]);
TttLoadStatus ttt_stats_deserialize(TttStats *s, const uint8_t *buf, size_t len);

/* Storage-level API. On anything but TTT_LOAD_OK the output is set to defaults
 * (settings/stats) or left untouched (game). Unknown versions are never trusted. */
bool ttt_save_game(const TttStorage *st, const TttGame *g);  /* completed game => record removed */
TttLoadStatus ttt_load_game(const TttStorage *st, TttGame *g);
void ttt_clear_game(const TttStorage *st);
bool ttt_has_active_game(const TttStorage *st);
bool ttt_save_settings(const TttStorage *st, const TttSettings *s);
TttLoadStatus ttt_load_settings(const TttStorage *st, TttSettings *s);
bool ttt_save_stats(const TttStorage *st, const TttStats *s);
TttLoadStatus ttt_load_stats(const TttStorage *st, TttStats *s);

/* Terminal-state handling: updates+saves stats (when a single human exists)
 * and removes the active-game record. Returns result from the stats player's
 * perspective (TTT_RESULT_NONE if the game is not over). */
TttResult ttt_complete_game(const TttStorage *st, const TttGame *g, TttStats *stats);

#endif
