org 0x7c00

; Store boot disk
mov [BOOT_DISK], dl

; Simple segment setup
mov ax, 0
mov ds, ax
mov es, ax

; Set up stack like in the reference code
mov bp, 0x8000
mov sp, bp

; Boot started
mov si, boot_msg
call print_string
;call delay

; Load stage2 from sector 2 to 0x1000 (following your preference)
mov ah, 0x02    ; Read function
mov al, 2       ; 2 sectors (to load more stage2 code)
mov ch, 0       ; Cylinder 0
mov cl, 2       ; Sector 2
mov dh, 0       ; Head 0
mov dl, [BOOT_DISK]    ; Use stored boot disk

; Load to 0x1000 as you prefer
mov bx, 0x1000

int 0x13        ; BIOS disk read
jc error        ; Jump if error

; Print success and jump
mov si, success_msg
call print_string

; Jump to stage2 at 0x1000
jmp 0x1000

error:
mov si, error_msg
call print_string
hlt

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

boot_msg: db 'Boot started', 13, 10, 0
success_msg: db 'Jumping to stage2', 13, 10, 0
error_msg: db 'Disk error!', 13, 10, 0

BOOT_DISK: db 0

times 510-($-$$) db 0
dw 0xAA55

