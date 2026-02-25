#include "led.h"


#define NODE_IDENT_LEDS    DT_PARENT(NODE_IDENT_LED0)

#define DELAY_MS           DT_PROP_OR(NODE_IDENT_LEDS, blink_period_ms, 500)

#define LED_LABLE          DT_PROP_OR(NODE_IDENT_LED0, label, "USER_LED")

static bool led_state = false;

int led_init(const struct gpio_dt_spec *led)
{

    if(led == NULL) {
        return -EINVAL;
    }

    //Verify the device is ready with device_is_ready.
    if (!gpio_is_ready_dt(led)) {
        return -ENODEV;
    }

    led_state = false;

    //Configure the LED pin as a GPIO output with initial state off.
    return gpio_pin_configure_dt(led, GPIO_OUTPUT_INACTIVE);
}

int led_toggle(const struct gpio_dt_spec *led)
{
    if(led == NULL) {
        return -EINVAL;
    }

    int ret = gpio_pin_toggle_dt(led);
    if (ret == 0) {
        led_state = !led_state;
    }

    return ret;
}


int led_off(const struct gpio_dt_spec *led) {
    
    if(led == NULL) {
        return -EINVAL;
    }

    return gpio_pin_set_dt(led, 0);
}

bool led_get_state(void)
{
    return led_state;
}


uint32_t led_get_blink_period_ms(void)
{
    return DELAY_MS;
}


const char *led_get_label(void)
{
    return LED_LABLE;
}
