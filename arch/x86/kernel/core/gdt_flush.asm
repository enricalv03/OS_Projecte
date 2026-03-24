[bits 32]

; =============================================================================
; GDT and TSS assembly helpers
; =============================================================================

global gdt_flush
global tss_flush

section .text

; void gdt_flush(unsigned int gdtr_ptr)
; Loads the new GDT and reloads all segment registers.
; Argument: pointer to the 6-byte GDT descriptor (limit + base).
gdt_flush:
    mov eax, [esp + 4]       ; gdtr_ptr
    lgdt [eax]                ; Load the GDT

    ; Reload data segment registers to point to the new kernel data (0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Reload CS by performing a far jump to the new kernel code segment (0x08)
    jmp 0x08:.flush_done

.flush_done:
    ret

; void tss_flush(unsigned int tss_selector)
; Loads the TSS selector into the Task Register.
tss_flush:
    mov ax, [esp + 4]        ; TSS selector (0x28)
    ltr ax                    ; Load task register
    ret
