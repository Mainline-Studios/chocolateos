#include <choc/ps2.h>
#include <choc/keyboard.h>
#include <choc/mouse.h>
#include <choc/io.h>

#define PS2_DATA 0x60
#define PS2_CMD  0x64

static int wait_write(void) {
    for (int i = 0; i < 100000; i++) {
        if (!(inb(PS2_CMD) & 2)) {
            return 1;
        }
        io_wait();
    }
    return 0;
}

static int wait_read(void) {
    for (int i = 0; i < 100000; i++) {
        if (inb(PS2_CMD) & 1) {
            return 1;
        }
        io_wait();
    }
    return 0;
}

static void flush(void) {
    for (int i = 0; i < 64; i++) {
        if (!(inb(PS2_CMD) & 1)) {
            break;
        }
        (void)inb(PS2_DATA);
        io_wait();
    }
}

static void write_cmd(uint8_t cmd) {
    wait_write();
    outb(PS2_CMD, cmd);
}

static void write_data(uint8_t data) {
    wait_write();
    outb(PS2_DATA, data);
}

static uint8_t read_data(void) {
    if (!wait_read()) {
        return 0;
    }
    return inb(PS2_DATA);
}

static void mouse_cmd(uint8_t data) {
    write_cmd(0xD4);
    write_data(data);
    read_data();
}

void ps2_init(void) {
    flush();

    /* Enable keyboard and mouse devices. */
    write_cmd(0xAE);
    write_cmd(0xA8);
    flush();

    write_cmd(0x20);
    uint8_t status = read_data();
    status |= 0x03;          /* IRQ1 + IRQ12 */
    status &= (uint8_t)~0x30; /* clocks on */
    write_cmd(0x60);
    write_data(status);
    flush();

    /* Enable keyboard scanning. */
    write_data(0xF4);
    read_data();

    mouse_cmd(0xF6);
    mouse_cmd(0xF4);
    flush();
}

void ps2_irq(void) {
    for (int i = 0; i < 16; i++) {
        uint8_t st = inb(PS2_CMD);
        if (!(st & 1)) {
            break;
        }
        uint8_t data = inb(PS2_DATA);
        if (st & 0x20) {
            mouse_feed(data);
        } else {
            keyboard_feed(data);
        }
    }
}
