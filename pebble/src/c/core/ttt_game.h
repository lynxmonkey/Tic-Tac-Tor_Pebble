/* Game state, turn management, settings. Platform independent. */
#ifndef TTT_GAME_H
#define TTT_GAME_H
#include "ttt_board.h"

typedef enum { TTT_HUMAN = 0, TTT_COMPUTER = 1 } TttParticipant;
typedef enum { TTT_MODE_HVC = 0, TTT_MODE_HVH = 1, TTT_MODE_CVC = 2 } TttMode;
typedef enum { TTT_DIFF_EASY = 0, TTT_DIFF_PERFECT = 1 } TttDifficulty;
typedef enum { TTT_FIRST_P1 = 0, TTT_FIRST_P2 = 1, TTT_FIRST_RANDOM = 2 } TttFirstSetting;
typedef enum { TTT_VIEW_TORUS = 0, TTT_VIEW_FLAT = 1 } TttBoardView;
typedef enum { TTT_STATUS_IN_PROGRESS = 0, TTT_STATUS_WON = 1, TTT_STATUS_DRAW = 2 } TttStatus;
typedef enum { TTT_RESULT_NONE = 0, TTT_RESULT_WIN, TTT_RESULT_LOSS, TTT_RESULT_DRAW } TttResult;
typedef enum { TTT_MOVE_OK = 0, TTT_MOVE_BAD_CELL, TTT_MOVE_OCCUPIED, TTT_MOVE_GAME_OVER } TttMoveResult;

/* Abstract input (platform maps physical controls to these). */
typedef enum { TTT_ACTION_NEXT = 0, TTT_ACTION_PREVIOUS, TTT_ACTION_SELECT, TTT_ACTION_BACK } TttAction;

/* Small deterministic RNG (xorshift32); platform supplies the seed. */
typedef struct { uint32_t s; } TttRng;
void     ttt_rng_seed(TttRng *r, uint32_t seed);
uint32_t ttt_rng_next(TttRng *r);
uint32_t ttt_rng_range(TttRng *r, uint32_t n);   /* 0..n-1 */

/* Persistent defaults for new games. */
typedef struct {
    uint8_t mode;                 /* TttMode */
    uint8_t participant[2];       /* [0]=Player 1, [1]=Player 2 */
    uint8_t glyph[2];             /* printable char per player */
    uint8_t first_player_setting; /* TttFirstSetting */
    uint8_t difficulty;           /* TttDifficulty */
    uint8_t board_view;           /* TttBoardView */
} TttSettings;

/* Authoritative active game. Selection and camera are NOT stored here. */
typedef struct {
    TttBoard board;
    uint8_t current_player;       /* TttPlayer 1|2 */
    uint8_t status;               /* TttStatus */
    uint8_t winner;               /* TttPlayer, 0 if none */
    uint8_t mode;
    uint8_t participant[2];
    uint8_t glyph[2];
    uint8_t difficulty;
    uint8_t first_player;         /* resolved: 1|2 (never Random) */
    uint8_t board_view;
    uint8_t move_count;
    int8_t  win_line;             /* index into ttt_lines(), -1 none */
} TttGame;

void ttt_settings_default(TttSettings *s);
bool ttt_settings_valid(const TttSettings *s);
TttMode ttt_mode_from_participants(uint8_t p1, uint8_t p2);
/* human_player is used for Human-vs-Computer only. */
void ttt_settings_set_mode(TttSettings *s, TttMode mode, TttPlayer human_player);

/* Random first player is resolved here, once, and stored in the game. */
void ttt_game_new(TttGame *g, const TttSettings *s, TttRng *rng);
TttMoveResult ttt_game_apply_move(TttGame *g, int cell);
bool ttt_game_is_over(const TttGame *g);
TttParticipant ttt_game_current_participant(const TttGame *g);
char ttt_game_glyph(const TttGame *g, TttPlayer p);
TttResult ttt_game_result_for(const TttGame *g, TttPlayer p);
/* Player role whose perspective statistics use: the single human in a
 * Human-vs-Computer game, else TTT_EMPTY (no stats for HvH / CvC). */
TttPlayer ttt_game_stats_player(const TttGame *g);
/* Re-derive status/winner/win_line from the board (used after load). */
void ttt_game_refresh_status(TttGame *g);

#endif
