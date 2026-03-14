#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/byteorder.h>

#include "display.h"
#include "game_types.h"
#include "font5x7.h"
#include "ui_layout.h"

/* Declared in util.c */
extern int isqrt_i(int n);

static const struct device *disp_dev;

#define BUF_PIXELS 1024 /* 1024 × 2 bytes = 2 KB */
static uint16_t line_buf[BUF_PIXELS];

/* ---- Internal: write line_buf[0..w*h-1] to display at (x,y) ---- */
static void commit_block(int x, int y, int w, int h)
{
	int count = w * h;

	for (int i = 0; i < count; i++) {
		line_buf[i] = sys_cpu_to_be16(line_buf[i]);
	}
	struct display_buffer_descriptor d = {
		.buf_size = (uint32_t)count * sizeof(uint16_t),
		.width    = (uint16_t)w,
		.height   = (uint16_t)h,
		.pitch    = (uint16_t)w,
	};
	display_write(disp_dev, (uint16_t)x, (uint16_t)y, &d, line_buf);
}

static void commit_line(int x, int y, int w)
{
	commit_block(x, y, w, 1);
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

	int row = 0;

	while (row < h) {
		int cy = y + row;
		int vis_min, vis_max;

		if (!circle_row_span(cy, &vis_min, &vis_max)) {
			row++;
			continue;
		}

		int x0 = x, x1 = x + w - 1;

		if (x0 < vis_min) x0 = vis_min;
		if (x1 > vis_max) x1 = vis_max;

		int cw = x1 - x0 + 1;

		if (cw <= 0) {
			row++;
			continue;
		}

		/* Count consecutive rows with identical clipped span */
		int max_rows = BUF_PIXELS / cw;

		if (max_rows < 1) max_rows = 1;

		int batch = 1;

		while (batch < max_rows && row + batch < h) {
			int ny = y + row + batch;
			int nmin, nmax;

			if (!circle_row_span(ny, &nmin, &nmax)) break;

			int nx0 = x, nx1 = x + w - 1;

			if (nx0 < nmin) nx0 = nmin;
			if (nx1 > nmax) nx1 = nmax;
			if (nx0 != x0 || nx1 != x1) break;
			batch++;
		}

		int count = cw * batch;

		for (int i = 0; i < count; i++) {
			line_buf[i] = color;
		}
		commit_block(x0, cy, cw, batch);
		row += batch;
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

/* ---- Text rendering ---- */

void draw_char(int x, int y, char c, uint16_t fg, uint16_t bg, uint8_t scale)
{
	if (c < 0x20 || c > 0x7F) {
		c = 0x7F; /* show box for out-of-range */
	}
	const uint8_t *glyph = font5x7[c - 0x20];

	for (int col = 0; col < 5; col++) {
		uint8_t coldata = glyph[col];
		for (int row = 0; row < 7; row++) {
			uint16_t color = (coldata & (1 << row)) ? fg : bg;
			if (scale == 1) {
				draw_pixel(x + col, y + row, color);
			} else {
				fill_rect(x + col * scale, y + row * scale,
					  scale, scale, color);
			}
		}
	}
}

void draw_string(int x, int y, const char *s, uint16_t fg, uint16_t bg,
		 uint8_t scale)
{
	while (*s) {
		draw_char(x, y, *s, fg, bg, scale);
		x += 6 * scale; /* 5 columns + 1 gap */
		s++;
	}
}

void draw_int(int x, int y, int val, uint16_t fg, uint16_t bg, uint8_t scale)
{
	char buf[12];
	int len = 0;
	int neg = 0;

	if (val < 0) {
		neg = 1;
		val = -val;
	}
	if (val == 0) {
		buf[len++] = '0';
	} else {
		while (val > 0) {
			buf[len++] = '0' + (val % 10);
			val /= 10;
		}
	}
	if (neg) {
		buf[len++] = '-';
	}

	/* Reverse in-place */
	for (int i = 0; i < len / 2; i++) {
		char tmp = buf[i];
		buf[i] = buf[len - 1 - i];
		buf[len - 1 - i] = tmp;
	}
	buf[len] = '\0';

	draw_string(x, y, buf, fg, bg, scale);
}

void draw_int_right_aligned(int x_right, int y, int val, uint16_t fg,
			    uint16_t bg, uint8_t scale)
{
	char buf[12];
	int len = 0;
	int neg = 0;

	if (val < 0) {
		neg = 1;
		val = -val;
	}
	if (val == 0) {
		buf[len++] = '0';
	} else {
		while (val > 0) {
			buf[len++] = '0' + (val % 10);
			val /= 10;
		}
	}
	if (neg) {
		buf[len++] = '-';
	}

	for (int i = 0; i < len / 2; i++) {
		char tmp = buf[i];
		buf[i] = buf[len - 1 - i];
		buf[len - 1 - i] = tmp;
	}
	buf[len] = '\0';

	int width = len * 6 * scale - scale;
	draw_string(x_right - width, y, buf, fg, bg, scale);
}

/* ---- Screen helpers ---- */

void screen_clear(uint16_t color)
{
	uint16_t be_color = sys_cpu_to_be16(color);
	int rows_per_batch = BUF_PIXELS / CONFIG_DISPLAY_WIDTH;
	int pixels_per_batch = rows_per_batch * CONFIG_DISPLAY_WIDTH;

	for (int i = 0; i < pixels_per_batch; i++) {
		line_buf[i] = be_color;
	}
	struct display_buffer_descriptor d = {
		.buf_size = (uint32_t)pixels_per_batch * sizeof(uint16_t),
		.width    = CONFIG_DISPLAY_WIDTH,
		.height   = (uint16_t)rows_per_batch,
		.pitch    = CONFIG_DISPLAY_WIDTH,
	};
	for (int y = 0; y < CONFIG_DISPLAY_HEIGHT; y += rows_per_batch) {
		int h = rows_per_batch;

		if (y + h > CONFIG_DISPLAY_HEIGHT) {
			h = CONFIG_DISPLAY_HEIGHT - y;
			d.buf_size = (uint32_t)(h * CONFIG_DISPLAY_WIDTH)
				   * sizeof(uint16_t);
			d.height = (uint16_t)h;
		}
		display_write(disp_dev, 0, (uint16_t)y, &d, line_buf);
	}
}

void screen_draw_startup(void)
{
	int cx = CONFIG_DISPLAY_CIRCLE_CENTER_X;
	int cy = CONFIG_DISPLAY_CIRCLE_CENTER_Y;

	display_blanking_on(disp_dev);
	screen_clear(COLOR_DARK_BLUE);

	/* Decorative brick row */
	static const uint16_t brick_colors[] = {
		COLOR_RED, COLOR_ORANGE, COLOR_YELLOW, COLOR_GREEN, COLOR_CYAN
	};
	int total_bricks_w = STARTUP_BRICK_COUNT * STARTUP_BRICK_W
			   + (STARTUP_BRICK_COUNT - 1) * STARTUP_BRICK_GAP;
	int bx = cx - total_bricks_w / 2;

	for (int i = 0; i < STARTUP_BRICK_COUNT; i++) {
		fill_rect(bx + i * (STARTUP_BRICK_W + STARTUP_BRICK_GAP),
			  cy + STARTUP_BRICKS_Y_OFF,
			  STARTUP_BRICK_W, STARTUP_BRICK_H,
			  brick_colors[i % ARRAY_SIZE(brick_colors)]);
	}

	/* Title: "BRICK" */
	int tw = TEXT_WIDTH(5, STARTUP_TITLE_SCALE);
	draw_string(cx - tw / 2, cy + STARTUP_TITLE_Y_OFF,
		    "BRICK", COLOR_WHITE, COLOR_DARK_BLUE,
		    STARTUP_TITLE_SCALE);

	/* Subtitle: "BREAKER" */
	int sw = TEXT_WIDTH(7, STARTUP_SUBTITLE_SCALE);
	draw_string(cx - sw / 2, cy + STARTUP_SUBTITLE_Y_OFF,
		    "BREAKER", COLOR_CYAN, COLOR_DARK_BLUE,
		    STARTUP_SUBTITLE_SCALE);

	/* Decorative ball */
	fill_circle(cx, cy + STARTUP_BALL_Y_OFF, BALL_RADIUS, COLOR_YELLOW);

	/* Decorative paddle */
	fill_rect(cx - STARTUP_PADDLE_W / 2, cy + STARTUP_PADDLE_Y_OFF,
		  STARTUP_PADDLE_W, STARTUP_PADDLE_H, COLOR_WHITE);

	/* "TILT TO PLAY" hint */
	int hw = TEXT_WIDTH(12, STARTUP_HINT_SCALE);
	draw_string(cx - hw / 2, cy + STARTUP_HINT_Y_OFF,
		    "TILT TO PLAY", COLOR_WHITE, COLOR_DARK_BLUE,
		    STARTUP_HINT_SCALE);
	display_blanking_off(disp_dev);
}

void screen_draw_countdown(int n)
{
	int cx = CONFIG_DISPLAY_CIRCLE_CENTER_X;
	int cy = CONFIG_DISPLAY_CIRCLE_CENTER_Y;

	display_blanking_on(disp_dev);
	screen_clear(COLOR_BLACK);

	/* Large centered digit */
	int dw = TEXT_WIDTH(1, COUNTDOWN_SCALE);
	int dh = 7 * COUNTDOWN_SCALE;

	draw_int(cx - dw / 2, cy - dh / 2, n, COLOR_WHITE, COLOR_BLACK,
		 COUNTDOWN_SCALE);
	display_blanking_off(disp_dev);
}

/* ---- Gameplay screen helpers ---- */

static const uint16_t gameplay_brick_colors[BRICK_ROWS] = {
	COLOR_RED, COLOR_ORANGE, COLOR_YELLOW, COLOR_GREEN, COLOR_BLUE
};

static void draw_heart(int cx, int cy, uint16_t color)
{
	/* Two bumps at top */
	fill_circle(cx - 2, cy - 1, 2, color);
	fill_circle(cx + 2, cy - 1, 2, color);
	/* V-shape bottom */
	for (int dy = 1; dy <= 4; dy++) {
		int hw = 4 - dy;
		draw_hline(cx - hw, cy + dy, 2 * hw + 1, color);
	}
}

static void draw_score_text(int x, int y, int score, uint16_t fg,
			    uint16_t bg, uint8_t scale)
{
	char buf[18];

	snprintf(buf, sizeof(buf), "SCORE %d", score);
	draw_string(x, y, buf, fg, bg, scale);
}

static void draw_hud(int score, int lives)
{
	/* HUD band background */
	fill_rect(0, 0, CONFIG_DISPLAY_WIDTH, HUD_HEIGHT, COLOR_BLACK);

	/* Score — left side of visible area */
	draw_score_text(HUD_SCORE_X, HUD_SCORE_Y, score,
			COLOR_CYAN, COLOR_BLACK, HUD_SCORE_SCALE);

	/* Lives — hearts on the right, max 3 */
	int n = lives > 3 ? 3 : lives;
	for (int i = 0; i < n; i++) {
		draw_heart(HUD_LIVES_X - i * HUD_HEART_SPACING,
			   HUD_LIVES_Y, COLOR_RED);
	}
}

static void draw_bricks(const uint8_t bricks[BRICK_ROWS][BRICK_COLS])
{
	for (int r = 0; r < BRICK_ROWS; r++) {
		for (int c = 0; c < BRICK_COLS; c++) {
			uint16_t color = bricks[r][c]
				? gameplay_brick_colors[r]
				: COLOR_DARK_BLUE;
			fill_rect(BRICK_X(c), BRICK_Y(r),
				  BRICK_W, BRICK_H, color);
		}
	}
}

static void draw_paddle(int x)
{
	fill_rect(x, PADDLE_Y, PADDLE_W, PADDLE_H, COLOR_WHITE);
}

static void draw_ball(int x, int y)
{
	fill_circle(x, y, BALL_RADIUS, COLOR_YELLOW);
}

void screen_draw_gameplay(const uint8_t bricks[BRICK_ROWS][BRICK_COLS],
			  int paddle_x, int ball_x, int ball_y,
			  int score, int lives)
{
	display_blanking_on(disp_dev);

	screen_clear(COLOR_DARK_BLUE);
	draw_hud(score, lives);
	draw_bricks(bricks);
	draw_paddle(paddle_x);
	draw_ball(ball_x, ball_y);

	display_blanking_off(disp_dev);
}

/* ---- End screen helpers ---- */

static void draw_trophy(int cx, int cy, uint16_t bg)
{
	/* Cup body: bottom half of a filled circle */
	fill_circle(cx, cy, 22, COLOR_YELLOW);
	fill_rect(cx - 22, cy - 23, 44, 23, bg);

	/* Handles */
	fill_rect(cx - 28, cy - 8, 6, 16, COLOR_YELLOW);
	fill_rect(cx + 22, cy - 8, 6, 16, COLOR_YELLOW);

	/* Stem */
	fill_rect(cx - 4, cy + 22, 8, 14, COLOR_YELLOW);

	/* Base */
	fill_rect(cx - 18, cy + 36, 36, 6, COLOR_YELLOW);
}

static void draw_x_icon(int cx, int cy)
{
	for (int i = -20; i <= 20; i++) {
		draw_hline(cx + i - 3, cy + i, 7, COLOR_WHITE);
		draw_hline(cx - i - 3, cy + i, 7, COLOR_WHITE);
	}
}

void screen_draw_end(bool won, int score)
{
	int cx = CONFIG_DISPLAY_CIRCLE_CENTER_X;
	int cy = CONFIG_DISPLAY_CIRCLE_CENTER_Y;
	uint16_t bg = won ? COLOR_DARK_GREEN : COLOR_DARK_RED;

	display_blanking_on(disp_dev);
	screen_clear(bg);

	if (won) {
		draw_trophy(cx, cy + END_ICON_Y_OFF, bg);
	} else {
		draw_x_icon(cx, cy + END_ICON_Y_OFF);
	}

	const char *msg = won ? "WIN!" : "LOSE";
	int mw = TEXT_WIDTH(4, END_MSG_SCALE);

	draw_string(cx - mw / 2, cy + END_MSG_Y_OFF, msg,
		    COLOR_YELLOW, bg, END_MSG_SCALE);

	char sbuf[18];

	snprintf(sbuf, sizeof(sbuf), "SCORE %d", score);

	int slen = 0;

	for (const char *p = sbuf; *p; p++) slen++;

	int sw = TEXT_WIDTH(slen, END_SCORE_SCALE);

	draw_string(cx - sw / 2, cy + END_SCORE_Y_OFF, sbuf,
		    COLOR_WHITE, bg, END_SCORE_SCALE);

	display_blanking_off(disp_dev);
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
