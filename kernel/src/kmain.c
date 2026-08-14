#include <choc/types.h>
#include <choc/io.h>
#include <choc/serial.h>
#include <choc/console.h>
#include <choc/kprintf.h>
#include <choc/gdt.h>
#include <choc/idt.h>
#include <choc/pic.h>
#include <choc/pit.h>
#include <choc/keyboard.h>
#include <choc/mouse.h>
#include <choc/ps2.h>
#include <choc/pmm.h>
#include <choc/shell.h>
#include <choc/cxx.h>
#include <limine.h>

__attribute__((used, section(".requests_start_marker")))
static volatile LIMINE_REQUESTS_START_MARKER

__attribute__((used, section(".requests")))
static volatile LIMINE_BASE_REVISION(3)

__attribute__((used, section(".requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0,
    .response = 0,
};

__attribute__((used, section(".requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0,
    .response = 0,
};

__attribute__((used, section(".requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
    .response = 0,
};

__attribute__((used, section(".requests")))
static volatile struct limine_bootloader_info_request bootloader_request = {
    .id = LIMINE_BOOTLOADER_INFO_REQUEST,
    .revision = 0,
    .response = 0,
};

__attribute__((used, section(".requests_end_marker")))
static volatile LIMINE_REQUESTS_END_MARKER

void kmain(void) {
    serial_init();

    if (!LIMINE_BASE_REVISION_SUPPORTED) {
        serial_write_str("Limine base revision not supported\n");
        cpu_halt();
    }

    if (!framebuffer_request.response || framebuffer_request.response->framebuffer_count < 1) {
        serial_write_str("no framebuffer\n");
        cpu_halt();
    }

    console_init(framebuffer_request.response->framebuffers[0]);

    kprintf("ChocolateOS %s\n", CHOCOLATEOS_VERSION);
    kprintf("================\n");

    if (bootloader_request.response) {
        kprintf("bootloader: %s %s\n",
                bootloader_request.response->name,
                bootloader_request.response->version);
    }

    uint64_t hhdm = 0;
    if (hhdm_request.response) {
        hhdm = hhdm_request.response->offset;
        kprintf("hhdm offset: %p\n", hhdm);
    }

    pmm_init(memmap_request.response, hhdm);
    kprintf("usable RAM: %llu MiB\n", pmm_usable_bytes() / (1024 * 1024));

    gdt_init();
    idt_init();
    pic_init();
    pit_init(100);
    keyboard_init();
    mouse_init();
    ps2_init();
    irq_enable();

    cxx_call_constructors();
    cxx_banner();

    kprintf("interrupts online. keyboard ready.\n");
    shell_run();
}
