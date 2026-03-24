[bits 32]

global setup_paging
global page_directory
global page_table0

section .bss
align 4096
page_directory:   resb 4096
page_table0:      resb 4096
page_table1:      resb 4096      ; page table covering 4-8 MB (user stack lives here)
page_table_apic:  resb 4096      ; page table covering LAPIC MMIO (0xFEE00000)

section .text

setup_paging:
    ; -----------------------------------------------------------------
    ; Clear page directory and page tables
    ; -----------------------------------------------------------------
    mov edi, page_directory
    mov ecx, 1024
    xor eax, eax
    rep stosd                  ; Clear page directory
    
    mov edi, page_table0
    mov ecx, 1024
    xor eax, eax
    rep stosd                  ; Clear page table 0 (low 4 MB)

    mov edi, page_table1
    mov ecx, 1024
    xor eax, eax
    rep stosd                  ; Clear page table 1 (4-8 MB)

    mov edi, page_table_apic
    mov ecx, 1024
    xor eax, eax
    rep stosd                  ; Clear APIC page table
    
    ; -----------------------------------------------------------------
    ; Identity-map first 4 MB (0x00000000 - 0x003FFFFF) as user+RW
    ; -----------------------------------------------------------------
    mov edi, page_table0
    mov eax, 0x00000007        ; Present + RW + User
    mov ecx, 1024
    
.map_loop:
    mov [edi], eax
    add eax, 0x1000
    add edi, 4
    loop .map_loop
    
    mov eax, page_table0
    or eax, 0x00000007         ; Present + RW + User
    mov [page_directory], eax  ; PDE 0 -> low 4 MB

    ; -----------------------------------------------------------------
    ; Identity-map 4-8 MB (0x00400000 - 0x007FFFFF) as user+RW
    ; Needed for USER_STACK_TOP at 0x00800000.
    ; -----------------------------------------------------------------
    mov edi, page_table1
    mov eax, 0x00400007        ; phys 0x00400000 | Present + RW + User
    mov ecx, 1024

.map_loop1:
    mov [edi], eax
    add eax, 0x1000
    add edi, 4
    loop .map_loop1

    mov eax, page_table1
    or eax, 0x00000007         ; Present + RW + User
    mov [page_directory + 4], eax  ; PDE 1 -> 4-8 MB

    ; -----------------------------------------------------------------
    ; Map LAPIC MMIO (0xFEE00000-0xFEE00FFF) as kernel-only RW.
    ; Virtual address == physical address so existing LAPIC_BASE works.
    ;
    ; PDE index for 0xFEE00000: bits 31..22 = 0x3FB (1019)
    ; PTE index within that 4 MB range: (0xFEE00000 >> 12) & 0x3FF = 512
    ; -----------------------------------------------------------------
    mov eax, page_table_apic
    or eax, 0x00000003         ; Present + RW (supervisor only)
    mov edi, page_directory
    add edi, 4 * 1019          ; PDE[1019] covers 0xFEC00000-0xFEFFFFFF
    mov [edi], eax

    ; Set PTE for LAPIC base at index 512 -> 0xFEE00000
    mov edi, page_table_apic
    add edi, 4 * 512
    mov eax, 0xFEE00000 | 0x00000003   ; Present + RW (supervisor)
    mov [edi], eax
       
    ; -----------------------------------------------------------------
    ; Load CR3 and enable paging
    ; -----------------------------------------------------------------
    mov eax, page_directory
    mov cr3, eax
    
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    
    ret