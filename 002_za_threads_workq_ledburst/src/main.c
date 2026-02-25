
#include <zephyr/devicetree.h>
#include <stdio.h>
#include "led.h"
#include "button.h"
#include "led_flash.h"
#include "main.h"



//check for alias_name of the led
#if !DT_HAS_ALIAS(led0)
    #error "Missing devicetree alias led0, Add it under /aliases node"
#endif

#if !DT_HAS_ALIAS(button0)
    #error "Missing devicetree alias button0, Add it under /aliases node"
#endif


// 1. Get the LED device from the devicetree using the led0 alias.
static const struct gpio_dt_spec led_spec = GPIO_DT_SPEC_GET(NODE_IDENT_LED0, gpios);
static const struct gpio_dt_spec button_spec = GPIO_DT_SPEC_GET(NODE_IDENT_BUTTON0, gpios);

static struct button usr_button;
struct led_flash_ctx led_flash;

K_MSGQ_DEFINE(button_msgq, sizeof(struct button_event), 10, 1);

void on_button(uint32_t t_ms, uint8_t state) {

    struct button_event evt;
    evt.t_ms = t_ms;
    evt.state = state;

    //put the event into message queue
      if (k_msgq_put(&button_msgq, &evt, K_NO_WAIT) != 0) {

      }
  
}

int main(void)
{

    usr_button.button_spec = &button_spec;
    usr_button.app_cb = on_button;

    if(led_init(&led_spec) < 0) {
        return 0;
    }

    if(button_init(&usr_button) < 0) {
        return 0;
    }

    led_flash_init(&led_flash, &led_spec);

    // Loop forever
    while (true)
    {
        
    }
    
    
	return 0;
}

