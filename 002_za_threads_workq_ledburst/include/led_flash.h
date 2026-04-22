#ifndef LED_FLASH_H
#define LED_FLASH_H

#include "main.h"

struct led_flash_ctx
{
    uint32_t toggles_remaining;
    uint32_t period_ms;
    uint32_t count;
    struct k_spinlock lock;
    struct k_work_delayable work;
    
    const struct gpio_dt_spec *led;
};

void led_flash_init(struct led_flash_ctx *led_flash, const struct gpio_dt_spec *led);
void led_flash_burst_cancel(struct led_flash_ctx *led_flash);
void led_flash_burst_start_or_restart(struct led_flash_ctx *led_flash);


#endif