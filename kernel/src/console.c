#include <choc/console.h>
#include <choc/gfx.h>

#define COLOR_BG 0x2B1708
#define COLOR_FG 0xF3E5D0

static struct {
    uint32_t fg;
    uint32_t bg;
    uint32_t col;
    uint32_t row;
    uint32_t cols;
    uint32_t rows;
    int enabled;
} con;

void console_set_enabled(int on) {
    con.enabled = on;
}

void console_set_color(uint32_t fg, uint32_t bg) {
    con.fg = fg;
    con.bg = bg;
}

void console_clear(void) {
    gfx_fill(0, 0, (int)gfx_width(), (int)gfx_height(), con.bg);
    con.col = 0;
    con.row = 0;
}

static void newline(void) {
    con.col = 0;
    con.row++;
    if (con.row >= con.rows) {
        con.row = 0;
    }
}

void console_init(struct limine_framebuffer *fb) {
    gfx_init(fb);
    con.fg = COLOR_FG;
    con.bg = COLOR_BG;
    con.cols = gfx_width() / 8;
    con.rows = gfx_height() / 16;
    con.enabled = 1;
    console_clear();
}

void console_putc(char c) {
    if (!con.enabled) {
        return;
    }
    if (c == '\n') {
        newline();
        return;
    }
    if (c == '\r') {
        con.col = 0;
        return;
    }
    if (c == '\b') {
        if (con.col > 0) {
            con.col--;
            gfx_char((int)(con.col * 8), (int)(con.row * 16), ' ', con.fg, con.bg);
        }
        return;
    }
    if (c == '\t') {
        con.col = (con.col + 4) & ~3u;
        if (con.col >= con.cols) {
            newline();
        }
        return;
    }
    gfx_char((int)(con.col * 8), (int)(con.row * 16), c, con.fg, con.bg);
    con.col++;
    if (con.col >= con.cols) {
        newline();
    }
}

void console_write(const char *s) {
    while (*s) {
        console_putc(*s++);
    }
}
