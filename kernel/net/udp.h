#ifndef UDP_H
#define UDP_H

#include "net.h"

typedef struct __attribute__((packed)) {
    u16 src_port;   /* network byte order */
    u16 dst_port;   /* network byte order */
    u16 length;     /* network byte order — header + data */
    u16 checksum;   /* network byte order — 0 = disabled */
} udp_hdr_t;

#define UDP_HDR_LEN  8

/* Send a UDP datagram.
 * src_port and dst_port in HOST byte order. */
int udp_send(ip4_addr_t dst_ip, u16 src_port, u16 dst_port,
             const void* data, unsigned int len);

/* Called by ip_receive() for UDP packets. */
void udp_receive(net_device_t* dev, ip4_addr_t src_ip, ip4_addr_t dst_ip,
                 const void* udp_pkt, unsigned int len);

/* Register a callback for incoming UDP on a given port.
 * Replaces any existing callback for that port. */
typedef void (*udp_recv_cb_t)(ip4_addr_t src_ip, u16 src_port,
                              const void* data, unsigned int len);
void udp_register_port(u16 port, udp_recv_cb_t cb);

#endif
