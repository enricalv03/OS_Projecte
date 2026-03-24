[bits 32]

; =============================================================================
; Virtual Memory Manager (VMM)
; =============================================================================
; Manages virtual-to-physical page mappings via x86 two-level page tables.
;
; ALL PUBLIC FUNCTIONS use the C calling convention (cdecl):
;   - Arguments passed on the stack (right to left)
;   - Return value in EAX
;   - Caller cleans up the stack
;   - Callee preserves EBX, ESI, EDI, EBP
;
; Internal helpers use register-based calling (documented per function).
;
; Page directory layout:
;   page_directory (defined in paging.asm, .bss, 4096 bytes)
;   - 1024 entries, each 4 bytes
;   - Entry = physical_addr_of_page_table | flags
;   - PDE index = virtual_addr >> 22
;
; Page table layout:
;   - 1024 entries, each 4 bytes
;   - Entry = physical_addr_of_page | flags
;   - PTE index = (virtual_addr >> 12) & 0x3FF
; =============================================================================

section .text

extern pmm_alloc_page
extern pmm_free_page
extern page_directory          ; Label in paging.asm BSS (NOT a pointer)

global vmm_init
global vmm_map_page
global vmm_unmap_page
global vmm_get_physical
global vmm_alloc_and_map
global vmm_unmap_and_free

; Constants
PAGE_SIZE           equ 4096
PAGE_DIR_ENTRIES    equ 1024
PAGE_TABLE_ENTRIES  equ 1024

; =============================================================================
; vmm_init() -- placeholder for future initialization
; =============================================================================
vmm_init:
    ret

; =============================================================================
; INTERNAL: _vmm_clear_page_table
; =============================================================================
; Zeros a 4KB page table at a given PHYSICAL address.
; Assumes the address is within the identity-mapped region (< 4MB).
;
; Input:  EAX = physical address of page table to clear
; Clobbers: ECX, EDI, EAX (via rep stosd)
; =============================================================================
_vmm_clear_page_table:
    mov edi, eax               ; Destination = physical address (identity-mapped)
    mov ecx, PAGE_TABLE_ENTRIES ; 1024 dwords = 4096 bytes
    xor eax, eax
    rep stosd
    ret

; =============================================================================
; int vmm_map_page(unsigned int virtual_addr,
;                  unsigned int physical_addr,
;                  unsigned int flags);
; =============================================================================
; Maps a virtual page to a physical page.
; If the page table for the virtual address doesn't exist, allocates one.
;
; Returns: 1 on success, 0 on failure
; =============================================================================
vmm_map_page:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi

    ; Load arguments from stack
    mov eax, [ebp + 8]         ; virtual_addr
    mov ebx, [ebp + 12]        ; physical_addr
    mov ecx, [ebp + 16]        ; flags

    ; Calculate PDE index (bits 31:22 of virtual address)
    mov edx, eax
    shr edx, 22                ; EDX = PDE index (0-1023)

    ; Calculate PTE index (bits 21:12 of virtual address)
    mov esi, eax
    shr esi, 12
    and esi, 0x3FF             ; ESI = PTE index (0-1023)

    ; Read page directory entry: PDE[edx]
    mov edi, page_directory    ; EDI = address of page directory
    mov eax, [edi + edx * 4]  ; EAX = PDE[edx]

    ; Check if a page table already exists for this PDE
    test eax, 0x01             ; Present bit?
    jnz .have_page_table

    ; --- No page table exists: allocate and initialize one ---
    ; Save our working registers before calling pmm_alloc_page
    push edx                   ; PDE index
    push esi                   ; PTE index
    push ebx                   ; physical_addr to map
    push ecx                   ; flags

    call pmm_alloc_page        ; Returns physical address in EAX, or 0
    test eax, eax
    jz .alloc_failed

    ; Clear the new page table (zero all 1024 entries)
    ; The page comes from PMM which allocates within the first ~127MB.
    ; For early boot, allocations are within the 4MB identity-mapped region.
    push eax                   ; Save page table physical address
    call _vmm_clear_page_table
    pop eax                    ; Restore page table physical address

    ; Install the new page table in the page directory
    mov edi, page_directory
    pop ecx                    ; Restore flags
    pop ebx                    ; Restore physical_addr to map
    pop esi                    ; Restore PTE index
    pop edx                    ; Restore PDE index

    ; PDE = page_table_physical | Present | Read/Write
    push eax                   ; Save page table physical address
    or eax, 0x03               ; Present + R/W (page tables should always be accessible)
    mov [edi + edx * 4], eax   ; Write PDE[edx]
    pop eax                    ; EAX = page table physical address (clean, no flags)
    jmp .write_pte

.alloc_failed:
    ; Clean up stack and return failure
    pop ecx
    pop ebx
    pop esi
    pop edx
    xor eax, eax              ; Return 0 (failure)
    jmp .done

.have_page_table:
    ; Page table exists. Extract its physical address (strip flag bits).
    and eax, 0xFFFFF000        ; EAX = page table physical address

.write_pte:
    ; EAX = page table physical address (identity-mapped, so usable as virtual)
    ; ESI = PTE index
    ; EBX = physical address to map
    ; ECX = flags
    or ebx, ecx               ; Combine physical address with flags
    mov [eax + esi * 4], ebx   ; Write PTE[esi]

    ; Flush TLB by reloading CR3
    mov eax, cr3
    mov cr3, eax

    mov eax, 1                ; Return 1 (success)

.done:
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

; =============================================================================
; int vmm_unmap_page(unsigned int virtual_addr);
; =============================================================================
; Removes the mapping for a virtual page (clears the PTE).
; Does NOT free the physical page -- use vmm_unmap_and_free for that.
;
; Returns: 1 on success, 0 if not mapped
; =============================================================================
vmm_unmap_page:
    push ebp
    mov ebp, esp
    push esi
    push edi

    mov eax, [ebp + 8]         ; virtual_addr

    ; PDE index
    mov edx, eax
    shr edx, 22

    ; PTE index
    mov esi, eax
    shr esi, 12
    and esi, 0x3FF

    ; Read PDE
    mov edi, page_directory
    mov eax, [edi + edx * 4]
    test eax, 0x01
    jz .not_mapped

    ; Get page table address
    and eax, 0xFFFFF000

    ; Clear PTE
    mov dword [eax + esi * 4], 0

    ; Flush TLB
    mov eax, cr3
    mov cr3, eax

    mov eax, 1
    jmp .done

.not_mapped:
    xor eax, eax

.done:
    pop edi
    pop esi
    pop ebp
    ret

; =============================================================================
; unsigned int vmm_get_physical(unsigned int virtual_addr);
; =============================================================================
; Translates a virtual address to its physical address.
;
; Returns: physical address, or 0 if not mapped
; =============================================================================
vmm_get_physical:
    push ebp
    mov ebp, esp
    push esi
    push edi

    mov eax, [ebp + 8]         ; virtual_addr

    ; Save page offset (bits 0:11) for later
    mov ecx, eax
    and ecx, 0xFFF

    ; PDE index
    mov edx, eax
    shr edx, 22

    ; PTE index
    mov esi, eax
    shr esi, 12
    and esi, 0x3FF

    ; Read PDE
    mov edi, page_directory
    mov eax, [edi + edx * 4]
    test eax, 0x01
    jz .not_mapped

    ; Get page table
    and eax, 0xFFFFF000
    mov eax, [eax + esi * 4]
    test eax, 0x01
    jz .not_mapped

    ; Extract physical page base and add offset
    and eax, 0xFFFFF000
    add eax, ecx              ; physical_base + page_offset
    jmp .done

.not_mapped:
    xor eax, eax

.done:
    pop edi
    pop esi
    pop ebp
    ret

; =============================================================================
; unsigned int vmm_alloc_and_map(unsigned int virtual_addr, unsigned int flags);
; =============================================================================
; Allocates a physical page and maps it at the given virtual address.
;
; Returns: physical address of the allocated page, or 0 on failure
; =============================================================================
vmm_alloc_and_map:
    push ebp
    mov ebp, esp
    push ebx
    push esi

    mov esi, [ebp + 8]         ; virtual_addr
    mov ebx, [ebp + 12]        ; flags

    ; Step 1: Allocate a physical page
    call pmm_alloc_page        ; Returns phys addr in EAX, or 0
    test eax, eax
    jz .fail

    ; Save the physical address we got
    push eax                   ; [esp] = physical address

    ; Step 2: Map it: vmm_map_page(virtual_addr, physical_addr, flags)
    push ebx                   ; arg3: flags
    push eax                   ; arg2: physical_addr
    push esi                   ; arg1: virtual_addr
    call vmm_map_page
    add esp, 12                ; Clean up 3 arguments

    test eax, eax
    jz .map_failed

    ; Success: return the physical address
    pop eax                    ; Restore physical address
    jmp .done

.map_failed:
    ; Mapping failed, free the physical page we allocated
    pop eax                    ; Get physical address back
    push eax                   ; Pass as argument to pmm_free_page
    ; Note: pmm_free_page uses register calling (EAX), so just call it
    call pmm_free_page
    ; (pmm_free_page reads EAX which we already set above, but we also
    ;  pushed it -- that's fine, just clean up)
    add esp, 4

.fail:
    xor eax, eax

.done:
    pop esi
    pop ebx
    pop ebp
    ret

; =============================================================================
; int vmm_unmap_and_free(unsigned int virtual_addr);
; =============================================================================
; Unmaps a virtual page AND frees the underlying physical page.
;
; Returns: 1 on success, 0 on failure
; =============================================================================
vmm_unmap_and_free:
    push ebp
    mov ebp, esp
    push ebx

    mov eax, [ebp + 8]         ; virtual_addr

    ; Step 1: Get the physical address before unmapping
    push eax                   ; arg1: virtual_addr
    call vmm_get_physical
    add esp, 4
    test eax, eax
    jz .fail

    mov ebx, eax               ; EBX = physical address

    ; Step 2: Unmap the virtual page
    push dword [ebp + 8]       ; arg1: virtual_addr
    call vmm_unmap_page
    add esp, 4
    test eax, eax
    jz .fail

    ; Step 3: Free the physical page
    ; pmm_free_page expects address in EAX (register convention)
    mov eax, ebx
    call pmm_free_page

    mov eax, 1
    jmp .done

.fail:
    xor eax, eax

.done:
    pop ebx
    pop ebp
    ret
