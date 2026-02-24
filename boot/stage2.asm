[bits 16]
org 0x1000 ; Now we're loaded at 0x1000

; Print stage2 loaded message
mov si, stage2_msg
call print_string
call delay

; Entering protected mode
mov si, switching_msg
call print_string
call delay

; Load kernel from disk to 0x3000.
; QEMU hard-disk geometry has 63 sectors/track, so a single INT 0x13
; read from CHS(0,0,4) can load at most 60 sectors (30 KB).
; The kernel is now >30 KB, so we do TWO reads across the track boundary:
;   Read 1: 60 sectors from head 0, CHS(0,0,4)  -> LBA  3..62   -> 0x3000
;   Read 2: 60 sectors from head 1, CHS(0,1,1)  -> LBA 63..122  -> 0xA800
; Total capacity: 120 sectors = 60 KB.

; --- Read 1: head 0 ---
mov ah, 0x02
mov al, 60       ; 60 sectors (30 KB)
mov ch, 0
mov cl, 4        ; CHS sector 4 (= LBA 3)
mov dh, 0        ; head 0
mov dl, 0x80
mov bx, 0x3000   ; destination
int 0x13
jc error

; --- Read 2: head 1 (continues right after read 1) ---
; ES:BX must point to 0xA800. Use ES=0x0A80, BX=0 to avoid
; crossing a 64 KB segment boundary during the transfer.
mov ax, 0x0A80
mov es, ax
mov ah, 0x02
mov al, 60       ; 60 more sectors (30 KB)
mov ch, 0
mov cl, 1        ; CHS sector 1
mov dh, 1        ; head 1
mov dl, 0x80
xor bx, bx       ; BX = 0  (ES:BX = 0x0A80:0x0000 = phys 0xA800)
int 0x13
jc error

; Restore ES = 0 for E820 and other BIOS calls
xor ax, ax
mov es, ax

; Stack must be above the kernel binary (.text + .rodata + .data).
; With .data now ending at ~0xB700, 0xC000 had only ~2 KB margin which
; is too tight for BIOS E820 calls. 0xF000 gives >6 KB of safe room.
; This is in BSS territory, which gets zeroed by the kernel on boot.
mov sp, 0xF000
mov bp, sp

; clear screen option
; mov ah, 0x0
; mov al, 0x3  
; int 0x10        ; text mode (this clears screen)

; === E820 MEMORY DETECTION

mov di, 0x2000
mov ebx, 0
mov edx, 0x534D4150 ; SMAP signature
mov ecx, 24

e820_loop:
    mov eax, 0xE820
    int 0x15
    jc e820_done

    cmp eax, 0x534D4150
    jne e820_done

    add di, 24
    cmp ebx, 0
    jne e820_loop

e820_done:
    mov ax, di
    sub ax, 0x2000       ; ax = total bytes used
    mov bx, 24
    xor dx, dx
    div bx               ; ax = entry count
    mov [0x1FFC], ax     ; Store count (16-bit)

    ; Display memory map info (for debugging)
    mov si, memmap_msg
    call print_string
    
    ; Test: just print a fixed number first
    mov ax, [0x1FFC]            ; Fixed test number
    call print_number

CODE_SEG equ gdt_code - GDT_start
DATA_SEG equ gdt_data - GDT_start

cli
lgdt [gdt_descriptor]
mov eax, cr0
or eax, 1
mov cr0, eax
jmp CODE_SEG:start_protected_mode

print_number:
    push ax
    push bx
    push cx
    push dx
    
    ; Just print the number as a single digit for now
    add al, '0'          ; Convert to ASCII
    mov ah, 0x0E
    int 0x10             ; Print character
    
    pop dx
    pop cx
    pop bx
    pop ax
    ret
    
.divide_loop:
    xor dx, dx           ; Clear dx for division
    div bx               ; ax = ax/10, dx = remainder
    push dx              ; Save digit
    inc cx               ; Count digits
    cmp ax, 0
    jne .divide_loop     ; Continue if not zero
    
.print_loop:
    pop dx               ; Get digit
    add dl, '0'          ; Convert to ASCII
    mov ah, 0x0E
    int 0x10             ; Print character
    loop .print_loop
    
    pop dx
    pop cx
    pop bx
    pop ax
    ret

delay:
    push ax
    push dx
    mov ax, 0x8600  ; BIOS delay function  
    mov dx, 0x4000  ; Delay value
    int 0x15
    pop dx
    pop ax
    ret

print_string:
    mov ah, 0x0E
.loop:
    lodsb
    cmp al, 0
    je .done
    int 0x10
    jmp .loop
.done:
    ret

error:
    mov si, error_msg
    call print_string
    hlt

; === GDT SECTION ====
GDT_start:
    gdt_null:
        dd 0x0
        dd 0x0

    gdt_code:
        dw 0xffff
        dw 0x0
        db 0x0
        db 0b10011010
        db 0b11001111
        db 0x0

    gdt_data:
        dw 0xffff
        dw 0x0
        db 0x0
        db 0b10010010
        db 0b11001111
        db 0x0

gdt_end:

gdt_descriptor:
    dw gdt_end - GDT_start - 1
    dd GDT_start

[bits 32]
start_protected_mode:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    mov ebp, 0x90000    ; 32 bit stack base pointer
    mov esp, ebp

    mov bl, 0x0F
    call protected_message
    
    ; Jump to kernel at 0x3000
    jmp 0x3000

protected_message:
    mov edx, 0xb8000
    add edx, 160*13
    mov esi, protected_msg

.loop:
    lodsb
    cmp al, 0
    je .done
    mov [edx], al
    mov [edx+1], bl
    add edx, 2
    jmp .loop
.done:
    ret

stage2_msg: db 'Stage 2 loaded :) ', 13, 10, 0
switching_msg: db 'Entering protected mode...', 13, 10, 0
error_msg: db 'Error loading kernel', 13, 10, 0
protected_msg: db 'Protected mode activated! Jumping to kenrel...', 0
memmap_msg: db 'Memory entries: ', 0
hex_digits: db '0123456789ABCDEF'
