#pragma once

#include <choc/types.h>

void keyboard_init(void);
void keyboard_irq(void);
int keyboard_getchar(void);
