
#include <zephyr/devicetree.h>
#include <stdio.h>
#include "led.h"
#include "button.h"
#include "led_flash.h"
#include "main.h"


/* If you really use led0/button0 aliases, this is the correct check */
#if !DT_NODE_HAS_STATUS(DT_ALIAS(led0), okay)
#error "Missing devicetree alias led0"
#endif

#if !DT_NODE_HAS_STATUS(DT_ALIAS(button0), okay)
#error "Missing devicetree alias button0"
#endif

static const struct gpio_dt_spec led_spec    = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec button_spec = GPIO_DT_SPEC_GET(DT_ALIAS(button0), gpios);

static struct button usr_button;
struct led_flash_ctx led_flash;

K_MSGQ_DEFINE(button_msgq, sizeof(struct button_event), 10, _Alignof(struct button_event));

void on_button(uint32_t t_ms, uint8_t state)
{
    struct button_event evt = {
        .t_ms = t_ms,
        .state = state,
    };

    if (k_msgq_put(&button_msgq, &evt, K_NO_WAIT) != 0) {
        /* optional: increment drop counter */
        /* atomic_inc(&drop_count); */
    }
}

int main(void)
{
    usr_button.button_spec = &button_spec;
    usr_button.app_cb = on_button;

    if (led_init(&led_spec) < 0) {
        return 0;
    }

    if (button_init(&usr_button) < 0) {
        return 0;
    }

    led_flash_init(&led_flash, &led_spec);

    k_sleep(K_FOREVER);
    return 0;
}
