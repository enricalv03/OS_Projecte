/* =============================================================================
 * kernel/net/tcp.c — minimal TCP implementation (client-only, no options)
 * =============================================================================
 * This is a simplified TCP stack sufficient for outgoing connections.
 * Features: SYN handshake, data transfer, FIN close, ACK for received data.
 * Missing: retransmission, congestion control, listen/accept, Nagle.
 * =========================================================================== */

#include "tcp.h"
#include "ip.h"
#include "lib/kstring.h"

static tcp_conn_t conn_table[TCP_MAX_CONNECTIONS];
static u16 next_ephemeral_port = 49152;

/* ---- TX scratch buffer ---------------------------------------------------- */
#define TCP_TX_BUF  1500
static unsigned char tcp_tx_buf[TCP_TX_BUF];

/* ---- Pseudo-header checksum for TCP -------------------------------------- */
typedef struct __attribute__((packed)) {
    u32 src_ip;
    u32 dst_ip;
    u8  zero;
    u8  proto;
    u16 tcp_len;
} tcp_pseudo_hdr_t;

static u16 tcp_checksum(ip4_addr_t src, ip4_addr_t dst,
                        const void* tcp_seg, unsigned int len) {
    /* Combine pseudo-header + segment for checksum calculation.
     * We use a temporary buffer on the stack for the pseudo-header. */
    tcp_pseudo_hdr_t ph;
    ph.src_ip  = htonl(src);
    ph.dst_ip  = htonl(dst);
    ph.zero    = 0;
    ph.proto   = IP_PROTO_TCP;
    ph.tcp_len = htons((u16)len);

    /* Two-pass checksum: pseudo-header then segment. */
    u32 sum = 0;
    const u16* p;
    unsigned int n;

    p = (const u16*)&ph; n = sizeof(ph);
    while (n > 1) { sum += *p++; n -= 2; }

    p = (const u16*)tcp_seg; n = len;
    while (n > 1) { sum += *p++; n -= 2; }
    if (n) sum += *(const u8*)p;

    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (u16)~sum;
}

/* ---- Send a TCP segment --------------------------------------------------- */
static int tcp_send_segment(tcp_conn_t* c, u8 flags,
                            const void* data, unsigned int data_len) {
    unsigned int seg_len = TCP_HDR_LEN + data_len;
    if (seg_len > TCP_TX_BUF) return -1;

    tcp_hdr_t* hdr = (tcp_hdr_t*)tcp_tx_buf;
    hdr->src_port   = htons(c->local_port);
    hdr->dst_port   = htons(c->remote_port);
    hdr->seq_num    = htonl(c->send_seq);
    hdr->ack_num    = (flags & TCP_ACK) ? htonl(c->recv_ack) : 0;
    hdr->data_offset= (TCP_HDR_LEN / 4) << 4;
    hdr->flags      = flags;
    hdr->window     = htons(4096);
    hdr->checksum   = 0;
    hdr->urgent     = 0;

    if (data_len > 0) {
        unsigned char* dst = tcp_tx_buf + TCP_HDR_LEN;
        const unsigned char* src = (const unsigned char*)data;
        for (unsigned int i = 0; i < data_len; i++) dst[i] = src[i];
    }

    hdr->checksum = tcp_checksum(c->local_ip, c->remote_ip,
                                 tcp_tx_buf, seg_len);

    if (flags & (TCP_SYN | TCP_FIN)) c->send_seq++;
    c->send_seq += data_len;

    return ip_send(c->remote_ip, IP_PROTO_TCP, tcp_tx_buf, seg_len);
}

/* ---- Connect -------------------------------------------------------------- */
int tcp_connect(ip4_addr_t dst_ip, u16 dst_port) {
    extern net_device_t* net_get_default_device(void);

    /* Find a free slot. */
    int idx = -1;
    for (int i = 0; i < TCP_MAX_CONNECTIONS; i++) {
        if (conn_table[i].state == TCP_CLOSED) { idx = i; break; }
    }
    if (idx < 0) return -1;

    tcp_conn_t* c = &conn_table[idx];
    memset(c, 0, sizeof(*c));

    net_device_t* dev = net_get_default_device();
    if (!dev) return -1;

    c->local_ip    = dev->ip;
    c->remote_ip   = dst_ip;
    c->local_port  = next_ephemeral_port++;
    c->remote_port = dst_port;
    c->send_seq    = 0x12345678u;  /* initial sequence number */
    c->recv_ack    = 0;
    c->state       = TCP_SYN_SENT;

    if (tcp_send_segment(c, TCP_SYN, 0, 0) != 0) {
        c->state = TCP_CLOSED;
        return -1;
    }
    return idx;
}

/* ---- Send data ------------------------------------------------------------ */
int tcp_send(int idx, const void* data, unsigned int len) {
    if (idx < 0 || idx >= TCP_MAX_CONNECTIONS) return -1;
    tcp_conn_t* c = &conn_table[idx];
    if (c->state != TCP_ESTABLISHED) return -1;
    return tcp_send_segment(c, TCP_ACK | TCP_PSH, data, len);
}

/* ---- Close ---------------------------------------------------------------- */
int tcp_close(int idx) {
    if (idx < 0 || idx >= TCP_MAX_CONNECTIONS) return -1;
    tcp_conn_t* c = &conn_table[idx];
    if (c->state != TCP_ESTABLISHED) return -1;
    c->state = TCP_FIN_WAIT_1;
    return tcp_send_segment(c, TCP_FIN | TCP_ACK, 0, 0);
}

/* ---- Read buffered data -------------------------------------------------- */
int tcp_read(int idx, void* buf, unsigned int max) {
    if (idx < 0 || idx >= TCP_MAX_CONNECTIONS) return -1;
    tcp_conn_t* c = &conn_table[idx];
    unsigned int avail = c->recv_tail - c->recv_head;
    if (avail == 0) return 0;
    if (max > avail) max = avail;
    unsigned char* dst = (unsigned char*)buf;
    for (unsigned int i = 0; i < max; i++)
        dst[i] = c->recv_buf[(c->recv_head + i) & 4095u];
    c->recv_head += max;
    return (int)max;
}

/* ---- Receive -------------------------------------------------------------- */
void tcp_receive(net_device_t* dev, ip4_addr_t src_ip, ip4_addr_t dst_ip,
                 const void* pkt, unsigned int len) {
    (void)dev; (void)dst_ip;
    if (len < TCP_HDR_LEN) return;
    const tcp_hdr_t* hdr = (const tcp_hdr_t*)pkt;

    u16 src_port = ntohs(hdr->src_port);
    u16 dst_port = ntohs(hdr->dst_port);
    u8  flags    = hdr->flags;
    u32 seq      = ntohl(hdr->seq_num);
    u32 ack      = ntohl(hdr->ack_num);

    unsigned int hdr_len = (hdr->data_offset >> 4) * 4;
    const unsigned char* data = (const unsigned char*)pkt + hdr_len;
    unsigned int data_len = (len > hdr_len) ? len - hdr_len : 0;

    /* Find matching connection. */
    tcp_conn_t* c = 0;
    for (int i = 0; i < TCP_MAX_CONNECTIONS; i++) {
        tcp_conn_t* t = &conn_table[i];
        if (t->state == TCP_CLOSED) continue;
        if (t->remote_ip == src_ip &&
            t->remote_port == src_port &&
            t->local_port  == dst_port) {
            c = t; break;
        }
    }
    if (!c) return;

    switch (c->state) {
        case TCP_SYN_SENT:
            if ((flags & (TCP_SYN | TCP_ACK)) == (TCP_SYN | TCP_ACK)) {
                c->recv_ack = seq + 1;
                c->send_seq = ack;
                c->state    = TCP_ESTABLISHED;
                /* Send ACK to complete the handshake. */
                tcp_send_segment(c, TCP_ACK, 0, 0);
            } else if (flags & TCP_RST) {
                c->state = TCP_CLOSED;
            }
            break;

        case TCP_ESTABLISHED:
            if (flags & TCP_RST) { c->state = TCP_CLOSED; break; }

            if (data_len > 0) {
                /* Buffer received data. */
                for (unsigned int i = 0; i < data_len; i++) {
                    c->recv_buf[c->recv_tail & 4095u] = data[i];
                    c->recv_tail++;
                }
                c->recv_ack = seq + data_len;
                tcp_send_segment(c, TCP_ACK, 0, 0);
            }

            if (flags & TCP_FIN) {
                c->recv_ack = seq + 1;
                c->state    = TCP_CLOSE_WAIT;
                tcp_send_segment(c, TCP_ACK, 0, 0);
                /* Send our FIN immediately (simplified). */
                c->state = TCP_LAST_ACK;
                tcp_send_segment(c, TCP_FIN | TCP_ACK, 0, 0);
            }
            break;

        case TCP_FIN_WAIT_1:
            if (flags & TCP_ACK) c->state = TCP_FIN_WAIT_2;
            break;

        case TCP_FIN_WAIT_2:
            if (flags & TCP_FIN) {
                c->recv_ack = seq + 1;
                tcp_send_segment(c, TCP_ACK, 0, 0);
                c->state = TCP_TIME_WAIT;
                /* In a real stack we'd wait 2*MSL; here we just close. */
                c->state = TCP_CLOSED;
            }
            break;

        case TCP_LAST_ACK:
            if (flags & TCP_ACK) c->state = TCP_CLOSED;
            break;

        default:
            break;
    }
}
