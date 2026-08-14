#pragma once

#include <choc/types.h>
#include <limine.h>

void console_init(struct limine_framebuffer *fb);
void console_clear(void);
void console_putc(char c);
void console_write(const char *s);
void console_set_color(uint32_t fg, uint32_t bg);
void console_set_enabled(int on);
