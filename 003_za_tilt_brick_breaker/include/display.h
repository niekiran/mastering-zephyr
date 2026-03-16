#ifndef INCLUDE_DISPLAY_H_
#define INCLUDE_DISPLAY_H_

#include <stdint.h>
#include <stdbool.h>
#include "game_types.h"

/* Init — call once in main() */
int  display_module_init(void);

/* Primitives */
void draw_pixel(int x, int y, uint16_t color);
void draw_hline(int x, int y, int len, uint16_t color);
void fill_rect(int x, int y, int w, int h, uint16_t color);
void draw_rect_outline(int x, int y, int w, int h, uint16_t color);
void fill_circle(int cx, int cy, int r, uint16_t color);

/* Text (scale=1 -> 5x7 px, scale=2 -> 10x14 px, etc.) */
void draw_char(int x, int y, char c, uint16_t fg, uint16_t bg, uint8_t scale);
void draw_string(int x, int y, const char *s, uint16_t fg, uint16_t bg,
                 uint8_t scale);
void draw_int(int x, int y, int val, uint16_t fg, uint16_t bg, uint8_t scale);
void draw_int_right_aligned(int x_right, int y, int val, uint16_t fg,
                            uint16_t bg, uint8_t scale);

/* Screens */
void screen_clear(uint16_t color);
void screen_draw_startup(void);
void screen_draw_countdown(int n);
void screen_draw_gameplay(const uint8_t bricks[BRICK_ROWS][BRICK_COLS],
                          int paddle_x, int ball_x, int ball_y,
                          int score, int lives);
void screen_draw_end(bool won, int score);

/* Targeted HUD updates — no full-band redraw needed since ball never
 * enters the HUD area (bounces at WALL_TOP = HUD_HEIGHT). */
void screen_update_score(int score);
void screen_erase_heart(int lives_remaining);

/* Brick color for a given row (needed by main.c for brick repair) */
uint16_t gameplay_brick_color(int row);

#endif /* INCLUDE_DISPLAY_H_ */
