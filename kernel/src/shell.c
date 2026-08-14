#include <choc/shell.h>
#include <choc/kprintf.h>
#include <choc/keyboard.h>
#include <choc/console.h>
#include <choc/pmm.h>
#include <choc/pit.h>
#include <choc/string.h>
#include <choc/io.h>
#include <choc/types.h>
#include <choc/gui.h>

static void prompt(void) {
    kprintf("choc> ");
}

static void cmd_help(void) {
    kprintf("commands:\n");
    kprintf("  help     this list\n");
    kprintf("  about    kernel info\n");
    kprintf("  clear    clear the screen\n");
    kprintf("  mem      dump the memory map\n");
    kprintf("  ticks    PIT tick counter\n");
    kprintf("  choc     start the graphical desktop\n");
    kprintf("  reboot   triple-fault reboot (QEMU)\n");
}

static void cmd_about(void) {
    kprintf("ChocolateOS %s\n", CHOCOLATEOS_VERSION);
    kprintf("x86_64 kernel in C, C++, and assembly\n");
    kprintf("booted by Limine, running in long mode\n");
}

static void cmd_reboot(void) {
    kprintf("rebooting...\n");
    irq_disable();
    /* Pulse the keyboard controller reset line. */
    while (inb(0x64) & 0x02) {
    }
    outb(0x64, 0xFE);
    cpu_halt();
}

static void run_line(char *line) {
    while (*line == ' ') {
        line++;
    }
    if (*line == '\0') {
        return;
    }
    if (strcmp(line, "help") == 0) {
        cmd_help();
    } else if (strcmp(line, "about") == 0) {
        cmd_about();
    } else if (strcmp(line, "clear") == 0) {
        console_clear();
    } else if (strcmp(line, "mem") == 0) {
        pmm_dump();
    } else if (strcmp(line, "ticks") == 0) {
        kprintf("%llu\n", pit_ticks());
    } else if (strcmp(line, "choc") == 0) {
        kprintf("starting desktop...\n");
        gui_run();
        kprintf("back at the terminal. type 'choc' to open the GUI again.\n");
    } else if (strcmp(line, "reboot") == 0) {
        cmd_reboot();
    } else {
        kprintf("unknown command: %s\n", line);
    }
}

void shell_run(void) {
    char line[128];
    uint32_t len = 0;
    kprintf("\nType 'choc' for the desktop, or 'help' for commands.\n");
    prompt();
    for (;;) {
        __asm__ volatile("hlt");
        int c;
        while ((c = keyboard_getchar()) >= 0) {
            if (c == '\n') {
                kprintf("\n");
                line[len] = '\0';
                run_line(line);
                len = 0;
                prompt();
                continue;
            }
            if (c == '\b') {
                if (len > 0) {
                    len--;
                    kprintf("\b \b");
                }
                continue;
            }
            if (c >= 32 && c < 127 && len + 1 < sizeof(line)) {
                line[len++] = (char)c;
                kprintf("%c", c);
            }
        }
    }
}
