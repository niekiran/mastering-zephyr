#ifndef BTN_H
#define BTN_H


typedef void (*button_isr_cb_t)(uint32_t t_ms, uint8_t level);

struct button {
 const struct gpio_dt_spec *button_spec;
 struct gpio_callback button_cb;
 button_isr_cb_t app_cb;
 //user data
};


int button_init(struct button *button);

#endif