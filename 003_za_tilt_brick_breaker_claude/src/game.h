#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <stdbool.h>

/* Screen */
#define SCREEN_W  240
#define SCREEN_H  240

/* Score bar (top strip)
 * 1.28" GC9A01 round display: at y=28 the circle is ~154 px wide (x=43..197),
 * wide enough for "SCORE XXXX" + life dots without hitting the circular edge. */
#define SCORE_BAR_H  46

/* Paddle: right side, moves vertically.
 * At y=46 (top of travel) the circle right-edge is x≈209.
 * PADDLE_X=200, PADDLE_W=8 → rightmost pixel x=207 < 209. Always visible. */
#define PADDLE_W        8
#define PADDLE_H        50
#define PADDLE_X        200
#define PADDLE_INIT_Y   ((SCREEN_H / 2) - (PADDLE_H / 2))

/* Ball */
#define BALL_W    8
#define BALL_H    8
#define BALL_INIT_X  (PADDLE_X - BALL_W - 2)
#define BALL_INIT_Y  (PADDLE_INIT_Y + (PADDLE_H / 2) - (BALL_H / 2))
#define BALL_INIT_VX (-3)
#define BALL_INIT_VY   2

/* Bricks: left side, 5 rows x 4 cols.
 * BRICK_LEFT_X=30: at y=46 the circle left-edge is x≈30 — just inside.
 * BRICK_ROWS=5: bottom of last row is y≈212 where circle is still ~162 px wide;
 *   row 6 (old y=206–240) had <100 px visible width — removed to avoid clipping. */
#define BRICK_COLS     4
#define BRICK_ROWS     5
#define BRICK_W        28
#define BRICK_H        30
#define BRICK_GAP      4
#define BRICK_TOP_Y    SCORE_BAR_H
#define BRICK_LEFT_X   30

/* Scoring */
#define SCORE_PER_BRICK  10
#define INIT_LIVES       3

typedef enum {
    GAME_READY,
    GAME_PLAYING,
    GAME_LIFE_LOST,
    GAME_OVER,
    GAME_WIN,
} GameStateEnum;

typedef enum {
    EV_NONE,
    EV_BRICK_HIT,    /* brick_col / brick_row set in GameState */
    EV_SCORE_CHANGED,
    EV_LIFE_LOST,
    EV_GAME_OVER,
    EV_GAME_WIN,
} GameEvent;

typedef struct {
    int16_t x, y;
    int16_t vx, vy;
} Ball;

typedef struct {
    int16_t y;   /* vertical position; x is fixed at PADDLE_X */
} Paddle;

typedef struct {
    bool active;
} Brick;

typedef struct {
    Ball          ball;
    Paddle        paddle;
    Brick         bricks[BRICK_ROWS][BRICK_COLS];
    uint8_t       lives;
    uint16_t      score;
    GameStateEnum state;

    /* Set by game_update() when EV_BRICK_HIT */
    int8_t last_brick_row;
    int8_t last_brick_col;
} GameState;

/* Public API */
void       game_init(GameState *gs);
GameEvent  game_update(GameState *gs, int8_t paddle_delta);

#endif /* GAME_H */
