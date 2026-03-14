#ifndef INCLUDE_GAME_TYPES_H_
#define INCLUDE_GAME_TYPES_H_

#include <stdint.h>

/* ---- RGB565 colour palette (standard: R[15:11] G[10:5] B[4:0]) ----
 * Byte-swapping for the big-endian LCD happens inside commit_line(). */
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_DARK_BLUE 0x0008
#define COLOR_CYAN      0x07FF
#define COLOR_YELLOW    0xFFE0
#define COLOR_RED       0xF800
#define COLOR_GREEN     0x07E0
#define COLOR_ORANGE    0xFD20
#define COLOR_BLUE      0x001F
#define COLOR_DARK_GREEN 0x0320
#define COLOR_DARK_RED   0x4000

/* ---- Brick grid layout ---- */
#define BRICK_COLS      8
#define BRICK_ROWS      5
#define BRICK_W         26
#define BRICK_H         14
#define BRICK_GAP       2
#define BRICK_ORIGIN_X  9
#define BRICK_ORIGIN_Y  30
#define BRICK_X(c)      (BRICK_ORIGIN_X + (c) * (BRICK_W + BRICK_GAP))
#define BRICK_Y(r)      (BRICK_ORIGIN_Y + (r) * (BRICK_H + BRICK_GAP))

/* ---- Paddle ---- */
#define PADDLE_W        60
#define PADDLE_H        10
#define PADDLE_Y        210
#define PADDLE_X_INIT   90  /* (240 - 60) / 2 */

/* ---- Ball ---- */
#define BALL_RADIUS     6
#define BALL_X_INIT     120
#define BALL_Y_INIT     170

/* ---- HUD ---- */
#define HUD_HEIGHT      25

/* ---- Brick state ---- */
typedef uint8_t brick_state_t; /* 0 = destroyed, 1 = active */

#endif /* INCLUDE_GAME_TYPES_H_ */
