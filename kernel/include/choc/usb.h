#pragma once

#include <choc/types.h>

void usb_init(void);
void usb_poll(void);

int xhci_init_bar(uint64_t bar);
void xhci_poll(void);
int ehci_init_bar(uint64_t bar);
void ehci_poll(void);

void usb_hid_boot_keyboard(const uint8_t *report, int len);
void usb_hid_boot_mouse(const uint8_t *report, int len);
