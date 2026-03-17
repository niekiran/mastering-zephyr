#include <zephyr/kernel.h>
#include <string.h>
#include "display.h"
#include "game.h"
#include "input.h"
#include "config.h"
#include "util.h"

/* ---- Differential rendering helpers ---- */

static void repair_bricks_under(int cx, int cy, const struct game_ctx *ctx)
{
	int x0 = cx - BALL_RADIUS;
	int x1 = cx + BALL_RADIUS;
	int y0 = cy - BALL_RADIUS;
	int y1 = cy + BALL_RADIUS;

	int c_min = (x0 - BRICK_ORIGIN_X) / (BRICK_W + BRICK_GAP);
	int c_max = (x1 - BRICK_ORIGIN_X) / (BRICK_W + BRICK_GAP);
	int r_min = (y0 - BRICK_ORIGIN_Y) / (BRICK_H + BRICK_GAP);
	int r_max = (y1 - BRICK_ORIGIN_Y) / (BRICK_H + BRICK_GAP);

	if (c_min < 0) c_min = 0;
	if (c_max >= BRICK_COLS) c_max = BRICK_COLS - 1;
	if (r_min < 0) r_min = 0;
	if (r_max >= BRICK_ROWS) r_max = BRICK_ROWS - 1;

	for (int r = r_min; r <= r_max; r++) {
		for (int c = c_min; c <= c_max; c++) {
			if (ctx->bricks[r][c]) {
				fill_rect(BRICK_X(c), BRICK_Y(r),
					  BRICK_W, BRICK_H,
					  gameplay_brick_color(r));
			}
		}
	}
}

int main(void)
{
	int ret = display_module_init();
	if (ret) { return ret; }

	/* Init input during startup screen to hide calibration delay */
	ret = input_init();
	if (ret) { return ret; }

	screen_draw_startup();
	k_msleep(2000);

	for (int i = 3; i >= 1; i--) {
		screen_draw_countdown(i);
		k_msleep(1000);
	}

	/* Initialize game state */
	static struct game_ctx ctx;
	game_init(&ctx);

	/* Initial full render */
	screen_draw_gameplay(ctx.bricks, ctx.paddle_x, ctx.ball_x, ctx.ball_y,
			     ctx.score, ctx.lives);

	/* Game loop with differential rendering */
	int64_t next_tick = k_uptime_get();

	while (ctx.state == GAME_STATE_RUNNING) {
		next_tick += FRAME_MS;

		/* Save old state for differential rendering */
		int old_bx = ctx.ball_x;
		int old_by = ctx.ball_y;
		int old_px = ctx.paddle_x;
		int old_score = ctx.score;

		/* Read input — absolute paddle position from tilt */
		ctx.paddle_x = input_read();

		game_update(&ctx);

		if (ctx.state != GAME_STATE_RUNNING && !ctx.life_lost) {
			break;
		}

		if (ctx.life_lost) {
			/* Incremental update: erase old positions, redraw at center */
			fill_circle(old_bx, old_by, BALL_RADIUS, COLOR_DARK_BLUE);
			repair_bricks_under(old_bx, old_by, &ctx);

			/* Erase old paddle, draw new paddle at center */
			fill_rect(old_px, PADDLE_Y, PADDLE_W, PADDLE_H,
				  COLOR_DARK_BLUE);
			fill_rect(ctx.paddle_x, PADDLE_Y, PADDLE_W, PADDLE_H,
				  COLOR_WHITE);

			/* Draw ball at reset position */
			fill_circle(ctx.ball_x, ctx.ball_y, BALL_RADIUS,
				    COLOR_YELLOW);

			/* Erase lost heart */
			screen_erase_heart(ctx.lives);

			/* Pause before resuming */
			k_msleep(500);
			next_tick = k_uptime_get();
		} else {
			/* Incremental update */

			/* 1. Erase old ball */
			fill_circle(old_bx, old_by, BALL_RADIUS, COLOR_DARK_BLUE);

			/* 2. Repair bricks under old ball position */
			repair_bricks_under(old_bx, old_by, &ctx);

			/* 3. Handle paddle movement + repair */
			if (ctx.paddle_x != old_px) {
				fill_rect(old_px, PADDLE_Y,
					  PADDLE_W, PADDLE_H,
					  COLOR_DARK_BLUE);
				fill_rect(ctx.paddle_x, PADDLE_Y,
					  PADDLE_W, PADDLE_H,
					  COLOR_WHITE);
			} else if (old_by + BALL_RADIUS >= PADDLE_Y) {
				fill_rect(ctx.paddle_x, PADDLE_Y,
					  PADDLE_W, PADDLE_H,
					  COLOR_WHITE);
			}

			/* 4. Erase destroyed brick */
			if (ctx.hit_row >= 0) {
				fill_rect(BRICK_X(ctx.hit_col),
					  BRICK_Y(ctx.hit_row),
					  BRICK_W, BRICK_H, COLOR_DARK_BLUE);
			}

			/* 5. Draw new ball */
			fill_circle(ctx.ball_x, ctx.ball_y,
				    BALL_RADIUS, COLOR_YELLOW);

			/* 6. Update score text if changed */
			if (ctx.score != old_score) {
				screen_update_score(ctx.score);
			}
		}

		/* Frame timing — drop missed frames to keep speed consistent */
		int64_t now = k_uptime_get();
		int64_t remaining = next_tick - now;
		if (remaining > 0) {
			k_msleep((int32_t)remaining);
		} else {
			next_tick = now;
		}
	}

	/* End screen */
	screen_draw_end(ctx.state == GAME_STATE_WON, ctx.score);

	while (true) { k_msleep(1000); }
	return 0;
}
