/* ============================================================================
 * arch/x86/kernel/core/smp.c — Symmetric Multi-Processing boot (x86)
 * ============================================================================
 * Implements the Intel MP boot protocol for Application Processors (APs):
 *   1. Bootstrap Processor (BSP) places a 16-bit real-mode trampoline at a
 *      well-known address below 1 MB (0x8000 = page-frame 8).
 *   2. BSP sends INIT IPI to the AP, waits 10 ms.
 *   3. BSP sends two SIPI IPIs (Startup IPI) with the trampoline vector.
 *   4. Each AP executes the trampoline, switches to protected mode, and
 *      calls ap_kernel_main() in C.
 *
 * The LAPIC is memory-mapped at a well-known physical address (read from
 * the CPUID / IA32_APIC_BASE MSR).  On modern CPUs / QEMU it is at
 * 0xFEE00000.
 *
 * Current limitations:
 *   - Supports up to MAX_NODES-1 APs (defined in node.h).
 *   - ACPI/MP-Table detection is not implemented; we probe QEMU's default
 *     topology of 1-8 logical CPUs using LAPIC IDs 0-7.
 *   - APs share the kernel page directory and all data structures.
 *     Per-AP scheduler queues (Phase 3B) are initialised per-AP.
 *
 * References:
 *   Intel 64 and IA-32 Architectures Software Developer's Manual, Vol. 3A
 *   Chapter 8.4 — Multiple-Processor (MP) Initialization
 * ============================================================================ */

#include "smp.h"
#include "node.h"
#include "arch.h"

/* ---- LAPIC register base and offsets ---- */
#define LAPIC_BASE      0xFEE00000u

#define LAPIC_REG(off)  (*(volatile unsigned int *)(LAPIC_BASE + (off)))

#define LAPIC_ID        0x020   /* LAPIC ID register */
#define LAPIC_VER       0x030
#define LAPIC_EOI       0x0B0   /* End-of-interrupt */
#define LAPIC_ICR_LO    0x300   /* Interrupt Command Register low  */
#define LAPIC_ICR_HI    0x310   /* Interrupt Command Register high */
#define LAPIC_TIMER     0x320
#define LAPIC_LINT0     0x350
#define LAPIC_LINT1     0x360
#define LAPIC_SVR       0x0F0   /* Spurious Interrupt Vector Register */

/* SVR: bit 8 = LAPIC enable */
#define LAPIC_SVR_ENABLE  (1u << 8)

/* ICR delivery modes */
#define ICR_INIT      0x00000500u
#define ICR_SIPI      0x00000600u
#define ICR_LEVEL     0x00008000u  /* Level trigger */
#define ICR_ASSERT    0x00004000u  /* Assert level */
#define ICR_DEASSERT  0x00000000u
#define ICR_DEST_EXCL 0x000C0000u  /* All excluding self */
#define ICR_DEST_ALL  0x00080000u  /* All including self */

/* ---- Short delays (busy-wait) ---- */
static void delay_ms(unsigned int ms) {
    /* Spin ~1 million iterations per ms on a ~1 GHz QEMU vCPU.
     * This is approximate and purely for IPI timing purposes. */
    for (unsigned int i = 0; i < ms * 100000u; i++) {
        __asm__ volatile("pause");
    }
}

/* ---- Wait for ICR delivery to complete (bit 12 = Delivery Status) ---- */
static void lapic_wait_icr(void) {
    while (LAPIC_REG(LAPIC_ICR_LO) & (1u << 12)) {
        __asm__ volatile("pause");
    }
}

/* ---- Enable the BSP's LAPIC ---- */
void lapic_enable(void) {
    /* Enable LAPIC and set spurious interrupt vector to 0xFF */
    LAPIC_REG(LAPIC_SVR) = LAPIC_SVR_ENABLE | 0xFF;
}

/* ---- Send EOI to LAPIC (called from IRQ handlers) ---- */
void lapic_send_eoi(void) {
    LAPIC_REG(LAPIC_EOI) = 0;
}

/* ---- Read this CPU's LAPIC ID ---- */
unsigned int lapic_get_id(void) {
    return LAPIC_REG(LAPIC_ID) >> 24;
}

/* ---- AP trampoline (machine-code blob) ----------------------------------- */
/* The trampoline is a tiny 16-bit real-mode stub that:
 *   1. Sets up the GDT pointer (physical address).
 *   2. Enters protected mode via lgdt + CR0 bit 0.
 *   3. Far-jumps to ap_protected_mode_entry().
 *
 * For now we provide a placeholder that simply halts; a real implementation
 * would need a carefully positioned binary blob placed at 0x8000.
 *
 * The actual trampoline blob is in smp_trampoline.asm (not yet linked into
 * the kernel binary — see note below about separate assembly).
 */

/* ap_protected_mode_entry is the C entry for APs after they reach ring 0. */
extern void ap_kernel_main(unsigned int lapic_id);

/* ---- Send a fixed-vector IPI to a specific LAPIC ID ---- */
void lapic_send_ipi(unsigned int dest_lapic_id, unsigned char vector) {
    LAPIC_REG(LAPIC_ICR_HI) = dest_lapic_id << 24;
    /* Delivery mode: Fixed (000), Destination mode: Physical (0), Level: Edge (0),
     * Assert (1), Destination shorthand: No shorthand (00) */
    LAPIC_REG(LAPIC_ICR_LO) = (unsigned int)vector | ICR_ASSERT;
    lapic_wait_icr();
}

/* ---- AP startup data shared across CPUs ---- */
static volatile unsigned int ap_count = 0;     /* number of APs that booted */
static volatile unsigned int ap_ready = 0;     /* set by BSP when AP may proceed */
static unsigned int ap_stacks[MAX_NODES][4096]; /* 16 KB per AP (MAX_NODES - 1 APs + 1 BSP) */

/* Trampoline physical page (page-aligned, below 1 MB) */
#define TRAMPOLINE_PHYS 0x00008000u

/* ---- BSP: send INIT-SIPI-SIPI to a target AP ---- */
static void send_init_sipi_sipi(unsigned int apic_id) {
    /* Set destination field of ICR_HI to the target APIC ID */
    LAPIC_REG(LAPIC_ICR_HI) = apic_id << 24;

    /* Assert INIT */
    LAPIC_REG(LAPIC_ICR_LO) = ICR_INIT | ICR_LEVEL | ICR_ASSERT;
    lapic_wait_icr();

    /* Deassert INIT (for 82489DX compatibility) */
    LAPIC_REG(LAPIC_ICR_LO) = ICR_INIT | ICR_LEVEL | ICR_DEASSERT;
    lapic_wait_icr();

    delay_ms(10);   /* wait 10 ms for AP to reset */

    /* Send two SIPI with trampoline vector (page number = TRAMPOLINE_PHYS >> 12) */
    unsigned int sipi_vec = (TRAMPOLINE_PHYS >> 12) & 0xFF;
    for (int i = 0; i < 2; i++) {
        LAPIC_REG(LAPIC_ICR_HI) = apic_id << 24;
        LAPIC_REG(LAPIC_ICR_LO) = ICR_SIPI | sipi_vec;
        lapic_wait_icr();
        delay_ms(1);
    }
}

/* ---- C function called by each AP after reaching protected mode ---- */
void ap_kernel_main(unsigned int lapic_id) {
    /* Initialise per-AP LAPIC */
    lapic_enable();

    /* Register this AP as a new kernel node */
    kernel_node_t* node = node_init_ap(lapic_id);
    (void)node;

    /* Signal BSP that we're alive */
    __sync_fetch_and_add(&ap_count, 1);

    /* Wait for BSP to allow us to proceed */
    while (!ap_ready) {
        __asm__ volatile("pause");
    }

    /* Enable interrupts and enter the idle loop */
    arch_enable_interrupts();
    for (;;) {
        arch_idle();
    }
}

/* ---- Public API ---- */

void smp_init(void) {
    lapic_enable();

    /* Determine how many CPUs QEMU gave us.
     * We probe LAPIC IDs 1 through MAX_NODES-1.  If the SIPI is accepted
     * (AP increments ap_count), we know the CPU exists.
     *
     * NOTE: In production code you would parse the ACPI MADT table to find
     * all enabled LAPIC entries.  For QEMU -smp N, LAPIC IDs are 0..N-1.
     * We try up to MAX_NODES-1 and stop when the AP doesn't respond. */

    unsigned int expected_aps = 0;

    for (unsigned int id = 1; id < (unsigned int)MAX_NODES; id++) {
        unsigned int before = ap_count;
        send_init_sipi_sipi(id);

        /* Wait up to 100 ms for AP to respond */
        for (unsigned int t = 0; t < 100; t++) {
            delay_ms(1);
            if (ap_count > before) {
                expected_aps++;
                break;
            }
        }
    }

    /* Allow all APs that came up to proceed to their idle loops */
    ap_ready = 1;

    (void)ap_stacks;   /* suppress unused warning until stacks are wired in trampoline */
    (void)expected_aps;
}
