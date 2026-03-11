#include <zephyr/kernel.h>
#include "display.h"
#include "game_types.h"

int main(void)
{
	int ret = display_module_init();
	if (ret) { return ret; }

	screen_clear(COLOR_BLACK);
	fill_rect(50, 35, 65, 35, COLOR_RED);
	fill_circle(165, 65, 25, COLOR_CYAN);
	draw_rect_outline(35, 105, 170, 60, COLOR_WHITE);

	while (true) { k_msleep(1000); }
	return 0;
}
