#include <choc/mouse.h>
#include <choc/io.h>
#include <choc/gfx.h>

#define KBD_DATA 0x60
#define KBD_CMD  0x64

static volatile int32_t mx;
static volatile int32_t my;
static volatile uint8_t buttons;
static volatile uint8_t packet[3];
static volatile int packet_i;
static volatile uint8_t dirty;
static int ready;

static int wait_write(void) {
    for (int i = 0; i < 100000; i++) {
        if (!(inb(KBD_CMD) & 2)) {
            return 1;
        }
    }
    return 0;
}

static int wait_read(void) {
    for (int i = 0; i < 100000; i++) {
        if (inb(KBD_CMD) & 1) {
            return 1;
        }
    }
    return 0;
}

static void write_cmd(uint8_t cmd) {
    wait_write();
    outb(KBD_CMD, cmd);
}

static void write_data(uint8_t data) {
    wait_write();
    outb(KBD_DATA, data);
}

static uint8_t read_data(void) {
    if (!wait_read()) {
        return 0;
    }
    return inb(KBD_DATA);
}

static void mouse_write(uint8_t data) {
    write_cmd(0xD4);
    write_data(data);
    read_data(); /* ACK */
}

void mouse_init(void) {
    mx = 40;
    my = 40;
    buttons = 0;
    packet_i = 0;
    dirty = 1;
    ready = 0;

    write_cmd(0xA8);
    write_cmd(0x20);
    uint8_t status = read_data();
    status |= 2;
    status &= (uint8_t)~0x20;
    write_cmd(0x60);
    write_data(status);

    mouse_write(0xF6);
    mouse_write(0xF4);
    ready = 1;
}

void mouse_irq(void) {
    uint8_t b = inb(KBD_DATA);
    if (!ready) {
        return;
    }
    if (packet_i == 0 && !(b & 0x08)) {
        return;
    }
    packet[packet_i++] = b;
    if (packet_i < 3) {
        return;
    }
    packet_i = 0;

    int32_t dx = (int8_t)packet[1];
    int32_t dy = (int8_t)packet[2];
    if (packet[0] & 0x40) {
        dx = 0;
    }
    if (packet[0] & 0x80) {
        dy = 0;
    }

    mx += dx;
    my -= dy;
    if (mx < 0) {
        mx = 0;
    }
    if (my < 0) {
        my = 0;
    }
    int32_t w = (int32_t)gfx_width();
    int32_t h = (int32_t)gfx_height();
    if (w > 0 && mx >= w) {
        mx = w - 1;
    }
    if (h > 0 && my >= h) {
        my = h - 1;
    }
    buttons = packet[0] & 0x07;
    dirty = 1;
}

struct mouse_state mouse_poll(void) {
    struct mouse_state s;
    irq_disable();
    s.x = mx;
    s.y = my;
    s.buttons = buttons;
    s.changed = dirty;
    dirty = 0;
    irq_enable();
    return s;
}
