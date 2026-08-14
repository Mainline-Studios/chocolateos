#include <choc/keyboard.h>
#include <choc/serial.h>
#include <choc/io.h>

#define BUF_SIZE 256

static volatile char buf[BUF_SIZE];
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

static void push(char c) {
    uint32_t next = (head + 1) % BUF_SIZE;
    if (next != tail) {
        buf[head] = c;
        head = next;
    }
}

void keyboard_init(void) {
    head = tail = 0;
    shift = 0;
    ext = 0;
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
    if (ext) {
        ext = 0;
        if (sc == 0x4B) {
            push('\b');
        }
        return;
    }
    if (sc < 128) {
        char c = shift ? map_shift[sc] : map[sc];
        if (c) {
            push(c);
        }
    }
}

int keyboard_getchar(void) {
    if (head != tail) {
        char c = buf[tail];
        tail = (tail + 1) % BUF_SIZE;
        return (unsigned char)c;
    }
    return serial_getchar();
}
