#include <choc/kprintf.h>
#include <choc/io.h>

extern "C" void __cxa_pure_virtual(void) {
    kprintf("pure virtual call\n");
    cpu_halt();
}
