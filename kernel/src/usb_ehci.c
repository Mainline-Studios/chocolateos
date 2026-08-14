#include <choc/usb.h>
#include <choc/pmm.h>
#include <choc/pit.h>
#include <choc/kprintf.h>
#include <choc/string.h>
#include <choc/types.h>

#define R32(p) (*(volatile uint32_t *)(p))

typedef struct {
    uint32_t next;
    uint32_t alt;
    uint32_t token;
    uint32_t buf[5];
    uint32_t bufhi[5];
} __attribute__((packed, aligned(32))) qtd_t;

typedef struct {
    uint32_t horiz;
    uint32_t caps;
    uint32_t caps2;
    uint32_t current;
    qtd_t overlay;
    uint32_t pad[4];
} __attribute__((packed, aligned(32))) qh_t;

static uint8_t *cap;
static uint8_t *op;
static uint32_t nports;
static int ready;
static qh_t *async;
static uint32_t *framelist;
static qh_t *intr_qh;
static qtd_t *intr_td;
static uint8_t *intr_buf;
static int hid_kind;
static uint8_t prev_report[8];

static int wait_ms(volatile uint32_t *reg, uint32_t mask, int set, int ms) {
    uint64_t start = pit_ticks();
    uint64_t need = (uint64_t)ms / 10 + 2;
    while (1) {
        uint32_t v = R32(reg);
        if (set ? (v & mask) : !(v & mask)) {
            return 1;
        }
        if (pit_ticks() - start > need) {
            return 0;
        }
    }
}

static void sleep_ticks(uint64_t t) {
    uint64_t s = pit_ticks();
    while (pit_ticks() - s < t) {
    }
}

static qtd_t *td_fill(qtd_t *td, void *data, uint32_t len, uint32_t pid, uint32_t toggle, int ioc) {
    memset(td, 0, sizeof(*td));
    td->next = 1;
    td->alt = 1;
    td->token = (len << 16) | (pid << 8) | (3u << 10) | (toggle << 31) | 0x80 | (ioc ? (1u << 15) : 0);
    uint64_t phys = data ? pmm_to_phys(data) : 0;
    td->buf[0] = (uint32_t)phys;
    td->bufhi[0] = (uint32_t)(phys >> 32);
    return td;
}

static int async_run(qtd_t *first) {
    async->overlay.next = pmm_to_phys(first);
    async->overlay.alt = 1;
    async->overlay.token = 0;
    async->current = pmm_to_phys(first);
    R32(op + 0x10) |= 0x20; /* ASE */
    uint64_t start = pit_ticks();
    qtd_t *td = first;
    while (pit_ticks() - start < 80) {
        if ((td->token & 0x80) == 0) {
            if (td->next & 1) {
                return (td->token & 0x7C) == 0;
            }
            td = pmm_to_virt(td->next & ~0x1Fu);
            continue;
        }
        if (td->token & 0x7C) {
            return 0;
        }
    }
    return 0;
}

static int control(uint8_t addr, uint8_t bm, uint8_t req, uint16_t value, uint16_t index, void *data, uint16_t len, int in) {
    uint8_t *setup = pmm_alloc(8, 16);
    setup[0] = bm;
    setup[1] = req;
    setup[2] = (uint8_t)value;
    setup[3] = (uint8_t)(value >> 8);
    setup[4] = (uint8_t)index;
    setup[5] = (uint8_t)(index >> 8);
    setup[6] = (uint8_t)len;
    setup[7] = (uint8_t)(len >> 8);

    qtd_t *td0 = pmm_alloc(sizeof(qtd_t), 32);
    qtd_t *td1 = pmm_alloc(sizeof(qtd_t), 32);
    qtd_t *td2 = pmm_alloc(sizeof(qtd_t), 32);
    td_fill(td0, setup, 8, 2, 0, 0);
    if (len) {
        td_fill(td1, data, len, in ? 1 : 0, 1, 0);
        td0->next = pmm_to_phys(td1);
        td_fill(td2, 0, 0, in ? 0 : 1, 1, 1);
        td1->next = pmm_to_phys(td2);
    } else {
        td_fill(td2, 0, 0, 1, 1, 1);
        td0->next = pmm_to_phys(td2);
    }
    async->caps = 0x0040E000 | addr; /* maxpkt 64, DTC, H */
    async->caps2 = 0x40000000;
    return async_run(td0);
}

static int setup_hid(uint8_t addr, uint8_t *cfg, int len) {
    int iface = -1, proto = 0, ep = 1, maxpkt = 8, interval = 10, cfgval = 1;
    for (int off = 0; off + 2 <= len;) {
        uint8_t l = cfg[off], t = cfg[off + 1];
        if (l < 2) {
            break;
        }
        if (t == 2 && l >= 6) {
            cfgval = cfg[off + 5];
        }
        if (t == 4 && l >= 9 && cfg[off + 5] == 3 && cfg[off + 6] == 1) {
            iface = cfg[off + 2];
            proto = cfg[off + 7];
        }
        if (t == 5 && iface >= 0 && (cfg[off + 2] & 0x80) && ((cfg[off + 3] & 3) == 3)) {
            ep = cfg[off + 2] & 0x0F;
            maxpkt = cfg[off + 4] | (cfg[off + 5] << 8);
            interval = cfg[off + 6];
            break;
        }
        off += l;
    }
    if (iface < 0) {
        return 0;
    }
    control(addr, 0x00, 9, (uint16_t)cfgval, 0, 0, 0, 0);
    control(addr, 0x21, 0x0B, 0, (uint16_t)iface, 0, 0, 0);
    control(addr, 0x21, 0x0A, 0, (uint16_t)iface, 0, 0, 0);

    hid_kind = proto == 2 ? 2 : 1;
    intr_buf = pmm_alloc(16, 16);
    intr_td = pmm_alloc(sizeof(qtd_t), 32);
    td_fill(intr_td, intr_buf, (uint32_t)maxpkt > 8 ? 8 : (uint32_t)maxpkt, 1, 0, 1);
    intr_td->next = pmm_to_phys(intr_td);

    intr_qh = pmm_alloc(sizeof(qh_t), 32);
    intr_qh->horiz = 1;
    intr_qh->caps = 0x0000E000 | ((uint32_t)maxpkt << 16) | (1u << 14) | addr | ((uint32_t)ep << 8);
    intr_qh->caps2 = ((uint32_t)interval) << 16;
    intr_qh->overlay.next = pmm_to_phys(intr_td);
    intr_qh->overlay.alt = 1;
    intr_qh->overlay.token = 0;

    uint32_t link = (uint32_t)pmm_to_phys(intr_qh) | 2;
    for (int i = 0; i < 1024; i++) {
        framelist[i] = link;
    }
    kprintf("ehci hid %s addr %u\n", hid_kind == 1 ? "keyboard" : "mouse", addr);
    return 1;
}

static int enumerate_port(uint32_t i, uint8_t addr) {
    volatile uint32_t *psc = (volatile uint32_t *)(op + 0x44 + 4 * i);
    uint32_t sc = R32(psc);
    if (!(sc & 1)) {
        return 0;
    }
    R32(psc) |= 1u << 8;
    sleep_ticks(6);
    R32(psc) &= ~(1u << 8);
    sleep_ticks(2);
    sc = R32(psc);
    if (!(sc & 4)) {
        return 0;
    }
    uint8_t *desc = pmm_alloc(18, 16);
    if (!control(0, 0x80, 6, 0x0100, 0, desc, 8, 1)) {
        return 0;
    }
    if (!control(0, 0x00, 5, addr, 0, 0, 0, 0)) {
        return 0;
    }
    if (!control(addr, 0x80, 6, 0x0100, 0, desc, 18, 1)) {
        return 0;
    }
    uint8_t *cfg = pmm_alloc(256, 16);
    if (!control(addr, 0x80, 6, 0x0200, 0, cfg, 9, 1)) {
        return 0;
    }
    uint16_t total = (uint16_t)(cfg[2] | (cfg[3] << 8));
    if (total > 256) {
        total = 256;
    }
    if (!control(addr, 0x80, 6, 0x0200, 0, cfg, total, 1)) {
        return 0;
    }
    return setup_hid(addr, cfg, total);
}

int ehci_init_bar(uint64_t bar) {
    if (!bar) {
        return 0;
    }
    cap = pmm_to_virt(bar);
    uint8_t caplen = cap[0];
    op = cap + caplen;
    nports = R32(cap + 0x04) & 0xF;

    if (R32(op) & 1) {
        R32(op) &= ~1u;
        wait_ms((volatile uint32_t *)op, 1, 0, 200);
    }
    R32(op) |= 2;
    if (!wait_ms((volatile uint32_t *)op, 2, 0, 200)) {
        kprintf("ehci reset timeout\n");
        return 0;
    }

    framelist = pmm_alloc(1024 * 4, 4096);
    for (int i = 0; i < 1024; i++) {
        framelist[i] = 1;
    }
    async = pmm_alloc(sizeof(qh_t), 32);
    async->horiz = (uint32_t)pmm_to_phys(async) | 2;
    async->caps = 0x00008000; /* H */
    async->caps2 = 0;
    async->overlay.next = 1;
    async->overlay.alt = 1;
    async->overlay.token = 0x40;

    R32(op + 0x08) = 0;
    R32(op + 0x14) = (uint32_t)pmm_to_phys(framelist);
    R32(op + 0x18) = (uint32_t)pmm_to_phys(async);
    R32(op + 0x40) = 1;
    R32(op) = 0x80011; /* run, periodic, ITC=8 */

    ready = 1;
    kprintf("ehci usb2 host: %u ports\n", nports);

    uint8_t addr = 1;
    int n = 0;
    for (uint32_t i = 0; i < nports; i++) {
        if (enumerate_port(i, addr)) {
            n++;
            addr++;
        }
    }
    return n;
}

void ehci_poll(void) {
    if (!ready || !intr_td) {
        return;
    }
    if (intr_td->token & 0x80) {
        return;
    }
    if ((intr_td->token & 0x7C) == 0) {
        if (memcmp(intr_buf, prev_report, 8) != 0) {
            if (hid_kind == 1) {
                usb_hid_boot_keyboard(intr_buf, 8);
            } else {
                usb_hid_boot_mouse(intr_buf, 8);
            }
            memcpy(prev_report, intr_buf, 8);
        }
    }
    td_fill(intr_td, intr_buf, 8, 1, 0, 1);
    intr_td->next = pmm_to_phys(intr_td);
    intr_qh->overlay.next = pmm_to_phys(intr_td);
    intr_qh->overlay.token = 0;
}
