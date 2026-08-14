#pragma once

#include <choc/types.h>

void keyboard_init(void);
void keyboard_feed(uint8_t scancode);
void keyboard_push(char c);
int keyboard_getchar(void);
