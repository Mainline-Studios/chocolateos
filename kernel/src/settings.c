#include <choc/settings.h>
#include <choc/string.h>

static struct choc_settings g;

static const char *langs[] = {"English", "Espanol", "Francais", "Deutsch", "Italiano", "Portugues"};
static const char *themes[] = {"Chocolate", "Dark roast", "Cream", "Midnight"};

void settings_init(void) {
    memset(&g, 0, sizeof(g));
    g.input_usb = 1;
    g.input_ps2 = 1;
    g.input_serial = 1;
    g.mouse_speed = 3;
    g.notifications = 1;
    g.desktop_icons = 1;
    g.sound = 1;
    g.wifi = 1;
    g.updates = 1;
    g.clock_24h = 1;
    strcpy(g.hostname, "chocolate");
    strcpy(g.username, "guest");
}

struct choc_settings *settings_get(void) {
    return &g;
}

uint32_t settings_wall_top(void) {
    switch (g.theme) {
    case 1: return 0x2A2A32;
    case 2: return 0xE8D5B5;
    case 3: return 0x1A1030;
    default: return 0x5C3317;
    }
}

uint32_t settings_wall_bot(void) {
    switch (g.theme) {
    case 1: return 0x101014;
    case 2: return 0xC4A882;
    case 3: return 0x080818;
    default: return 0x1A0C06;
    }
}

const char *settings_language_name(void) {
    if (g.language < 0 || g.language >= 6) {
        return langs[0];
    }
    return langs[g.language];
}

const char *settings_theme_name(void) {
    if (g.theme < 0 || g.theme >= 4) {
        return themes[0];
    }
    return themes[g.theme];
}
