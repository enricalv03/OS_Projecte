[bits 32]

; =============================================================================
; PIT (Programmable Interval Timer) IRQ Handler -- IRQ 0 (INT 32)
; =============================================================================
; Fires at 100 Hz (configured in kernel.asm: divisor 11932).
;
; Safety: The handler checks a 'scheduler_ready' flag before calling
; scheduler_tick(). This prevents crashes if a PIT interrupt somehow
; fires before kernel_c_init() has finished setting up the scheduler.
; The flag is set by calling pit_enable_scheduler() from C.
; =============================================================================

global pit_handler
global pit_get_ticks
global pit_enable_scheduler

extern scheduler_tick

section .data
align 4
ticks:           dd 0
sched_enabled:   dd 0        ; Set to 1 once scheduler is safe to call

section .text

pit_handler:
    pushad                      ; Save all general-purpose registers
    push ds                     ; Save segment registers
    push es
    push fs
    push gs

    ; Load kernel data segments (important for ring transitions later)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Increment global tick counter (always, even before scheduler is ready)
    inc dword [ticks]

    ; Only call scheduler_tick if the scheduler has been initialized.
    ; This prevents triple-faults during early boot.
    cmp dword [sched_enabled], 0
    je .skip_scheduler
    call scheduler_tick

.skip_scheduler:
    ; Send End-Of-Interrupt (EOI) to master PIC
    mov al, 0x20
    out 0x20, al

    pop gs
    pop fs
    pop es
    pop ds
    popad
    iret

; =============================================================================
; pit_get_ticks() -- returns current tick count
; =============================================================================
pit_get_ticks:
    mov eax, [ticks]
    ret

; =============================================================================
; pit_enable_scheduler() -- call from C once scheduler is initialized
; =============================================================================
; After this, every PIT tick will invoke scheduler_tick().
; void pit_enable_scheduler(void);
; =============================================================================
pit_enable_scheduler:
    mov dword [sched_enabled], 1
    ret
