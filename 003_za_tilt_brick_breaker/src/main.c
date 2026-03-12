#include <zephyr/kernel.h>
#include <string.h>
#include "display.h"
#include "game_types.h"

int main(void)
{
	int ret = display_module_init();
	if (ret) { return ret; }

	screen_draw_startup();
	k_msleep(2000);

	for (int i = 3; i >= 1; i--) {
		screen_draw_countdown(i);
		k_msleep(1000);
	}

	static uint8_t bricks[BRICK_ROWS][BRICK_COLS];
	memset(bricks, 1, sizeof(bricks));
	screen_draw_gameplay(bricks, PADDLE_X_INIT, BALL_X_INIT, BALL_Y_INIT,
			     0, 3);
	while (true) { k_msleep(1000); }
	return 0;
}
