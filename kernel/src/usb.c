#include <choc/usb.h>
#include <choc/pci.h>
#include <choc/vmm.h>
#include <choc/kprintf.h>
#include <choc/types.h>

static void on_pci(uint8_t bus, uint8_t slot, uint8_t func, uint16_t vendor, uint16_t device,
                   uint8_t classc, uint8_t subclass, uint8_t prog, void *user) {
    (void)vendor;
    (void)device;
    (void)user;
    if (classc != 0x0C || subclass != 0x03) {
        return;
    }
    uint64_t bar = pci_bar(bus, slot, func, 0);
    pci_enable_busmaster(bus, slot, func);
    if (!bar) {
        return;
    }
    vmm_map_mmio(bar, 0x10000);
    if (prog == 0x30) {
        kprintf("pci xhci %x:%x.%x bar=%p\n", bus, slot, func, bar);
        xhci_init_bar(bar);
    } else if (prog == 0x20) {
        kprintf("pci ehci %x:%x.%x bar=%p\n", bus, slot, func, bar);
        ehci_init_bar(bar);
    }
}

void usb_init(void) {
    kprintf("scanning USB 2.0 (EHCI) and USB 3.x (xHCI)...\n");
    pci_scan(on_pci, 0);
}

void usb_poll(void) {
    xhci_poll();
    ehci_poll();
}
