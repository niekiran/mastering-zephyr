/*
 * input_tilt.c — MPU6050 tilt input backend (position mode)
 *
 * Reads the onboard MPU6050 accelerometer and maps tilt to an absolute
 * paddle X position.  Signal processing pipeline:
 *
 *   raw accel → calibrate → axis swap/select → invert →
 *   LPF → dead zone → clamp → map to absolute X
 *
 * All tuning parameters come from Kconfig (CONFIG_TILT_*).
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "input_backend.h"
#include "config.h"
#include "game_types.h"

LOG_MODULE_REGISTER(input_tilt, CONFIG_APP_LOG_LEVEL);

/* ---- Position mapping constants (circular display) ---- */
#define PADDLE_CENTER     ((PADDLE_X_MIN + PADDLE_X_MAX) / 2)
#define PADDLE_HALF_RANGE (PADDLE_X_MAX - PADDLE_CENTER)

/* ---- State ---- */
static const struct device *mpu_dev;
static int32_t cal_bias_mg;  /* calibration offset in milli-g */
static float   filtered_mg;  /* LPF state */
static bool    first_sample; /* true until first LPF sample   */

/* ---- Helpers ---- */

/**
 * read_tilt_mg() - Read the configured accelerometer axis in milli-g.
 *
 * Returns 0 on sensor failure so the caller can treat it as "no tilt".
 */
static int32_t read_tilt_mg(void)
{
	struct sensor_value accel[3];
	int rc;

	rc = sensor_sample_fetch(mpu_dev);
	if (rc != 0) {
		LOG_ERR("sensor_sample_fetch failed: %d", rc);
		return 0;
	}

	rc = sensor_channel_get(mpu_dev, SENSOR_CHAN_ACCEL_XYZ, accel);
	if (rc != 0) {
		LOG_ERR("sensor_channel_get failed: %d", rc);
		return 0;
	}

	/* Axis swap: exchange X and Y if enabled */
	if (IS_ENABLED(CONFIG_TILT_AXIS_SWAP)) {
		struct sensor_value tmp = accel[0];

		accel[0] = accel[1];
		accel[1] = tmp;
	}

	/* Select configured axis and convert to milli-g */
	return sensor_ms2_to_mg(&accel[CONFIG_TILT_AXIS]);
}

/* ---- Backend contract implementation ---- */

int input_backend_init(void)
{
	mpu_dev = DEVICE_DT_GET_ONE(invensense_mpu6050);
	if (!device_is_ready(mpu_dev)) {
		LOG_ERR("MPU6050 device not ready");
		return -ENODEV;
	}

	cal_bias_mg = 0;
	filtered_mg = 0.0f;
	first_sample = true;

	/* Startup calibration: average samples to find the rest bias */
	if (IS_ENABLED(CONFIG_TILT_CALIBRATION_ENABLE)) {
		int32_t sum = 0;

		LOG_INF("Calibrating (%d samples)...",
			CONFIG_TILT_CALIBRATION_SAMPLES);

		for (int i = 0; i < CONFIG_TILT_CALIBRATION_SAMPLES; i++) {
			sum += read_tilt_mg();
			k_msleep(10); /* ~100 Hz cadence */
		}
		cal_bias_mg = sum / CONFIG_TILT_CALIBRATION_SAMPLES;
		LOG_INF("Calibration done, bias = %d mg", cal_bias_mg);
	}

	return 0;
}

int input_backend_read(void)
{
	/* 1. Read raw milli-g from selected axis */
	int32_t raw_mg = read_tilt_mg();

	/* 2. Subtract calibration bias */
	int32_t tilt_mg = raw_mg - cal_bias_mg;

	/* 3. Apply sign inversion (spec: paddle_velocity ∝ −accel_x) */
	if (IS_ENABLED(CONFIG_TILT_INVERT)) {
		tilt_mg = -tilt_mg;
	}

	/* 4. Low-pass filter (exponential moving average) */
	float sample = (float)tilt_mg;

	if (first_sample) {
		filtered_mg = sample;
		first_sample = false;
	} else {
		filtered_mg = TILT_LPF_ALPHA * sample
			    + (1.0f - TILT_LPF_ALPHA) * filtered_mg;
	}

	/* 5. Dead zone — readings below threshold map to center */
	int32_t filt = (int32_t)filtered_mg;
	int32_t sign = (filt >= 0) ? 1 : -1;
	int32_t abs_mg = (filt >= 0) ? filt : -filt;

	if (abs_mg < CONFIG_TILT_DEAD_ZONE_MG) {
		return PADDLE_CENTER;
	}

	/* 6. Remove dead zone offset and clamp to effective range */
	abs_mg -= CONFIG_TILT_DEAD_ZONE_MG;

	int32_t effective_range = CONFIG_TILT_MAX_MG - CONFIG_TILT_DEAD_ZONE_MG;

	if (effective_range <= 0) {
		effective_range = 1;
	}
	if (abs_mg > effective_range) {
		abs_mg = effective_range;
	}

	/* 7. Map to absolute paddle X position */
	int32_t offset = (abs_mg * PADDLE_HALF_RANGE) / effective_range;
	int32_t paddle_x = PADDLE_CENTER + sign * offset;

	/* 8. Clamp to valid circular range */
	if (paddle_x < PADDLE_X_MIN) {
		paddle_x = PADDLE_X_MIN;
	}
	if (paddle_x > PADDLE_X_MAX) {
		paddle_x = PADDLE_X_MAX;
	}

	return (int)paddle_x;
}
