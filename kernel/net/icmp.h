#ifndef ICMP_H
#define ICMP_H

#include "net.h"

#define ICMP_TYPE_ECHO_REQUEST  8
#define ICMP_TYPE_ECHO_REPLY    0

typedef struct __attribute__((packed)) {
    u8  type;
    u8  code;
    u16 checksum;
    u16 id;
    u16 seq;
} icmp_hdr_t;

#define ICMP_HDR_LEN  8

/* Called by ip_receive() for ICMP packets.
 * Automatically replies to Echo Requests. */
void icmp_receive(net_device_t* dev, ip4_addr_t src_ip,
                  const void* icmp_pkt, unsigned int len);

/* Send an ICMP Echo Request to `dst_ip`. */
int icmp_ping(ip4_addr_t dst_ip, u16 id, u16 seq,
              const void* data, unsigned int data_len);

#endif
