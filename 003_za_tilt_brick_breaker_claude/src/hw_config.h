/*
 * hw_config.h — Hardware configuration for the tilt brick-breaker game.
 *
 * Edit this file to match your physical board setup.
 * No other source files need to be touched for hardware personalisation.
 */

#ifndef HW_CONFIG_H
#define HW_CONFIG_H

#include <zephyr/drivers/sensor.h>

/* -----------------------------------------------------------------------
 * TILT AXIS
 *
 * Choose which accelerometer axis controls the paddle.
 * The game is landscape: paddle is on the RIGHT and moves UP/DOWN.
 *
 * Device is tilted around the Y axis (pitch — nose up / nose down).
 * Pitch puts gravity onto the X accelerometer axis, so use ACCEL_X.
 *
 *   SENSOR_CHAN_ACCEL_X  —  pitch (nose-up/nose-down, tilt around Y axis)
 *                           → moves paddle up/down (correct for this board)
 *
 *   SENSOR_CHAN_ACCEL_Y  —  roll (lean left/right, tilt around X axis)
 *                           → use only if the board is mounted differently
 * ----------------------------------------------------------------------- */
#define HW_TILT_AXIS        SENSOR_CHAN_ACCEL_X

/* -----------------------------------------------------------------------
 * INVERT TILT DIRECTION
 *
 *   0  —  tilt right → paddle moves right  (default)
 *   1  —  tilt right → paddle moves left   (mirror if axis is reversed)
 * ----------------------------------------------------------------------- */
#define HW_TILT_INVERT      0

/* -----------------------------------------------------------------------
 * DEAD ZONE  (m/s²)
 *
 * Accelerometer readings whose absolute value is below this threshold
 * are treated as "no tilt" to prevent paddle drift when the device is
 * resting flat on a surface.  Raise this if your board is noisy.
 * ----------------------------------------------------------------------- */
#define HW_DEAD_ZONE_MS2    0.5

/* -----------------------------------------------------------------------
 * PADDLE SPEED FACTOR
 *
 * Multiplied against the raw acceleration (m/s²) to produce the pixel
 * delta per game tick.  Increase for a faster paddle.
 * ----------------------------------------------------------------------- */
#define HW_PADDLE_SPEED     3.0

/* -----------------------------------------------------------------------
 * PADDLE DELTA CLAMP  (pixels per tick)
 *
 * Maximum pixels the paddle can move in a single game tick regardless of
 * how hard the device is tilted.
 * ----------------------------------------------------------------------- */
#define HW_PADDLE_MAX_DELTA 8

#endif /* HW_CONFIG_H */
