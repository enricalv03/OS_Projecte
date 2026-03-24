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
global pit_send_eoi
global pit_get_irq_frame_esp

extern scheduler_tick

section .data
align 4
global irq_frame_esp
ticks:           dd 0
sched_enabled:   dd 0        ; Set to 1 once scheduler is safe to call
irq_frame_esp:   dd 0        ; ESP after push gs (base of our IRQ frame)

section .text

pit_handler:
    pushad                      ; Save all general-purpose registers
    push ds                     ; Save segment registers
    push es
    push fs
    push gs

    ; Save ESP pointing to our frame - needed for preemptive context switch
    mov eax, esp
    mov [irq_frame_esp], eax

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
; pit_send_eoi() -- send End-Of-Interrupt to master PIC
; Must be called before preemptive context switch from IRQ handler.
; =============================================================================
pit_send_eoi:
    mov al, 0x20
    out 0x20, al
    ret

; =============================================================================
; pit_get_irq_frame_esp() -- returns ESP of current IRQ frame (for preemption)
; =============================================================================
pit_get_irq_frame_esp:
    mov eax, [irq_frame_esp]
    ret

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
