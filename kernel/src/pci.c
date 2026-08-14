#include <choc/pci.h>
#include <choc/io.h>

uint32_t pci_read32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off) {
    outl(0xCF8, 0x80000000u | ((uint32_t)bus << 16) | ((uint32_t)slot << 11) |
                    ((uint32_t)func << 8) | (off & 0xFC));
    return inl(0xCFC);
}

void pci_write32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off, uint32_t val) {
    outl(0xCF8, 0x80000000u | ((uint32_t)bus << 16) | ((uint32_t)slot << 11) |
                    ((uint32_t)func << 8) | (off & 0xFC));
    outl(0xCFC, val);
}

uint16_t pci_read16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off) {
    uint32_t v = pci_read32(bus, slot, func, off & 0xFC);
    return (uint16_t)(v >> ((off & 2) * 8));
}

void pci_write16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off, uint16_t val) {
    uint32_t old = pci_read32(bus, slot, func, off & 0xFC);
    uint32_t shift = (off & 2) * 8;
    old &= ~(0xFFFFu << shift);
    old |= ((uint32_t)val) << shift;
    pci_write32(bus, slot, func, off & 0xFC, old);
}

uint64_t pci_bar(uint8_t bus, uint8_t slot, uint8_t func, uint8_t bar) {
    uint32_t lo = pci_read32(bus, slot, func, 0x10 + bar * 4);
    if (lo & 1) {
        return 0;
    }
    uint64_t addr = lo & 0xFFFFFFF0u;
    if ((lo & 6) == 4) {
        uint32_t hi = pci_read32(bus, slot, func, 0x10 + bar * 4 + 4);
        addr |= ((uint64_t)hi) << 32;
    }
    return addr;
}

void pci_enable_busmaster(uint8_t bus, uint8_t slot, uint8_t func) {
    uint16_t cmd = pci_read16(bus, slot, func, 0x04);
    cmd |= 0x0006;
    pci_write16(bus, slot, func, 0x04, cmd);
}

void pci_scan(pci_iter_fn fn, void *user) {
    for (uint32_t bus = 0; bus < 256; bus++) {
        for (uint32_t slot = 0; slot < 32; slot++) {
            for (uint32_t func = 0; func < 8; func++) {
                uint32_t id = pci_read32((uint8_t)bus, (uint8_t)slot, (uint8_t)func, 0);
                uint16_t vendor = (uint16_t)id;
                if (vendor == 0xFFFF) {
                    if (func == 0) {
                        break;
                    }
                    continue;
                }
                uint32_t classr = pci_read32((uint8_t)bus, (uint8_t)slot, (uint8_t)func, 0x08);
                fn((uint8_t)bus, (uint8_t)slot, (uint8_t)func, vendor, (uint16_t)(id >> 16),
                   (uint8_t)(classr >> 24), (uint8_t)(classr >> 16), (uint8_t)(classr >> 8), user);
                if (func == 0) {
                    uint8_t header = (uint8_t)(pci_read32((uint8_t)bus, (uint8_t)slot, 0, 0x0C) >> 16);
                    if ((header & 0x80) == 0) {
                        break;
                    }
                }
            }
        }
    }
}
