/*
 * input_backend.h — Backend contract for input drivers
 *
 * Every input backend (tilt, buttons, touch, …) must implement these
 * two functions.  Only one backend is compiled at a time, selected via
 * Kconfig (INPUT_BACKEND_*) and CMake target_sources_ifdef.
 *
 * This header is internal — game code includes input.h, not this file.
 */

#ifndef INCLUDE_INPUT_BACKEND_H_
#define INCLUDE_INPUT_BACKEND_H_

/**
 * input_backend_init() - Hardware-specific initialization.
 *
 * Acquire the device handle, perform calibration, initialize filters, etc.
 *
 * @return 0 on success, negative errno on failure.
 */
int input_backend_init(void);

/**
 * input_backend_read() - Hardware-specific input read.
 *
 * Fetch a fresh sample, run the signal processing pipeline, and return
 * the absolute paddle X position.
 *
 * @return Absolute paddle X position [0, DISPLAY_WIDTH - PADDLE_W].
 */
int input_backend_read(void);

#endif /* INCLUDE_INPUT_BACKEND_H_ */
