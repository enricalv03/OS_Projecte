/* =============================================================================
 * kernel/net/ethernet.c — Ethernet layer
 * =========================================================================== */

#include "ethernet.h"
#include "arp.h"
#include "ip.h"
#include "lib/kstring.h"

/* Scratch transmit buffer for assembling Ethernet frames.
 * Must be large enough for MTU (1514 bytes + some headroom). */
#define ETH_TX_BUF_SIZE 1536
static unsigned char eth_tx_buf[ETH_TX_BUF_SIZE];

int eth_send(net_device_t* dev, const mac_addr_t* dst_mac,
             u16 ethertype, const void* payload, unsigned int payload_len) {
    if (!dev || !dev->transmit) return -1;
    if (ETH_HDR_LEN + payload_len > ETH_TX_BUF_SIZE) return -1;

    eth_hdr_t* hdr = (eth_hdr_t*)eth_tx_buf;
    for (int i = 0; i < MAC_LEN; i++) {
        hdr->dst.b[i] = dst_mac->b[i];
        hdr->src.b[i] = dev->mac.b[i];
    }
    hdr->ethertype = htons(ethertype);

    /* Copy payload after header */
    unsigned char* dst = eth_tx_buf + ETH_HDR_LEN;
    const unsigned char* src = (const unsigned char*)payload;
    for (unsigned int i = 0; i < payload_len; i++) dst[i] = src[i];

    return dev->transmit(dev, eth_tx_buf, ETH_HDR_LEN + payload_len);
}

void eth_receive(net_device_t* dev, const void* frame, unsigned int len) {
    if (len < ETH_HDR_LEN) return;
    const eth_hdr_t* hdr = (const eth_hdr_t*)frame;
    const unsigned char* payload = (const unsigned char*)frame + ETH_HDR_LEN;
    unsigned int payload_len = len - ETH_HDR_LEN;

    u16 etype = ntohs(hdr->ethertype);
    switch (etype) {
        case ETH_TYPE_ARP:
            arp_receive(dev, payload, payload_len);
            break;
        case ETH_TYPE_IP:
            ip_receive(dev, payload, payload_len);
            break;
        default:
            break;
    }
}
