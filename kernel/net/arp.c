/* =============================================================================
 * kernel/net/arp.c — ARP (Address Resolution Protocol) for IPv4 over Ethernet
 * =========================================================================== */

#include "arp.h"
#include "ethernet.h"
#include "lib/kstring.h"

/* ARP packet format for Ethernet/IPv4 (28 bytes). */
typedef struct __attribute__((packed)) {
    u16 htype;      /* hardware type: 1 = Ethernet */
    u16 ptype;      /* protocol type: 0x0800 = IPv4 */
    u8  hlen;       /* hardware address length: 6 */
    u8  plen;       /* protocol address length: 4 */
    u16 oper;       /* operation: 1 = request, 2 = reply */
    mac_addr_t sha; /* sender hardware address */
    u32        spa; /* sender protocol address (network byte order) */
    mac_addr_t tha; /* target hardware address */
    u32        tpa; /* target protocol address (network byte order) */
} arp_pkt_t;

#define ARP_REQUEST  1
#define ARP_REPLY    2
#define ARP_PKT_LEN  28

/* ---- ARP cache ----------------------------------------------------------- */

typedef struct {
    ip4_addr_t  ip;
    mac_addr_t  mac;
    unsigned int valid;
} arp_entry_t;

static arp_entry_t arp_table[ARP_TABLE_SIZE];

const mac_addr_t* arp_lookup(net_device_t* dev, ip4_addr_t ip) {
    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
        if (arp_table[i].valid && arp_table[i].ip == ip)
            return &arp_table[i].mac;
    }
    /* Not in cache: send a request and return NULL. */
    arp_request(dev, ip);
    return 0;
}

void arp_set_static(ip4_addr_t ip, const mac_addr_t* mac) {
    /* Overwrite the oldest (lowest index, wrapping) entry. */
    static unsigned int next_slot = 0;
    unsigned int slot = next_slot % ARP_TABLE_SIZE;
    arp_table[slot].ip    = ip;
    arp_table[slot].mac   = *mac;
    arp_table[slot].valid = 1;
    next_slot++;
}

static void arp_cache_update(ip4_addr_t ip, const mac_addr_t* mac) {
    /* Update if already present. */
    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
        if (arp_table[i].valid && arp_table[i].ip == ip) {
            arp_table[i].mac = *mac;
            return;
        }
    }
    arp_set_static(ip, mac);
}

/* ---- ARP Request --------------------------------------------------------- */

void arp_request(net_device_t* dev, ip4_addr_t target_ip) {
    arp_pkt_t pkt;
    memset(&pkt, 0, sizeof(pkt));

    pkt.htype = htons(1);
    pkt.ptype = htons(0x0800);
    pkt.hlen  = 6;
    pkt.plen  = 4;
    pkt.oper  = htons(ARP_REQUEST);
    pkt.sha   = dev->mac;
    pkt.spa   = htonl(dev->ip);
    /* tha = 0:0:0:0:0:0 (unknown) */
    pkt.tpa   = htonl(target_ip);

    /* Broadcast destination */
    mac_addr_t bcast = {{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }};
    eth_send(dev, &bcast, ETH_TYPE_ARP, &pkt, ARP_PKT_LEN);
}

/* ---- ARP Receive --------------------------------------------------------- */

void arp_receive(net_device_t* dev, const void* data, unsigned int len) {
    if (len < ARP_PKT_LEN) return;
    const arp_pkt_t* pkt = (const arp_pkt_t*)data;

    if (ntohs(pkt->htype) != 1)       return;  /* not Ethernet */
    if (ntohs(pkt->ptype) != 0x0800)  return;  /* not IPv4 */

    ip4_addr_t sender_ip  = ntohl(pkt->spa);
    ip4_addr_t target_ip  = ntohl(pkt->tpa);

    /* Always update cache with sender's mapping. */
    arp_cache_update(sender_ip, &pkt->sha);

    /* Reply if this request is for our IP. */
    if (ntohs(pkt->oper) == ARP_REQUEST && target_ip == dev->ip) {
        arp_pkt_t reply;
        reply.htype = htons(1);
        reply.ptype = htons(0x0800);
        reply.hlen  = 6;
        reply.plen  = 4;
        reply.oper  = htons(ARP_REPLY);
        reply.sha   = dev->mac;
        reply.spa   = htonl(dev->ip);
        reply.tha   = pkt->sha;
        reply.tpa   = pkt->spa;
        eth_send(dev, &pkt->sha, ETH_TYPE_ARP, &reply, ARP_PKT_LEN);
    }
}
