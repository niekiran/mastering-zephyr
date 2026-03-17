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
#define PADDLE_W        40
#define PADDLE_H        10
#define PADDLE_Y        210
#define PADDLE_X_INIT   100 /* (240 - 40) / 2 */

/* Paddle X bounds: circular display narrows at PADDLE_Y.
 * Circle (120,120) r=120, at y=210: dx=floor(sqrt(120²-90²))=79.
 * Visible x: [41, 199]. Paddle (w=40) fits in [41, 160]. */
#if defined(CONFIG_DISPLAY_IS_CIRCULAR) && CONFIG_DISPLAY_IS_CIRCULAR
#define PADDLE_X_MIN    41
#define PADDLE_X_MAX    160
#else
#define PADDLE_X_MIN    0
#define PADDLE_X_MAX    (CONFIG_DISPLAY_WIDTH - PADDLE_W)
#endif

/* ---- Ball ---- */
#define BALL_RADIUS     6
#define BALL_X_INIT     120
#define BALL_Y_INIT     (PADDLE_Y - BALL_RADIUS)  /* ball rests on paddle */

/* ---- Circular display geometry (from Kconfig, when enabled) ---- */
#if defined(CONFIG_DISPLAY_IS_CIRCULAR) && CONFIG_DISPLAY_IS_CIRCULAR
#define DISPLAY_CX      CONFIG_DISPLAY_CIRCLE_CENTER_X
#define DISPLAY_CY      CONFIG_DISPLAY_CIRCLE_CENTER_Y
#define DISPLAY_R       CONFIG_DISPLAY_CIRCLE_RADIUS
#endif

/* ---- HUD ---- */
#define HUD_HEIGHT      25

/* ---- Bounce boundary — ball bounces at bottom edge of HUD ---- */
#define WALL_TOP         HUD_HEIGHT

/* ---- Brick state ---- */
typedef uint8_t brick_state_t; /* 0 = destroyed, 1 = active */

#endif /* INCLUDE_GAME_TYPES_H_ */
