#ifndef IP_H
#define IP_H

#include "net.h"

#define IP_PROTO_ICMP  1
#define IP_PROTO_UDP   17
#define IP_PROTO_TCP   6

/* IPv4 header (20 bytes, no options). */
typedef struct __attribute__((packed)) {
    u8  version_ihl;    /* version=4 in upper nibble, IHL=5 in lower */
    u8  tos;
    u16 total_length;   /* network byte order */
    u16 id;             /* network byte order */
    u16 flags_frag;     /* network byte order */
    u8  ttl;
    u8  protocol;
    u16 checksum;       /* network byte order */
    u32 src_ip;         /* network byte order */
    u32 dst_ip;         /* network byte order */
} ip_hdr_t;

#define IP_HDR_LEN 20

/* Send an IP packet on the best available device. */
int ip_send(ip4_addr_t dst_ip, u8 protocol, const void* payload, unsigned int len);

/* Called by eth_receive() for IP packets. */
void ip_receive(net_device_t* dev, const void* ip_pkt, unsigned int len);

#endif
