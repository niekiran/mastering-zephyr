# Plan: Display-Only Implementation — Startup, Gameplay, and End Screens

## Context

All source files are empty stubs. The Kconfig is complete. The device tree overlay
and prj.conf have no display configuration yet. CMakeLists.txt only compiles main.c.

This plan slices the GC9A01 display driver, builds a small drawing library on top of
the Zephyr `display.h` API, and implements the three required screens — startup
(logo + countdown), gameplay (background, bricks, paddle, ball, HUD), and end (win/lose
with custom graphics). No MPU6050 or game logic is included.

**Memory constraint:** 40 KB RAM, 16 KB flash. A full 240x240 RGB565 framebuffer
needs 115 KB — too large. All rendering uses a **480-byte static row buffer**
(one scan line) that is filled with a colour and committed to the device via
`display_write()`. This avoids heap allocation entirely.

**Pixel format:** RGB565 (2 bytes/pixel). Each pixel colour is a `uint16_t`.

**No circular clipping.** The GC9A01 glass physically masks pixels outside the
inscribed circle — no artefacts, just a tiny SPI overhead at corners. Add clipping
later if profiling shows it matters.

---

## Screen Layout (240x240)

```
y=0
       +-----------------+
y=25   |  HUD (25 px): score left, lives
       +-----------------+
y=105  |  Brick zone (80 px)
       |  6 cols x 5 rows, rainbow colors
       +-----------------+
y=200  |  Open play area
       |  Ball placeholder at (120, 170)
       +-----------------+
y=210  |  Paddle zone (10 px)
       +-----------------+
       |  Bottom margin
y=240  +-----------------+
```

**Brick grid constants**

| Symbol | Value | Meaning |
|--------|-------|---------|
| BRICK_COLS | 6 | columns |
| BRICK_ROWS | 5 | rows |
| BRICK_W | 26 | pixels wide |
| BRICK_H | 14 | pixels tall |
| BRICK_GAP | 2 | pixels between bricks |
| BRICK_ORIGIN_X | 5 | left edge of grid |
| BRICK_ORIGIN_Y | 30 | top edge of grid |
| BRICK_X(c) | `BRICK_ORIGIN_X + c*(BRICK_W+BRICK_GAP)` | |
| BRICK_Y(r) | `BRICK_ORIGIN_Y + r*(BRICK_H+BRICK_GAP)` | |

**Paddle / ball constants**

| Symbol | Value |
|--------|-------|
| PADDLE_W | 50 |
| PADDLE_H | 6 |
| PADDLE_Y | 210 |
| PADDLE_X_INIT | 90 = (240-60)/2 |
| BALL_RADIUS | 5 |
| BALL_X_INIT | 120 |
| BALL_Y_INIT | 170 |

**RGB565 palette**

| Name | Value | Use |

> [partial — palette table rows beyond this header were not visible in the captured frames]

---

## File Map

| File | Action | Purpose |
|------|--------|---------|
| `boards/fastbit_stm32nano_stm32f303xc.overlay` | Write | SPI1 pinctrl + MIPI DBI + GC9A01 DT node |
| `prj.conf` | Edit | Add `CONFIG_DISPLAY=y`, `CONFIG_SPI=y` |
| `CMakeLists.txt` | Edit | Add `src/display.c` |
| `include/game_types.h` | Create | Palette, layout constants, `brick_state_t` |
| `include/display.h` | Create | Public API for all drawing functions |
| `include/font5x7.h` | Create | 96-char 5x7 ASCII bitmap font (flash) |
| `src/util.c` | Write | `rgb565()`, `clamp_i()`, `isqrt_i()`, `point_in_circle()` |
| `src/display.c` | Write | All drawing, text, and screen functions |
| `src/main.c` | Edit | Screen sequencer |

---

## API (include/display.h)

```c
/* Init — call once in main() */

/* ...primitives... */
void fill_circle(int cx, int cy, int r, uint16_t color);
void draw_rect_outline(int x, int y, int w, int h, uint16_t color);

/* Text (scale=1 = 5x7 px, scale=2 = 10x14 px, etc.) */
void draw_char(int x, int y, char c, uint16_t fg, uint16_t bg, uint8_t scale);
void draw_string(int x, int y, const char *s, uint16_t fg, uint16_t bg, uint8_t scale);
void draw_int(int x, int y, int val, uint16_t fg, uint16_t bg, uint8_t scale);
void draw_int_right_aligned(int x_right, int y, int val, uint16_t fg,
                            uint16_t bg, uint8_t scale);

/* Screens */
void screen_clear(uint16_t color);
void screen_draw_startup(void);
void screen_draw_countdown(int n);
void screen_draw_gameplay(const uint8_t bricks[BRICK_ROWS][BRICK_COLS],
                          int paddle_x, int ball_x, int ball_y,
                          int score, int lives);
void screen_draw_end(bool won, int score);
```

`src/display.c` also has these **private** (static) helpers:

```c
static void commit_line(int x, int y, int w);   // writes line_buf[0..w-1]
static void draw_logo(int cx, int cy);          // startup screen decoration
static void draw_hud(int score, int lives);
static void draw_bricks(const uint8_t bricks[BRICK_ROWS][BRICK_COLS]);
static void draw_paddle(int x);
static void draw_ball(int x, int y);
static void draw_trophy(int cx, int cy);        // win graphic
static void draw_x_icon(int cx, int cy);        // lose graphic
```

---

## Checkpoint 1 — Display Hardware Bring-Up

**Git commit message:** `feat: wire GC9A01 display — SPI1 device tree + prj.conf`

`boards/fastbit_stm32nano_stm32f303xc.overlay` (full replacement)

```dts
#include <zephyr/dt-bindings/mipi_dbi/mipi_dbi.h>
#include <zephyr/dt-bindings/display/panel.h>

/ {
    chosen {
        zephyr,display = &gc9a01_display;
    };
};
```

> [gap — between the `chosen` block and the pinctrl `group1` block, frames captured
> at 0.5 s intervals did not show this region; recapture needed for that segment]

```dts
        group1 {
            pinmux = <STM32_PINMUX('A', 5, ANALOG)>,
                     <STM32_PINMUX('A', 6, ANALOG)>,
                     <STM32_PINMUX('A', 7, ANALOG)>;
        };
    };
};

&spi1 {
    status = "okay";
    pinctrl-0 = <&spi1_default>;
    pinctrl-1 = <&spi1_sleep>;
    pinctrl-names = "default", "sleep";
    cs-gpios = <&gpioa 4 GPIO_ACTIVE_LOW>;

    mipi_dbi_gc9x01: mipi-dbi-spi {
        compatible = "zephyr,mipi-dbi-spi";
        reg = <0>;
        spi-max-frequency = <DT_FREQ_M(60)>;
        dc-gpios = <&gpiob 1 GPIO_ACTIVE_HIGH>;
        reset-gpios = <&gpiob 8 GPIO_ACTIVE_LOW>;
        #address-cells = <1>;
        #size-cells = <0>;

        gc9a01_display: gc9a01@0 {
            compatible = "galaxycore,gc9x01x";
            reg = <0>;
            mipi-max-frequency = <DT_FREQ_M(60)>;
```

> [gap — gc9a01 node body and prj.conf edits not captured between this block
> and the following bring-up code]

```c
/* Fill entire screen red using a 240-pixel row buffer */
static uint16_t row[240];

for (int i = 0; i < 240; i++) row[i] = 0xF800; /* red RGB565 */

struct display_buffer_descriptor d = {
    .buf_size = sizeof(row), .width = 240, .height = 1, .pitch = 240,
};
for (int y = 0; y < 240; y++) {
    display_write(dev, 0, y, &d, row);
}
display_blanking_off(dev);

while (true) { k_msleep(1000); }
return 0;
}
```

**Hardware test:** Flash → solid red circle visible on display.

---

## Checkpoint 2 — Drawing Primitives

**Git commit message:** `feat: add display primitives, util helpers, game layout constants`

### New files

**`include/game_types.h`** — palette macros + layout constants (see table above).
Also defines:

```c
typedef uint8_t brick_state_t; /* 0 = destroyed, 1 = active */
```

**`include/display.h`** — full API as listed above.

**`src/util.c`**

```c
uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b);
int      clamp_i(int v, int lo, int hi);
int      isqrt_i(int n);              /* integer square root, no <math.h> */
bool     point_in_circle(int x, int y, int cx, int cy, int r);
```

`isqrt_i` uses a simple binary-search or Newton's method loop — no floats.

**`src/display.c`** — static state

```c
static const struct device *disp_dev;
static uint16_t            line_buf[CONFIG_DISPLAY_WIDTH];
```

> [gap — primitive implementations (`fill_rect`, `fill_circle`, `draw_rect_outline`,
> `commit_line`) not captured in the frames between the static state and `screen_clear`]

```c
return 0;
}
```

`screen_clear(color)` → calls `fill_rect(0, 0, 240, 240, color)`.

### CMakeLists.txt

```cmake
target_sources(app PRIVATE
    src/main.c
    src/display.c
    src/util.c
)
```

### src/main.c — primitive test

```c
display_module_init();
screen_clear(COLOR_BLACK);
fill_rect(20, 20, 80, 40, COLOR_RED);
fill_circle(170, 60, 30, COLOR_CYAN);
draw_rect_outline(100, 100, 220, 80, COLOR_WHITE);
```

**Hardware test:** Black background, red rectangle top-left, cyan circle top-right,
white rectangle outline below.

---

## Checkpoint 3 — Bitmap Font and Text

**Git commit message:** `feat: add 5x7 bitmap font and text rendering`

### `include/font5x7.h`

Standard public-domain 5x7 ASCII font (96 glyphs, 0x20–0x7F).
Each glyph is 5 bytes; byte `b` is column `c`; bit 0 is the top row.

```c
static const uint8_t font5x7[][5] = {
    /* 0x20 space */ { 0x00, 0x00, 0x00, 0x00, 0x00 },
    /* 0x21 !     */ { 0x00, 0x00, 0x5F, 0x00, 0x00 },
    /* ... */
};
```

> [gap — `draw_char` / `draw_string` implementations not captured between font
> declaration and the `draw_int*` notes]

`draw_int(x, y, val, fg, bg, scale)`
- Formats to a local `char[12]` buffer using a small digit-extraction loop (no `printf`)
- Calls `draw_string`

`draw_int_right_aligned(x_right, y, val, fg, bg, scale)`
- Computes string width first, then calls `draw_string` at `x_right - width`

### src/main.c — text test

```c
display_module_init();
screen_clear(COLOR_BLACK);
draw_string(10, 10, "SCORE:", COLOR_CYAN, COLOR_BLACK, 1);
draw_int(52, 10, 42, COLOR_CYAN, COLOR_BLACK, 1);
draw_string(40, 80, "HELLO", COLOR_WHITE, COLOR_BLACK, 2);
```

**Hardware test:** "SCORE: 42" small top-left; "HELLO" large below.

---

## Checkpoint 4 — Startup / Splash Screen

**Git commit message:** `feat: implement startup screen with logo and countdown`

### `draw_logo(cx, cy)` — static, in `display.c`

A mini brick-breaker scene centred at `(cx, cy)`.

> [gap — logo composition body (rows of mini bricks, mini paddle, mini ball)
> not captured between this header and the title-text snippet below]

```c
draw_string_centered("BRICK"  , /*scale*/ 2, /*y=*/155, COLOR_WHITE);
draw_string_centered("BREAKER", /*scale*/ 2, /*y=*/173, COLOR_WHITE);
```

Centering: string width = `num_chars * 6 * scale - scale`; x = `(240 - width) / 2`.

### `screen_draw_countdown(n)` — public

```c
/* Erase previous number area */
fill_rect(85, 80, 70, 60, COLOR_BLACK);
/* Draw single digit, scale 8 = 40x56 px */
draw_int(centered_x, 90, n, COLOR_YELLOW, COLOR_BLACK, 8);
```

### src/main.c

```c
display_module_init();
screen_draw_startup();
for (int i = 3; i >= 1; i--) {
    screen_draw_countdown(i);
    k_msleep(1000);
}
screen_clear(COLOR_BLACK);
while (true) { k_msleep(1000); }
```

**Hardware test:** Logo + title appear, then 3 → 2 → 1 counts down with 1-second
intervals, screen clears.

---

## Checkpoint 5 — Gameplay Screen

**Git commit message:** `feat: implement gameplay screen — bricks, paddle, ball, HUD`

### `draw_hud(score, lives)` — static

```c
fill_rect(0, 0, 240, HUD_HEIGHT, COLOR_BLACK);
draw_string(4, 5, "SC", COLOR_CYAN, COLOR_BLACK, 1);
draw_int(16, 5, score, COLOR_CYAN, COLOR_BLACK, 1);
/* Lives: small filled circles on the right */
for (int i = 0; i < lives; i++) {
    fill_circle(228 + i*14, 12, 5, COLOR_RED);
}
```

### `draw_bricks(bricks)` — static

```c
for (r in 0..BRICK_ROWS-1):
    for (c in 0..BRICK_COLS-1):
        if (bricks[r][c]):
            fill_rect(BRICK_X(c), BRICK_Y(r), BRICK_W, BRICK_H, brick_colors[r])
        else:
            fill_rect(BRICK_X(c), BRICK_Y(r), BRICK_W, BRICK_H, COLOR_DARK_BLUE)
```

### `draw_paddle(x)` — static

```c
fill_rect(x, PADDLE_Y, PADDLE_W, PADDLE_H, COLOR_WHITE);
```

### `draw_ball(x, y)` — static

```c
fill_circle(x, y, BALL_RADIUS, COLOR_YELLOW);
```

### src/main.c test

```c
display_module_init();
static uint8_t bricks[BRICK_ROWS][BRICK_COLS];
memset(bricks, 1, sizeof(bricks));
screen_draw_gameplay(bricks, PADDLE_X_INIT, BALL_X_INIT, BALL_Y_INIT, 0, 3);
while (true) { k_msleep(1000); }
```

**Hardware test:** Dark background, 5 rows × 6 cols rainbow bricks, white paddle
centred, yellow ball, "SC 0" and 3 red life dots in the HUD.

---

## Checkpoint 6 — End Screens

**Git commit message:** `feat: implement win/lose end screens with custom graphics`

### `draw_trophy(cx, cy)` — static (win)

Drawn entirely with `fill_rect` and `fill_circle`:

```c
/* Cup body: filled circle minus top half */
fill_circle(cx, cy, 22, COLOR_YELLOW);
fill_rect(cx-22, cy-23, 44, 23, bg_color);   /* erase top half */
/* Handles: two small rects on sides */
fill_rect(cx-28, cy-6, 6, 16, COLOR_YELLOW);
fill_rect(cx+22, cy-6, 6, 16, COLOR_YELLOW);
/* Stem */
fill_rect(cx-4, cy+22, 8, 14, COLOR_YELLOW);
/* Base */
fill_rect(cx-18, cy+36, 36, 6, COLOR_YELLOW);
```

### `draw_x_icon(cx, cy)` — static (lose)

Two overlapping thick diagonals using offset hlines:

```c
for (int i = -20; i <= 20; i++) {
    draw_hline(cx + i - 3, cy + i - 3, 7, COLOR_WHITE); /* \ stroke */
    draw_hline(cx - i - 3, cy + i - 3, 7, COLOR_WHITE); /* / stroke */
}
```

### `screen_draw_end(won, score)` — public

```c
uint16_t bg = won ? 0x0320 : 0x4000;   /* dark green or dark red */
screen_clear(bg);
if (won) draw_trophy(120, 80);
else     draw_x_icon(120, 80);
const char *msg = won ? "WIN!" : "LOSE";
draw_string(centered_x(msg, 3), 140, msg, COLOR_YELLOW, bg, 3);
draw_string(centered_x("SCORE", 1), 195, "SCORE", COLOR_WHITE, bg, 1);
draw_int_right_aligned(160, 210, score, COLOR_WHITE, bg, 2);
```

### src/main.c test

```c
display_module_init();
screen_draw_end(true, 999);
k_msleep(3000);
screen_draw_end(false, 0);
while (true) { k_msleep(1000); }
```

**Hardware test:** Green background + trophy + "WIN!" + score for 3 s, then red + X + "LOSE".

---

## Checkpoint 7 — Full Display Sequence (Main Loop)

**Git commit message:** `feat: wire all screens into complete display-only sequence`

### src/main.c final

```c
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <string.h>
#include "display.h"
#include "game_types.h"

int main(void)
{
    if (display_module_init() != 0) {
        return -1;
    }

    /* 1. Startup screen — logo + title held for 2 s */
    screen_draw_startup();
    k_msleep(2000);

    /* 2. Countdown 3 → 2 → 1, 1 s each */
    for (int i = 3; i >= 1; i--) {
        screen_draw_countdown(i);
        k_msleep(1000);
    }

    /* 3. Gameplay screen — full brick grid, paddle centred, ball at start,
     *    score 0, 3 lives. Held for 3 s (no input, no motion). */
    static uint8_t bricks[BRICK_ROWS][BRICK_COLS];
    memset(bricks, 1, sizeof(bricks));
    screen_draw_gameplay(bricks, PADDLE_X_INIT,
                         BALL_X_INIT, BALL_Y_INIT,
                         0, 3);
    k_msleep(3000);

    /* 4. End screen — win demo, then lose demo. */
    screen_draw_end(true, 999);
    k_msleep(3000);
    screen_draw_end(false, 0);

    while (true) {
        k_msleep(1000);
    }
    return 0;
}
```

**Hardware test:** Power-cycle the board and watch the round display run the
full sequence end-to-end with no input: title screen → 3-2-1 countdown →
gameplay screen with bricks/paddle/ball/HUD held for 3 s → green WIN screen
with trophy + score 999 for 3 s → red LOSE screen with X + score 0 (held).
No flicker, no torn frames, no garbage at the panel edge.

---

## Done

The display layer is now complete and self-contained: every screen renders
correctly, every helper has a single responsibility, and the full visual flow
runs from a stub `main()` with no sensor or game-state input. The next plan
will replace the scripted `main()` above with a real game loop driven by
MPU6050 tilt input and ball/brick physics.
