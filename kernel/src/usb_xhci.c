#include <choc/usb.h>
#include <choc/pmm.h>
#include <choc/pit.h>
#include <choc/kprintf.h>
#include <choc/string.h>
#include <choc/types.h>

#define R32(p) (*(volatile uint32_t *)(p))
#define R64(p) (*(volatile uint64_t *)(p))
#define RING 32
#define TRB_NORMAL  1u
#define TRB_SETUP   2u
#define TRB_DATA    3u
#define TRB_STATUS  4u
#define TRB_LINK    6u
#define TRB_IOC     (1u << 5)
#define TRB_ISP     (1u << 2)
#define TRB_IDT     (1u << 6)

typedef struct {
    uint64_t ptr;
    uint32_t status;
    uint32_t ctrl;
} __attribute__((packed)) trb_t;

struct ring {
    trb_t *trb;
    uint32_t i;
    uint32_t cycle;
};

struct hid_ep {
    int used;
    int slot;
    int dci;
    int kind;
    struct ring ring;
    uint8_t *buf;
};

static uint8_t *cap;
static uint8_t *op;
static uint8_t *rt;
static uint32_t *db;
static uint32_t max_slots;
static uint32_t max_ports;
static uint32_t ctxsz;
static struct ring cmd;
static struct ring ep0_ring[64];
static trb_t *evring;
static uint32_t ev_i;
static uint32_t ev_cycle;
static uint64_t *dcbaa;
static struct hid_ep hid[8];
static int ready;
static int last_cmd_cc;
static int last_cmd_slot;
static int last_xfer_cc;
static int last_xfer_slot;
static uint8_t slot_port[64];

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

static uint8_t *ctx_at(uint8_t *base, int i) {
    return base + (uint32_t)i * ctxsz;
}

static void ring_init(struct ring *r) {
    r->trb = pmm_alloc(sizeof(trb_t) * RING, 64);
    r->i = 0;
    r->cycle = 1;
    r->trb[RING - 1].ptr = pmm_to_phys(r->trb);
    r->trb[RING - 1].ctrl = (TRB_LINK << 10) | (1u << 1) | 1u;
}

static trb_t *ring_place(struct ring *r) {
    if (r->i >= RING - 1) {
        r->trb[RING - 1].ptr = pmm_to_phys(r->trb);
        r->trb[RING - 1].ctrl = (TRB_LINK << 10) | (1u << 1) | r->cycle;
        r->cycle ^= 1;
        r->i = 0;
    }
    trb_t *t = &r->trb[r->i];
    memset(t, 0, sizeof(*t));
    r->i++;
    return t;
}

static void hid_arm(struct hid_ep *h) {
    trb_t *n = ring_place(&h->ring);
    n->ptr = pmm_to_phys(h->buf);
    n->status = 8;
    n->ctrl = TRB_IOC | TRB_ISP | (TRB_NORMAL << 10) | h->ring.cycle;
    db[h->slot] = (uint32_t)h->dci;
}

static void process_events(void) {
    if (!evring) {
        return;
    }
    for (;;) {
        trb_t *t = &evring[ev_i];
        if ((t->ctrl & 1u) != ev_cycle) {
            break;
        }
        uint32_t type = (t->ctrl >> 10) & 0x3F;
        if (type == 33) {
            last_cmd_cc = (int)((t->status >> 24) & 0xFF);
            last_cmd_slot = (int)((t->ctrl >> 24) & 0xFF);
        } else if (type == 32) {
            last_xfer_cc = (int)((t->status >> 24) & 0xFF);
            last_xfer_slot = (int)((t->ctrl >> 24) & 0xFF);
            uint32_t epid = (t->ctrl >> 16) & 0x1F;
            uint32_t code = (uint32_t)last_xfer_cc;
            for (int i = 0; i < 8; i++) {
                if (!hid[i].used || hid[i].slot != last_xfer_slot || hid[i].dci != (int)epid) {
                    continue;
                }
                if (code == 1 || code == 13) {
                    if (hid[i].kind == 1) {
                        usb_hid_boot_keyboard(hid[i].buf, 8);
                    } else {
                        usb_hid_boot_mouse(hid[i].buf, 8);
                    }
                }
                hid_arm(&hid[i]);
            }
        }
        ev_i++;
        if (ev_i == RING) {
            ev_i = 0;
            ev_cycle ^= 1;
        }
        R64(rt + 0x20 + 0x18) = pmm_to_phys(&evring[ev_i]) | (1ull << 3);
    }
}

static int issue_cmd(uint64_t ptr, uint32_t status, uint32_t ctrl) {
    last_cmd_cc = 0;
    trb_t *t = ring_place(&cmd);
    t->ptr = ptr;
    t->status = status;
    t->ctrl = ctrl | cmd.cycle;
    db[0] = 0;
    uint64_t start = pit_ticks();
    while (pit_ticks() - start < 80) {
        process_events();
        if (last_cmd_cc) {
            return last_cmd_cc == 1;
        }
    }
    return 0;
}

static int control_xfer(int slot, uint8_t bm, uint8_t req, uint16_t value, uint16_t index, void *data, uint16_t len, int in) {
    uint8_t setup[8];
    setup[0] = bm;
    setup[1] = req;
    setup[2] = (uint8_t)value;
    setup[3] = (uint8_t)(value >> 8);
    setup[4] = (uint8_t)index;
    setup[5] = (uint8_t)(index >> 8);
    setup[6] = (uint8_t)len;
    setup[7] = (uint8_t)(len >> 8);

    uint32_t trt = len == 0 ? 0 : (in ? 3u : 2u);
    trb_t *s = ring_place(&ep0_ring[slot]);
    memcpy(&s->ptr, setup, 8);
    s->status = 8;
    s->ctrl = (TRB_SETUP << 10) | TRB_IDT | (trt << 16) | ep0_ring[slot].cycle;

    if (len) {
        trb_t *d = ring_place(&ep0_ring[slot]);
        d->ptr = pmm_to_phys(data);
        d->status = len;
        d->ctrl = (TRB_DATA << 10) | ((in ? 1u : 0u) << 16) | ep0_ring[slot].cycle;
    }

    trb_t *st = ring_place(&ep0_ring[slot]);
    uint32_t dir = (len == 0 || !in) ? 1u : 0u;
    st->ctrl = (TRB_STATUS << 10) | TRB_IOC | (dir << 16) | ep0_ring[slot].cycle;

    last_xfer_cc = 0;
    db[slot] = 1;
    uint64_t start = pit_ticks();
    while (pit_ticks() - start < 80) {
        process_events();
        if (last_xfer_cc && last_xfer_slot == slot) {
            return last_xfer_cc == 1 || last_xfer_cc == 13;
        }
    }
    return 0;
}

static int configure_hid(int slot, int speed, uint8_t *cfg, int cfglen) {
    int iface = -1, proto = 0, epaddr = 0, maxpkt = 8, interval = 8, cfgval = 1;
    for (int off = 0; off + 2 <= cfglen;) {
        uint8_t l = cfg[off];
        uint8_t t = cfg[off + 1];
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
        if (t == 5 && iface >= 0 && l >= 7 && (cfg[off + 2] & 0x80) && ((cfg[off + 3] & 3) == 3)) {
            epaddr = cfg[off + 2];
            maxpkt = cfg[off + 4] | (cfg[off + 5] << 8);
            interval = cfg[off + 6];
            break;
        }
        off += l;
    }
    if (iface < 0 || !epaddr) {
        return 0;
    }

    control_xfer(slot, 0x00, 9, (uint16_t)cfgval, 0, 0, 0, 0);
    control_xfer(slot, 0x21, 0x0B, 0, (uint16_t)iface, 0, 0, 0);
    control_xfer(slot, 0x21, 0x0A, 0, (uint16_t)iface, 0, 0, 0);

    int ep = epaddr & 0x0F;
    int dci = ep * 2 + ((epaddr & 0x80) ? 1 : 0);

    struct hid_ep *h = 0;
    for (int i = 0; i < 8; i++) {
        if (!hid[i].used) {
            h = &hid[i];
            break;
        }
    }
    if (!h) {
        return 0;
    }
    ring_init(&h->ring);
    h->buf = pmm_alloc(16, 64);
    h->used = 1;
    h->slot = slot;
    h->dci = dci;
    h->kind = proto == 2 ? 2 : 1;

    uint8_t *inctx = pmm_alloc(ctxsz * 33, 64);
    uint32_t *icc = (uint32_t *)inctx;
    icc[1] = (1u << 0) | (1u << (unsigned)dci);

    uint32_t *slotc = (uint32_t *)ctx_at(inctx, 1);
    slotc[0] = ((uint32_t)dci << 27) | ((uint32_t)speed << 20);
    slotc[1] = (uint32_t)slot_port[slot] << 16;

    uint32_t *epc = (uint32_t *)ctx_at(inctx, dci + 1);
    epc[0] = ((uint32_t)interval) << 16;
    epc[1] = (3u << 1) | (7u << 3) | ((uint32_t)maxpkt << 16);
    uint64_t dq = pmm_to_phys(h->ring.trb) | 1;
    memcpy(&epc[2], &dq, 8);
    epc[4] = 8;

    if (!issue_cmd(pmm_to_phys(inctx), 0, (12u << 10) | ((uint32_t)slot << 24))) {
        h->used = 0;
        return 0;
    }

    hid_arm(h);
    kprintf("xhci hid %s slot %d\n", h->kind == 1 ? "keyboard" : "mouse", slot);
    return 1;
}

static int enumerate_port(uint32_t port) {
    volatile uint32_t *portsc = (volatile uint32_t *)(op + 0x400 + 0x10 * (port - 1));
    uint32_t sc = R32(portsc);
    if (!(sc & 1)) {
        return 0;
    }
    /* Keep PP, start a port reset. USB3 uses warm reset. */
    if (((sc >> 10) & 0xF) >= 4) {
        R32(portsc) = (R32(portsc) & 0x00E0C3E0u) | (1u << 9) | (1u << 31);
        if (!wait_ms(portsc, 1u << 19, 1, 800)) {
            kprintf("xhci port %u warm reset timeout\n", port);
            return 0;
        }
        R32(portsc) = (R32(portsc) & 0x00E0C3E0u) | (1u << 9) | (1u << 19);
    } else {
        R32(portsc) = (R32(portsc) & 0x00E0C3E0u) | (1u << 9) | (1u << 4);
        if (!wait_ms(portsc, 1u << 4, 0, 800)) {
            kprintf("xhci port %u reset timeout sc=%x\n", port, R32(portsc));
            return 0;
        }
        if (R32(portsc) & (1u << 21)) {
            R32(portsc) = (R32(portsc) & 0x00E0C3E0u) | (1u << 9) | (1u << 21);
        }
    }
    if (!wait_ms(portsc, 2, 1, 800)) {
        kprintf("xhci port %u not enabled sc=%x\n", port, R32(portsc));
        return 0;
    }
    uint32_t speed = (R32(portsc) >> 10) & 0xF;

    if (!issue_cmd(0, 0, 9u << 10)) {
        kprintf("xhci enable slot failed cc=%d\n", last_cmd_cc);
        return 0;
    }
    int slot = last_cmd_slot;
    if (slot <= 0 || slot >= (int)max_slots) {
        return 0;
    }

    uint8_t *dctx = pmm_alloc(ctxsz * 32, 64);
    uint8_t *inctx = pmm_alloc(ctxsz * 33, 64);
    dcbaa[slot] = pmm_to_phys(dctx);
    slot_port[slot] = (uint8_t)port;

    uint32_t *icc = (uint32_t *)inctx;
    icc[1] = 0x3;
    uint32_t *slotc = (uint32_t *)ctx_at(inctx, 1);
    slotc[0] = (1u << 27) | (speed << 20);
    slotc[1] = port << 16;

    ring_init(&ep0_ring[slot]);
    uint32_t mps = speed >= 4 ? 512 : (speed == 3 ? 64 : 8);
    uint32_t *ep0c = (uint32_t *)ctx_at(inctx, 2);
    ep0c[1] = (3u << 1) | (4u << 3) | (mps << 16);
    uint64_t dq = pmm_to_phys(ep0_ring[slot].trb) | 1;
    memcpy(&ep0c[2], &dq, 8);
    ep0c[4] = 8;

    if (!issue_cmd(pmm_to_phys(inctx), 0, (11u << 10) | ((uint32_t)slot << 24))) {
        kprintf("xhci address device failed cc=%d\n", last_cmd_cc);
        return 0;
    }

    uint8_t *desc = pmm_alloc(32, 16);
    if (!control_xfer(slot, 0x80, 6, 0x0100, 0, desc, 18, 1)) {
        return 0;
    }
    uint8_t *cfg = pmm_alloc(256, 16);
    if (!control_xfer(slot, 0x80, 6, 0x0200, 0, cfg, 9, 1)) {
        return 0;
    }
    uint16_t total = (uint16_t)(cfg[2] | (cfg[3] << 8));
    if (total < 9) {
        return 0;
    }
    if (total > 256) {
        total = 256;
    }
    if (!control_xfer(slot, 0x80, 6, 0x0200, 0, cfg, total, 1)) {
        return 0;
    }
    return configure_hid(slot, (int)speed, cfg, total);
}

int xhci_init_bar(uint64_t bar) {
    if (!bar) {
        return 0;
    }
    cap = pmm_to_virt(bar);
    uint8_t caplen = cap[0];
    op = cap + caplen;
    uint32_t hcs1 = R32(cap + 0x04);
    max_slots = hcs1 & 0xFF;
    max_ports = (hcs1 >> 24) & 0xFF;
    uint32_t hcs2 = R32(cap + 0x08);
    uint32_t hcc1 = R32(cap + 0x10);
    ctxsz = (hcc1 & 4) ? 64 : 32;
    uint32_t rtsoff = R32(cap + 0x18) & ~0x1Fu;
    uint32_t dboff = R32(cap + 0x14) & ~0x3u;
    rt = cap + rtsoff;
    db = (uint32_t *)(cap + dboff);

    if (R32(op) & 1) {
        R32(op) &= ~1u;
        wait_ms((volatile uint32_t *)op, 1, 0, 200);
    }
    R32(op) |= 1u << 1;
    if (!wait_ms((volatile uint32_t *)op, 1u << 1, 0, 200)) {
        kprintf("xhci reset timeout\n");
        return 0;
    }

    R32(op + 0x38) = max_slots;
    dcbaa = pmm_alloc(8 * (max_slots + 1), 64);
    uint32_t sp = ((hcs2 >> 21) & 0x1F) << 5 | ((hcs2 >> 27) & 0x1F);
    if (sp) {
        uint64_t *arr = pmm_alloc(8 * sp, 64);
        for (uint32_t i = 0; i < sp; i++) {
            arr[i] = pmm_to_phys(pmm_alloc(4096, 4096));
        }
        dcbaa[0] = pmm_to_phys(arr);
    }
    R64(op + 0x30) = pmm_to_phys(dcbaa);

    ring_init(&cmd);
    R64(op + 0x18) = pmm_to_phys(cmd.trb) | 1;

    evring = pmm_alloc(sizeof(trb_t) * RING, 64);
    ev_i = 0;
    ev_cycle = 1;
    uint8_t *erst = pmm_alloc(16, 64);
    uint64_t erst_addr = pmm_to_phys(evring);
    memcpy(erst, &erst_addr, 8);
    uint16_t rss = RING;
    memcpy(erst + 8, &rss, 2);

    R32(rt + 0x20 + 0x08) = 1;
    R64(rt + 0x20 + 0x10) = pmm_to_phys(erst);
    R64(rt + 0x20 + 0x18) = pmm_to_phys(evring);

    R32(op) |= 1;
    if (!wait_ms((volatile uint32_t *)(op + 4), 1, 0, 200)) {
        kprintf("xhci run timeout\n");
        return 0;
    }
    ready = 1;
    kprintf("xhci usb3/usb2 host: %u ports\n", max_ports);

    for (uint32_t p = 1; p <= max_ports; p++) {
        volatile uint32_t *portsc = (volatile uint32_t *)(op + 0x400 + 0x10 * (p - 1));
        R32(portsc) = (R32(portsc) & 0x0E00C3E0u) | (1u << 9);
    }
    uint64_t w = pit_ticks();
    while (pit_ticks() - w < 20) {
        process_events();
    }

    int n = 0;
    for (uint32_t p = 1; p <= max_ports; p++) {
        n += enumerate_port(p);
    }
    return n;
}

void xhci_poll(void) {
    if (ready) {
        process_events();
    }
}
