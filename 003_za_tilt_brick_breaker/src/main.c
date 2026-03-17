#include <zephyr/kernel.h>
#include "display.h"
#include "game.h"
#include "render.h"
#include "input.h"
#include "config.h"

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

	/* Game loop */
	struct game_event_data evt;
	int64_t next_tick = k_uptime_get();

	while (ctx.state == GAME_STATE_RUNNING) {
		next_tick += FRAME_MS;

		game_events_t events = game_update(&ctx, input_read(), &evt);

		if (ctx.state != GAME_STATE_RUNNING) {
			break;
		}

		int pause_ms = render_frame(&ctx, events, &evt);

		if (pause_ms > 0) {
			k_msleep(pause_ms);
			next_tick = k_uptime_get();
			continue;
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
