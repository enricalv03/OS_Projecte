/* =============================================================================
 * kernel/net/icmp.c — ICMP (Internet Control Message Protocol)
 * =========================================================================== */

#include "icmp.h"
#include "ip.h"
#include "lib/kstring.h"

/* Scratch buffer for assembling ICMP replies/requests. */
#define ICMP_TX_BUF  512
static unsigned char icmp_tx_buf[ICMP_TX_BUF];

void icmp_receive(net_device_t* dev, ip4_addr_t src_ip, const void* pkt, unsigned int len) {
    (void)dev;
    if (len < ICMP_HDR_LEN) return;
    const icmp_hdr_t* hdr = (const icmp_hdr_t*)pkt;

    if (hdr->type == ICMP_TYPE_ECHO_REQUEST) {
        /* Send an Echo Reply back to the sender. */
        unsigned int reply_len = len;
        if (reply_len > ICMP_TX_BUF) reply_len = ICMP_TX_BUF;

        const unsigned char* src = (const unsigned char*)pkt;
        unsigned char* dst = icmp_tx_buf;
        for (unsigned int i = 0; i < reply_len; i++) dst[i] = src[i];

        icmp_hdr_t* reply = (icmp_hdr_t*)icmp_tx_buf;
        reply->type     = ICMP_TYPE_ECHO_REPLY;
        reply->code     = 0;
        reply->checksum = 0;
        reply->checksum = net_checksum(icmp_tx_buf, reply_len);

        ip_send(src_ip, IP_PROTO_ICMP, icmp_tx_buf, reply_len);
    }
}

int icmp_ping(ip4_addr_t dst_ip, u16 id, u16 seq,
              const void* data, unsigned int data_len) {
    unsigned int total = ICMP_HDR_LEN + data_len;
    if (total > ICMP_TX_BUF) return -1;

    icmp_hdr_t* hdr = (icmp_hdr_t*)icmp_tx_buf;
    hdr->type     = ICMP_TYPE_ECHO_REQUEST;
    hdr->code     = 0;
    hdr->checksum = 0;
    hdr->id       = htons(id);
    hdr->seq      = htons(seq);

    unsigned char* payload = icmp_tx_buf + ICMP_HDR_LEN;
    const unsigned char* src = (const unsigned char*)data;
    for (unsigned int i = 0; i < data_len; i++) payload[i] = src[i];

    hdr->checksum = net_checksum(icmp_tx_buf, total);
    return ip_send(dst_ip, IP_PROTO_ICMP, icmp_tx_buf, total);
}
