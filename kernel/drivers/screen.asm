[bits 32]

VIDEO_MEMORY equ 0xb8000
SCREEN_WIDTH equ 80
SCREEN_HEIGHT equ 25

global print_string_pm
global print_char_pm
global clear_screen
global print_hex32_pm
global scroll_up_one_row

print_string_pm:
  lodsb
  cmp al, 0
  je .done
  mov [edi], al
  mov [edi+1], bl
  add edi, 2
  jmp print_string_pm
.done:
  ret

print_char_pm:
  mov [edi], al
  mov [edi+1], bl
  add edi, 2
  ret
  
clear_screen:
  mov edi, VIDEO_MEMORY
  mov ecx, 2000
.loop:
  mov byte [edi], ' '          ; Space character
  mov byte [edi+1], 0x07       ; Light gray on black (standard VGA text attribute)
  add edi, 2
  dec ecx
  jnz .loop
  ret

print_hex32_pm:
  push ecx
  push edx
  push esi

  mov edx, eax
  mov ecx, 8
  mov esi, hex_digits
.hex_loop:
  mov eax, edx
  shr eax, 28
  mov al, [esi + eax]
  call print_char_pm
  shl edx, 4
  dec ecx
  jnz .hex_loop

  pop esi
  pop edx
  pop ecx
  ret

scroll_up_one_row:
  push eax
  push ecx
  push esi
  push edi

  ; Copy row 1 to row 0, row 2 to row 1, etc.
  mov esi, VIDEO_MEMORY + 160    ; Start from row 1
  mov edi, VIDEO_MEMORY          ; Copy to row 0
  mov ecx, (SCREEN_HEIGHT - 1) * SCREEN_WIDTH  ; 24 rows * 80 chars

.copy_loop:
  mov eax, [esi]
  mov [edi], eax
  add esi, 4
  add edi, 4
  loop .copy_loop

  ; Clear last row
  mov edi, VIDEO_MEMORY + 160 * (SCREEN_HEIGHT - 1)
  mov ecx, SCREEN_WIDTH
  mov ax, 0x0720  ; Space with default attribute

.clear_loop:
  mov [edi], ax
  add edi, 2
  loop .clear_loop

  pop edi
  pop esi
  pop ecx
  pop eax
  ret

section .data
hex_digits: db '0123456789ABCDEF'