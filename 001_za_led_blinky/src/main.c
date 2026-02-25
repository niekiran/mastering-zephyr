
#include <zephyr/kernel.h> 
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <stdio.h>

//check for alias_name of the led

#if !DT_HAS_ALIAS(led0)
    #error "Missing devicetree alias led0, Add it under /aliases node"
#endif


#define NODE_IDENT_LED0    DT_ALIAS(led0)
#define NODE_IDENT_LEDS    DT_PARENT(NODE_IDENT_LED0)

#define DELAY_MS           DT_PROP_OR(NODE_IDENT_LEDS, blink_period_ms, 500)

#define LED_LABLE          DT_PROP_OR(NODE_IDENT_LED0, label, "USER_LED")

// 1. Get the LED device from the devicetree using the led0 alias.
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(NODE_IDENT_LED0, gpios);

int main(void)
{

    bool led_state = false;

    // 2. Verify the device is ready with device_is_ready.
    if (!gpio_is_ready_dt(&led)) {
        return 0;
    }

    // 3. Configure the LED pin as a GPIO output with initial state off.
    if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE) < 0) {
        return 0;
    }
    // 4. Loop forever: toggle the LED and sleep for 500 milliseconds.
    while (true)
    {
        led_state = !led_state;
        if(gpio_pin_toggle_dt(&led) < 0) {
            return 0;
        }

        // 5. Optionally print a message to the console each cycle.
        printf("\nLED_STATE: %s %s\n\r", LED_LABLE, led_state ? "ON" : "OFF");
        k_msleep(DELAY_MS);
    }
    
    
	return 0;
}

