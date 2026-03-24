[bits 32]

global idt_install
global enable_interrupts
global set_idt_entry
global set_idt_entry_user
global dummy_interrupt
global idt_table

global disable_interrupts
global halt_cpu
global idle_cpu

section .text
  idt_install:
    lidt [idt_descriptor]
    ret

  enable_interrupts:
    sti
    ret

  dummy_interrupt:
    iret

  set_idt_entry:
    push eax
    push ebx
    push ecx
    push edi

    mov edi, idt_table
    mov ecx, 8
    mul ecx
    add edi, eax

    mov [edi], bx
    mov word [edi+2], 0x08
    mov word [edi+4], 0x8E00
    shr ebx, 16
    mov [edi+6], bx

    pop edi
    pop ecx
    pop ebx
    pop eax
    ret

  ; Same as set_idt_entry, but with DPL=3 so that user-mode (ring 3)
  ; can invoke the interrupt via INT n (used for INT 0x80 syscalls).
  set_idt_entry_user:
    push eax
    push ebx
    push ecx
    push edi

    mov edi, idt_table
    mov ecx, 8
    mul ecx
    add edi, eax

    mov [edi], bx
    mov word [edi+2], 0x08
    mov word [edi+4], 0xEE00    ; P=1, DPL=3, type=0xE (32-bit int gate)
    shr ebx, 16
    mov [edi+6], bx

    pop edi
    pop ecx
    pop ebx
    pop eax
    ret

disable_interrupts:
  cli
  ret

halt_cpu:
.halt_loop:
  hlt
  jmp .halt_loop

  ; Wait for one interrupt then return (for idle loop). Arch-specific.
idle_cpu:
  hlt
  ret

section .data
align 8
idt_descriptor:
    dw (256*8)-1
    dd idt_table
section .bss
align 8
idt_table: resb 256*8
  