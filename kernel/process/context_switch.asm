[bits 32]

section .text

; =============================================================================
; Context Switch Module
; =============================================================================
; Handles saving/restoring CPU state for process scheduling.
;
; PCB register layout (offsets from start of pcb_t):
;   +0   pid            (not touched here)
;   +4   parent_pid
;   +8   state
;   +12  priority
;   +16  base_priority
;   +20  eax
;   +24  ebx
;   +28  ecx
;   +32  edx
;   +36  esi
;   +40  edi
;   +44  ebp
;   +48  esp
;   +52  eip
;   +56  eflags
;   +60  cs   (stored as 32-bit for alignment)
;   +64  ds
;   +68  es
;   +72  fs
;   +76  gs
;   +80  ss
; =============================================================================

; PCB register offsets (matching the unsigned int fields in process.h)
%define PCB_EAX      20
%define PCB_EBX      24
%define PCB_ECX      28
%define PCB_EDX      32
%define PCB_ESI      36
%define PCB_EDI      40
%define PCB_EBP      44
%define PCB_ESP      48
%define PCB_EIP      52
%define PCB_EFLAGS   56
%define PCB_CS       60
%define PCB_DS       64
%define PCB_ES       68
%define PCB_FS       72
%define PCB_GS       76
%define PCB_SS       80

global context_save_asm
global context_restore_asm
global context_switch_asm

; =============================================================================
; context_save_asm(pcb_t* pcb)
; =============================================================================
; Saves the current CPU register state into the given PCB.
; Called from C: argument is on the stack at [esp+4].
; Returns: 0 (so caller knows this is the save path)
;
; NOTE: We save the caller's registers, not the interrupt frame.
;       EIP is saved as the return address (caller's next instruction).
; =============================================================================
context_save_asm:
    push ebp
    mov ebp, esp
    push ebx                    ; Save EBX since we use it as PCB pointer

    mov ebx, [ebp + 8]         ; EBX = pcb_t* argument
    test ebx, ebx
    jz .save_fail

    ; Save general-purpose registers
    mov [ebx + PCB_EAX], eax
    mov [ebx + PCB_ECX], ecx
    mov [ebx + PCB_EDX], edx
    mov [ebx + PCB_ESI], esi
    mov [ebx + PCB_EDI], edi

    ; Save caller's EBP (it's what we pushed)
    mov eax, [ebp + 0]         ; Original EBP before this function
    mov [ebx + PCB_EBP], eax

    ; Save caller's ESP (before the call instruction pushed return addr)
    lea eax, [ebp + 8]         ; ESP as it was before pushing the argument
    mov [ebx + PCB_ESP], eax

    ; Save return address as EIP (where to resume)
    mov eax, [ebp + 4]         ; Return address on stack
    mov [ebx + PCB_EIP], eax

    ; Save EFLAGS
    pushfd
    pop eax
    mov [ebx + PCB_EFLAGS], eax

    ; Save segment registers
    mov eax, 0
    mov ax, cs
    mov [ebx + PCB_CS], eax
    mov ax, ds
    mov [ebx + PCB_DS], eax
    mov ax, es
    mov [ebx + PCB_ES], eax
    mov ax, fs
    mov [ebx + PCB_FS], eax
    mov ax, gs
    mov [ebx + PCB_GS], eax
    mov ax, ss
    mov [ebx + PCB_SS], eax

    ; Save the original EBX (it's on the stack)
    pop eax                     ; Restore our saved EBX into EAX
    mov [ebx + PCB_EBX], eax   ; Save original EBX to PCB
    push eax                    ; Put it back for our pop below

    xor eax, eax               ; Return 0 (save path)
    pop ebx
    pop ebp
    ret

.save_fail:
    mov eax, -1
    pop ebx
    pop ebp
    ret

; =============================================================================
; context_restore_asm(pcb_t* pcb)
; =============================================================================
; Restores CPU state from a PCB and jumps to the saved EIP.
; This function does NOT return to its caller -- it jumps directly
; to the instruction pointer stored in the PCB.
;
; For kernel-to-kernel context switches (same privilege level),
; we use a simple JMP instead of IRET to avoid needing a TSS.
; =============================================================================
context_restore_asm:
    mov eax, [esp + 4]         ; EAX = pcb_t* argument
    test eax, eax
    jz .restore_fail

    ; Restore segment registers (data segments only -- CS via far jmp if needed)
    mov bx, [eax + PCB_DS]
    mov ds, bx
    mov bx, [eax + PCB_ES]
    mov es, bx
    mov bx, [eax + PCB_FS]
    mov fs, bx
    mov bx, [eax + PCB_GS]
    mov gs, bx

    ; Restore EFLAGS
    push dword [eax + PCB_EFLAGS]
    popfd

    ; Restore general-purpose registers
    mov ecx, [eax + PCB_ECX]
    mov edx, [eax + PCB_EDX]
    mov esi, [eax + PCB_ESI]
    mov edi, [eax + PCB_EDI]
    mov ebp, [eax + PCB_EBP]

    ; Set up the target stack
    mov esp, [eax + PCB_ESP]

    ; Push the saved EIP so we can RET to it
    push dword [eax + PCB_EIP]

    ; Restore EBX and EAX last (we were using them)
    mov ebx, [eax + PCB_EBX]
    mov eax, [eax + PCB_EAX]

    ; "Return" to the saved EIP
    ret

.restore_fail:
    hlt
    jmp .restore_fail

; =============================================================================
; context_switch_asm(pcb_t* from, pcb_t* to)
; =============================================================================
; Performs a full context switch between two kernel-mode processes.
;
; 1. Saves all registers of the current process into 'from' PCB
; 2. Restores all registers from 'to' PCB
; 3. Execution continues at 'to' process's saved EIP
;
; The 'from' process will resume at the instruction after the call
; to context_switch_asm when it is next scheduled.
; =============================================================================
context_switch_asm:
    push ebp
    mov ebp, esp

    ; --- Save 'from' process context ---
    mov eax, [ebp + 8]         ; EAX = from (pcb_t*)
    test eax, eax
    jz .skip_save               ; If from is NULL, skip save (first schedule)

    ; Save general-purpose registers into 'from' PCB
    mov [eax + PCB_EBX], ebx
    mov [eax + PCB_ECX], ecx
    mov [eax + PCB_EDX], edx
    mov [eax + PCB_ESI], esi
    mov [eax + PCB_EDI], edi

    ; Save caller's EBP
    mov ebx, [ebp + 0]
    mov [eax + PCB_EBP], ebx

    ; Save caller's ESP (what it was before calling us)
    lea ebx, [ebp + 12]        ; Skip saved EBP and return address
    mov [eax + PCB_ESP], ebx

    ; Save return address as EIP
    mov ebx, [ebp + 4]
    mov [eax + PCB_EIP], ebx

    ; Save EFLAGS
    pushfd
    pop ebx
    mov [eax + PCB_EFLAGS], ebx

    ; Save EAX itself (it currently holds the 'from' pointer)
    mov [eax + PCB_EAX], eax

    ; Save segment registers
    xor ebx, ebx
    mov bx, ds
    mov [eax + PCB_DS], ebx
    mov bx, es
    mov [eax + PCB_ES], ebx
    mov bx, fs
    mov [eax + PCB_FS], ebx
    mov bx, gs
    mov [eax + PCB_GS], ebx
    mov bx, ss
    mov [eax + PCB_SS], ebx
    mov bx, cs
    mov [eax + PCB_CS], ebx

.skip_save:
    ; --- Restore 'to' process context ---
    mov eax, [ebp + 12]        ; EAX = to (pcb_t*)
    test eax, eax
    jz .switch_fail

    ; Restore segment registers
    mov bx, [eax + PCB_DS]
    mov ds, bx
    mov bx, [eax + PCB_ES]
    mov es, bx
    mov bx, [eax + PCB_FS]
    mov fs, bx
    mov bx, [eax + PCB_GS]
    mov gs, bx

    ; Restore EFLAGS
    push dword [eax + PCB_EFLAGS]
    popfd

    ; Restore general-purpose registers
    mov ecx, [eax + PCB_ECX]
    mov edx, [eax + PCB_EDX]
    mov esi, [eax + PCB_ESI]
    mov edi, [eax + PCB_EDI]
    mov ebp, [eax + PCB_EBP]

    ; Switch to the target process's stack
    mov esp, [eax + PCB_ESP]

    ; Push the EIP we want to return to
    push dword [eax + PCB_EIP]

    ; Restore EBX and EAX last
    mov ebx, [eax + PCB_EBX]
    mov eax, [eax + PCB_EAX]

    ; Jump to saved EIP (resumes 'to' process)
    ret

.switch_fail:
    pop ebp
    ret
