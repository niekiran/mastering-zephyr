# Development Notes

## Display Blanking for Instant Screen Transitions

**Problem**: Screen transitions (startup, countdown, gameplay) showed visible
top-to-bottom painting (~45 ms rendering time at 32 MHz SPI).

**Fix**: Use GC9A01 display blanking (DISPOFF/DISPON) to hide rendering.

1. `display_blanking_on()` — freezes display on previous frame
2. Draw all content to display RAM (~45 ms, invisible to user)
3. `display_blanking_off()` — new frame appears instantly

**Functions affected** (all in `src/display.c`):

### screen_draw_startup()

Before:
```c
void screen_draw_startup(void)
{
    screen_clear(COLOR_DARK_BLUE);
    /* ... draw title, bricks, ball, paddle, hint ... */
}
```

Now:
```c
void screen_draw_startup(void)
{
    display_blanking_on(disp_dev);
    screen_clear(COLOR_DARK_BLUE);
    /* ... draw title, bricks, ball, paddle, hint ... */
    display_blanking_off(disp_dev);
}
```

### screen_draw_countdown()

Before:
```c
void screen_draw_countdown(int n)
{
    screen_clear(COLOR_BLACK);
    draw_int(cx - dw / 2, cy - dh / 2, n, ...);
}
```

Now:
```c
void screen_draw_countdown(int n)
{
    display_blanking_on(disp_dev);
    screen_clear(COLOR_BLACK);
    draw_int(cx - dw / 2, cy - dh / 2, n, ...);
    display_blanking_off(disp_dev);
}
```

### screen_draw_gameplay()

Before:
```c
void screen_draw_gameplay(...)
{
    /* Single-pass top-to-bottom: fill background per region to avoid
     * clear flash, then draw elements on top. */
    draw_hud(score, lives);
    fill_rect(0, HUD_HEIGHT, WIDTH, BRICK_ORIGIN_Y - HUD_HEIGHT, DARK_BLUE);
    fill_rect(0, BRICK_ORIGIN_Y, WIDTH, bricks_bottom - BRICK_ORIGIN_Y, DARK_BLUE);
    draw_bricks(bricks);
    fill_rect(0, bricks_bottom, WIDTH, HEIGHT - bricks_bottom, DARK_BLUE);
    draw_paddle(paddle_x);
    draw_ball(ball_x, ball_y);
}
```

Now:
```c
void screen_draw_gameplay(...)
{
    display_blanking_on(disp_dev);
    screen_clear(COLOR_DARK_BLUE);
    draw_hud(score, lives);
    draw_bricks(bricks);
    draw_paddle(paddle_x);
    draw_ball(ball_x, ball_y);
    display_blanking_off(disp_dev);
}
```

Reverted to simple `screen_clear()` + draw since the rendering is invisible
during blanking. The single-pass region-by-region approach is no longer needed.

**Note**: This technique is for full-screen transitions only. During live
gameplay with dirty-region rendering (Fix 2), blanking is not needed — small
per-frame updates (~5 ms) are fast enough to be imperceptible.
