/* =============================================================================
 * kernel/net/net.c — core networking layer
 * =========================================================================== */

#include "net.h"
#include "ethernet.h"
#include "lib/kstring.h"

/* ---- Device registry ----------------------------------------------------- */

static net_device_t* dev_list = 0;

void net_register_device(net_device_t* dev) {
    dev->next = dev_list;
    dev_list  = dev;
}

net_device_t* net_get_device(const char* name) {
    for (net_device_t* d = dev_list; d; d = d->next) {
        int eq = 1;
        for (int i = 0; i < 8; i++) {
            if (d->name[i] != name[i]) { eq = 0; break; }
            if (d->name[i] == 0) break;
        }
        if (eq) return d;
    }
    return 0;
}

/* Return the first registered device (used as default by ip_send). */
net_device_t* net_get_default_device(void) {
    return dev_list;
}

/* Called by a NIC driver when a frame arrives. */
void net_receive(net_device_t* dev, const void* data, unsigned int len) {
    eth_receive(dev, data, len);
}

/* ---- One's-complement checksum (used by IP, ICMP, TCP, UDP) -------------- */
u16 net_checksum(const void* data, unsigned int len) {
    const u16* ptr = (const u16*)data;
    u32 sum = 0;

    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }
    if (len == 1) {
        sum += *(const u8*)ptr;
    }

    /* Fold 32-bit sum to 16 bits */
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return (u16)~sum;
}
