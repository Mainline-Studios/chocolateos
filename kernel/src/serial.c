#include <choc/serial.h>
#include <choc/io.h>

#define COM1 0x3F8

void serial_init(void) {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
}

static int serial_tx_ready(void) {
    return inb(COM1 + 5) & 0x20;
}

void serial_write(char c) {
    if (c == '\n') {
        serial_write('\r');
    }
    while (!serial_tx_ready()) {
    }
    outb(COM1, (uint8_t)c);
}

void serial_write_str(const char *s) {
    while (*s) {
        serial_write(*s++);
    }
}

int serial_getchar(void) {
    if (!(inb(COM1 + 5) & 0x01)) {
        return -1;
    }
    uint8_t c = inb(COM1);
    if (c == '\r') {
        return '\n';
    }
    return (int)c;
}
