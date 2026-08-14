#include <choc/pmm.h>
#include <choc/kprintf.h>
#include <choc/string.h>

static uint64_t usable;
static uint64_t total;
static uint64_t hhdm;
static struct limine_memmap_response *saved;
static uint64_t bump;
static uint64_t bump_end;

void pmm_init(struct limine_memmap_response *map, uint64_t offset) {
    hhdm = offset;
    saved = map;
    usable = 0;
    total = 0;
    bump = 0;
    bump_end = 0;
    if (!map) {
        return;
    }
    for (uint64_t i = 0; i < map->entry_count; i++) {
        struct limine_memmap_entry *e = map->entries[i];
        total += e->length;
        if (e->type != LIMINE_MEMMAP_USABLE) {
            continue;
        }
        usable += e->length;
        uint64_t base = e->base;
        uint64_t end = e->base + e->length;
        if (base < 0x100000) {
            base = 0x100000;
        }
        if (end > base && (end - base) > (bump_end - bump)) {
            bump = base;
            bump_end = end;
        }
    }
}

uint64_t hhdm_offset(void) {
    return hhdm;
}

void *pmm_to_virt(uint64_t phys) {
    return (void *)(phys + hhdm);
}

uint64_t pmm_to_phys(const void *virt) {
    return (uint64_t)virt - hhdm;
}

void *pmm_alloc(size_t bytes, size_t align) {
    if (align < 16) {
        align = 16;
    }
    uint64_t a = (uint64_t)align;
    uint64_t p = (bump + a - 1) & ~(a - 1);
    if (p + bytes > bump_end) {
        return 0;
    }
    bump = p + bytes;
    void *v = pmm_to_virt(p);
    memset(v, 0, bytes);
    return v;
}

uint64_t pmm_usable_bytes(void) {
    return usable;
}

uint64_t pmm_total_reported_bytes(void) {
    return total;
}

static const char *type_name(uint64_t type) {
    switch (type) {
    case LIMINE_MEMMAP_USABLE: return "usable";
    case LIMINE_MEMMAP_RESERVED: return "reserved";
    case LIMINE_MEMMAP_ACPI_RECLAIMABLE: return "acpi-reclaim";
    case LIMINE_MEMMAP_ACPI_NVS: return "acpi-nvs";
    case LIMINE_MEMMAP_BAD_MEMORY: return "bad";
    case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE: return "bootloader";
    case LIMINE_MEMMAP_EXECUTABLE_AND_MODULES: return "kernel";
    case LIMINE_MEMMAP_FRAMEBUFFER: return "framebuffer";
    default: return "other";
    }
}

void pmm_dump(void) {
    if (!saved) {
        kprintf("no memory map\n");
        return;
    }
    kprintf("memory map (%llu entries)\n", saved->entry_count);
    for (uint64_t i = 0; i < saved->entry_count && i < 24; i++) {
        struct limine_memmap_entry *e = saved->entries[i];
        kprintf("  %p +%llx  %s\n", e->base, e->length, type_name(e->type));
    }
    kprintf("usable: %llu MiB\n", usable / (1024 * 1024));
}
