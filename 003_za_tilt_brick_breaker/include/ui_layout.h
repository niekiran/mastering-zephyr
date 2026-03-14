#ifndef INCLUDE_UI_LAYOUT_H_
#define INCLUDE_UI_LAYOUT_H_

/*
 * UI layout constants for all game screens.
 *
 * HOW POSITIONING WORKS
 * ---------------------
 * All vertical positions are signed pixel offsets from the display
 * centre (CONFIG_DISPLAY_CIRCLE_CENTER_Y).
 *   Negative = above centre,  Positive = below centre.
 *
 * Horizontal positions are computed at runtime so that text and
 * decorations are always centred on CONFIG_DISPLAY_CIRCLE_CENTER_X.
 *
 * HOW TEXT SIZE WORKS
 * -------------------
 * The 5x7 bitmap font is drawn at an integer "scale" multiplier:
 *   scale 1 -> each character is  5 x 7  pixels  (tiny)
 *   scale 2 -> each character is 10 x 14 pixels  (small)
 *   scale 3 -> each character is 15 x 21 pixels  (medium)
 *   scale 4 -> each character is 20 x 28 pixels  (large)
 * Character spacing is 6*scale pixels (5 glyph + 1 gap column).
 *
 * TEXT_WIDTH(len, scale) gives the total pixel width of a string
 * with `len` characters at the given scale.
 */

/* ---- Helper ---- */

/* Total pixel width of `len` characters at `scale`.
 * Each character occupies 6*scale pixels (5 glyph + 1 gap),
 * minus the trailing gap on the last character.              */
#define TEXT_WIDTH(len, scale) ((len) * 6 * (scale) - (scale))

/* ---- Startup / splash screen ---- */

/* Font scale for each text element.
 * Increase for larger text, decrease for smaller.
 * Valid range: 1-6 (larger values may exceed the display).     */
#define STARTUP_TITLE_SCALE        4   /* "TILT"         */
#define STARTUP_SUBTITLE_SCALE     3   /* "BREAKER"      */
#define STARTUP_HINT_SCALE         2   /* "TILT TO PLAY" */

/* Vertical pixel offset from display centre for each element.
 * Negative = higher on screen, positive = lower.
 * Adjust these to move elements up or down.                    */
#define STARTUP_BRICKS_Y_OFF     (-82) /* decorative brick row  */
#define STARTUP_TITLE_Y_OFF      (-52) /* game title top edge   */
#define STARTUP_SUBTITLE_Y_OFF   (-18) /* subtitle top edge     */
#define STARTUP_BALL_Y_OFF        (18) /* decorative ball centre */
#define STARTUP_PADDLE_Y_OFF      (35) /* decorative paddle top  */
#define STARTUP_HINT_Y_OFF        (65) /* hint text top edge    */

/* Decorative brick row — small coloured rectangles across the top.
 * Width/height are per-brick in pixels; gap is spacing between them.
 * Count is how many bricks to draw (colours cycle automatically).  */
#define STARTUP_BRICK_W            30  /* single brick width  (px) */
#define STARTUP_BRICK_H            12  /* single brick height (px) */
#define STARTUP_BRICK_GAP           4  /* gap between bricks  (px) */
#define STARTUP_BRICK_COUNT         5  /* number of bricks         */

/* Decorative paddle rectangle shown below the ball.               */
#define STARTUP_PADDLE_W           60  /* paddle width  (px)       */
#define STARTUP_PADDLE_H            8  /* paddle height (px)       */

/* ---- HUD (Heads-Up Display) ---- */

/* Font scale for HUD text elements.                                     */
#define HUD_SCORE_SCALE        1

/* Score label + value position.
 * X is left edge; Y is top edge of the text baseline.
 * At y ~ 16 the circular display spans roughly x = 61..179.            */
#define HUD_SCORE_X           65
#define HUD_SCORE_Y           16

/* Lives display — hearts drawn from right to left.
 * X is the centre of the rightmost heart; Y is the heart centre.       */
#define HUD_LIVES_X          172
#define HUD_LIVES_Y           19
#define HUD_HEART_SPACING     13  /* centre-to-centre distance (px)     */

/* ---- Countdown screen ---- */

/* Font scale for the countdown digit (3, 2, 1).
 * scale 4 -> digit is 20x28 px, clearly visible at screen centre. */
#define COUNTDOWN_SCALE             4

/* ---- End screen (win / lose) ---- */

/* Vertical pixel offsets from display centre.                           */
#define END_ICON_Y_OFF           (-40) /* trophy / X icon centre         */
#define END_MSG_Y_OFF             (28) /* "WIN!" / "LOSE" top edge       */
#define END_SCORE_Y_OFF          (75) /* "SCORE 123" line top edge       */

/* Font scales.                                                          */
#define END_MSG_SCALE              3   /* result message                  */
#define END_SCORE_SCALE            2   /* "SCORE 123" line               */

#endif /* INCLUDE_UI_LAYOUT_H_ */
