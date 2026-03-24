[bits 32]

; =============================================================================
; CPU Exception Handlers (ISR 0-31)
; =============================================================================
; Each ISR stub normalises the stack to a uniform isr_frame_t layout and
; then calls the C fault_handler(isr_frame_t*) for a full register dump.
;
; Stack layout after pusha inside isr_common (low addr → high addr):
;   +0  EDI  \
;   +4  ESI   |
;   +8  EBP   |  pusha block (32 bytes)
;   +12 ESP*  |  (* unreliable — value of ESP before pusha)
;   +16 EBX   |
;   +20 EDX   |
;   +24 ECX   |
;   +28 EAX  /
;   +32 vec     (pushed by ISR stub)
;   +36 err     (pushed by ISR stub OR CPU)
;   +40 EIP     (pushed by CPU)
;   +44 CS      (pushed by CPU)
;   +48 EFLAGS  (pushed by CPU)
;   +52 old ESP (ring-3 fault only, CPU-pushed)
;   +56 old SS  (ring-3 fault only, CPU-pushed)
;
; We save CR2 manually (it is only meaningful for #PF but costs nothing
; to save always), then pass &frame to the C handler.
; =============================================================================

global isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7
global isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15
global isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23
global isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
global isr_common

extern fault_handler

; Macro for exceptions that do NOT push an error code
%macro ISR_NOERR 1
  isr%1:
    push dword 0          ; Dummy error code
    push dword %1         ; Exception number
    jmp isr_common
%endmacro

; Macro for exceptions that DO push an error code (CPU already pushed it)
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
section .text
isr_common:
    pusha                   ; save all GP regs (32 bytes)

    ; Save CR2 (page-fault address) into a static slot so the C handler
    ; can read it without worrying about which ISR fired.
    mov eax, cr2
    mov [saved_cr2], eax

    ; Pass a pointer to the top of the stack (= &isr_frame_t) to C.
    push esp                ; arg1: isr_frame_t*
    call fault_handler      ; never returns (calls kernel_panic internally)
    add esp, 4

    ; Unreachable, but assembler needs valid epilogue:
    popa
    add esp, 8              ; pop vec + err
    iretd

section .bss
saved_cr2: resd 1

section .data
global saved_cr2
