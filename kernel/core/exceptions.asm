[bits 32]

; =============================================================================
; CPU Exception Handlers (ISR 0-31)
; =============================================================================
; Each exception pushes the exception number (and error code if applicable)
; onto the stack, then jumps to isr_common which displays debug info and halts.
;
; For Page Faults (#14), CR2 is also printed -- it contains the virtual
; address that caused the fault.
; =============================================================================

global isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7
global isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15
global isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23
global isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
global isr_common

extern print_char_pm
extern print_string_pm
extern print_hex32_pm

; Macro for exceptions that do NOT push an error code (CPU pushes nothing)
; We push a dummy 0 so the stack frame is uniform.
%macro ISR_NOERR 1
  isr%1:
    push dword 0          ; Dummy error code
    push dword %1         ; Exception number
    jmp isr_common
%endmacro

; Macro for exceptions that DO push an error code (CPU pushes it automatically)
%macro ISR_ERR 1
  isr%1:
    push dword %1         ; Exception number (error code already on stack)
    jmp isr_common
%endmacro

; --- Exception stubs (0-31) ---
ISR_NOERR 0    ; #DE  Divide Error
ISR_NOERR 1    ; #DB  Debug
ISR_NOERR 2    ;      NMI
ISR_NOERR 3    ; #BP  Breakpoint
ISR_NOERR 4    ; #OF  Overflow
ISR_NOERR 5    ; #BR  Bound Range Exceeded
ISR_NOERR 6    ; #UD  Invalid Opcode
ISR_NOERR 7    ; #NM  Device Not Available
ISR_ERR   8    ; #DF  Double Fault
ISR_NOERR 9    ;      Coprocessor Segment Overrun
ISR_ERR   10   ; #TS  Invalid TSS
ISR_ERR   11   ; #NP  Segment Not Present
ISR_ERR   12   ; #SS  Stack-Segment Fault
ISR_ERR   13   ; #GP  General Protection Fault
ISR_ERR   14   ; #PF  Page Fault
ISR_NOERR 15   ;      Reserved
ISR_NOERR 16   ; #MF  x87 FPU Error
ISR_ERR   17   ; #AC  Alignment Check
ISR_NOERR 18   ; #MC  Machine Check
ISR_NOERR 19   ; #XM  SIMD FP Exception
ISR_NOERR 20   ; #VE  Virtualization Exception
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_ERR   30   ; #SX  Security Exception
ISR_NOERR 31

; =============================================================================
; Common exception handler
; =============================================================================
; Stack on entry (after ISR stub pushes):
;   [ESP + 0]  = exception number
;   [ESP + 4]  = error code (real or dummy 0)
;   [ESP + 8]  = EIP (pushed by CPU)
;   [ESP + 12] = CS  (pushed by CPU)
;   [ESP + 16] = EFLAGS (pushed by CPU)
; =============================================================================
section .text
isr_common:
    pusha

    ; Grab exception number and error code from the stack
    ; After pusha (8 regs * 4 bytes = 32 bytes offset):
    mov ecx, [esp + 32 + 0]     ; Exception number
    mov edx, [esp + 32 + 4]     ; Error code

    ; --- Line 1: "CPU Exception #XX" ---
    mov edi, 0xb8000 + 160*18
    mov bl, 0x0C                 ; Bright red
    mov esi, msg_exc
    call print_string_pm

    mov al, ' '
    call print_char_pm
    mov al, '#'
    call print_char_pm

    mov eax, ecx                 ; Exception number
    call print_hex32_pm

    ; --- Line 2: "Error code: 0xXXXXXXXX" ---
    mov edi, 0xb8000 + 160*19
    mov bl, 0x0E                 ; Yellow
    mov esi, msg_err
    call print_string_pm

    mov eax, edx                 ; Error code
    call print_hex32_pm

    ; --- Line 3: For Page Faults (#14), print CR2 (faulting address) ---
    cmp ecx, 14
    jne .not_page_fault

    mov edi, 0xb8000 + 160*20
    mov bl, 0x0E                 ; Yellow
    mov esi, msg_cr2
    call print_string_pm

    mov eax, cr2                 ; CR2 = faulting virtual address
    call print_hex32_pm

.not_page_fault:
    ; --- Line 4: Print EIP where the fault occurred ---
    mov edi, 0xb8000 + 160*21
    mov bl, 0x0F                 ; White
    mov esi, msg_eip
    call print_string_pm

    mov eax, [esp + 32 + 8]     ; EIP from interrupt frame
    call print_hex32_pm

    ; Halt the CPU -- no recovery from exceptions (for now)
    cli
.halt:
    hlt
    jmp .halt

section .data
msg_exc: db 'CPU Exception', 0
msg_err: db 'Error code: 0x', 0
msg_cr2: db 'CR2 (fault addr): 0x', 0
msg_eip: db 'EIP: 0x', 0
