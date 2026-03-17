#include "render.h"
#include "display.h"
#include "config.h"

/* ---- Purpose-based render helpers ---- */

static void erase_ball(int x, int y)
{
	fill_circle(x, y, BALL_RADIUS, COLOR_DARK_BLUE);
}

static void draw_ball(int x, int y)
{
	fill_circle(x, y, BALL_RADIUS, COLOR_YELLOW);
}

static void erase_paddle(int x)
{
	fill_rect(x, PADDLE_Y, PADDLE_W, PADDLE_H, COLOR_DARK_BLUE);
}

static void draw_paddle(int x)
{
	fill_rect(x, PADDLE_Y, PADDLE_W, PADDLE_H, COLOR_WHITE);
}

static void erase_brick(int row, int col)
{
	fill_rect(BRICK_X(col), BRICK_Y(row), BRICK_W, BRICK_H,
		  COLOR_DARK_BLUE);
}

static void repair_bricks_under_ball(int cx, int cy,
				     const uint8_t bricks[BRICK_ROWS][BRICK_COLS])
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
			if (bricks[r][c]) {
				fill_rect(BRICK_X(c), BRICK_Y(r),
					  BRICK_W, BRICK_H,
					  gameplay_brick_color(r));
			}
		}
	}
}

/* ---- Event-driven frame renderer ---- */

int render_frame(const struct game_ctx *ctx, game_events_t events,
		 const struct game_event_data *evt)
{
	if (events & EVT_LIFE_LOST) {
		/* Life-lost: erase old positions, draw at reset positions */
		erase_ball(evt->old_ball_x, evt->old_ball_y);
		repair_bricks_under_ball(evt->old_ball_x, evt->old_ball_y,
					 ctx->bricks);
		erase_paddle(evt->old_paddle_x);
		draw_paddle(ctx->paddle_x);
		draw_ball(ctx->ball_x, ctx->ball_y);
		screen_erase_heart(evt->lives);
		return 500; /* pause 500 ms before resuming */
	}

	/* Normal frame — differential updates */

	if (events & EVT_BALL_MOVED) {
		erase_ball(evt->old_ball_x, evt->old_ball_y);
		repair_bricks_under_ball(evt->old_ball_x, evt->old_ball_y,
					 ctx->bricks);
	}

	if (events & EVT_PADDLE_MOVED) {
		erase_paddle(evt->old_paddle_x);
		draw_paddle(ctx->paddle_x);
	} else if ((events & EVT_BALL_MOVED) &&
		   evt->old_ball_y + BALL_RADIUS >= PADDLE_Y) {
		/* Ball was overlapping paddle area — redraw paddle */
		draw_paddle(ctx->paddle_x);
	}

	if (events & EVT_BRICK_HIT) {
		erase_brick(evt->hit_row, evt->hit_col);
	}

	if (events & EVT_BALL_MOVED) {
		draw_ball(ctx->ball_x, ctx->ball_y);
	}

	if (events & EVT_SCORE_CHANGED) {
		screen_update_score(evt->score);
	}

	return 0;
}
