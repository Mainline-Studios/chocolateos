#pragma once

#include <choc/types.h>

struct mouse_state {
    int32_t x;
    int32_t y;
    uint8_t buttons;
    uint8_t changed;
};

void mouse_init(void);
void mouse_feed(uint8_t byte);
void mouse_usb_rel(int dx, int dy, uint8_t btn);
struct mouse_state mouse_poll(void);
