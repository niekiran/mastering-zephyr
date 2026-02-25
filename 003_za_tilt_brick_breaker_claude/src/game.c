#include "game.h"
#include <string.h>

static void reset_ball_and_paddle(GameState *gs)
{
    gs->ball.x  = BALL_INIT_X;
    gs->ball.y  = BALL_INIT_Y;
    gs->ball.vx = BALL_INIT_VX;
    gs->ball.vy = BALL_INIT_VY;
    gs->paddle.y = PADDLE_INIT_Y;
}

static bool all_bricks_cleared(const GameState *gs)
{
    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            if (gs->bricks[r][c].active) {
                return false;
            }
        }
    }
    return true;
}

void game_init(GameState *gs)
{
    memset(gs, 0, sizeof(*gs));
    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            gs->bricks[r][c].active = true;
        }
    }
    gs->lives = INIT_LIVES;
    gs->score = 0;
    gs->state = GAME_PLAYING;
    gs->last_brick_row = -1;
    gs->last_brick_col = -1;
    reset_ball_and_paddle(gs);
}

GameEvent game_update(GameState *gs, int8_t paddle_delta)
{
    if (gs->state == GAME_OVER || gs->state == GAME_WIN) {
        return EV_NONE;
    }

    /* --- Move paddle (up/down) --- */
    gs->paddle.y += paddle_delta;
    if (gs->paddle.y < SCORE_BAR_H) {
        gs->paddle.y = SCORE_BAR_H;
    }
    if (gs->paddle.y > SCREEN_H - PADDLE_H) {
        gs->paddle.y = SCREEN_H - PADDLE_H;
    }

    /* --- Move ball --- */
    gs->ball.x += gs->ball.vx;
    gs->ball.y += gs->ball.vy;

    /* --- Wall collisions (left, top, bottom) --- */
    if (gs->ball.x <= 0) {
        gs->ball.x = 0;
        gs->ball.vx = -gs->ball.vx;
    }
    /* Top wall is the bottom edge of the score bar, not the raw screen top */
    if (gs->ball.y <= SCORE_BAR_H) {
        gs->ball.y = SCORE_BAR_H;
        gs->ball.vy = -gs->ball.vy;
    }
    if (gs->ball.y + BALL_H >= SCREEN_H) {
        gs->ball.y = SCREEN_H - BALL_H;
        gs->ball.vy = -gs->ball.vy;
    }

    /* --- Paddle collision (right side, vertical paddle) --- */
    bool ball_at_paddle = (gs->ball.x + BALL_W >= PADDLE_X) &&
                          (gs->ball.x + BALL_W <= PADDLE_X + PADDLE_W + 2);
    bool ball_in_paddle = (gs->ball.y + BALL_H > gs->paddle.y) &&
                          (gs->ball.y < gs->paddle.y + PADDLE_H);

    if (ball_at_paddle && ball_in_paddle && gs->ball.vx > 0) {
        gs->ball.vx = -gs->ball.vx;
        /* Nudge vertical velocity based on hit offset from paddle centre */
        int16_t hit_offset = (gs->ball.y + BALL_H / 2) -
                             (gs->paddle.y + PADDLE_H / 2);
        gs->ball.vy = hit_offset / 8;
        if (gs->ball.vy >  4) gs->ball.vy =  4;
        if (gs->ball.vy < -4) gs->ball.vy = -4;
        if (gs->ball.vy ==  0) gs->ball.vy = 1;
        /* Push ball clear of the paddle face */
        gs->ball.x = PADDLE_X - BALL_W - 1;
    }

    /* --- Brick collisions --- */
    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            if (!gs->bricks[r][c].active) {
                continue;
            }

            int16_t bx = BRICK_LEFT_X + c * (BRICK_W + BRICK_GAP);
            int16_t by = BRICK_TOP_Y  + r * (BRICK_H + BRICK_GAP);

            bool overlap_x = (gs->ball.x + BALL_W > bx) &&
                             (gs->ball.x < bx + BRICK_W);
            bool overlap_y = (gs->ball.y + BALL_H > by) &&
                             (gs->ball.y < by + BRICK_H);

            if (overlap_x && overlap_y) {
                gs->bricks[r][c].active = false;
                /* Horizontal bounce: ball mainly travels left-right */
                gs->ball.vx = -gs->ball.vx;
                gs->score += SCORE_PER_BRICK;
                gs->last_brick_row = r;
                gs->last_brick_col = c;

                if (all_bricks_cleared(gs)) {
                    gs->state = GAME_WIN;
                    return EV_GAME_WIN;
                }
                return EV_BRICK_HIT;
            }
        }
    }

    /* --- Ball out of bounds: passed the paddle on the right --- */
    if (gs->ball.x > PADDLE_X + PADDLE_W) {
        gs->lives--;
        if (gs->lives == 0) {
            gs->state = GAME_OVER;
            return EV_GAME_OVER;
        }
        gs->state = GAME_LIFE_LOST;
        reset_ball_and_paddle(gs);
        return EV_LIFE_LOST;
    }

    return EV_NONE;
}
