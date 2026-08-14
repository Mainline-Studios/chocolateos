#pragma once

#include <choc/types.h>

uint32_t pci_read32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off);
void pci_write32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off, uint32_t val);
uint16_t pci_read16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off);
void pci_write16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off, uint16_t val);
uint64_t pci_bar(uint8_t bus, uint8_t slot, uint8_t func, uint8_t bar);
void pci_enable_busmaster(uint8_t bus, uint8_t slot, uint8_t func);

typedef void (*pci_iter_fn)(uint8_t bus, uint8_t slot, uint8_t func, uint16_t vendor, uint16_t device, uint8_t classc, uint8_t subclass, uint8_t prog, void *user);
void pci_scan(pci_iter_fn fn, void *user);
