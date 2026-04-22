#include "main.h"
#include "led_flash.h"

extern struct led_flash_ctx led_flash;
extern struct k_msgq button_msgq;

static void button_evt_handler(void *p1, void *p2, void *p3);

K_THREAD_DEFINE(button_evt_tobj,
                1024,
                button_evt_handler,
                NULL, NULL, NULL,
                5,
                0,
                0);

static void button_evt_handler(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    struct button_event evt;

    while (1) {
        k_msgq_get(&button_msgq, &evt, K_FOREVER);

        if (evt.state) {
            printf("button: press detected at %u ms\n", evt.t_ms);
            led_flash_burst_start_or_restart(&led_flash);
        } else {
            /* release ignored */
        }
    }
}