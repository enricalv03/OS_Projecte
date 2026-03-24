#ifndef VIRTIO_NET_H
#define VIRTIO_NET_H

/* =============================================================================
 * kernel/net/virtio_net.h — virtio-net PCI NIC driver
 * =============================================================================
 * Supports QEMU's virtio-net device (PCI vendor 0x1AF4, device 0x1000).
 * Uses the legacy virtio interface (virtio spec 0.9) with two virtqueues:
 *   VQ 0 = receive queue (RX)
 *   VQ 1 = transmit queue (TX)
 * =========================================================================== */

/* Initialise the virtio-net driver.
 * Scans the PCI bus for a virtio-net device, sets up the virtqueues, and
 * registers the device with net_register_device() as "eth0".
 * Returns 0 on success, -1 if no virtio-net device found. */
int virtio_net_init(void);

#endif
