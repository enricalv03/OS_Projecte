[bits 32]

; Simple INT 0x80 syscall handler.
; Convention:
;   On entry (from user mode):
;     EAX = syscall number
;     EBX = arg1
;     ECX = arg2
;     EDX = arg3
;     ESI = arg4
;
;   Returns:
;     EAX = syscall return value (visible to user mode).
;
; We save all registers and segment registers, switch to kernel data
; segments, call the C dispatcher, then store the return value into
; the saved EAX slot so popad restores it for user mode.

global syscall_handler
extern syscall_handle

syscall_handler:
  pushad                 ; save general-purpose registers
  push ds
  push es
  push fs
  push gs

  ; Switch to kernel data segments while in the handler.
  ; NOTE: this clobbers AX, so we must read the original register
  ; values from the pushad frame on the stack, not from registers.
  mov ax, 0x10
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax

  ; Read original register values from the pushad save area.
  ; Stack layout: [esp+0]=gs [4]=fs [8]=es [12]=ds
  ;   [16]=EDI [20]=ESI [24]=EBP [28]=ESP [32]=EBX [36]=EDX [40]=ECX [44]=EAX
  ; Each push below shifts ESP by -4, so offsets increase by +4.
  push dword [esp + 20]  ; arg4 = original ESI
  push dword [esp + 40]  ; arg3 = original EDX  (+4 for 1 push above)
  push dword [esp + 48]  ; arg2 = original ECX  (+8 for 2 pushes)
  push dword [esp + 44]  ; arg1 = original EBX  (+12 for 3 pushes)
  push dword [esp + 60]  ; syscall number = original EAX (+16 for 4 pushes)

  call syscall_handle
  add esp, 20            ; pop 5 args

  ; At this point, EAX holds the syscall return value.
  ; We must place it into the saved EAX slot so that popad
  ; will restore this value back to user mode.
  ;
  ; Stack layout here (top -> bottom):
  ;   [0]  gs
  ;   [4]  fs
  ;   [8]  es
  ;   [12] ds
  ;   [16] EDI  \
  ;   [20] ESI   |
  ;   [24] EBP   |  pushad area
  ;   [28] ESP   |
  ;   [32] EBX   |
  ;   [36] EDX   |
  ;   [40] ECX   |
  ;   [44] EAX  /
  ;   [48]+ CPU-pushed EIP/CS/EFLAGS/(old ESP/SS)
  ;
  ; So saved EAX is at [esp + 44].
  mov [esp + 44], eax

  pop gs
  pop fs
  pop es
  pop ds
  popad

  iret