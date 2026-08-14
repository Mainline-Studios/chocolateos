#include <choc/gdt.h>
#include <choc/types.h>

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct gdt_entry gdt[3];
static struct gdt_ptr gdtr;

extern void gdt_load(struct gdt_ptr *ptr);

static void set_entry(int i, uint8_t access, uint8_t gran) {
    gdt[i].limit_low = 0;
    gdt[i].base_low = 0;
    gdt[i].base_mid = 0;
    gdt[i].access = access;
    gdt[i].granularity = gran;
    gdt[i].base_high = 0;
}

void gdt_init(void) {
    set_entry(0, 0, 0);
    set_entry(1, 0x9A, 0x20); /* 64-bit kernel code */
    set_entry(2, 0x92, 0x00); /* 64-bit kernel data */
    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base = (uint64_t)&gdt;
    gdt_load(&gdtr);
}
