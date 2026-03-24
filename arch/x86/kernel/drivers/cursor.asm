[bits 32]

global set_hw_cursor
global update_hw_cursor_from_ptr
global enable_cursor

; =============================================================================
; enable_cursor -- makes the VGA text-mode cursor visible
; =============================================================================
; Programs the Cursor Start / End registers (CRTC indices 0x0A, 0x0B)
; to show an underline cursor (scanlines 13-15).
;
; Bit 5 of the Start register = 0 means cursor is ENABLED.
; Must be called at boot since protected mode may leave these in
; an undefined state.
; =============================================================================
enable_cursor:
    push edx

    ; Cursor Start Register (index 0x0A)
    ; Value: scanline 13, bit 5 = 0 (cursor enabled)
    mov dx, 0x3D4
    mov al, 0x0A
    out dx, al
    mov dx, 0x3D5
    mov al, 13               ; Start scanline 13, enable bit clear
    out dx, al

    ; Cursor End Register (index 0x0B)
    ; Value: scanline 15
    mov dx, 0x3D4
    mov al, 0x0B
    out dx, al
    mov dx, 0x3D5
    mov al, 15               ; End scanline 15
    out dx, al

    pop edx
    ret

; Set hardware text-mode cursor position.
; Input: EAX = character cell index (row * 80 + col)
set_hw_cursor:
  push edx
  push ecx

  mov ecx, eax          ; save index in CX (fits in 16 bits: 0..2000)

  ; Low byte of cursor index
  mov dx, 0x3D4
  mov al, 0x0F
  out dx, al

  mov dx, 0x3D5
  mov al, cl            ; low 8 bits of index
  out dx, al

  ; High byte of cursor index
  mov dx, 0x3D4
  mov al, 0x0E
  out dx, al

  mov dx, 0x3D5
  mov al, ch            ; high 8 bits of index
  out dx, al

  pop ecx
  pop edx
  ret

; Convenience: update cursor from a video memory pointer.
; Input: EAX = address in 0xB8000 text buffer
update_hw_cursor_from_ptr:
  sub eax, 0xb8000
  shr eax, 1
  jmp set_hw_cursor