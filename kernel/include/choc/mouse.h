#pragma once

#include <choc/types.h>

struct mouse_state {
    int32_t x;
    int32_t y;
    uint8_t buttons;
    uint8_t changed;
};

void mouse_init(void);
void mouse_irq(void);
struct mouse_state mouse_poll(void);
