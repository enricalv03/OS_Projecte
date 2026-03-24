#ifndef SMP_H
#define SMP_H

/* Initialise SMP: enable BSP LAPIC, send INIT-SIPI-SIPI to all APs. */
void smp_init(void);

/* Enable the current CPU's LAPIC. */
void lapic_enable(void);

/* Signal end-of-interrupt to the current CPU's LAPIC. */
void lapic_send_eoi(void);

/* Return the current CPU's LAPIC ID (used to index the node array). */
unsigned int lapic_get_id(void);

/* Send a fixed-vector IPI to the CPU identified by lapic_id.
 * vector: IDT vector (e.g. 0xF0 for scheduler wake-up).
 * Delivery mode: Fixed (000). */
void lapic_send_ipi(unsigned int dest_lapic_id, unsigned char vector);

/* x86 IPI vector assigned to scheduler "process migrated" notifications. */
#define IPI_VECTOR_SCHED_WAKE  0xF0

#endif
