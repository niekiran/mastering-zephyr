#ifndef MAIN_H
#define MAIN_H


#include <errno.h>
#include <zephyr/kernel.h> 

#define NODE_IDENT_LED0        DT_ALIAS(led0)
#define NODE_IDENT_BUTTON0     DT_ALIAS(button0)

struct button_event {
    uint32_t t_ms;
    uint8_t  state;
};


#endif