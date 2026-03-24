#ifndef ARP_H
#define ARP_H

#include "net.h"

#define ARP_TABLE_SIZE 16

/* Lookup the MAC address for `ip` on `dev`.
 * Returns a pointer to the cached mac_addr_t, or NULL if not in table.
 * Sends an ARP Request if not cached (caller should retry after a delay). */
const mac_addr_t* arp_lookup(net_device_t* dev, ip4_addr_t ip);

/* Called by eth_receive() when an ARP packet arrives. */
void arp_receive(net_device_t* dev, const void* arp_pkt, unsigned int len);

/* Send an ARP request for `target_ip` on `dev`. */
void arp_request(net_device_t* dev, ip4_addr_t target_ip);

/* Seed the ARP table with a static entry (e.g. gateway). */
void arp_set_static(ip4_addr_t ip, const mac_addr_t* mac);

#endif
