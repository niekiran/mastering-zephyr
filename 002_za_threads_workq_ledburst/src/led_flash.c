#include "led_flash.h"
#include "led.h"

static void led_flash_work_handler(struct k_work *work)
{
    uint32_t rem;

    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    struct led_flash_ctx *led_flash = CONTAINER_OF(dwork, struct led_flash_ctx, work);

    k_spinlock_key_t key = k_spin_lock(&led_flash->lock);

    if (led_flash->toggles_remaining == 0U) {
        k_spin_unlock(&led_flash->lock, key);
        led_off(led_flash->led);
        return;
    }

    led_flash->toggles_remaining--;
    rem = led_flash->toggles_remaining;

    k_spin_unlock(&led_flash->lock, key);

    printf("led_flash: toggles_remaining=%u\n", rem);

    led_toggle(led_flash->led);

    if (rem > 0U) {
        (void)k_work_reschedule(&led_flash->work, K_MSEC(led_flash->period_ms));
    } else {
        printf("led_flash: burst over, LED off\n");
        led_off(led_flash->led);
    }
}

void led_flash_init(struct led_flash_ctx *led_flash, const struct gpio_dt_spec *led)
{
    led_flash->period_ms = 250;
    led_flash->count = 6;
    led_flash->toggles_remaining = led_flash->count * 2U;  /* kept as requested */
    led_flash->led = led;

    led_off(led_flash->led);

    (void)k_work_init_delayable(&led_flash->work, led_flash_work_handler);
}

void led_flash_burst_start_or_restart(struct led_flash_ctx *led_flash)
{
    k_spinlock_key_t key = k_spin_lock(&led_flash->lock);
    led_flash->toggles_remaining = led_flash->count * 2U;
    k_spin_unlock(&led_flash->lock, key);

    (void)k_work_reschedule(&led_flash->work, K_MSEC(500));
}

void led_flash_burst_cancel(struct led_flash_ctx *led_flash)
{
    (void)k_work_cancel_delayable(&led_flash->work);

    k_spinlock_key_t key = k_spin_lock(&led_flash->lock);
    led_flash->toggles_remaining = 0U;
    k_spin_unlock(&led_flash->lock, key);

    led_off(led_flash->led);
}