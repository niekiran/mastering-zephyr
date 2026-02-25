
#include <zephyr/drivers/gpio.h>
#include "button.h"

#define NODE_IDENT_BUTTONS    DT_PARENT(NODE_IDENT_BUTTON0)
#define BTN_LABLE          DT_PROP_OR(NODE_IDENT_BUTTON0, label, "USER_BUTTON")


void button_isr(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins) {

    // CONTAINER_OF(ptr, type, member)
    // 'ptr' which points to the member field
    // 'type' which is the struct type that contains that member
    // 'member' which is the exact field name inside that struct, for example button_cb
    struct button *button = CONTAINER_OF(cb, struct button, button_cb);
    if(button->app_cb != NULL) {
        uint32_t t_ms = k_uptime_get_32();
        uint8_t state = (gpio_pin_get_dt(button->button_spec) > 0) ? 1U : 0U;
         button->app_cb(t_ms, state);
    }
       
}

int button_init(struct button *button)
{
    int ret;
 
    if(button == NULL || button->button_spec == NULL) {
        return -EINVAL;
    }
    
    if (!gpio_is_ready_dt(button->button_spec)) {
        return -ENODEV;
    }

    if ((ret = gpio_pin_configure_dt(button->button_spec, GPIO_INPUT)) < 0) {
        return ret;
    }

    //1. init the gpio_callback structure 
    gpio_init_callback(&button->button_cb, button_isr, BIT(button->button_spec->pin));
    //2. install the button isr (add callback)
    if ((ret = gpio_add_callback_dt(button->button_spec, &button->button_cb)) < 0) {
        return ret;
    }
    //3. confgirue the pin for interrupt(edge or level triggering)
    if ((ret = gpio_pin_interrupt_configure_dt(button->button_spec, \ 
                                        GPIO_INT_EDGE_TO_ACTIVE)) < 0) {
        return ret;
    }

    return 0;
}


