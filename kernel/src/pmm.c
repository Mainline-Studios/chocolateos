#include <choc/pmm.h>
#include <choc/kprintf.h>

static uint64_t usable;
static uint64_t total;
static struct limine_memmap_response *saved;

void pmm_init(struct limine_memmap_response *map, uint64_t hhdm) {
    (void)hhdm;
    saved = map;
    usable = 0;
    total = 0;
    if (!map) {
        return;
    }
    for (uint64_t i = 0; i < map->entry_count; i++) {
        struct limine_memmap_entry *e = map->entries[i];
        total += e->length;
        if (e->type == LIMINE_MEMMAP_USABLE) {
            usable += e->length;
        }
    }
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
