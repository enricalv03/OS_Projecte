#ifndef TCP_H
#define TCP_H

#include "net.h"

/* TCP header (20 bytes, no options). */
typedef struct __attribute__((packed)) {
    u16 src_port;
    u16 dst_port;
    u32 seq_num;
    u32 ack_num;
    u8  data_offset;  /* upper nibble = header length in 32-bit words */
    u8  flags;
    u16 window;
    u16 checksum;
    u16 urgent;
} tcp_hdr_t;

#define TCP_HDR_LEN  20

/* TCP flag bits */
#define TCP_FIN  0x01
#define TCP_SYN  0x02
#define TCP_RST  0x04
#define TCP_PSH  0x08
#define TCP_ACK  0x10
#define TCP_URG  0x20

/* TCP connection states */
typedef enum {
    TCP_CLOSED      = 0,
    TCP_LISTEN,
    TCP_SYN_SENT,
    TCP_SYN_RECEIVED,
    TCP_ESTABLISHED,
    TCP_FIN_WAIT_1,
    TCP_FIN_WAIT_2,
    TCP_CLOSE_WAIT,
    TCP_CLOSING,
    TCP_LAST_ACK,
    TCP_TIME_WAIT,
} tcp_state_t;

/* Maximum simultaneous TCP connections. */
#define TCP_MAX_CONNECTIONS  8

typedef struct tcp_connection {
    tcp_state_t state;
    ip4_addr_t  local_ip;
    ip4_addr_t  remote_ip;
    u16         local_port;
    u16         remote_port;
    u32         send_seq;      /* next sequence number to send */
    u32         recv_ack;      /* next expected sequence number from peer */
    u32         send_window;
    /* Receive buffer */
    unsigned char recv_buf[4096];
    unsigned int  recv_head;
    unsigned int  recv_tail;
} tcp_conn_t;

/* Connect to a remote host (three-way handshake).
 * Returns a connection index ≥ 0 on success, -1 on failure. */
int tcp_connect(ip4_addr_t dst_ip, u16 dst_port);

/* Send data on an established connection. */
int tcp_send(int conn_idx, const void* data, unsigned int len);

/* Close a connection gracefully (FIN). */
int tcp_close(int conn_idx);

/* Called by ip_receive() for TCP segments. */
void tcp_receive(net_device_t* dev, ip4_addr_t src_ip, ip4_addr_t dst_ip,
                 const void* tcp_pkt, unsigned int len);

/* Read buffered received data (non-blocking). */
int tcp_read(int conn_idx, void* buf, unsigned int max);

#endif
