#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/byteorder.h>

int main(void)
{
    const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (!device_is_ready(dev)) { return -ENODEV; }

    /* Fill entire screen red using a 240-pixel row buffer */
    static uint16_t row[240];
    for (int i = 0; i < 240; i++) row[i] = sys_cpu_to_be16(0xF800); /* red RGB565, byte-swapped */

    struct display_buffer_descriptor d = {
        .buf_size = sizeof(row), .width = 240, .height = 1, .pitch = 240,
    };
    for (int y = 0; y < 240; y++) {
        display_write(dev, 0, y, &d, row);
    }
    display_blanking_off(dev);

    while (true) { k_msleep(1000); }
    return 0;
}
