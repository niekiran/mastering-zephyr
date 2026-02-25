#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "game.h"
#include "display.h"
#include "input.h"

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

#define FRAME_MS             33   /* ~30 FPS */
#define LIFE_LOST_PAUSE_MS   1500
#define TITLE_DISPLAY_MS     2000

static void run_countdown(void)
{
    /* Draw digits below the title using the dedicated countdown slot so the
     * title text is never cleared or distorted. */
    display_draw_countdown("3");  k_msleep(1000);
    display_draw_countdown("2");  k_msleep(1000);
    display_draw_countdown("1");  k_msleep(1000);
    display_draw_countdown("GO"); k_msleep(600);
    display_clear_screen();
}

int main(void)
{
    LOG_INF("Brick Breaker starting");

    display_init();
    input_init();

    /* Title screen */
    display_draw_start_screen();
    k_msleep(TITLE_DISPLAY_MS);

    /* Countdown */
    run_countdown();

    /* Game setup */
    GameState gs;
    game_init(&gs);

    display_draw_all_bricks(gs.bricks);
    display_draw_score(gs.score, gs.lives);
    display_draw_paddle(gs.paddle.y, gs.paddle.y);
    display_draw_ball(gs.ball.x, gs.ball.y, gs.ball.x, gs.ball.y);

    int16_t old_ball_x   = gs.ball.x;
    int16_t old_ball_y   = gs.ball.y;
    int16_t old_paddle_y = gs.paddle.y;

    while (true) {
        int8_t delta = input_get_paddle_delta();
        GameEvent ev = game_update(&gs, delta);

        /* Redraw paddle if it moved */
        if (gs.paddle.y != old_paddle_y) {
            display_draw_paddle(gs.paddle.y, old_paddle_y);
            old_paddle_y = gs.paddle.y;
        }

        /* Redraw ball */
        display_draw_ball(gs.ball.x, gs.ball.y, old_ball_x, old_ball_y);
        old_ball_x = gs.ball.x;
        old_ball_y = gs.ball.y;

        switch (ev) {
        case EV_BRICK_HIT:
            display_draw_brick(gs.last_brick_row, gs.last_brick_col, false);
            display_draw_score(gs.score, gs.lives);
            break;

        case EV_LIFE_LOST:
            display_draw_score(gs.score, gs.lives);
            display_draw_message("READY");
            k_msleep(LIFE_LOST_PAUSE_MS);
            /* Clear message and redraw paddle/ball at reset positions */
            display_draw_message("     ");
            display_draw_paddle(gs.paddle.y, old_paddle_y);
            display_draw_ball(gs.ball.x, gs.ball.y, old_ball_x, old_ball_y);
            old_paddle_y = gs.paddle.y;
            old_ball_x   = gs.ball.x;
            old_ball_y   = gs.ball.y;
            gs.state = GAME_PLAYING;
            break;

        case EV_GAME_OVER:
            LOG_INF("Game over. Score: %d", gs.score);
            display_draw_end_screen(false, gs.score);
            k_sleep(K_FOREVER);
            break;

        case EV_GAME_WIN:
            LOG_INF("You win! Score: %d", gs.score);
            display_draw_end_screen(true, gs.score);
            k_sleep(K_FOREVER);
            break;

        default:
            break;
        }

        k_msleep(FRAME_MS);
    }

    return 0;
}
