/*
 * input.h — Public input API for the game
 *
 * The game calls input_init() and input_read() without knowing which
 * hardware backend (tilt sensor, buttons, touch) is active.  The
 * active backend is selected at build time via Kconfig (INPUT_BACKEND_*).
 */

#ifndef INCLUDE_INPUT_H_
#define INCLUDE_INPUT_H_

/**
 * input_init() - Initialize the input subsystem.
 *
 * Delegates to the selected backend.  May perform startup calibration
 * (e.g. averaging accelerometer samples).  Call once before the game loop.
 *
 * @return 0 on success, negative errno on failure.
 */
int input_init(void);

/**
 * input_read() - Read one frame of input.
 *
 * Returns the absolute paddle X position mapped from the current tilt:
 *   0                          = full left
 *   (DISPLAY_WIDTH - PADDLE_W) = full right
 *   center                     = board level (or in dead zone)
 *
 * The caller assigns directly:
 *   paddle_x = input_read();
 */
int input_read(void);

#endif /* INCLUDE_INPUT_H_ */
