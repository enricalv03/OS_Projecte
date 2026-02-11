[bits 32]

global keyboard_handler
global shift_pressed

extern get_char_from_scan_code
extern kbd_buffer_put

section .text

; Keyboard interrupt handler (IRQ1 / vector 33)
keyboard_handler:
  ; Guardar tots els registres generals
  pushad

  ; Llegir el codi de tecla del port del teclat
  in al, 0x60                  ; AL = scan code

  ; --- Handle 0xE0 prefix (extended keys like arrow keys) ---
  cmp al, 0xE0
  je .handle_e0_prefix

  ; If we're expecting an extended scan code, process it
  cmp byte [e0_pending], 1
  je .handle_extended_key

  ; --- Tecles modificadores ---
  cmp al, 0x2A                 ; Left Shift down
  je .handle_shift_press
  cmp al, 0xAA                 ; Left Shift up
  je .handle_shift_release
  cmp al, 0x36                 ; Right Shift down
  je .handle_shift_press
  cmp al, 0xB6                 ; Right Shift up
  je .handle_shift_release
  cmp al, 0x38                 ; AltGr down
  je .handle_altgr_press
  cmp al, 0xB8                 ; AltGr up
  je .handle_altgr_release
  cmp al, 0x1D                 ; Ctrl down
  je .handle_ctrl_press
  cmp al, 0x9D                 ; Ctrl up
  je .handle_ctrl_release

  ; Si és un "key release" normal (bit 7 = 1), sortim
  cmp al, 0x80
  jae .end_handler

  ; --- Tecles especials Enter i Backspace ---
  cmp al, 0x1C                 ; Enter
  je .handle_enter
  cmp al, 0x0E                 ; Backspace
  je .handle_backspace

  ; --- Construir byte de modificadors a BL ---
  mov bl, 0
  cmp byte [shift_pressed], 1
  jne .no_shift
  or bl, 0x01                  ; bit 0 = Shift
.no_shift:
  cmp byte [altgr_pressed], 1
  jne .no_altgr
  or bl, 0x02                  ; bit 1 = AltGr
.no_altgr:
  cmp byte [ctrl_pressed], 1
  jne .no_ctrl
  or bl, 0x04                  ; bit 2 = Ctrl
.no_ctrl:

  ; EAX = scan code, BL = modificadors
  movzx eax, al                ; EAX = scan code zero-extès
  push ebx                     ; segon argument: modifiers
  push eax                     ; primer argument: scan code
  call get_char_from_scan_code
  add esp, 8                   ; netejar arguments

  ; AL = caràcter (0 si no hi ha mapping)
  cmp al, 0
  je .end_handler

  movzx eax, al
  push eax
  call kbd_buffer_put
  add esp, 4
  jmp .end_handler

.handle_enter:
  mov eax, 10                  ; '\n'
  push eax
  call kbd_buffer_put
  add esp, 4
  jmp .end_handler

.handle_backspace:
  mov eax, 8
  push eax
  call kbd_buffer_put
  add esp, 4
  jmp .end_handler

.handle_shift_press:
  mov byte [shift_pressed], 1
  jmp .end_handler

.handle_shift_release:
  mov byte [shift_pressed], 0
  jmp .end_handler

.handle_altgr_press:
  mov byte [altgr_pressed], 1
  jmp .end_handler

.handle_altgr_release:
  mov byte [altgr_pressed], 0
  jmp .end_handler

.handle_ctrl_press:
  mov byte [ctrl_pressed], 1
  jmp .end_handler

.handle_ctrl_release:
  mov byte [ctrl_pressed], 0
  jmp .end_handler

; --- Extended key prefix (0xE0) ---
.handle_e0_prefix:
  mov byte [e0_pending], 1
  jmp .end_handler

; --- Extended key processing (second byte after 0xE0) ---
.handle_extended_key:
  mov byte [e0_pending], 0     ; Clear the pending flag

  ; Ignore key release (bit 7 set)
  cmp al, 0x80
  jae .end_handler

  ; Up arrow = 0x48 -> send 0x80 to buffer
  cmp al, 0x48
  je .arrow_up
  ; Down arrow = 0x50 -> send 0x81 to buffer
  cmp al, 0x50
  je .arrow_down
  ; Other extended keys: ignore for now
  jmp .end_handler

.arrow_up:
  mov eax, 0x80
  push eax
  call kbd_buffer_put
  add esp, 4
  jmp .end_handler

.arrow_down:
  mov eax, 0x81
  push eax
  call kbd_buffer_put
  add esp, 4
  jmp .end_handler

.end_handler:
  ; Enviar EOI al PIC
  mov al, 0x20
  out 0x20, al

  ; Restaurar registres i tornar de la interrupció
  popad
  iret

section .data
cursor_pos:   dd 0xb8000 + 160*17
current_line: dd 16
shift_pressed: db 0
altgr_pressed: db 0
ctrl_pressed:  db 0
e0_pending:    db 0