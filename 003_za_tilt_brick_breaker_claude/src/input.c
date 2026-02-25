#include "input.h"
#include "hw_config.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(input, CONFIG_APP_LOG_LEVEL);

static const struct device *mpu;

void input_init(void)
{
    mpu = DEVICE_DT_GET_ONE(invensense_mpu6050);
    if (!device_is_ready(mpu)) {
        LOG_ERR("MPU6050 not ready");
        mpu = NULL;
    } else {
        LOG_INF("MPU6050 ready");
    }
}

int8_t input_get_paddle_delta(void)
{
    if (mpu == NULL) {
        return 0;
    }

    struct sensor_value accel;

    if (sensor_sample_fetch(mpu) < 0) {
        return 0;
    }

    if (sensor_channel_get(mpu, HW_TILT_AXIS, &accel) < 0) {
        return 0;
    }

    float tilt = (float)sensor_value_to_double(&accel);

    LOG_DBG("tilt (axis %d): %d.%06d", HW_TILT_AXIS, accel.val1, accel.val2);

    if (tilt > -HW_DEAD_ZONE_MS2 && tilt < HW_DEAD_ZONE_MS2) {
        return 0;
    }

#if HW_TILT_INVERT
    tilt = -tilt;
#endif

    int8_t delta = (int8_t)(tilt * HW_PADDLE_SPEED);

    if (delta >  HW_PADDLE_MAX_DELTA) delta =  HW_PADDLE_MAX_DELTA;
    if (delta < -HW_PADDLE_MAX_DELTA) delta = -HW_PADDLE_MAX_DELTA;

    return delta;
}
