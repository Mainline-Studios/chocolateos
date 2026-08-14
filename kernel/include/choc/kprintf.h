#pragma once

#include <choc/types.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

void kprintf(const char *fmt, ...);
void kvprintf(const char *fmt, va_list args);

#ifdef __cplusplus
}
#endif
