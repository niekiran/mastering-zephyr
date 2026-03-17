#ifndef INCLUDE_GAME_H_
#define INCLUDE_GAME_H_

#include <stdbool.h>
#include "game_types.h"
#include "game_events.h"

/* ---- Game lifecycle state ---- */
enum game_state {
	GAME_STATE_RUNNING,
	GAME_STATE_PAUSED,
	GAME_STATE_WON,
	GAME_STATE_LOST,
};

/* ---- Game runtime context (all per-session data) ---- */
struct game_ctx {
	enum game_state state;
	uint8_t bricks[BRICK_ROWS][BRICK_COLS];
	int paddle_x;
	int ball_x, ball_y;
	int ball_dx, ball_dy;
	int score;
	int lives;
	int bricks_left;
	int speed;          /* current speed level (BALL_SPEED_INIT..BALL_SPEED_MAX) */
	int start_dir;      /* +1 or -1, alternates each life loss */
};

void game_init(struct game_ctx *ctx);
game_events_t game_update(struct game_ctx *ctx, int paddle_x,
			   struct game_event_data *evt);

#endif /* INCLUDE_GAME_H_ */
