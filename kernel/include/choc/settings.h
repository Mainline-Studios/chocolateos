#pragma once

#include <choc/types.h>

struct choc_settings {
    int language;
    int region;
    int timezone;
    int kb_layout;
    int input_usb;
    int input_ps2;
    int input_serial;
    int mouse_speed;
    int theme;
    int wallpaper;
    int display_mode;
    int notifications;
    int power_save;
    int desktop_icons;
    int font_scale;
    int sound;
    int wifi;
    int bluetooth;
    char hostname[24];
    char username[24];
    int large_text;
    int high_contrast;
    int privacy;
    int updates;
    int clock_24h;
    int date_fmt;
    int setup_done;
};

void settings_init(void);
struct choc_settings *settings_get(void);
uint32_t settings_wall_top(void);
uint32_t settings_wall_bot(void);
const char *settings_language_name(void);
const char *settings_theme_name(void);
