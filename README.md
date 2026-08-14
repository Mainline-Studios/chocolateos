# ChocolateOS

A homemade x86_64 operating system. The kernel is written in C, with a little C++ (global constructors) and assembly (GDT reload, interrupt stubs). Limine boots it into 64-bit long mode; QEMU runs the ISO.

## What you get

- Framebuffer console (chocolate-on-cream) and COM1 serial output
- GDT, IDT, PIC, PIT timer, PS/2 keyboard
- Memory map from the bootloader
- A tiny kernel shell: `help`, `about`, `clear`, `mem`, `ticks`, `reboot`

## Build (macOS)

You already have `clang`, `ld.lld`, `qemu-system-x86_64`, and `xorriso` if Homebrew QEMU and `xorriso` are installed. First build clones Limine.

```sh
make
make run
```

Click the QEMU window so keyboard input goes to the guest. Serial output also appears in the terminal.

Headless (serial only; framebuffer still exists but you will not see it):

```sh
make run-nographic
```

## Layout

```
boot/limine.conf     Limine menu
kernel/linker.ld     Higher-half ELF layout
kernel/include/choc  Public kernel headers
kernel/src           C / C++ / assembly
```

## Next pieces worth adding

Paging of your own (Limine already identity-maps via HHDM), a real physical allocator, heap, ACPI/APIC, a VFS, and user-mode processes.
