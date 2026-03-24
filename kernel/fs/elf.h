#ifndef ELF_H
#define ELF_H

/* =============================================================================
 * Minimal ELF32 loader
 * =============================================================================
 * Supports statically-linked ELF32 executables (i686 / arm-none-eabi).
 * Loads PT_LOAD segments into the target process's address space and
 * returns the entry-point virtual address.
 *
 * Usage:
 *   unsigned int entry = 0;
 *   int rc = elf_load(node, pcb, &entry);
 *   if (rc == 0) { pcb->eip = entry; }
 * =========================================================================== */

struct vfs_node;
struct process_control_block;

/* ELF32 file / program header constants */
#define ELF_MAGIC       0x464C457F   /* 0x7F 'E' 'L' 'F' little-endian */
#define ET_EXEC         2            /* Executable file */
#define PT_LOAD         1            /* Loadable segment */
#define EM_386          3            /* Intel 80386 */
#define EM_ARM          40           /* ARM */
#define ELFCLASS32      1
#define ELFDATA2LSB     1            /* Little-endian */

/* ELF32 header (52 bytes) */
typedef struct {
    unsigned char  e_ident[16];
    unsigned short e_type;
    unsigned short e_machine;
    unsigned int   e_version;
    unsigned int   e_entry;     /* virtual address of entry point */
    unsigned int   e_phoff;     /* offset of program header table */
    unsigned int   e_shoff;     /* offset of section header table (unused) */
    unsigned int   e_flags;
    unsigned short e_ehsize;
    unsigned short e_phentsize;
    unsigned short e_phnum;     /* number of program headers */
    unsigned short e_shentsize;
    unsigned short e_shnum;
    unsigned short e_shstrndx;
} __attribute__((packed)) elf32_ehdr_t;

/* ELF32 program header (32 bytes) */
typedef struct {
    unsigned int p_type;
    unsigned int p_offset;   /* offset in file */
    unsigned int p_vaddr;    /* virtual address in process */
    unsigned int p_paddr;    /* physical address (ignored) */
    unsigned int p_filesz;   /* bytes in file */
    unsigned int p_memsz;    /* bytes in memory (>= p_filesz; extra = zero) */
    unsigned int p_flags;    /* PF_X / PF_W / PF_R */
    unsigned int p_align;
} __attribute__((packed)) elf32_phdr_t;

/* PF flags */
#define PF_X  0x1
#define PF_W  0x2
#define PF_R  0x4

/* Load an ELF32 binary from a VFS node into the process pcb's address space.
 * Switches CR3 to pcb->page_directory during loading, then restores it.
 * On success: *entry_out = entry virtual address, returns 0.
 * On failure: returns negative errno:
 *   -1  invalid ELF magic / class / endian
 *   -2  not an executable (ET_EXEC)
 *   -3  VFS read error
 *   -4  memory allocation failure
 */
int elf_load(struct vfs_node* node,
             struct process_control_block* pcb,
             unsigned int* entry_out);

#endif
