#include <choc/kprintf.h>
#include <choc/console.h>
#include <choc/serial.h>

static void emit(char c) {
    serial_write(c);
    console_putc(c);
}

static void emit_str(const char *s) {
    while (*s) {
        emit(*s++);
    }
}

static void emit_uint(uint64_t value, unsigned base, int width, char pad) {
    char buf[32];
    const char *digits = "0123456789abcdef";
    int i = 0;
    if (value == 0) {
        buf[i++] = '0';
    }
    while (value) {
        buf[i++] = digits[value % base];
        value /= base;
    }
    while (i < width) {
        buf[i++] = pad;
    }
    while (i--) {
        emit(buf[i]);
    }
}

void kvprintf(const char *fmt, va_list args) {
    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            emit(*fmt);
            continue;
        }
        fmt++;
        int width = 0;
        char pad = ' ';
        if (*fmt == '0') {
            pad = '0';
            fmt++;
        }
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }
        switch (*fmt) {
        case 's': {
            const char *s = va_arg(args, const char *);
            emit_str(s ? s : "(null)");
            break;
        }
        case 'c':
            emit((char)va_arg(args, int));
            break;
        case 'd': {
            int v = va_arg(args, int);
            if (v < 0) {
                emit('-');
                emit_uint((uint64_t)(-(int64_t)v), 10, width, pad);
            } else {
                emit_uint((uint64_t)v, 10, width, pad);
            }
            break;
        }
        case 'u':
            emit_uint(va_arg(args, unsigned), 10, width, pad);
            break;
        case 'x':
            emit_uint(va_arg(args, unsigned), 16, width, pad);
            break;
        case 'p':
            emit_str("0x");
            emit_uint(va_arg(args, uint64_t), 16, 16, '0');
            break;
        case 'l':
            if (fmt[1] == 'l' && fmt[2] == 'u') {
                fmt += 2;
                emit_uint(va_arg(args, uint64_t), 10, width, pad);
            } else if (fmt[1] == 'l' && fmt[2] == 'x') {
                fmt += 2;
                emit_uint(va_arg(args, uint64_t), 16, width, pad);
            }
            break;
        case '%':
            emit('%');
            break;
        default:
            emit('%');
            emit(*fmt);
            break;
        }
    }
}

void kprintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    kvprintf(fmt, args);
    va_end(args);
}
