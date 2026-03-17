#include <string.h>
#include "game.h"
#include "config.h"
#include "util.h"

void game_init(struct game_ctx *ctx)
{
	ctx->state = GAME_STATE_RUNNING;
	memset(ctx->bricks, 1, sizeof(ctx->bricks));
	ctx->paddle_x = PADDLE_X_INIT;
	ctx->ball_x = BALL_X_INIT;
	ctx->ball_y = BALL_Y_INIT;
	ctx->speed = BALL_SPEED_INIT;
	ctx->start_dir = 1;
	ctx->ball_dx = ctx->start_dir * ctx->speed;
	ctx->ball_dy = -ctx->speed;
	ctx->score = 0;
	ctx->lives = LIVES_INIT;
	ctx->bricks_left = BRICK_ROWS * BRICK_COLS;
	ctx->hit_row = -1;
	ctx->hit_col = -1;
	ctx->life_lost = false;
}

void game_update(struct game_ctx *ctx)
{
	if (ctx->state != GAME_STATE_RUNNING) {
		return;
	}

	/* Step 1 — Reset per-tick flags */
	ctx->hit_row = -1;
	ctx->hit_col = -1;
	ctx->life_lost = false;

	/* Step 2 — Move ball */
	ctx->ball_x += ctx->ball_dx;
	ctx->ball_y += ctx->ball_dy;

	/* Step 3 — Wall / boundary collisions */
#if defined(CONFIG_DISPLAY_IS_CIRCULAR) && CONFIG_DISPLAY_IS_CIRCULAR
	/* Circular boundary bounce (not below paddle) */
	{
		int nx = ctx->ball_x - DISPLAY_CX;
		int ny = ctx->ball_y - DISPLAY_CY;
		int dist_sq = nx * nx + ny * ny;
		int bounce_r = DISPLAY_R - BALL_RADIUS;

		if (dist_sq >= bounce_r * bounce_r &&
		    ctx->ball_y < PADDLE_Y) {
			/* Reflect velocity off circle normal */
			int dot = ctx->ball_dx * nx + ctx->ball_dy * ny;

			if (dot > 0) { /* moving outward */
				ctx->ball_dx -= 2 * dot * nx / dist_sq;
				ctx->ball_dy -= 2 * dot * ny / dist_sq;
			}

			/* Push ball back inside the boundary */
			int dist = isqrt_i(dist_sq);

			if (dist > 0) {
				ctx->ball_x = DISPLAY_CX + nx * bounce_r / dist;
				ctx->ball_y = DISPLAY_CY + ny * bounce_r / dist;
			}
		}
	}

	/* HUD ceiling bounce (flat wall at top of play area) */
	if (ctx->ball_y - BALL_RADIUS < WALL_TOP) {
		ctx->ball_y = WALL_TOP + BALL_RADIUS;
		if (ctx->ball_dy < 0) {
			ctx->ball_dy = -ctx->ball_dy;
		}
	}
#else
	/* Rectangular boundary bounce */
	if (ctx->ball_x - BALL_RADIUS < 0) {
		ctx->ball_x = BALL_RADIUS;
		ctx->ball_dx = -ctx->ball_dx;
	}
	if (ctx->ball_x + BALL_RADIUS > CONFIG_DISPLAY_WIDTH) {
		ctx->ball_x = CONFIG_DISPLAY_WIDTH - BALL_RADIUS;
		ctx->ball_dx = -ctx->ball_dx;
	}
	if (ctx->ball_y - BALL_RADIUS < WALL_TOP) {
		ctx->ball_y = WALL_TOP + BALL_RADIUS;
		ctx->ball_dy = -ctx->ball_dy;
	}
#endif

	/* Guard: integer truncation in circular reflection can zero out
	 * ball_dy, leaving the ball stuck moving horizontally forever. */
	if (ctx->ball_dy == 0) {
		ctx->ball_dy = (ctx->ball_y < CONFIG_DISPLAY_HEIGHT / 2) ? 1 : -1;
	}

	/* Step 4 — Paddle collision with angle-based bounce */
	if (ctx->ball_dy > 0) {
		if (ctx->ball_y + BALL_RADIUS >= PADDLE_Y &&
		    ctx->ball_y + BALL_RADIUS <= PADDLE_Y + PADDLE_H + ctx->ball_dy &&
		    ctx->ball_x >= ctx->paddle_x - BALL_RADIUS &&
		    ctx->ball_x <= ctx->paddle_x + PADDLE_W + BALL_RADIUS) {
			int half = PADDLE_W / 2;
			int offset = ctx->ball_x - (ctx->paddle_x + half);

			offset = clamp_i(offset, -half, half);
			int new_dx = (offset * ctx->speed) / half;

			if (new_dx == 0) {
				new_dx = (offset >= 0) ? 1 : -1;
			}
			ctx->ball_dx = new_dx;
			ctx->ball_dy = -ctx->speed;
			ctx->ball_y = PADDLE_Y - BALL_RADIUS;
		}
	}

	/* Step 5 — Brick collision (first hit only) */
	for (int r = 0; r < BRICK_ROWS && ctx->hit_row < 0; r++) {
		for (int c = 0; c < BRICK_COLS && ctx->hit_row < 0; c++) {
			if (!ctx->bricks[r][c]) {
				continue;
			}
			int bx = BRICK_X(c);
			int by = BRICK_Y(r);

			/* Closest point on brick rect to ball center */
			int cx = clamp_i(ctx->ball_x, bx, bx + BRICK_W);
			int cy = clamp_i(ctx->ball_y, by, by + BRICK_H);

			int dx = ctx->ball_x - cx;
			int dy = ctx->ball_y - cy;

			if (dx * dx + dy * dy <= BALL_RADIUS * BALL_RADIUS) {
				/* Destroy brick */
				ctx->bricks[r][c] = 0;
				ctx->bricks_left--;
				ctx->score += BRICK_SCORE;
				ctx->hit_row = r;
				ctx->hit_col = c;

				/* Determine bounce direction */
				int adx = dx < 0 ? -dx : dx;
				int ady = dy < 0 ? -dy : dy;

				if (adx > ady) {
					ctx->ball_dx = -ctx->ball_dx;
				} else if (ady > adx) {
					ctx->ball_dy = -ctx->ball_dy;
				} else {
					ctx->ball_dx = -ctx->ball_dx;
					ctx->ball_dy = -ctx->ball_dy;
				}

				/* Speed ramp: increase with bricks destroyed */
				int destroyed = (BRICK_ROWS * BRICK_COLS) - ctx->bricks_left;
				int spd = BALL_SPEED_INIT + destroyed / SPEED_UP_INTERVAL;

				if (spd > BALL_SPEED_MAX) {
					spd = BALL_SPEED_MAX;
				}
				ctx->speed = spd;
			}
		}
	}

	/* Step 6 — Ball miss check */
	if (ctx->ball_y - BALL_RADIUS > PADDLE_Y + PADDLE_H) {
		ctx->lives--;
		ctx->life_lost = true;
		if (ctx->lives == 0) {
			ctx->state = GAME_STATE_LOST;
			return;
		}
		/* Reset ball + paddle to center; alternate start direction */
		ctx->start_dir = -ctx->start_dir;
		ctx->paddle_x = PADDLE_X_INIT;
		ctx->ball_x = BALL_X_INIT;
		ctx->ball_y = BALL_Y_INIT;
		ctx->ball_dx = ctx->start_dir * ctx->speed;
		ctx->ball_dy = -ctx->speed;
	}

	/* Step 7 — Win check */
	if (ctx->bricks_left == 0) {
		ctx->state = GAME_STATE_WON;
	}
}
