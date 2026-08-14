#pragma once

#include <choc/types.h>

void serial_init(void);
void serial_write(char c);
void serial_write_str(const char *s);
int serial_getchar(void);
