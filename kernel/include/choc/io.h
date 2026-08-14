#pragma once

#include <choc/types.h>

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void io_wait(void) {
    outb(0x80, 0);
}

static inline void cpu_halt(void) {
    for (;;) {
        __asm__ volatile("hlt");
    }
}

static inline void irq_enable(void) {
    __asm__ volatile("sti");
}

static inline void irq_disable(void) {
    __asm__ volatile("cli");
}
