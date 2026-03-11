#include <zephyr/kernel.h>
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

	screen_clear(COLOR_BLACK);
	while (true) { k_msleep(1000); }
	return 0;
}
