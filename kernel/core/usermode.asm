[bits 32]

section .text

; ============================================================================
; enter_user_mode(user_eip, user_esp)
; ----------------------------------------------------------------------------
; Builds an IRET frame to transition from ring 0 to ring 3 using the
; already-initialized user code/data segments in the GDT:
;   CS = 0x1B  (GDT user code selector with RPL=3)
;   DS = ES = FS = GS = SS = 0x23 (GDT user data selector with RPL=3)
;
; C prototype:
;   void enter_user_mode(unsigned int user_eip, unsigned int user_esp);
;
; NOTE: If this succeeds, it does NOT return.
; ============================================================================
global enter_user_mode

enter_user_mode:
    ; Load arguments
    mov eax, [esp + 4]      ; user_eip
    mov ebx, [esp + 8]      ; user_esp

    ; Set data segments to user data selector (0x23)
    mov cx, 0x23
    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx

    ; Build IRET frame: SS, ESP, EFLAGS, CS, EIP
    push dword 0x23         ; user SS
    push ebx                ; user ESP

    pushfd                   ; current EFLAGS
    pop ecx
    or ecx, 0x200            ; ensure IF=1 so interrupts work
    push ecx

    push dword 0x1B         ; user CS
    push eax                ; user EIP

    iretd

