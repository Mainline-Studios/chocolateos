#pragma once

#include <choc/types.h>

void pic_init(void);
void pic_eoi(uint8_t irq);
void pic_set_mask(uint8_t irq, bool masked);
