[bits 32]

global setup_paging
global page_directory
global page_table0

section .bss
align 4096
page_directory: resb 4096
page_table0:   resb 4096

section .text

setup_paging:
    mov edi, page_directory
    mov ecx, 1024
    xor eax, eax
    rep stosd              ; Clear page directory
    
    mov edi, page_table0
    mov ecx, 1024
    xor eax, eax
    rep stosd              ; Clear page table 0
    
    mov edi, page_table0
    mov eax, 0x00000007
    mov ecx, 1024
    
.map_loop:
    mov [edi], eax
    add eax, 0x1000
    add edi, 4
    loop .map_loop
    
    mov eax, page_table0
    or eax, 0x00000007     ; Present + Read/Write
    mov [page_directory], eax
       
    mov eax, page_directory
    mov cr3, eax
    
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    
    ret