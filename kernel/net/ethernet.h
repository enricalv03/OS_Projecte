#ifndef ETHERNET_H
#define ETHERNET_H

#include "net.h"

/* Ethernet frame header (14 bytes). */
#define ETH_HDR_LEN  14
#define ETH_TYPE_IP  0x0800
#define ETH_TYPE_ARP 0x0806

typedef struct __attribute__((packed)) {
    mac_addr_t dst;
    mac_addr_t src;
    u16        ethertype;   /* network byte order */
} eth_hdr_t;

/* Send a raw Ethernet frame on device `dev`.
 * `payload` must be the L3 data; the function prepends the Ethernet header. */
int eth_send(net_device_t* dev, const mac_addr_t* dst_mac,
             u16 ethertype, const void* payload, unsigned int payload_len);

/* Called by net_receive() — strips the Ethernet header and dispatches. */
void eth_receive(net_device_t* dev, const void* frame, unsigned int len);

#endif
