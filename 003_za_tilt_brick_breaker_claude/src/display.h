#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include "game.h"

void display_init(void);
void display_clear_screen(void);
void display_draw_all_bricks(const Brick bricks[BRICK_ROWS][BRICK_COLS]);
void display_draw_brick(int row, int col, bool active);
void display_draw_paddle(int16_t y, int16_t old_y);
void display_draw_ball(int16_t x, int16_t y, int16_t old_x, int16_t old_y);
void display_draw_score(uint16_t score, uint8_t lives);
void display_draw_message(const char *msg);
void display_draw_countdown(const char *s);
void display_draw_start_screen(void);
void display_draw_end_screen(bool won, uint16_t score);

#endif /* DISPLAY_H */
