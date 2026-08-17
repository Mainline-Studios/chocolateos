#include <choc/setup.h>
#include <choc/settings.h>
#include <choc/gfx.h>
#include <choc/mouse.h>
#include <choc/keyboard.h>
#include <choc/keys.h>
#include <choc/console.h>
#include <choc/string.h>
#include <choc/types.h>

#define COL_BG    0x24140C
#define COL_PANEL 0xFFF6EA
#define COL_INK   0x2B1708
#define COL_ACC   0xC4783A
#define COL_DIM   0x8A6A50
#define COL_HI    0xE8A05A
#define COL_BTN   0x6B3E2A
#define COL_CREAM 0xF3E5D0

enum {
    PG_WELCOME = 0,
    PG_LANG,
    PG_REGION,
    PG_TZ,
    PG_LAYOUT,
    PG_INPUT,
    PG_KBTEST,
    PG_MOUSE,
    PG_THEME,
    PG_WALL,
    PG_DISPLAY,
    PG_NOTIFY,
    PG_POWER,
    PG_DESKTOP,
    PG_SOUND,
    PG_NET,
    PG_BT,
    PG_HOST,
    PG_USER,
    PG_ACCESS,
    PG_PRIVACY,
    PG_UPDATES,
    PG_CLOCK,
    PG_REVIEW,
    PG_DONE,
    PG_COUNT
};

static int page;
static int row;
static int running;
static int kb_ok;
static int mouse_ok;
static uint8_t prev_btns;
static int host_pos;
static int user_pos;

static const char *regions[] = {"Americas", "Europe", "Asia", "Africa", "Oceania"};
static const char *zones[] = {"UTC-8 Pacific", "UTC-5 Eastern", "UTC+0 London", "UTC+1 Paris", "UTC+9 Tokyo", "UTC+10 Sydney"};
static const char *layouts[] = {"US QWERTY", "UK", "German", "French", "Dvorak"};
static const char *walls[] = {"Cocoa gradient", "Flat", "Stripes", "Dots"};
static const char *displays[] = {"Auto", "800x600", "1024x768", "1280x800"};
static const char *dates[] = {"YYYY-MM-DD", "MM/DD/YYYY", "DD/MM/YYYY"};
static const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789-";

static int hit(int mx, int my, int x, int y, int w, int h) {
    return mx >= x && my >= y && mx < x + w && my < y + h;
}

static int wrap(int v, int n) {
    if (n <= 0) {
        return 0;
    }
    while (v < 0) {
        v += n;
    }
    return v % n;
}

static void spin_char(char *s, int pos, int dir) {
    int n = (int)strlen(charset);
    int i = 0;
    if (s[pos] == 0) {
        s[pos] = 'a';
        s[pos + 1] = 0;
        return;
    }
    while (charset[i] && charset[i] != s[pos]) {
        i++;
    }
    i = wrap(i + dir, n);
    s[pos] = charset[i];
}

static void draw_btn(int x, int y, int w, int h, const char *label, int hot) {
    gfx_fill(x, y, w, h, hot ? COL_HI : COL_BTN);
    gfx_rect(x, y, w, h, COL_INK);
    gfx_text(x + 12, y + (h - 16) / 2, label, COL_CREAM, hot ? COL_HI : COL_BTN);
}

static void draw_option(int x, int y, const char *label, const char *value, int hot) {
    uint32_t bg = hot ? COL_HI : COL_PANEL;
    gfx_fill(x, y, 520, 22, bg);
    gfx_text(x + 8, y + 4, label, COL_INK, bg);
    if (value) {
        gfx_text(x + 240, y + 4, value, COL_INK, bg);
    }
    if (hot) {
        gfx_text(x + 500, y + 4, "< >", COL_INK, bg);
    }
}

static const char *onoff(int v) {
    return v ? "On" : "Off";
}

static int option_count(void) {
    switch (page) {
    case PG_WELCOME:
    case PG_DONE:
        return 0;
    case PG_KBTEST:
        return 0;
    case PG_MOUSE:
        return 1;
    case PG_INPUT:
        return 3;
    case PG_ACCESS:
        return 2;
    case PG_HOST:
    case PG_USER:
        return 1;
    case PG_CLOCK:
        return 2;
    default:
        return 1;
    }
}

static void apply_delta(int d) {
    struct choc_settings *s = settings_get();
    switch (page) {
    case PG_LANG: s->language = wrap(s->language + d, 6); break;
    case PG_REGION: s->region = wrap(s->region + d, 5); break;
    case PG_TZ: s->timezone = wrap(s->timezone + d, 6); break;
    case PG_LAYOUT: s->kb_layout = wrap(s->kb_layout + d, 5); break;
    case PG_INPUT:
        if (row == 0) s->input_usb ^= 1;
        if (row == 1) s->input_ps2 ^= 1;
        if (row == 2) s->input_serial ^= 1;
        break;
    case PG_MOUSE: s->mouse_speed = wrap(s->mouse_speed - 1 + d, 5) + 1; break;
    case PG_THEME: s->theme = wrap(s->theme + d, 4); break;
    case PG_WALL: s->wallpaper = wrap(s->wallpaper + d, 4); break;
    case PG_DISPLAY: s->display_mode = wrap(s->display_mode + d, 4); break;
    case PG_NOTIFY: s->notifications ^= 1; break;
    case PG_POWER: s->power_save ^= 1; break;
    case PG_DESKTOP: s->desktop_icons ^= 1; break;
    case PG_SOUND: s->sound ^= 1; break;
    case PG_NET: s->wifi ^= 1; break;
    case PG_BT: s->bluetooth ^= 1; break;
    case PG_HOST: spin_char(s->hostname, host_pos, d); break;
    case PG_USER: spin_char(s->username, user_pos, d); break;
    case PG_ACCESS:
        if (row == 0) s->large_text ^= 1;
        if (row == 1) s->high_contrast ^= 1;
        break;
    case PG_PRIVACY: s->privacy ^= 1; break;
    case PG_UPDATES: s->updates ^= 1; break;
    case PG_CLOCK:
        if (row == 0) s->clock_24h ^= 1;
        else s->date_fmt = wrap(s->date_fmt + d, 3);
        break;
    default: break;
    }
}

static void draw_page(struct mouse_state m) {
    struct choc_settings *s = settings_get();
    uint32_t top = settings_wall_top();
    uint32_t bot = settings_wall_bot();
    uint32_t h = gfx_height();
    uint32_t w = gfx_width();
    for (uint32_t y = 0; y < h; y++) {
        uint32_t t = h ? (y * 256) / h : 0;
        uint32_t r = ((top >> 16) & 0xFF) * (256 - t) + ((bot >> 16) & 0xFF) * t;
        uint32_t g = ((top >> 8) & 0xFF) * (256 - t) + ((bot >> 8) & 0xFF) * t;
        uint32_t b = (top & 0xFF) * (256 - t) + (bot & 0xFF) * t;
        gfx_fill(0, (int)y, (int)w, 1, ((r / 256) << 16) | ((g / 256) << 8) | (b / 256));
    }

    int px = 80, py = 40, pw = 640, ph = (int)h - 80;
    if (pw > (int)w - 40) {
        pw = (int)w - 40;
        px = 20;
    }
    gfx_fill(px + 6, py + 6, pw, ph, 0x120804);
    gfx_fill(px, py, pw, ph, COL_PANEL);
    gfx_rect(px, py, pw, ph, COL_INK);
    gfx_fill(px, py, pw, 28, COL_ACC);
    gfx_text(px + 12, py + 6, "ChocolateOS Preview setup", COL_CREAM, COL_ACC);

    char step[32];
    step[0] = 's'; step[1] = 't'; step[2] = 'e'; step[3] = 'p'; step[4] = ' ';
    step[5] = (char)('0' + (page + 1) / 10);
    step[6] = (char)('0' + (page + 1) % 10);
    step[7] = '/';
    step[8] = (char)('0' + PG_COUNT / 10);
    step[9] = (char)('0' + PG_COUNT % 10);
    step[10] = 0;
    gfx_text(px + pw - 90, py + 6, step, COL_CREAM, COL_ACC);

    int ox = px + 24;
    int oy = py + 48;
    gfx_text(ox, oy, "Arrows move. Enter next. Esc back. Mouse works too.", COL_DIM, COL_PANEL);
    oy += 28;

    switch (page) {
    case PG_WELCOME:
        gfx_text(ox, oy, "Welcome to ChocolateOS.", COL_INK, COL_PANEL);
        gfx_text(ox, oy + 20, "This Preview ISO is not a versioned release.", COL_INK, COL_PANEL);
        gfx_text(ox, oy + 40, "We will set up input, display, network, and more.", COL_INK, COL_PANEL);
        gfx_text(ox, oy + 70, "If letters do not type, use arrow keys or the", COL_INK, COL_PANEL);
        gfx_text(ox, oy + 86, "on-screen buttons at the bottom.", COL_INK, COL_PANEL);
        break;
    case PG_LANG:
        gfx_text(ox, oy, "Language", COL_INK, COL_PANEL);
        draw_option(ox, oy + 24, "Interface", settings_language_name(), row == 0);
        break;
    case PG_REGION:
        gfx_text(ox, oy, "Region", COL_INK, COL_PANEL);
        draw_option(ox, oy + 24, "Where you are", regions[wrap(s->region, 5)], row == 0);
        break;
    case PG_TZ:
        gfx_text(ox, oy, "Time zone", COL_INK, COL_PANEL);
        draw_option(ox, oy + 24, "Clock offset", zones[wrap(s->timezone, 6)], row == 0);
        break;
    case PG_LAYOUT:
        gfx_text(ox, oy, "Keyboard layout", COL_INK, COL_PANEL);
        draw_option(ox, oy + 24, "Layout", layouts[wrap(s->kb_layout, 5)], row == 0);
        break;
    case PG_INPUT:
        gfx_text(ox, oy, "Input sources  (toggle with Left/Right)", COL_INK, COL_PANEL);
        draw_option(ox, oy + 24, "USB keyboard/mouse", onoff(s->input_usb), row == 0);
        draw_option(ox, oy + 48, "PS/2 fallback", onoff(s->input_ps2), row == 1);
        draw_option(ox, oy + 72, "Serial (QEMU terminal)", onoff(s->input_serial), row == 2);
        break;
    case PG_KBTEST:
        gfx_text(ox, oy, "Keyboard test", COL_INK, COL_PANEL);
        gfx_text(ox, oy + 24, "Press any letter, or just use arrows.", COL_INK, COL_PANEL);
        gfx_text(ox, oy + 48, kb_ok ? "Keys received: yes" : "Waiting for a key...", COL_INK, COL_PANEL);
        break;
    case PG_MOUSE:
        gfx_text(ox, oy, "Mouse", COL_INK, COL_PANEL);
        gfx_text(ox, oy + 24, mouse_ok ? "Mouse moved: yes" : "Move the mouse to test.", COL_INK, COL_PANEL);
        draw_option(ox, oy + 48, "Pointer speed", "1-5 (Left/Right)", row == 0);
        break;
    case PG_THEME:
        gfx_text(ox, oy, "Appearance", COL_INK, COL_PANEL);
        draw_option(ox, oy + 24, "Theme", settings_theme_name(), row == 0);
        break;
    case PG_WALL:
        gfx_text(ox, oy, "Wallpaper", COL_INK, COL_PANEL);
        draw_option(ox, oy + 24, "Style", walls[wrap(s->wallpaper, 4)], row == 0);
        break;
    case PG_DISPLAY:
        gfx_text(ox, oy, "Display", COL_INK, COL_PANEL);
        draw_option(ox, oy + 24, "Resolution", displays[wrap(s->display_mode, 4)], row == 0);
        gfx_text(ox, oy + 52, "Preview keeps the framebuffer Limine gave us.", COL_DIM, COL_PANEL);
        break;
    case PG_NOTIFY:
        gfx_text(ox, oy, "Notifications", COL_INK, COL_PANEL);
        draw_option(ox, oy + 24, "Show alerts", onoff(s->notifications), row == 0);
        break;
    case PG_POWER:
        gfx_text(ox, oy, "Power", COL_INK, COL_PANEL);
        draw_option(ox, oy + 24, "Idle dim (stub)", onoff(s->power_save), row == 0);
        break;
    case PG_DESKTOP:
        gfx_text(ox, oy, "Desktop", COL_INK, COL_PANEL);
        draw_option(ox, oy + 24, "Show icons", onoff(s->desktop_icons), row == 0);
        break;
    case PG_SOUND:
        gfx_text(ox, oy, "Sound", COL_INK, COL_PANEL);
        draw_option(ox, oy + 24, "System sounds", onoff(s->sound), row == 0);
        break;
    case PG_NET:
        gfx_text(ox, oy, "Network", COL_INK, COL_PANEL);
        draw_option(ox, oy + 24, "Wi-Fi (stub)", onoff(s->wifi), row == 0);
        break;
    case PG_BT:
        gfx_text(ox, oy, "Bluetooth", COL_INK, COL_PANEL);
        draw_option(ox, oy + 24, "Bluetooth (stub)", onoff(s->bluetooth), row == 0);
        break;
    case PG_HOST:
        gfx_text(ox, oy, "Computer name  (Left/Right change letter)", COL_INK, COL_PANEL);
        draw_option(ox, oy + 24, "Hostname", s->hostname, row == 0);
        gfx_text(ox, oy + 52, "Up/Down move cursor in the name.", COL_DIM, COL_PANEL);
        break;
    case PG_USER:
        gfx_text(ox, oy, "User account", COL_INK, COL_PANEL);
        draw_option(ox, oy + 24, "Username", s->username, row == 0);
        break;
    case PG_ACCESS:
        gfx_text(ox, oy, "Accessibility", COL_INK, COL_PANEL);
        draw_option(ox, oy + 24, "Large UI", onoff(s->large_text), row == 0);
        draw_option(ox, oy + 48, "High contrast", onoff(s->high_contrast), row == 1);
        break;
    case PG_PRIVACY:
        gfx_text(ox, oy, "Privacy", COL_INK, COL_PANEL);
        draw_option(ox, oy + 24, "Share diagnostics", onoff(s->privacy), row == 0);
        break;
    case PG_UPDATES:
        gfx_text(ox, oy, "Updates", COL_INK, COL_PANEL);
        draw_option(ox, oy + 24, "Check for builds", onoff(s->updates), row == 0);
        break;
    case PG_CLOCK:
        gfx_text(ox, oy, "Date and time", COL_INK, COL_PANEL);
        draw_option(ox, oy + 24, "24-hour clock", onoff(s->clock_24h), row == 0);
        draw_option(ox, oy + 48, "Date format", dates[wrap(s->date_fmt, 3)], row == 1);
        break;
    case PG_REVIEW: {
        gfx_text(ox, oy, "Review", COL_INK, COL_PANEL);
        gfx_text(ox, oy + 22, settings_language_name(), COL_INK, COL_PANEL);
        gfx_text(ox, oy + 38, settings_theme_name(), COL_INK, COL_PANEL);
        gfx_text(ox, oy + 54, s->hostname, COL_INK, COL_PANEL);
        gfx_text(ox, oy + 70, s->username, COL_INK, COL_PANEL);
        gfx_text(ox, oy + 94, "Enter starts the desktop.", COL_DIM, COL_PANEL);
        break;
    }
    case PG_DONE:
        gfx_text(ox, oy, "You are ready.", COL_INK, COL_PANEL);
        gfx_text(ox, oy + 24, "Enter opens the ChocolateOS desktop.", COL_INK, COL_PANEL);
        break;
    default:
        break;
    }

    int by = py + ph - 70;
    draw_btn(px + 24, by, 100, 32, "< Back", 0);
    draw_btn(px + pw - 140, by, 110, 32, "Next >", 1);
    draw_btn(px + 160, by, 48, 32, "^", 0);
    draw_btn(px + 214, by, 48, 32, "v", 0);
    draw_btn(px + 268, by, 48, 32, "<", 0);
    draw_btn(px + 322, by, 48, 32, ">", 0);

    gfx_fill(m.x, m.y, 10, 10, 0xFFFFFF);
    gfx_rect(m.x, m.y, 10, 10, 0);
}

static void go_next(void) {
    if (page < PG_COUNT - 1) {
        page++;
        row = 0;
    } else {
        settings_get()->setup_done = 1;
        running = 0;
    }
}

static void go_back(void) {
    if (page > 0) {
        page--;
        row = 0;
    }
}

static void on_key(int c) {
    int n = option_count();
    if (c >= 32 && c < 127) {
        kb_ok = 1;
        if (page == PG_HOST) {
            struct choc_settings *s = settings_get();
            if (host_pos + 1 < (int)sizeof(s->hostname)) {
                s->hostname[host_pos++] = (char)c;
                s->hostname[host_pos] = 0;
            }
        }
        if (page == PG_USER) {
            struct choc_settings *s = settings_get();
            if (user_pos + 1 < (int)sizeof(s->username)) {
                s->username[user_pos++] = (char)c;
                s->username[user_pos] = 0;
            }
        }
        return;
    }
    if (c == KEY_DOWN || c == '\t') {
        if (n) {
            row = wrap(row + 1, n);
        }
        if (page == PG_HOST) {
            host_pos++;
        }
        if (page == PG_USER) {
            user_pos++;
        }
        return;
    }
    if (c == KEY_UP) {
        if (n) {
            row = wrap(row - 1, n);
        }
        if (page == PG_HOST && host_pos > 0) {
            host_pos--;
        }
        if (page == PG_USER && user_pos > 0) {
            user_pos--;
        }
        return;
    }
    if (c == KEY_RIGHT) {
        apply_delta(1);
        return;
    }
    if (c == KEY_LEFT) {
        apply_delta(-1);
        return;
    }
    if (c == KEY_PGDN || c == KEY_F1 || c == '\n') {
        go_next();
        return;
    }
    if (c == KEY_PGUP || c == 27 || c == '\b') {
        go_back();
        return;
    }
}

static void on_click(int mx, int my) {
    int px = 80, py = 40, pw = 640, ph = (int)gfx_height() - 80;
    if (pw > (int)gfx_width() - 40) {
        pw = (int)gfx_width() - 40;
        px = 20;
    }
    int by = py + ph - 70;
    if (hit(mx, my, px + 24, by, 100, 32)) {
        go_back();
        return;
    }
    if (hit(mx, my, px + pw - 140, by, 110, 32)) {
        go_next();
        return;
    }
    if (hit(mx, my, px + 160, by, 48, 32)) {
        on_key(KEY_UP);
        return;
    }
    if (hit(mx, my, px + 214, by, 48, 32)) {
        on_key(KEY_DOWN);
        return;
    }
    if (hit(mx, my, px + 268, by, 48, 32)) {
        on_key(KEY_LEFT);
        return;
    }
    if (hit(mx, my, px + 322, by, 48, 32)) {
        on_key(KEY_RIGHT);
        return;
    }
    int n = option_count();
    int ox = px + 24, oy = py + 48 + 28 + 24;
    for (int i = 0; i < n; i++) {
        if (hit(mx, my, ox, oy + i * 24, 520, 22)) {
            row = i;
            apply_delta(1);
            return;
        }
    }
}

void setup_run(void) {
    page = 0;
    row = 0;
    running = 1;
    kb_ok = 0;
    mouse_ok = 0;
    host_pos = (int)strlen(settings_get()->hostname);
    user_pos = (int)strlen(settings_get()->username);
    prev_btns = 0;
    console_set_enabled(0);
    while (keyboard_getchar() >= 0) {
    }
    struct mouse_state last = mouse_poll();
    draw_page(last);

    while (running) {
        __asm__ volatile("hlt");
        int dirty = 0;
        struct mouse_state m = mouse_poll();
        if (m.changed) {
            mouse_ok = 1;
            dirty = 1;
        }
        uint8_t pressed = m.buttons & ~prev_btns;
        if (pressed & 1) {
            on_click(m.x, m.y);
            dirty = 1;
        }
        prev_btns = m.buttons;
        int c;
        while ((c = keyboard_getchar()) >= 0) {
            on_key(c);
            dirty = 1;
        }
        if (dirty) {
            draw_page(m);
        }
    }
    settings_get()->setup_done = 1;
    console_set_enabled(1);
}
