[bits 32]

global keyboard_handler
global shift_pressed

extern print_char_pm

section .text

keyboard_handler:
  push eax
  push ebx

  in al, 0x60                    ; Read scan code FIRST
  call show_scan_code            ; Debug (optional)

  cmp al, 0x80                   ; Check for key release
  jae .end_handler

  cmp al, 0x1C                   ; Check for Enter
  je .handle_enter

  cmp al, 0x0E                   ; Check for Backspace  
  je .handle_backspace

  ; Select correct table based on shift state
  mov esi, normal_scancode_table
  cmp byte [shift_pressed], 1
  jne .convert_char
  mov esi, shift_scancode_table

.convert_char:
  mov ebx, 0
  mov bl, al
  mov al, [esi + ebx]           ; Use selected table
  cmp al, 0
  je .end_handler

  ; Display character
  mov edi, [cursor_pos]
  mov bl, 0x0F
  call print_char_pm

  add dword [cursor_pos], 2
  jmp .end_handler

.handle_enter:
  inc dword [current_line]
  mov eax, [current_line]
  mov ebx, 160
  mul ebx
  add eax, 0xb8000
  mov [cursor_pos], eax
  jmp .end_handler

.handle_backspace:
  mov eax, [cursor_pos]
  mov ebx, 0xb8000 + 160*16
  cmp eax, ebx
  je .end_handler

  sub dword [cursor_pos], 2

  mov edi, [cursor_pos]
  mov al, ' '
  mov bl, 0x0F
  call print_char_pm

  jmp .end_handler

.handle_shift_press:
  jmp .end_handler

.handle_shift_release:
  jmp .end_handler

.end_handler:
  mov al, 0x20
  out 0x20, al ; sends to hardware port

  pop ebx
  pop eax

  iret

section .data
cursor_pos: dd 0xb8000 + 160*16
current_line: dd 16
shift_pressed: db 0

normal_scancode_table:
  db 0, 0, '1', '2', '3', '4', '5', '6'       ; 0x00-0x07
  db '7', '8', '9', '0', 27h, 0A1h, 0, 0      ; 0x08-0x0F (27h = ', A1h = ¡)

  db 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i'  ; 0x10-0x17
  db 'o', 'p', '`', '+', 0, 0, 'a', 's'      ; 0x18-0x1F

  db 'd', 'f', 'g', 'h', 'j', 'k', 'l', 0F1h ; 0x20-0x27 (F1h = ñ)
  db 0B4h, 0E7h, 0, '<', 'z', 'x', 'c', 'v'  ; 0x28-0x2F (B4h = ´, E7h = ç)

  db 'b', 'n', 'm', ',', '.', '-', 0, 0       ; 0x30-0x37
  db 0, ' '                                   ; 0x38-0x39

  times (256-58) db 0

shift_scancode_table:
  db 0, 0, '!', '"', 0B7h, '$', '%', '&'      ; 0x00-0x07
  db '(', ')', '=', '?', 0BFh, 0, 0, 0        ; 0x08-0x0F
  
  db 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I'  ; 0x10-0x17
  db 'O', 'P', '^', '*', 0, 0, 'A', 'S'      ; 0x18-0x1F
  
  db 'D', 'F', 'G', 'H', 'J', 'K', 'L', 0D1h ; 0x20-0x27
  db 0A8h, 0C7h, 0, '>', 'Z', 'X', 'C', 'V'  ; 0x28-0x2F
  
  db 'B', 'N', 'M', ';', ':', '_', 0, 0       ; 0x30-0x37
  db 0, ' '                                   ; 0x38-0x39
  
  times (256-58) db 0

; Simple function to show scan code as two hex digits
show_scan_code:
    push eax
    push edi
    
    ; Show scan code at top-right corner
    mov edi, 0xb8000 + 160*2 + 70*2  ; Line 2, column 70
    
    ; Convert scan code to hex and display
    mov ah, al          ; Save original
    shr al, 4           ; Get upper 4 bits
    add al, '0'         ; Convert to ASCII
    cmp al, '9'
    jle .first_digit
    add al, 7           ; A-F
.first_digit:
    mov [edi], al       ; Display first hex digit
    mov byte [edi+1], 0x0E  ; Yellow color
    
    mov al, ah          ; Get original back
    and al, 0x0F        ; Get lower 4 bits  
    add al, '0'
    cmp al, '9'
    jle .second_digit
    add al, 7
.second_digit:
    mov [edi+2], al     ; Display second hex digit
    mov byte [edi+3], 0x0E  ; Yellow color
    
    pop edi
    pop eax
    ret