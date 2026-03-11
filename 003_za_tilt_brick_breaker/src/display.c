#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/byteorder.h>

#include "display.h"
#include "game_types.h"

/* Declared in util.c */
extern int isqrt_i(int n);

static const struct device *disp_dev;
static uint16_t line_buf[CONFIG_DISPLAY_WIDTH];

/* ---- Internal: write line_buf[0..w-1] to display at (x,y) ---- */
static void commit_line(int x, int y, int w)
{
	for (int i = 0; i < w; i++) {
		line_buf[i] = sys_cpu_to_be16(line_buf[i]);
	}
	struct display_buffer_descriptor d = {
		.buf_size = (uint32_t)w * sizeof(uint16_t),
		.width    = (uint16_t)w,
		.height   = 1,
		.pitch    = (uint16_t)w,
	};
	display_write(disp_dev, (uint16_t)x, (uint16_t)y, &d, line_buf);
}

/* ---- Circular clipping ---- */

static bool circle_row_span(int y, int *x_min, int *x_max)
{
#if defined(CONFIG_DISPLAY_IS_CIRCULAR) && CONFIG_DISPLAY_IS_CIRCULAR
	int dy  = y - CONFIG_DISPLAY_CIRCLE_CENTER_Y;
	int r   = CONFIG_DISPLAY_CIRCLE_RADIUS;
	int r2  = r * r;
	int dy2 = dy * dy;
	if (dy2 >= r2) {
		return false;
	}
	int dx = isqrt_i(r2 - dy2);
	*x_min = CONFIG_DISPLAY_CIRCLE_CENTER_X - dx;
	*x_max = CONFIG_DISPLAY_CIRCLE_CENTER_X + dx;
	return true;
#else
	*x_min = 0;
	*x_max = CONFIG_DISPLAY_WIDTH - 1;
	return true;
#endif
}

/* ---- Primitives ---- */

void draw_pixel(int x, int y, uint16_t color)
{
	int vis_min, vis_max;
	if (!circle_row_span(y, &vis_min, &vis_max)) return;
	if (x < vis_min || x > vis_max) return;
	line_buf[0] = color;
	commit_line(x, y, 1);
}

void draw_hline(int x, int y, int len, uint16_t color)
{
	if (len <= 0) return;

	int vis_min, vis_max;
	if (!circle_row_span(y, &vis_min, &vis_max)) return;

	int x_end = x + len - 1;
	if (x < vis_min) x = vis_min;
	if (x_end > vis_max) x_end = vis_max;
	int clipped_len = x_end - x + 1;
	if (clipped_len <= 0) return;
	if (clipped_len > CONFIG_DISPLAY_WIDTH) clipped_len = CONFIG_DISPLAY_WIDTH;

	for (int i = 0; i < clipped_len; i++) {
		line_buf[i] = color;
	}
	commit_line(x, y, clipped_len);
}

void fill_rect(int x, int y, int w, int h, uint16_t color)
{
	if (w <= 0 || h <= 0) return;
	for (int row = 0; row < h; row++) {
		draw_hline(x, y + row, w, color);
	}
}

void draw_rect_outline(int x, int y, int w, int h, uint16_t color)
{
	draw_hline(x, y, w, color);             /* top */
	draw_hline(x, y + h - 1, w, color);     /* bottom */
	for (int row = 1; row < h - 1; row++) { /* left + right columns */
		draw_pixel(x, y + row, color);
		draw_pixel(x + w - 1, y + row, color);
	}
}

void fill_circle(int cx, int cy, int r, uint16_t color)
{
	for (int dy = -r; dy <= r; dy++) {
		int dx = isqrt_i(r * r - dy * dy);
		draw_hline(cx - dx, cy + dy, 2 * dx + 1, color);
	}
}

/* ---- Screen helpers ---- */

void screen_clear(uint16_t color)
{
	fill_rect(0, 0, CONFIG_DISPLAY_WIDTH, CONFIG_DISPLAY_HEIGHT, color);
}

/* ---- Module init ---- */

int display_module_init(void)
{
	disp_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(disp_dev)) {
		return -ENODEV;
	}

#if defined(CONFIG_DISPLAY_ROTATION_90) && CONFIG_DISPLAY_ROTATION_90
	display_set_orientation(disp_dev, DISPLAY_ORIENTATION_ROTATED_90);
#elif defined(CONFIG_DISPLAY_ROTATION_180) && CONFIG_DISPLAY_ROTATION_180
	display_set_orientation(disp_dev, DISPLAY_ORIENTATION_ROTATED_180);
#elif defined(CONFIG_DISPLAY_ROTATION_270) && CONFIG_DISPLAY_ROTATION_270
	display_set_orientation(disp_dev, DISPLAY_ORIENTATION_ROTATED_270);
#else
	display_set_orientation(disp_dev, DISPLAY_ORIENTATION_NORMAL);
#endif

	display_blanking_off(disp_dev);
	return 0;
}
