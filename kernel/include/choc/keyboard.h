#pragma once

#include <choc/types.h>

void keyboard_init(void);
void keyboard_feed(uint8_t scancode);
int keyboard_getchar(void);
