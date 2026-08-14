#include <choc/usb.h>
#include <choc/keyboard.h>
#include <choc/mouse.h>
#include <choc/types.h>

static const char hid_keys[128] = {
    0, 0, 0, 0, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
    'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '\n', 27, '\b', '\t', ' ',
    '-', '=', '[', ']', '\\', 0, ';', '\'', '`', ',', '.', '/',
};

static const char hid_shift[128] = {
    0, 0, 0, 0, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
    'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '\n', 27, '\b', '\t', ' ',
    '_', '+', '{', '}', '|', 0, ':', '"', '~', '<', '>', '?',
};

static uint8_t last_keys[6];

void usb_hid_boot_keyboard(const uint8_t *report, int len) {
    if (len < 8) {
        return;
    }
    int shift = (report[0] & 0x22) != 0;
    for (int i = 2; i < 8; i++) {
        uint8_t k = report[i];
        if (!k) {
            continue;
        }
        int was = 0;
        for (int j = 0; j < 6; j++) {
            if (last_keys[j] == k) {
                was = 1;
            }
        }
        if (was) {
            continue;
        }
        if (k < 128) {
            char c = shift ? hid_shift[k] : hid_keys[k];
            if (c) {
                keyboard_push(c);
            }
        }
    }
    for (int i = 0; i < 6; i++) {
        last_keys[i] = report[i + 2];
    }
}

void usb_hid_boot_mouse(const uint8_t *report, int len) {
    if (len < 3) {
        return;
    }
    mouse_usb_rel((int8_t)report[1], (int8_t)report[2], report[0]);
}
