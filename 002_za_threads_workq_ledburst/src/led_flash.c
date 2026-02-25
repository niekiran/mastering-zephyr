
#include "led_flash.h"
#include "led.h"


static void led_flash_work_handler(struct k_work *work)
{
    uint32_t rem = 0;
    /* flash LED here, then reschedule if needed. */

    //1. first check remaining led toggles counter; if == 0 , do led_off()
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    
    struct led_flash_ctx *led_flash = CONTAINER_OF(dwork, struct led_flash_ctx, work);

    if (led_flash->toggles_remaining == 0U) {
            led_off(led_flash->led);
            return;
    }

    printf("led flash: toggles_remaining = %d\n", led_flash->toggles_remaining);
    
    //2. decrement number of led toggles required
    led_flash->toggles_remaining--;
    rem = led_flash->toggles_remaining;
    //3. Toggle an LED
    led_toggle(led_flash->led);
    //4. Reschedule the work item with 'led_flash_period_ms'
    if(rem > 0U) {
         (void)k_work_reschedule(&led_flash->work, K_MSEC(led_flash->period_ms));
    } else {
        printf("led Flash: Brust over. LED is off\n");
        led_off(led_flash->led);
    }
}

void led_flash_init(struct led_flash_ctx *led_flash, const struct gpio_dt_spec *led)
{
    led_flash->period_ms= 250;
    led_flash->count = 6;
    led_flash->toggles_remaining = led_flash->count * 2U;
    led_flash->led  = led;

    led_off(led_flash->led);

    (void)k_work_init_delayable(&led_flash->work, led_flash_work_handler);
}

void led_flash_burst_start_or_restart(struct led_flash_ctx *led_flash)
{
    led_flash->toggles_remaining = led_flash->count * 2U;
    (void)k_work_reschedule(&led_flash->work, K_MSEC(500));
}

void led_flash_burst_cancel(struct led_flash_ctx *led_flash)
{
   (void)k_work_cancel_delayable(&led_flash->work);
   led_flash->toggles_remaining = 0;
   led_off(led_flash->led);
}
