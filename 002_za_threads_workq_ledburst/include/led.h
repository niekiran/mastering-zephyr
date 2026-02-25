#ifndef LED_H
#define LED_H

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/drivers/gpio.h>
#include "main.h"

int led_init(const struct gpio_dt_spec *led);
int led_toggle(const struct gpio_dt_spec *led);
int led_off(const struct gpio_dt_spec *led);
bool led_get_state(void);
uint32_t led_get_blink_period_ms(void);
const char *led_get_label(void);

#endif