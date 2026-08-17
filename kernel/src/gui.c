#include <choc/gui.h>
#include <choc/gfx.h>
#include <choc/mouse.h>
#include <choc/keyboard.h>
#include <choc/console.h>
#include <choc/pit.h>
#include <choc/string.h>
#include <choc/types.h>
#include <choc/io.h>
#include <choc/settings.h>

#define COL_BAR       0x3A2214
#define COL_BAR_HI    0x6B4228
#define COL_CREAM     0xF3E5D0
#define COL_INK       0x2B1708
#define COL_ACCENT    0xD2691E
#define COL_TITLE     0x8B4513
#define COL_TITLE_A   0xC4783A
#define COL_WIN       0xFFF8F0
#define COL_BTN       0xA0522D
#define COL_BTN_X     0x8B2E1F
#define COL_SHADOW    0x120804
#define COL_ICON      0xE8C9A0

#define TITLE_H 22
#define BAR_H   36
#define CLOSE_S 16

enum {
    WIN_NONE = 0,
    WIN_NOTES,
    WIN_ABOUT,
};

static int running;
static int menu_open;
static int notes_open;
static int about_open;
static int focus;
static int dragging;
static int drag_off_x;
static int drag_off_y;
static int notes_x, notes_y, notes_w, notes_h;
static int about_x, about_y, about_w, about_h;
static char notes[1024];
static uint32_t notes_len;
static uint8_t prev_buttons;

static int hit(int mx, int my, int x, int y, int w, int h) {
    return mx >= x && my >= y && mx < x + w && my < y + h;
}

static void mix_fill_wallpaper(void) {
    uint32_t top = settings_wall_top();
    uint32_t bot = settings_wall_bot();
    uint32_t h = gfx_height();
    uint32_t w = gfx_width();
    int style = settings_get()->wallpaper;
    for (uint32_t y = 0; y < h; y++) {
        uint32_t t = style == 1 ? 0 : (y * 256) / (h ? h : 1);
        uint32_t r = ((top >> 16) & 0xFF) * (256 - t) + ((bot >> 16) & 0xFF) * t;
        uint32_t g = ((top >> 8) & 0xFF) * (256 - t) + ((bot >> 8) & 0xFF) * t;
        uint32_t b = (top & 0xFF) * (256 - t) + (bot & 0xFF) * t;
        uint32_t c = ((r / 256) << 16) | ((g / 256) << 8) | (b / 256);
        if (style == 2 && (y / 16) % 2 == 0) {
            c = top;
        }
        gfx_fill(0, (int)y, (int)w, 1, c);
        if (style == 3 && (y % 20) == 10) {
            for (uint32_t x = 10; x < w; x += 20) {
                gfx_fill((int)x, (int)y, 3, 3, COL_CREAM);
            }
        }
    }
}

static void draw_cursor(int x, int y) {
    static const uint16_t bits[16] = {
        0x8000, 0xC000, 0xE000, 0xF000, 0xF800, 0xFC00, 0xFE00, 0xFF00,
        0xF800, 0xD800, 0x8C00, 0x0C00, 0x0600, 0x0600, 0x0300, 0x0300,
    };
    for (int row = 0; row < 16; row++) {
        for (int col = 0; col < 16; col++) {
            if (bits[row] & (0x8000 >> col)) {
                gfx_pixel(x + col, y + row, 0xFFFFFF);
                gfx_pixel(x + col + 1, y + row + 1, 0x000000);
            }
        }
    }
}

static void draw_icon(int x, int y, const char *label) {
    gfx_fill(x + 4, y + 4, 40, 40, COL_SHADOW);
    gfx_fill(x, y, 40, 40, COL_ICON);
    gfx_rect(x, y, 40, 40, COL_INK);
    gfx_fill(x + 8, y + 8, 24, 4, COL_TITLE);
    gfx_fill(x + 8, y + 16, 24, 16, COL_WIN);
    gfx_text_trans(x - 8, y + 46, label, COL_CREAM);
}

static void draw_window(int x, int y, int w, int h, const char *title, int active) {
    gfx_fill(x + 4, y + 4, w, h, COL_SHADOW);
    gfx_fill(x, y, w, h, COL_WIN);
    gfx_fill(x, y, w, TITLE_H, active ? COL_TITLE_A : COL_TITLE);
    gfx_rect(x, y, w, h, COL_INK);
    gfx_text(x + 8, y + 4, title, COL_CREAM, active ? COL_TITLE_A : COL_TITLE);
    gfx_fill(x + w - CLOSE_S - 4, y + 3, CLOSE_S, CLOSE_S, COL_BTN_X);
    gfx_text(x + w - CLOSE_S - 2, y + 4, "x", COL_CREAM, COL_BTN_X);
}

static void draw_notes_body(void) {
    int x = notes_x + 8;
    int y = notes_y + TITLE_H + 8;
    int max_w = (notes_w - 16) / 8;
    int col = 0;
    gfx_text(x, y, "Type here. Esc leaves the desktop.", COL_INK, COL_WIN);
    y += 20;
    for (uint32_t i = 0; i < notes_len; i++) {
        char c = notes[i];
        if (c == '\n' || col >= max_w) {
            col = 0;
            y += 16;
            if (c == '\n') {
                continue;
            }
        }
        gfx_char(x + col * 8, y, c, COL_INK, COL_WIN);
        col++;
    }
    gfx_fill(x + col * 8, y, 8, 14, COL_ACCENT);
}

static void draw_about_body(void) {
    int x = about_x + 16;
    int y = about_y + TITLE_H + 16;
    gfx_text(about_x + 16, y, "ChocolateOS desktop", COL_INK, COL_WIN);
    y += 20;
    gfx_text(x, y, "A homemade GUI on the kernel", COL_INK, COL_WIN);
    y += 16;
    gfx_text(x, y, "framebuffer. Click icons, type", COL_INK, COL_WIN);
    y += 16;
    gfx_text(x, y, "in Notes, or open the Choc menu.", COL_INK, COL_WIN);
}

static void redraw(struct mouse_state m) {
    int bar_y = (int)gfx_height() - BAR_H;
    mix_fill_wallpaper();
    gfx_text_trans(16, 16, "ChocolateOS Preview", COL_CREAM);
    gfx_text_trans(16, 34, "click an icon or the Choc button", COL_ICON);

    if (settings_get()->desktop_icons) {
        draw_icon(32, 80, "Notes");
        draw_icon(32, 160, "About");
    }

    if (about_open && focus != WIN_ABOUT) {
        draw_window(about_x, about_y, about_w, about_h, "About", 0);
        draw_about_body();
    }
    if (notes_open && focus != WIN_NOTES) {
        draw_window(notes_x, notes_y, notes_w, notes_h, "Notes", 0);
        draw_notes_body();
    }
    if (about_open && focus == WIN_ABOUT) {
        draw_window(about_x, about_y, about_w, about_h, "About", 1);
        draw_about_body();
    }
    if (notes_open && focus == WIN_NOTES) {
        draw_window(notes_x, notes_y, notes_w, notes_h, "Notes", 1);
        draw_notes_body();
    }

    gfx_fill(0, bar_y, (int)gfx_width(), BAR_H, COL_BAR);
    gfx_fill(0, bar_y, (int)gfx_width(), 2, COL_BAR_HI);
    gfx_fill(8, bar_y + 6, 72, BAR_H - 12, COL_ACCENT);
    gfx_text(20, bar_y + 10, "Choc", COL_CREAM, COL_ACCENT);

    if (notes_open) {
        gfx_fill(92, bar_y + 8, 72, BAR_H - 16, COL_TITLE);
        gfx_text(100, bar_y + 12, "Notes", COL_CREAM, COL_TITLE);
    }
    if (about_open) {
        int tx = notes_open ? 172 : 92;
        gfx_fill(tx, bar_y + 8, 72, BAR_H - 16, COL_TITLE);
        gfx_text(tx + 8, bar_y + 12, "About", COL_CREAM, COL_TITLE);
    }

    if (menu_open) {
        int mx0 = 8;
        int my0 = bar_y - 108;
        gfx_fill(mx0 + 3, my0 + 3, 160, 104, COL_SHADOW);
        gfx_fill(mx0, my0, 160, 104, COL_BAR);
        gfx_rect(mx0, my0, 160, 104, COL_CREAM);
        gfx_text(mx0 + 12, my0 + 12, "Notes", COL_CREAM, COL_BAR);
        gfx_text(mx0 + 12, my0 + 36, "About", COL_CREAM, COL_BAR);
        gfx_text(mx0 + 12, my0 + 60, "Exit desktop", COL_CREAM, COL_BAR);
    }

    draw_cursor(m.x, m.y);
}

static int close_hit(int mx, int my, int x, int y, int w) {
    return hit(mx, my, x + w - CLOSE_S - 4, y + 3, CLOSE_S, CLOSE_S);
}

static void open_notes(void) {
    notes_open = 1;
    focus = WIN_NOTES;
    menu_open = 0;
}

static void open_about(void) {
    about_open = 1;
    focus = WIN_ABOUT;
    menu_open = 0;
}

static void on_click(int mx, int my) {
    int bar_y = (int)gfx_height() - BAR_H;

    if (menu_open) {
        int mx0 = 8;
        int my0 = bar_y - 108;
        if (hit(mx, my, mx0, my0 + 8, 160, 24)) {
            open_notes();
            return;
        }
        if (hit(mx, my, mx0, my0 + 32, 160, 24)) {
            open_about();
            return;
        }
        if (hit(mx, my, mx0, my0 + 56, 160, 24)) {
            running = 0;
            return;
        }
        if (!hit(mx, my, mx0, my0, 160, 104) && !hit(mx, my, 8, bar_y + 6, 72, BAR_H - 12)) {
            menu_open = 0;
        }
    }

    if (hit(mx, my, 8, bar_y + 6, 72, BAR_H - 12)) {
        menu_open = !menu_open;
        return;
    }

    if (notes_open && close_hit(mx, my, notes_x, notes_y, notes_w)) {
        notes_open = 0;
        dragging = 0;
        focus = about_open ? WIN_ABOUT : WIN_NONE;
        return;
    }
    if (about_open && close_hit(mx, my, about_x, about_y, about_w)) {
        about_open = 0;
        dragging = 0;
        focus = notes_open ? WIN_NOTES : WIN_NONE;
        return;
    }

    if (notes_open && hit(mx, my, notes_x, notes_y, notes_w, notes_h)) {
        focus = WIN_NOTES;
        if (hit(mx, my, notes_x, notes_y, notes_w, TITLE_H)) {
            dragging = WIN_NOTES;
            drag_off_x = mx - notes_x;
            drag_off_y = my - notes_y;
        }
        return;
    }
    if (about_open && hit(mx, my, about_x, about_y, about_w, about_h)) {
        focus = WIN_ABOUT;
        if (hit(mx, my, about_x, about_y, about_w, TITLE_H)) {
            dragging = WIN_ABOUT;
            drag_off_x = mx - about_x;
            drag_off_y = my - about_y;
        }
        return;
    }

    if (settings_get()->desktop_icons && hit(mx, my, 32, 80, 48, 64)) {
        open_notes();
        return;
    }
    if (settings_get()->desktop_icons && hit(mx, my, 32, 160, 48, 64)) {
        open_about();
        return;
    }
}

static void on_key(int c) {
    if (c == 27) {
        running = 0;
        return;
    }
    if (focus != WIN_NOTES || !notes_open) {
        return;
    }
    if (c == '\b') {
        if (notes_len) {
            notes_len--;
            notes[notes_len] = 0;
        }
        return;
    }
    if (c == '\n' || (c >= 32 && c < 127)) {
        if (notes_len + 1 < sizeof(notes)) {
            notes[notes_len++] = (char)c;
            notes[notes_len] = 0;
        }
    }
}

void gui_run(void) {
    running = 1;
    menu_open = 0;
    notes_open = 0;
    about_open = 0;
    focus = WIN_NONE;
    dragging = 0;
    notes_len = 0;
    notes[0] = 0;
    notes_w = 420;
    notes_h = 260;
    notes_x = 120;
    notes_y = 70;
    about_w = 360;
    about_h = 160;
    about_x = 200;
    about_y = 140;
    prev_buttons = 0;

    console_set_enabled(0);
    while (keyboard_getchar() >= 0) {
    }
    struct mouse_state m = mouse_poll();
    redraw(m);

    while (running) {
        __asm__ volatile("hlt");
        int dirty = 0;
        m = mouse_poll();
        uint8_t pressed = m.buttons & ~prev_buttons;
        uint8_t released = prev_buttons & ~m.buttons;

        if (m.changed) {
            dirty = 1;
        }
        if (pressed & 1) {
            on_click(m.x, m.y);
            dirty = 1;
        }
        if (released & 1) {
            dragging = 0;
        }
        if ((m.buttons & 1) && dragging == WIN_NOTES) {
            notes_x = m.x - drag_off_x;
            notes_y = m.y - drag_off_y;
            dirty = 1;
        }
        if ((m.buttons & 1) && dragging == WIN_ABOUT) {
            about_x = m.x - drag_off_x;
            about_y = m.y - drag_off_y;
            dirty = 1;
        }
        prev_buttons = m.buttons;

        int c;
        while ((c = keyboard_getchar()) >= 0) {
            on_key(c);
            dirty = 1;
        }
        if (dirty) {
            redraw(m);
        }
    }

    console_set_enabled(1);
    console_clear();
}
