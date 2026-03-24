/* =============================================================================
 * kernel/net/ip.c — IPv4 layer
 * =========================================================================== */

#include "ip.h"
#include "arp.h"
#include "ethernet.h"
#include "icmp.h"
#include "udp.h"
#include "tcp.h"
#include "lib/kstring.h"

extern net_device_t* net_get_default_device(void);

/* Monotonically-increasing packet identifier for fragmentation ID field. */
static u16 ip_id_counter = 0;

/* Scratch TX buffer: IP header + payload (max 1480 bytes payload). */
#define IP_TX_BUF_SIZE  1500
static unsigned char ip_tx_buf[IP_TX_BUF_SIZE];

int ip_send(ip4_addr_t dst_ip, u8 protocol,
            const void* payload, unsigned int payload_len) {
    net_device_t* dev = net_get_default_device();
    if (!dev) return -1;

    unsigned int total_len = IP_HDR_LEN + payload_len;
    if (total_len > IP_TX_BUF_SIZE) return -1;

    ip_hdr_t* hdr = (ip_hdr_t*)ip_tx_buf;
    hdr->version_ihl  = (4 << 4) | 5;   /* IPv4, IHL=5 dwords */
    hdr->tos          = 0;
    hdr->total_length = htons((u16)total_len);
    hdr->id           = htons(ip_id_counter++);
    hdr->flags_frag   = 0;
    hdr->ttl          = 64;
    hdr->protocol     = protocol;
    hdr->checksum     = 0;
    hdr->src_ip       = htonl(dev->ip);
    hdr->dst_ip       = htonl(dst_ip);

    /* Fill checksum over the IP header only. */
    hdr->checksum = net_checksum(ip_tx_buf, IP_HDR_LEN);

    /* Copy payload. */
    unsigned char* dst = ip_tx_buf + IP_HDR_LEN;
    const unsigned char* src = (const unsigned char*)payload;
    for (unsigned int i = 0; i < payload_len; i++) dst[i] = src[i];

    /* Resolve destination MAC.  Use next-hop (gateway) if not on same subnet. */
    ip4_addr_t nexthop = dst_ip;
    if ((dst_ip & dev->netmask) != (dev->ip & dev->netmask))
        nexthop = dev->gateway;

    const mac_addr_t* dst_mac = arp_lookup(dev, nexthop);
    if (!dst_mac) return -1;  /* ARP request sent; caller must retry */

    return eth_send(dev, dst_mac, ETH_TYPE_IP, ip_tx_buf, total_len);
}

void ip_receive(net_device_t* dev, const void* pkt, unsigned int len) {
    if (len < IP_HDR_LEN) return;
    const ip_hdr_t* hdr = (const ip_hdr_t*)pkt;

    unsigned int ihl = (hdr->version_ihl & 0x0F) * 4;
    if (ihl < IP_HDR_LEN || ihl > len) return;
    if ((hdr->version_ihl >> 4) != 4) return;  /* not IPv4 */

    /* Ignore fragmented packets for now. */
    if (ntohs(hdr->flags_frag) & 0x1FFF) return;

    ip4_addr_t src_ip = ntohl(hdr->src_ip);
    ip4_addr_t dst_ip = ntohl(hdr->dst_ip);

    const unsigned char* payload = (const unsigned char*)pkt + ihl;
    unsigned int payload_len = len - ihl;

    switch (hdr->protocol) {
        case IP_PROTO_ICMP:
            icmp_receive(dev, src_ip, payload, payload_len);
            break;
        case IP_PROTO_UDP:
            udp_receive(dev, src_ip, dst_ip, payload, payload_len);
            break;
        case IP_PROTO_TCP:
            tcp_receive(dev, src_ip, dst_ip, payload, payload_len);
            break;
        default:
            break;
    }
}
