#include <choc/idt.h>
#include <choc/pic.h>
#include <choc/pit.h>
#include <choc/keyboard.h>
#include <choc/mouse.h>
#include <choc/ps2.h>
#include <choc/kprintf.h>
#include <choc/io.h>

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t flags;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr idtr;

extern void isr_stub_0(void);
extern void isr_stub_1(void);
extern void isr_stub_2(void);
extern void isr_stub_3(void);
extern void isr_stub_4(void);
extern void isr_stub_5(void);
extern void isr_stub_6(void);
extern void isr_stub_7(void);
extern void isr_stub_8(void);
extern void isr_stub_9(void);
extern void isr_stub_10(void);
extern void isr_stub_11(void);
extern void isr_stub_12(void);
extern void isr_stub_13(void);
extern void isr_stub_14(void);
extern void isr_stub_15(void);
extern void isr_stub_16(void);
extern void isr_stub_17(void);
extern void isr_stub_18(void);
extern void isr_stub_19(void);
extern void isr_stub_20(void);
extern void isr_stub_21(void);
extern void isr_stub_32(void);
extern void isr_stub_33(void);
extern void isr_stub_39(void);
extern void isr_stub_44(void);

static void set_gate(int vec, void (*handler)(void), uint8_t flags) {
    uint64_t addr = (uint64_t)handler;
    idt[vec].offset_low = (uint16_t)addr;
    idt[vec].selector = 0x08;
    idt[vec].ist = 0;
    idt[vec].flags = flags;
    idt[vec].offset_mid = (uint16_t)(addr >> 16);
    idt[vec].offset_high = (uint32_t)(addr >> 32);
    idt[vec].reserved = 0;
}

void idt_init(void) {
    void (*stubs[])(void) = {
        isr_stub_0,  isr_stub_1,  isr_stub_2,  isr_stub_3,
        isr_stub_4,  isr_stub_5,  isr_stub_6,  isr_stub_7,
        isr_stub_8,  isr_stub_9,  isr_stub_10, isr_stub_11,
        isr_stub_12, isr_stub_13, isr_stub_14, isr_stub_15,
        isr_stub_16, isr_stub_17, isr_stub_18, isr_stub_19,
        isr_stub_20, isr_stub_21,
    };
    for (int i = 0; i < 22; i++) {
        set_gate(i, stubs[i], 0x8E);
    }
    set_gate(32, isr_stub_32, 0x8E);
    set_gate(33, isr_stub_33, 0x8E);
    set_gate(39, isr_stub_39, 0x8E);
    set_gate(44, isr_stub_44, 0x8E);

    idtr.limit = sizeof(idt) - 1;
    idtr.base = (uint64_t)&idt;
    __asm__ volatile("lidt %0" : : "m"(idtr));
}

void interrupt_dispatch(struct interrupt_frame *frame) {
    if (frame->vector == 32) {
        pit_on_irq();
        pic_eoi(0);
        return;
    }
    if (frame->vector == 33) {
        ps2_irq();
        pic_eoi(1);
        return;
    }
    if (frame->vector == 39) {
        pic_eoi(7);
        return;
    }
    if (frame->vector == 44) {
        ps2_irq();
        pic_eoi(12);
        return;
    }
    kprintf("\n*** exception vector %llu error=0x%llx rip=%p\n",
            frame->vector, frame->error, frame->rip);
    cpu_halt();
}
