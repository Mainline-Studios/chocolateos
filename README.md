# ChocolateOS

A homemade x86_64 operating system. The kernel is written in C, with a little C++ (global constructors) and assembly (GDT reload, interrupt stubs). Limine boots it into 64-bit long mode; QEMU runs the ISO.

The current ISO is **Preview** — not a numbered version. Typing `choc` opens a setup wizard (language, input, display, network stubs, hostname, and more), then the desktop.

## What you get

- Framebuffer console (chocolate-on-cream) and COM1 serial output
- GDT, IDT, PIC, PIT timer, PS/2 keyboard
- Memory map from the bootloader
- A kernel shell, plus a Preview setup wizard and graphical desktop
- USB 2.0 (EHCI) and USB 3.x (xHCI) host drivers with HID keyboard/mouse
- PS/2 fallback, plus serial input in the QEMU terminal

## Build (macOS)

You already have `clang`, `ld.lld`, `qemu-system-x86_64`, and `xorriso` if Homebrew QEMU and `xorriso` are installed. First build clones Limine.

```sh
make
make run
```

Click the **QEMU window** (not the terminal) so it has focus, then type `choc`. If letters do not type, press **F1** or an **arrow key**, or use **Page Up / Page Down**, **Enter** inside the wizard, **Esc** to go back, and the on-screen Back / Next / D-pad buttons with the mouse.

The shell also accepts keys from the serial console in that same terminal if the window is not focused.

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
