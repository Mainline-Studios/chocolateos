#pragma once

#include <choc/types.h>
#include <limine.h>

void gfx_init(struct limine_framebuffer *fb);
uint32_t gfx_width(void);
uint32_t gfx_height(void);
void gfx_pixel(int x, int y, uint32_t color);
void gfx_fill(int x, int y, int w, int h, uint32_t color);
void gfx_rect(int x, int y, int w, int h, uint32_t color);
void gfx_char(int x, int y, char c, uint32_t fg, uint32_t bg);
void gfx_text(int x, int y, const char *s, uint32_t fg, uint32_t bg);
void gfx_char_trans(int x, int y, char c, uint32_t fg);
void gfx_text_trans(int x, int y, const char *s, uint32_t fg);
