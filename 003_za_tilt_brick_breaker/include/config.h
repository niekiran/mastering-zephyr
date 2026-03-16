/*
 * config.h — User-configurable application settings
 *
 * Device orientation and tilt mapping settings are configured via Kconfig.
 * Browse and change them with: west build -t menuconfig
 * Look under "Tilt Brick Breaker" > "Tilt Input".
 *
 * This file contains only settings that cannot be expressed in Kconfig:
 *   - TILT_LPF_ALPHA: float conversion helper for CONFIG_TILT_LPF_ALPHA_PCT
 *     (Kconfig has no float type).
 *
 * All defaults are derived from docs/spec.md.
 */

#ifndef INCLUDE_CONFIG_H_
#define INCLUDE_CONFIG_H_

/* --------------------------------------------------------------------------
 * TILT_LPF_ALPHA — Float conversion helper for the Kconfig LPF setting
 * --------------------------------------------------------------------------
 * The smoothing factor is stored in Kconfig as an integer percentage
 * (CONFIG_TILT_LPF_ALPHA_PCT, range 0-100) because Kconfig has no float
 * type.  Use this macro in code for the float value; do not use
 * CONFIG_TILT_LPF_ALPHA_PCT directly.
 *
 * To change the value: west build -t menuconfig
 *   -> Tilt Brick Breaker -> Tilt Input -> Low-pass filter alpha percentage
 */
#define TILT_LPF_ALPHA  (CONFIG_TILT_LPF_ALPHA_PCT / 100.0f)

/* --------------------------------------------------------------------------
 * Game configuration — edit these macros to tune gameplay
 * -------------------------------------------------------------------------- */
#define BALL_SPEED_INIT      2      /* ball velocity magnitude (pixels/tick)  */
#define BALL_SPEED_MAX       5      /* maximum ball speed after ramp-up       */
#define SPEED_UP_INTERVAL    8      /* bricks destroyed per speed increase    */
#define LIVES_INIT           3      /* starting number of lives               */
#define BRICK_SCORE          10     /* points awarded per brick destroyed      */
#define FRAME_MS             33     /* target frame period in ms (~30 fps)    */

#endif /* INCLUDE_CONFIG_H_ */
