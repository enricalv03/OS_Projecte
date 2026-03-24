#include "elf.h"
#include "vfs.h"
#include "sched/process.h"
#include "memory/vmm.h"
#include "memory/heap.h"

/* =============================================================================
 * elf_load — load an ELF32 executable into a process address space
 * =============================================================================
 * Steps:
 *   1. Read and validate the ELF header.
 *   2. For each PT_LOAD segment:
 *      a. Map enough pages to cover [p_vaddr, p_vaddr + p_memsz).
 *      b. Read p_filesz bytes from the file at p_offset into those pages.
 *      c. Zero-fill the remaining (p_memsz - p_filesz) bytes (BSS).
 *   3. Return the entry-point virtual address.
 *
 * The target process's page directory is installed temporarily so that
 * vmm_alloc_and_map writes into the correct address space.
 * =========================================================================== */

/* Align `val` up to the nearest `align` (which must be a power of two). */
static unsigned int align_up(unsigned int val, unsigned int al) {
    return (val + al - 1) & ~(al - 1);
}

/* Zero a range of bytes inside the currently-mapped address space. */
static void memzero(void* dst, unsigned int n) {
    unsigned char* p = (unsigned char*)dst;
    for (unsigned int i = 0; i < n; i++) p[i] = 0;
}

int elf_load(vfs_node_t* node, pcb_t* pcb, unsigned int* entry_out) {
    if (!node || !pcb || !entry_out) return -1;

    /* ---- Step 1: Read the ELF header ---- */
    elf32_ehdr_t ehdr;
    if (vfs_read(node, 0, sizeof(ehdr), &ehdr) != (int)sizeof(ehdr)) return -3;

    /* Validate magic, class, and data encoding */
    unsigned int magic = *(unsigned int*)ehdr.e_ident;
    if (magic != ELF_MAGIC)           return -1;
    if (ehdr.e_ident[4] != ELFCLASS32) return -1;
    if (ehdr.e_ident[5] != ELFDATA2LSB) return -1;
    if (ehdr.e_type != ET_EXEC)       return -2;
    if (ehdr.e_phentsize < sizeof(elf32_phdr_t)) return -1;
    if (ehdr.e_phnum == 0)            return -1;

    /* ---- Step 2: Install target page directory ---- */
    unsigned int saved_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(saved_cr3));
    __asm__ volatile("mov %0, %%cr3" :: "r"(pcb->page_directory) : "memory");

    /* ---- Step 3: Process each program header ---- */
    for (unsigned int i = 0; i < ehdr.e_phnum; i++) {
        elf32_phdr_t phdr;
        unsigned int phdr_off = ehdr.e_phoff + i * ehdr.e_phentsize;

        if (vfs_read(node, phdr_off, sizeof(phdr), &phdr) != (int)sizeof(phdr)) {
            __asm__ volatile("mov %0, %%cr3" :: "r"(saved_cr3) : "memory");
            return -3;
        }

        if (phdr.p_type != PT_LOAD) continue;
        if (phdr.p_memsz == 0)      continue;

        /* Determine VMM flags from segment permissions */
        unsigned int flags = VMM_USER_RW;   /* always user-accessible */
        (void)flags;                        /* writable by default for simplicity */

        /* Map pages covering [p_vaddr, p_vaddr + p_memsz) */
        unsigned int vaddr_start = phdr.p_vaddr & ~0xFFFu;   /* page-align down */
        unsigned int vaddr_end   = align_up(phdr.p_vaddr + phdr.p_memsz, 4096);

        for (unsigned int va = vaddr_start; va < vaddr_end; va += 4096) {
            if (vmm_alloc_and_map(va, VMM_USER_RW) == 0) {
                __asm__ volatile("mov %0, %%cr3" :: "r"(saved_cr3) : "memory");
                return -4;
            }
        }

        /* Copy file content into memory */
        if (phdr.p_filesz > 0) {
            /* Read in 512-byte chunks to avoid large stack buffers */
            static unsigned char chunk[512];
            unsigned int remaining = phdr.p_filesz;
            unsigned int file_off  = phdr.p_offset;
            unsigned int mem_dst   = phdr.p_vaddr;

            while (remaining > 0) {
                unsigned int to_read = (remaining < 512) ? remaining : 512;
                int got = vfs_read(node, file_off, to_read, chunk);
                if (got <= 0) {
                    __asm__ volatile("mov %0, %%cr3" :: "r"(saved_cr3) : "memory");
                    return -3;
                }
                unsigned char* dst = (unsigned char*)mem_dst;
                for (int j = 0; j < got; j++) dst[j] = chunk[j];
                file_off  += (unsigned int)got;
                mem_dst   += (unsigned int)got;
                remaining -= (unsigned int)got;
            }
        }

        /* Zero-fill BSS (p_memsz > p_filesz) */
        if (phdr.p_memsz > phdr.p_filesz) {
            unsigned int bss_start = phdr.p_vaddr + phdr.p_filesz;
            unsigned int bss_size  = phdr.p_memsz - phdr.p_filesz;
            memzero((void*)bss_start, bss_size);
        }
    }

    /* ---- Step 4: Restore caller's page directory ---- */
    __asm__ volatile("mov %0, %%cr3" :: "r"(saved_cr3) : "memory");

    *entry_out = ehdr.e_entry;
    return 0;
}
