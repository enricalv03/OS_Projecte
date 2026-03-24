/* =============================================================================
 * kernel/net/udp.c — UDP (User Datagram Protocol)
 * =========================================================================== */

#include "udp.h"
#include "ip.h"
#include "lib/kstring.h"

/* UDP port handler table. */
#define UDP_MAX_PORTS  8
typedef struct {
    u16          port;
    udp_recv_cb_t cb;
} udp_binding_t;

static udp_binding_t udp_ports[UDP_MAX_PORTS];

void udp_register_port(u16 port, udp_recv_cb_t cb) {
    /* Update existing entry or add new one. */
    for (int i = 0; i < UDP_MAX_PORTS; i++) {
        if (udp_ports[i].port == port || udp_ports[i].cb == 0) {
            udp_ports[i].port = port;
            udp_ports[i].cb   = cb;
            return;
        }
    }
}

/* Scratch TX buffer: UDP header + payload. */
#define UDP_TX_BUF  1472
static unsigned char udp_tx_buf[UDP_TX_BUF];

int udp_send(ip4_addr_t dst_ip, u16 src_port, u16 dst_port,
             const void* data, unsigned int len) {
    unsigned int total = UDP_HDR_LEN + len;
    if (total > UDP_TX_BUF) return -1;

    udp_hdr_t* hdr = (udp_hdr_t*)udp_tx_buf;
    hdr->src_port = htons(src_port);
    hdr->dst_port = htons(dst_port);
    hdr->length   = htons((u16)total);
    hdr->checksum = 0;  /* optional for IPv4 */

    unsigned char* dst = udp_tx_buf + UDP_HDR_LEN;
    const unsigned char* src = (const unsigned char*)data;
    for (unsigned int i = 0; i < len; i++) dst[i] = src[i];

    return ip_send(dst_ip, IP_PROTO_UDP, udp_tx_buf, total);
}

void udp_receive(net_device_t* dev, ip4_addr_t src_ip, ip4_addr_t dst_ip,
                 const void* pkt, unsigned int len) {
    (void)dev; (void)dst_ip;
    if (len < UDP_HDR_LEN) return;
    const udp_hdr_t* hdr = (const udp_hdr_t*)pkt;
    u16 dst_port   = ntohs(hdr->dst_port);
    u16 src_port_n = ntohs(hdr->src_port);
    const void* payload = (const unsigned char*)pkt + UDP_HDR_LEN;
    unsigned int payload_len = len - UDP_HDR_LEN;

    for (int i = 0; i < UDP_MAX_PORTS; i++) {
        if (udp_ports[i].cb && udp_ports[i].port == dst_port) {
            udp_ports[i].cb(src_ip, src_port_n, payload, payload_len);
            return;
        }
    }
}
