#include "display.h"
#include "util.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(display_mod, CONFIG_APP_LOG_LEVEL);

/* -------------------------------------------------------------------------
 * 5x7 pixel font — characters: 0-9, space, A-Z subset used for messages
 * Each entry is 5 bytes; bit 7 of byte 0 = top-left pixel.
 * Row order: byte[0]=row0 ... byte[4]=row6 (7 rows, 5 cols per row)
 * ------------------------------------------------------------------------- */
#define FONT_CHAR_W  5
#define FONT_CHAR_H  7
#define FONT_SCALE   2   /* render each pixel as 2x2 for readability */

static const uint8_t font5x7[][FONT_CHAR_W] = {
    /* 0 */ {0x7E, 0x51, 0x49, 0x45, 0x7E},
    /* 1 */ {0x00, 0x42, 0x7F, 0x40, 0x00},
    /* 2 */ {0x42, 0x61, 0x51, 0x49, 0x46},
    /* 3 */ {0x21, 0x41, 0x45, 0x4B, 0x31},
    /* 4 */ {0x18, 0x14, 0x12, 0x7F, 0x10},
    /* 5 */ {0x27, 0x45, 0x45, 0x45, 0x39},
    /* 6 */ {0x3C, 0x4A, 0x49, 0x49, 0x30},
    /* 7 */ {0x01, 0x71, 0x09, 0x05, 0x03},
    /* 8 */ {0x36, 0x49, 0x49, 0x49, 0x36},
    /* 9 */ {0x06, 0x49, 0x49, 0x29, 0x1E},
    /* ' ' (10) */ {0x00, 0x00, 0x00, 0x00, 0x00},
    /* A  (11) */ {0x7C, 0x12, 0x11, 0x12, 0x7C},
    /* E  (12) */ {0x7F, 0x49, 0x49, 0x49, 0x41},
    /* G  (13) */ {0x3E, 0x41, 0x49, 0x49, 0x7A},
    /* I  (14) */ {0x00, 0x41, 0x7F, 0x41, 0x00},
    /* M  (15) */ {0x7F, 0x02, 0x0C, 0x02, 0x7F},
    /* N  (16) */ {0x7F, 0x04, 0x08, 0x10, 0x7F},
    /* O  (17) */ {0x3E, 0x41, 0x41, 0x41, 0x3E},
    /* R  (18) */ {0x7F, 0x09, 0x19, 0x29, 0x46},
    /* S  (19) */ {0x46, 0x49, 0x49, 0x49, 0x31},
    /* U  (20) */ {0x3F, 0x40, 0x40, 0x40, 0x3F},
    /* V  (21) */ {0x1F, 0x20, 0x40, 0x20, 0x1F},
    /* W  (22) */ {0x3F, 0x40, 0x38, 0x40, 0x3F},
    /* Y  (23) */ {0x07, 0x08, 0x70, 0x08, 0x07},
    /* B  (24) */ {0x7F, 0x49, 0x49, 0x49, 0x36},
    /* C  (25) */ {0x3E, 0x41, 0x41, 0x41, 0x22},
    /* K  (26) */ {0x7F, 0x08, 0x14, 0x22, 0x41},
    /* D  (27) */ {0x7F, 0x41, 0x41, 0x41, 0x3E},
};

/* Map ASCII character to font table index, -1 if not available */
static int char_to_idx(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    switch (c) {
    case ' ': return 10;
    case 'A': return 11;
    case 'E': return 12;
    case 'G': return 13;
    case 'I': return 14;
    case 'M': return 15;
    case 'N': return 16;
    case 'O': return 17;
    case 'R': return 18;
    case 'S': return 19;
    case 'U': return 20;
    case 'V': return 21;
    case 'W': return 22;
    case 'Y': return 23;
    case 'B': return 24;
    case 'C': return 25;
    case 'K': return 26;
    case 'D': return 27;
    default:  return 10; /* space for unknown */
    }
}

/* -------------------------------------------------------------------------
 * Internal state
 * ------------------------------------------------------------------------- */
static const struct device *disp;

/* Static strip buffer: 240 px wide, 16 px tall, RGB565 (7680 bytes) */
#define STRIP_H   16
static uint8_t strip_buf[SCREEN_W * STRIP_H * 2];

/* -------------------------------------------------------------------------
 * fill_rect — write a solid colour rectangle to the display
 * Uses a small stack buffer for narrow rects, strip_buf for wide ones.
 * ------------------------------------------------------------------------- */
static void fill_rect(int16_t x, int16_t y, int16_t w, int16_t h,
                      uint16_t color)
{
    if (w <= 0 || h <= 0) {
        return;
    }

    /* For narrow rects use a static buffer (single-threaded display) */
    if ((uint32_t)w * h * 2 <= 1024U) {
        static uint8_t buf[1024];
        uint16_t be = ((color >> 8) & 0xFF) | ((color & 0xFF) << 8);

        for (int i = 0; i < w * h; i++) {
            buf[i * 2]     = (uint8_t)(be >> 8);
            buf[i * 2 + 1] = (uint8_t)(be & 0xFF);
        }

        struct display_buffer_descriptor desc = {
            .buf_size = (uint32_t)w * h * 2,
            .width    = w,
            .height   = h,
            .pitch    = w,
        };
        display_write(disp, x, y, &desc, buf);
        return;
    }

    /* For wider/taller rects render in STRIP_H-row strips */
    uint16_t be = ((color >> 8) & 0xFF) | ((color & 0xFF) << 8);
    for (int i = 0; i < w * STRIP_H; i++) {
        strip_buf[i * 2]     = (uint8_t)(be >> 8);
        strip_buf[i * 2 + 1] = (uint8_t)(be & 0xFF);
    }

    int16_t rows_done = 0;
    while (rows_done < h) {
        int16_t rows = (h - rows_done < STRIP_H) ? (h - rows_done) : STRIP_H;
        struct display_buffer_descriptor desc = {
            .buf_size = (uint32_t)w * rows * 2,
            .width    = w,
            .height   = rows,
            .pitch    = w,
        };
        display_write(disp, x, y + rows_done, &desc, strip_buf);
        rows_done += rows;
    }
}

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

void display_init(void)
{
    disp = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (!device_is_ready(disp)) {
        LOG_ERR("Display not ready");
        return;
    }
    display_blanking_off(disp);
    LOG_INF("Display ready");
}

void display_clear_screen(void)
{
    fill_rect(0, 0, SCREEN_W, SCREEN_H, COLOR_BLACK);
}

void display_draw_brick(int row, int col, bool active)
{
    int16_t x = BRICK_LEFT_X + col * (BRICK_W + BRICK_GAP);
    int16_t y = BRICK_TOP_Y  + row * (BRICK_H + BRICK_GAP);
    uint16_t color = active ? brick_row_color(row) : COLOR_BLACK;
    fill_rect(x, y, BRICK_W, BRICK_H, color);
}

void display_draw_all_bricks(const Brick bricks[BRICK_ROWS][BRICK_COLS])
{
    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            display_draw_brick(r, c, bricks[r][c].active);
        }
    }
}

void display_draw_paddle(int16_t y, int16_t old_y)
{
    /* Erase old position, draw new position (paddle is vertical, fixed at PADDLE_X) */
    fill_rect(PADDLE_X, old_y, PADDLE_W, PADDLE_H, COLOR_BLACK);
    fill_rect(PADDLE_X, y,     PADDLE_W, PADDLE_H, COLOR_WHITE);
}

void display_draw_ball(int16_t x, int16_t y, int16_t old_x, int16_t old_y)
{
    /* Erase old position, clipped so it never overwrites the score bar */
    int16_t ey = (old_y < SCORE_BAR_H) ? SCORE_BAR_H : old_y;
    int16_t eh = BALL_H - (ey - old_y);
    if (eh > 0) {
        fill_rect(old_x, ey, BALL_W, eh, COLOR_BLACK);
    }
    fill_rect(x, y, BALL_W, BALL_H, COLOR_YELLOW);
}

/* Draw a single character at pixel position (px, py) in given colour.
 * Font encoding: each byte is a column (col 0 = leftmost), bit 0 = top row. */
static void draw_char(int16_t px, int16_t py, char c, uint16_t color)
{
    int idx = char_to_idx(c);

    for (int row = 0; row < FONT_CHAR_H; row++) {
        for (int col = 0; col < FONT_CHAR_W; col++) {
            if (font5x7[idx][col] & (1 << row)) {
                fill_rect(px + col * FONT_SCALE,
                          py + row * FONT_SCALE,
                          FONT_SCALE, FONT_SCALE, color);
            }
        }
    }
}

void display_draw_score(uint16_t score, uint8_t lives)
{
    /* Clear score bar content (separator at the bottom is redrawn below) */
    fill_rect(0, 0, SCREEN_W, SCORE_BAR_H - 2, COLOR_BLACK);

    /* 1.28" GC9A01: at cy=28 the circle is ~154 px wide (x≈43..197).
     * Layout uses x=44..164 for "SCORE XXXX" and x=166..194 for life dots.
     * Even with a 4-digit score the text ends at x=164, lives start at x=166. */
    int16_t cy = 28;
    int16_t cx = 44;
    int char_step = (FONT_CHAR_W + 1) * FONT_SCALE;

    /* Draw "SCORE " label */
    const char label[] = "SCORE ";
    for (int i = 0; label[i] != '\0'; i++) {
        draw_char(cx, cy, label[i], COLOR_WHITE);
        cx += char_step;
    }

    /* Draw up to 4 digits, skip leading zeros except the last */
    char buf[5];
    buf[0] = '0' + (score / 1000) % 10;
    buf[1] = '0' + (score /  100) % 10;
    buf[2] = '0' + (score /   10) % 10;
    buf[3] = '0' + (score       ) % 10;
    buf[4] = '\0';

    int start = 0;
    while (start < 3 && buf[start] == '0') {
        start++;
    }
    for (int i = start; i < 4; i++) {
        draw_char(cx, cy, buf[i], COLOR_WHITE);
        cx += char_step;
    }

    /* Life dots: 3 × 8-px squares at x=166, 177, 188.
     * Rightmost pixel x=195 < circle right-edge 197 at y=28.  */
    for (int i = 0; i < lives && i < INIT_LIVES; i++) {
        fill_rect(166 + i * 11, cy, 8, 8, COLOR_GREEN);
    }
    for (int i = lives; i < INIT_LIVES; i++) {
        fill_rect(166 + i * 11, cy, 8, 8, COLOR_BLACK);
    }

    /* 2-pixel white separator — clear boundary between score bar and game area */
    fill_rect(0, SCORE_BAR_H - 2, SCREEN_W, 2, COLOR_WHITE);
}

/* Draw message centered horizontally at a given y, in given colour.
 * Clears a band around the text first. */
static void draw_message_at(const char *msg, int16_t cy, uint16_t color)
{
    int len = 0;
    for (const char *p = msg; *p; p++) len++;

    int char_step = (FONT_CHAR_W + 1) * FONT_SCALE;
    int total_w   = len * char_step;
    int total_h   = FONT_CHAR_H * FONT_SCALE;

    int16_t band_y = cy - 4;
    fill_rect(0, band_y, SCREEN_W, total_h + 8, COLOR_BLACK);

    int16_t cx = (SCREEN_W - total_w) / 2;

    for (int i = 0; i < len; i++) {
        draw_char(cx + i * char_step, cy, msg[i], color);
    }
}

void display_draw_message(const char *msg)
{
    int total_h = FONT_CHAR_H * FONT_SCALE;
    draw_message_at(msg, SCREEN_H / 2 - total_h / 2, COLOR_YELLOW);
}

/* -------------------------------------------------------------------------
 * display_draw_countdown — draw a single digit/label (e.g. "3", "GO") at a
 * fixed position BELOW the title so it never overwrites the game name.
 * Clears only its own band; the title remains intact.
 * ------------------------------------------------------------------------- */
void display_draw_countdown(const char *s)
{
    /* y=158 is well below the title band (y=104..122) and inside the safe
     * circular arc (visible x ≈ 12..228 at that row). */
    int16_t cy = 158;

    int len = 0;
    for (const char *p = s; *p; p++) len++;

    int char_step = (FONT_CHAR_W + 1) * FONT_SCALE;
    int total_w   = len * char_step;
    int total_h   = FONT_CHAR_H * FONT_SCALE;

    /* Clear only this band */
    fill_rect(0, cy - 4, SCREEN_W, total_h + 8, COLOR_BLACK);

    int16_t cx = (SCREEN_W - total_w) / 2;
    for (int i = 0; i < len; i++) {
        draw_char(cx + i * char_step, cy, s[i], COLOR_YELLOW);
    }
}

void display_draw_start_screen(void)
{
    display_clear_screen();

    /* Score bar separator so the landscape layout is clear from the first frame */
    fill_rect(0, SCORE_BAR_H - 2, SCREEN_W, 2, COLOR_WHITE);

    /* Single wide title line — spans horizontally for a landscape feel.
     * At y=108 the circle is almost full-width (~239 px visible),
     * easily fitting "BRICK BREAKER" (13 chars × 12 px = 156 px). */
    draw_message_at("BRICK BREAKER", 108, COLOR_YELLOW);

    /* y=158 is reserved for the countdown digits drawn by display_draw_countdown() */
}

void display_draw_end_screen(bool won, uint16_t score)
{
    display_clear_screen();

    /* Separator mirrors the in-game landscape layout */
    fill_rect(0, SCORE_BAR_H - 2, SCREEN_W, 2, COLOR_WHITE);

    /* Result centred around y=120 (widest circle row) so the circular
     * boundary does not clip either text line.
     * "GAME OVER" (9 ch) = 108 px; "YOU WIN" (7 ch) = 84 px — both fit. */
    if (won) {
        draw_message_at("YOU WIN",   108, COLOR_GREEN);
    } else {
        draw_message_at("GAME OVER", 108, COLOR_RED);
    }

    /* Build "SCORE XXXX" string */
    char buf[12];
    uint16_t s = score;
    int pos = 0;
    buf[pos++] = 'S';
    buf[pos++] = 'C';
    buf[pos++] = 'O';
    buf[pos++] = 'R';
    buf[pos++] = 'E';
    buf[pos++] = ' ';

    /* Up to 4 digits, skip leading zeros except last */
    char digits[4];
    digits[0] = '0' + (s / 1000) % 10;
    digits[1] = '0' + (s /  100) % 10;
    digits[2] = '0' + (s /   10) % 10;
    digits[3] = '0' + (s       ) % 10;
    int start = 0;
    while (start < 3 && digits[start] == '0') {
        start++;
    }
    for (int i = start; i < 4; i++) {
        buf[pos++] = digits[i];
    }
    buf[pos] = '\0';

    draw_message_at(buf, 132, COLOR_WHITE);
}
