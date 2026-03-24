#ifndef NET_H
#define NET_H

/* =============================================================================
 * kernel/net/net.h — core networking types and byte-order helpers
 * =============================================================================
 * All multi-byte fields in network headers are in big-endian (network) byte
 * order.  The kernel runs on little-endian x86 and ARM, so we need explicit
 * byte-swap helpers.  We avoid __builtin_bswap* to keep gcc version portable.
 * =========================================================================== */

/* ---- Fundamental types ---------------------------------------------------- */
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

/* ---- Byte-order macros ---------------------------------------------------- */

static inline u16 htons(u16 x) {
    return (u16)((x >> 8) | (x << 8));
}
static inline u16 ntohs(u16 x) { return htons(x); }

static inline u32 htonl(u32 x) {
    return ((x & 0xFF000000u) >> 24) |
           ((x & 0x00FF0000u) >>  8) |
           ((x & 0x0000FF00u) <<  8) |
           ((x & 0x000000FFu) << 24);
}
static inline u32 ntohl(u32 x) { return htonl(x); }

/* ---- MAC address ---------------------------------------------------------- */
#define MAC_LEN 6
typedef struct { u8 b[MAC_LEN]; } mac_addr_t;

/* ---- IPv4 address --------------------------------------------------------- */
typedef u32 ip4_addr_t;   /* stored in host byte order internally */

/* ---- Network device interface --------------------------------------------- */

/* A minimal NIC abstraction.  Each driver fills in this struct and registers
 * it with net_register_device(). */
typedef struct net_device {
    char          name[8];      /* e.g. "eth0" */
    mac_addr_t    mac;
    ip4_addr_t    ip;           /* host-byte-order IP address */
    ip4_addr_t    netmask;
    ip4_addr_t    gateway;

    /* Transmit one Ethernet frame.  Returns 0 on success, -1 on error. */
    int (*transmit)(struct net_device* dev, const void* data, unsigned int len);

    struct net_device* next;    /* linked list of all devices */
} net_device_t;

/* Register / unregister a NIC. */
void net_register_device(net_device_t* dev);
net_device_t* net_get_device(const char* name);

/* Called by the NIC driver when a frame is received.
 * Dispatches to the appropriate Layer-3 handler. */
void net_receive(net_device_t* dev, const void* data, unsigned int len);

/* ---- Checksum ------------------------------------------------------------ */
u16 net_checksum(const void* data, unsigned int len);

#endif
