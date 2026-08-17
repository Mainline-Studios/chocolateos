#include <choc/keyboard.h>
#include <choc/keys.h>
#include <choc/usb.h>
#include <choc/serial.h>
#include <choc/io.h>

#define BUF_SIZE 256

static volatile int buf[BUF_SIZE];
static volatile uint32_t head;
static volatile uint32_t tail;
static int shift;
static int ext;

static const char map[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ',
};

static const char map_shift[128] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' ',
};

static void push_key(int k) {
    uint32_t next = (head + 1) % BUF_SIZE;
    if (next != tail) {
        buf[head] = k;
        head = next;
    }
}

void keyboard_init(void) {
    head = tail = 0;
    shift = 0;
    ext = 0;
}

void keyboard_push(char c) {
    push_key((unsigned char)c);
}

void keyboard_push_key(int key) {
    push_key(key);
}

void keyboard_feed(uint8_t sc) {
    if (sc == 0xE0) {
        ext = 1;
        return;
    }
    if (sc == 0x2A || sc == 0x36) {
        shift = 1;
        ext = 0;
        return;
    }
    if (sc == 0xAA || sc == 0xB6) {
        shift = 0;
        ext = 0;
        return;
    }
    if (sc & 0x80) {
        ext = 0;
        return;
    }
    if (sc == 0x3B) {
        push_key(KEY_F1);
        return;
    }
    if (sc == 0x3C) {
        push_key(KEY_F2);
        return;
    }
    if (ext) {
        ext = 0;
        if (sc == 0x48) {
            push_key(KEY_UP);
        } else if (sc == 0x50) {
            push_key(KEY_DOWN);
        } else if (sc == 0x4B) {
            push_key(KEY_LEFT);
        } else if (sc == 0x4D) {
            push_key(KEY_RIGHT);
        } else if (sc == 0x47) {
            push_key(KEY_HOME);
        } else if (sc == 0x4F) {
            push_key(KEY_END);
        } else if (sc == 0x49) {
            push_key(KEY_PGUP);
        } else if (sc == 0x51) {
            push_key(KEY_PGDN);
        } else if (sc == 0x53) {
            push_key(KEY_DEL);
        }
        return;
    }
    if (sc < 128) {
        char c = shift ? map_shift[sc] : map[sc];
        if (c) {
            push_key((unsigned char)c);
        }
    }
}

int keyboard_getchar(void) {
    usb_poll();
    if (head != tail) {
        int c = buf[tail];
        tail = (tail + 1) % BUF_SIZE;
        return c;
    }
    int c = serial_getchar();
    if (c != 27) {
        return c;
    }
    int n1 = serial_getchar();
    if (n1 < 0) {
        return 27;
    }
    if (n1 == '[') {
        int n2 = serial_getchar();
        if (n2 == 'A') {
            return KEY_UP;
        }
        if (n2 == 'B') {
            return KEY_DOWN;
        }
        if (n2 == 'C') {
            return KEY_RIGHT;
        }
        if (n2 == 'D') {
            return KEY_LEFT;
        }
        if (n2 == 'H') {
            return KEY_HOME;
        }
        if (n2 == 'F') {
            return KEY_END;
        }
        if (n2 == '5') {
            (void)serial_getchar();
            return KEY_PGUP;
        }
        if (n2 == '6') {
            (void)serial_getchar();
            return KEY_PGDN;
        }
        return n2;
    }
    if (n1 == 'O') {
        int n2 = serial_getchar();
        if (n2 == 'P') {
            return KEY_F1;
        }
        if (n2 == 'Q') {
            return KEY_F2;
        }
        return n2;
    }
    return n1;
}
