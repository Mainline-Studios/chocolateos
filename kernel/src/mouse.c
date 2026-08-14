#include <choc/mouse.h>
#include <choc/gfx.h>
#include <choc/io.h>

static volatile int32_t mx;
static volatile int32_t my;
static volatile uint8_t buttons;
static volatile uint8_t packet[3];
static volatile int packet_i;
static volatile uint8_t dirty;
static int ready;

void mouse_init(void) {
    mx = 80;
    my = 80;
    buttons = 0;
    packet_i = 0;
    dirty = 1;
    ready = 1;
}

void mouse_feed(uint8_t b) {
    if (!ready) {
        return;
    }
    if (packet_i == 0 && (b & 0x08) == 0) {
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

void mouse_usb_rel(int dx, int dy, uint8_t btn) {
    mx += dx;
    my += dy;
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
    buttons = btn & 0x07;
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
