#pragma once

#include <choc/types.h>

void pit_init(uint32_t hz);
void pit_on_irq(void);
uint64_t pit_ticks(void);
