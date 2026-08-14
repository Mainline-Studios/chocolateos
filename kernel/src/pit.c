#include <choc/pit.h>
#include <choc/io.h>

#define PIT_CH0 0x40
#define PIT_CMD 0x43
#define PIT_BASE_HZ 1193182

static volatile uint64_t ticks;

void pit_init(uint32_t hz) {
    uint32_t divisor = PIT_BASE_HZ / hz;
    outb(PIT_CMD, 0x36);
    outb(PIT_CH0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CH0, (uint8_t)((divisor >> 8) & 0xFF));
    ticks = 0;
}

void pit_on_irq(void) {
    ticks++;
}

uint64_t pit_ticks(void) {
    return ticks;
}
