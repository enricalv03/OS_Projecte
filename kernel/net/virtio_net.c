/* =============================================================================
 * kernel/net/virtio_net.c — virtio-net PCI NIC driver (legacy virtio 0.9)
 * =============================================================================
 * References:
 *   Virtual I/O Device (VIRTIO) Version 1.0, OASIS standard
 *   https://docs.oasis-open.org/virtio/virtio/v1.0/cs04/virtio-v1.0-cs04.html
 *
 * Legacy interface register map (PCI BAR0 I/O space):
 *   0x00  DEVICE_FEATURES      (4B, RO)
 *   0x04  GUEST_FEATURES       (4B, WO)
 *   0x08  QUEUE_ADDRESS        (4B, R/W)  — page-frame number of virt-queue
 *   0x0C  QUEUE_SIZE           (2B, RO)   — max descriptors in current VQ
 *   0x0E  QUEUE_SELECT         (2B, WO)   — select which VQ
 *   0x10  QUEUE_NOTIFY         (2B, WO)   — kick a VQ
 *   0x12  DEVICE_STATUS        (1B, R/W)
 *   0x13  ISR_STATUS           (1B, RO)
 *   0x14  (MAC address 6 bytes, when feature VIRTIO_NET_F_MAC set)
 *
 * Virtqueue layout (split-ring):
 *   Descriptor table:  queue_size * 16 bytes
 *   Available ring:    4 + queue_size * 2 bytes  (+ 2 padding)
 *   Used ring:         4 + queue_size * 8 bytes
 *
 * NOTE: This implementation does NOT use DMA ring buffers because our kernel
 * has no DMA-safe allocator yet.  We use a simplified polling loop that writes
 * a single TX descriptor, notifies the device, and reads the used ring.
 * RX is handled similarly.  This is not production-quality but is functional
 * for QEMU in single-core, single-thread mode.
 * =========================================================================== */

#include "virtio_net.h"
#include "net.h"
#include "lib/kstring.h"
#include "memory/heap.h"

/* ---- PCI scanning -------------------------------------------------------- */

/* I/O port helpers (x86) */
static inline unsigned int inl(unsigned short port) {
    unsigned int val;
    __asm__ volatile("inl %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}
static inline unsigned short inw(unsigned short port) {
    unsigned short val;
    __asm__ volatile("inw %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}
static inline unsigned char inb(unsigned short port) {
    unsigned char val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}
static inline void outl(unsigned short port, unsigned int val) {
    __asm__ volatile("outl %0, %1" :: "a"(val), "Nd"(port));
}
static inline void outw(unsigned short port, unsigned short val) {
    __asm__ volatile("outw %0, %1" :: "a"(val), "Nd"(port));
}
static inline void outb(unsigned short port, unsigned char val) {
    __asm__ volatile("outb %0, %1" :: "a"(val), "Nd"(port));
}

/* PCI config space access via CF8/CFC mechanism. */
static unsigned int pci_cfg_read32(unsigned int bus, unsigned int dev,
                                   unsigned int fn, unsigned int off) {
    unsigned int addr = (1u << 31) | (bus << 16) | (dev << 11) |
                        (fn << 8) | (off & 0xFC);
    outl(0xCF8, addr);
    return inl(0xCFC);
}
/* pci_cfg_read16: reserved for future use (PCI capability parsing). */
static unsigned short pci_cfg_read16(unsigned int bus, unsigned int dev,
                                     unsigned int fn, unsigned int off)
    __attribute__((unused));
static unsigned short pci_cfg_read16(unsigned int bus, unsigned int dev,
                                     unsigned int fn, unsigned int off) {
    unsigned int val = pci_cfg_read32(bus, dev, fn, off & ~3u);
    return (unsigned short)(val >> ((off & 2) * 8));
}

/* Find a PCI device by vendor:device ID.
 * Returns 1 and sets bus/dev/fn out params if found. */
static int pci_find_device(unsigned short vendor, unsigned short device,
                            unsigned int* bus_out, unsigned int* dev_out,
                            unsigned int* fn_out) {
    for (unsigned int bus = 0; bus < 256; bus++) {
        for (unsigned int slot = 0; slot < 32; slot++) {
            unsigned int vid = pci_cfg_read32(bus, slot, 0, 0);
            if ((vid & 0xFFFF) != vendor) continue;
            if (((vid >> 16) & 0xFFFF) != device) continue;
            *bus_out = bus; *dev_out = slot; *fn_out = 0;
            return 1;
        }
    }
    return 0;
}

/* ---- Virtio-net registers ------------------------------------------------ */

#define VIRTIO_STATUS_RESET       0
#define VIRTIO_STATUS_ACK         1
#define VIRTIO_STATUS_DRIVER      2
#define VIRTIO_STATUS_DRIVER_OK   4
#define VIRTIO_STATUS_FAILED    128

#define VIRTIO_NET_F_MAC    (1 << 5)

/* Legacy register offsets from BAR0 base. */
#define VIRT_REG_DEV_FEAT    0x00
#define VIRT_REG_GST_FEAT    0x04
#define VIRT_REG_Q_ADDR      0x08
#define VIRT_REG_Q_SIZE      0x0C
#define VIRT_REG_Q_SELECT    0x0E
#define VIRT_REG_Q_NOTIFY    0x10
#define VIRT_REG_DEV_STATUS  0x12
#define VIRT_REG_ISR         0x13
#define VIRT_REG_MAC         0x14  /* 6 bytes of MAC address */

/* ---- Virtqueue structures ------------------------------------------------ */

#define VQ_RX   0
#define VQ_TX   1
#define VQ_SIZE 16   /* must be power of 2 */

/* Virtqueue descriptor. */
typedef struct __attribute__((packed)) {
    unsigned long long addr;    /* physical address */
    unsigned int  len;
    unsigned short flags;
    unsigned short next;
} vq_desc_t;

#define VQ_DESC_F_NEXT   1
#define VQ_DESC_F_WRITE  2

/* Available ring. */
typedef struct __attribute__((packed)) {
    unsigned short flags;
    unsigned short idx;
    unsigned short ring[VQ_SIZE];
} vq_avail_t;

/* Used ring element. */
typedef struct __attribute__((packed)) {
    unsigned int id;
    unsigned int len;
} vq_used_elem_t;

/* Used ring. */
typedef struct __attribute__((packed)) {
    unsigned short flags;
    unsigned short idx;
    vq_used_elem_t ring[VQ_SIZE];
} vq_used_t;

/* Virtio-net header prepended to every packet. */
typedef struct __attribute__((packed)) {
    unsigned char  flags;
    unsigned char  gso_type;
    unsigned short hdr_len;
    unsigned short gso_size;
    unsigned short csum_start;
    unsigned short csum_offset;
} virtio_net_hdr_t;

/* Per-queue state. */
typedef struct {
    vq_desc_t  desc[VQ_SIZE];
    vq_avail_t avail;
    unsigned char _pad1[4096 - sizeof(vq_desc_t)*VQ_SIZE - sizeof(vq_avail_t)];
    vq_used_t  used;
} __attribute__((aligned(4096))) virtq_t;

/* RX and TX queues allocated statically (4 KB aligned). */
static virtq_t vq_rx __attribute__((aligned(4096)));
static virtq_t vq_tx __attribute__((aligned(4096)));

/* RX buffers (one per descriptor slot). */
#define RX_BUF_SIZE  1536
static unsigned char rx_bufs[VQ_SIZE][RX_BUF_SIZE];

static unsigned short io_base = 0;
static unsigned short vq_tx_last_used = 0;
static unsigned short vq_rx_last_used = 0;

/* ---- net_device_t for eth0 ----------------------------------------------- */

static int virtio_net_transmit(net_device_t* dev, const void* data, unsigned int len);
static net_device_t eth0 = {
    .name     = "eth0",
    .transmit = virtio_net_transmit,
};

/* ---- Helper: write a VQ page-frame number to the device ------------------ */
static void vq_setup(unsigned short vq_idx, virtq_t* vq) {
    outw(io_base + VIRT_REG_Q_SELECT, vq_idx);
    unsigned int pfn = (unsigned int)(unsigned long)vq / 4096;
    outl(io_base + VIRT_REG_Q_ADDR, pfn);
}

/* ---- Populate RX ring with empty buffers --------------------------------- */
static void vq_rx_refill(void) {
    unsigned short idx = vq_rx.avail.idx;
    for (int i = 0; i < VQ_SIZE; i++) {
        int d = i;
        /* Descriptor: writable buffer for the device to DMA into. */
        vq_rx.desc[d].addr  = (unsigned long long)(unsigned long)rx_bufs[i];
        vq_rx.desc[d].len   = RX_BUF_SIZE;
        vq_rx.desc[d].flags = VQ_DESC_F_WRITE;
        vq_rx.desc[d].next  = 0;
        vq_rx.avail.ring[idx % VQ_SIZE] = (unsigned short)d;
        idx++;
    }
    __sync_synchronize();
    vq_rx.avail.idx = idx;
    __sync_synchronize();
    outw(io_base + VIRT_REG_Q_NOTIFY, VQ_RX);
}

/* ---- Transmit ------------------------------------------------------------- */
static int virtio_net_transmit(net_device_t* dev, const void* data, unsigned int len) {
    (void)dev;

    /* Use descriptor 0 for the virtio-net header and descriptor 1 for data.
     * This is a simple non-ring approach — we reuse the same two descriptors
     * each time and wait for the device to consume them. */

    static virtio_net_hdr_t net_hdr;
    memset(&net_hdr, 0, sizeof(net_hdr));

    /* Descriptor 0: virtio-net header (read-only for device). */
    vq_tx.desc[0].addr  = (unsigned long long)(unsigned long)&net_hdr;
    vq_tx.desc[0].len   = sizeof(net_hdr);
    vq_tx.desc[0].flags = VQ_DESC_F_NEXT;
    vq_tx.desc[0].next  = 1;

    /* Descriptor 1: packet data (read-only for device). */
    vq_tx.desc[1].addr  = (unsigned long long)(unsigned long)data;
    vq_tx.desc[1].len   = len;
    vq_tx.desc[1].flags = 0;
    vq_tx.desc[1].next  = 0;

    /* Add to available ring. */
    unsigned short avail_idx = vq_tx.avail.idx;
    vq_tx.avail.ring[avail_idx % VQ_SIZE] = 0; /* head descriptor = 0 */
    __sync_synchronize();
    vq_tx.avail.idx = avail_idx + 1;
    __sync_synchronize();

    /* Kick the device. */
    outw(io_base + VIRT_REG_Q_NOTIFY, VQ_TX);

    /* Busy-wait for the device to process (simple polling). */
    unsigned int timeout = 1000000;
    while (vq_tx.used.idx == vq_tx_last_used && timeout--) {
        __asm__ volatile("pause");
    }
    vq_tx_last_used = vq_tx.used.idx;
    return 0;
}

/* ---- Poll for received packets (called from the IRQ handler or main loop) */
void virtio_net_poll(void) {
    while (vq_rx.used.idx != vq_rx_last_used) {
        unsigned short used_idx = vq_rx_last_used % VQ_SIZE;
        unsigned int   buf_id   = vq_rx.used.ring[used_idx].id;
        unsigned int   buf_len  = vq_rx.used.ring[used_idx].len;

        vq_rx_last_used++;

        /* Skip the virtio-net header (10 bytes). */
        if (buf_len > sizeof(virtio_net_hdr_t)) {
            unsigned char* pkt = rx_bufs[buf_id] + sizeof(virtio_net_hdr_t);
            unsigned int   pkt_len = buf_len - sizeof(virtio_net_hdr_t);
            net_receive(&eth0, pkt, pkt_len);
        }

        /* Re-offer the buffer. */
        unsigned short avail_idx = vq_rx.avail.idx;
        vq_rx.avail.ring[avail_idx % VQ_SIZE] = (unsigned short)buf_id;
        __sync_synchronize();
        vq_rx.avail.idx = avail_idx + 1;
        __sync_synchronize();
        outw(io_base + VIRT_REG_Q_NOTIFY, VQ_RX);
    }
}

/* ---- Init ----------------------------------------------------------------- */
int virtio_net_init(void) {
    unsigned int bus, dev, fn;
    if (!pci_find_device(0x1AF4, 0x1000, &bus, &dev, &fn)) {
        return -1;  /* no virtio-net device found */
    }

    /* Read BAR0 (I/O space). */
    unsigned int bar0 = pci_cfg_read32(bus, dev, fn, 0x10);
    if (!(bar0 & 1)) return -1;  /* BAR0 must be I/O type */
    io_base = (unsigned short)(bar0 & ~3u);

    /* Virtio initialisation sequence (legacy). */
    outb(io_base + VIRT_REG_DEV_STATUS, VIRTIO_STATUS_RESET);
    outb(io_base + VIRT_REG_DEV_STATUS, VIRTIO_STATUS_ACK);
    outb(io_base + VIRT_REG_DEV_STATUS, VIRTIO_STATUS_ACK | VIRTIO_STATUS_DRIVER);

    /* Negotiate features: request MAC feature only. */
    unsigned int dev_feat  = inl(io_base + VIRT_REG_DEV_FEAT);
    unsigned int our_feat  = dev_feat & VIRTIO_NET_F_MAC;
    outl(io_base + VIRT_REG_GST_FEAT, our_feat);

    /* Read MAC address if the feature was granted. */
    if (our_feat & VIRTIO_NET_F_MAC) {
        for (int i = 0; i < MAC_LEN; i++)
            eth0.mac.b[i] = inb(io_base + VIRT_REG_MAC + i);
    }

    /* Set up virtqueues. */
    memset(&vq_rx, 0, sizeof(vq_rx));
    memset(&vq_tx, 0, sizeof(vq_tx));
    vq_setup(VQ_RX, &vq_rx);
    vq_setup(VQ_TX, &vq_tx);

    /* Fill RX ring with buffers. */
    vq_rx_refill();

    /* Configure eth0 with a default IP (QEMU default: 10.0.2.15).
     * In a real system this would come from DHCP. */
    eth0.ip      = (10u << 24) | (0u << 16) | (2u << 8) | 15u;
    eth0.netmask = (255u << 24) | (255u << 16) | (255u << 8) | 0u;
    eth0.gateway = (10u << 24) | (0u << 16) | (2u << 8) | 2u;

    /* Mark driver as ready. */
    outb(io_base + VIRT_REG_DEV_STATUS,
         VIRTIO_STATUS_ACK | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_DRIVER_OK);

    /* Register with the network stack. */
    net_register_device(&eth0);

    return 0;
}
