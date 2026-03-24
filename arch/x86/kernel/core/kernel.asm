[bits 32]
section .text

extern print_char_pm
extern print_string_pm
extern clear_screen
extern keyboard_handler
extern idt_install
extern arch_enable_interrupts
extern arch_disable_interrupts
extern arch_idle
extern arch_halt
extern set_idt_entry
extern set_idt_entry_user
extern init_keyboard_layouts
extern dummy_interrupt
extern idt_table
extern isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7
extern isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15
extern isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23
extern isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
extern pit_handler
extern kbd_buffer_get
extern read_memory_map
extern pmm_dump_region_usage
extern print_hex32_pm
extern setup_paging
extern pmm_init
extern pmm_alloc_page
extern pmm_free_page
extern ramfs_init
extern update_hw_cursor_from_ptr
extern enable_cursor
extern parse_command
extern command_buffer
extern cmd_pos
extern kernel_c_init
extern syscall_handler
extern newline_from_cursor
extern process_get_uid
extern vfs_get_cwd_path
extern tab_complete
extern tab_reset

; BSS boundary symbols from linker.ld
extern __bss_start
extern __bss_end

global start
start:

; Set up segments for kernel (same as stage2 DATA_SEG)
mov ax, 0x10     ; Data segment selector
mov ds, ax
mov es, ax
mov fs, ax
mov gs, ax
mov esp, 0x90000 ; Set up stack (same as stage2)

; =========================================================================
; Zero the BSS section.
; C static variables default to 0, but since we're freestanding (no OS
; loaded us properly), BSS contains whatever garbage was in RAM.
; We MUST zero it before calling any C code.
; =========================================================================
mov edi, __bss_start
mov ecx, __bss_end
sub ecx, edi            ; ECX = BSS size in bytes
shr ecx, 2             ; ECX = size in dwords (divide by 4)
xor eax, eax           ; EAX = 0
rep stosd              ; Fill BSS with zeros

; Clear screen and enable the blinking hardware cursor
call clear_screen
call enable_cursor

mov esi, kernel_msg
mov edi, 0xb8000 + 160*0  ; Row 0
mov bl, 0x0A
call print_string_pm

; Set up paging (must be done before IDT)
call setup_paging

mov esi, paging_msg
mov edi, 0xb8000 + 160*1  ; Row 1
mov bl, 0x0B
call print_string_pm

call idt_install

mov eax, 0
mov ebx, isr0
call set_idt_entry

mov eax, 1
mov ebx, isr1
call set_idt_entry

mov eax, 2
mov ebx, isr2
call set_idt_entry

mov eax, 3
mov ebx, isr3
call set_idt_entry

mov eax, 4
mov ebx, isr4
call set_idt_entry

mov eax, 5
mov ebx, isr5
call set_idt_entry

mov eax, 6
mov ebx, isr6
call set_idt_entry

mov eax, 7
mov ebx, isr7
call set_idt_entry

mov eax, 8
mov ebx, isr8
call set_idt_entry

mov eax, 9
mov ebx, isr9
call set_idt_entry

mov eax, 10
mov ebx, isr10
call set_idt_entry

mov eax, 11
mov ebx, isr11
call set_idt_entry

mov eax, 12
mov ebx, isr12
call set_idt_entry

mov eax, 13
mov ebx, isr13
call set_idt_entry

mov eax, 14
mov ebx, isr14
call set_idt_entry

mov eax, 15
mov ebx, isr15
call set_idt_entry

mov eax, 16
mov ebx, isr16
call set_idt_entry

mov eax, 17
mov ebx, isr17
call set_idt_entry

mov eax, 18
mov ebx, isr18
call set_idt_entry

mov eax, 19
mov ebx, isr19
call set_idt_entry

mov eax, 20
mov ebx, isr20
call set_idt_entry

mov eax, 21
mov ebx, isr21
call set_idt_entry

mov eax, 22
mov ebx, isr22
call set_idt_entry

mov eax, 23
mov ebx, isr23
call set_idt_entry

mov eax, 24
mov ebx, isr24
call set_idt_entry

mov eax, 25
mov ebx, isr25
call set_idt_entry

mov eax, 26
mov ebx, isr26
call set_idt_entry

mov eax, 27
mov ebx, isr27
call set_idt_entry

mov eax, 28
mov ebx, isr28
call set_idt_entry

mov eax, 29
mov ebx, isr29
call set_idt_entry

mov eax, 30
mov ebx, isr30
call set_idt_entry

mov eax, 31
mov ebx, isr31
call set_idt_entry

mov eax, 33
mov ebx, keyboard_handler
call set_idt_entry

; Initialize keyboard layout tables
call init_keyboard_layouts

call arch_disable_interrupts

mov al, 0x11
out 0x20, al
out 0xA0, al

mov al, 0x20
out 0x21, al
mov al, 0x28
out 0xA1, al

mov al, 0x04
out 0x21, al
mov al, 0x02
out 0xA1, al

mov al, 0x01
out 0x21, al
mov al, 0x01
out 0xA1, al

mov al, 0xFC
out 0x21, al
mov al, 0xFF
out 0xA1, al

mov eax, 32
mov ebx, pit_handler
call set_idt_entry

;PIT 100HZ
mov al, 0x36
out 0x43, al
mov ax, 11932
out 0x40, al
mov al, ah
out 0x40, al

; =========================================================================
; ALL SUBSYSTEM INITIALIZATION -- interrupts still disabled (CLI from PIC setup)
; We must finish ALL init before STI so the PIT handler doesn't fire on
; uninitialized scheduler/process state.
; =========================================================================

; Read E820 memory map (data was placed at 0x2000 by stage2)
push memory_map_buffer
call read_memory_map
add esp, 4

; Initialize Physical Memory Manager (bitmap at 0x101000)
call pmm_init

; Initialize in-memory filesystem and mount as VFS root
call ramfs_init

; Register INT 0x80 as the system call gate, callable from user mode (DPL=3)
mov eax, 0x80
mov ebx, syscall_handler
call set_idt_entry_user

; Initialize C subsystems: heap, scheduler, processes, syscalls, block I/O
; This MUST complete before STI so scheduler_tick() has valid state.
call kernel_c_init

; =========================================================================
; NOW safe to enable interrupts -- scheduler is initialized
; =========================================================================
call arch_enable_interrupts

; --- Display boot info (cosmetic, interrupts are now running) ---

mov eax, [memory_map_buffer]
mov bl, 0x0E
mov edi, 0xb8000 + 160*2
call print_hex32_pm

; Test PMM: Allocate 3 pages and print their addresses
; (This will be shown after memory regions, before separator)
mov esi, pmm_test_msg
mov edi, 0xb8000 + 160*11   ; Row 11 (after memory regions)
mov bl, 0x0E                 ; Yellow
call print_string_pm

; Allocate first page
call pmm_alloc_page
push edi
push ebx
mov edi, 0xb8000 + 160*12   ; Row 12
mov bl, 0x0F                 ; White
call print_hex32_pm
mov al, ' '
call print_char_pm
pop ebx
pop edi

; Allocate second page
call pmm_alloc_page
push edi
push ebx
mov edi, 0xb8000 + 160*12 + 10*2  ; Continue on same row
mov bl, 0x0F
call print_hex32_pm
mov al, ' '
call print_char_pm
pop ebx
pop edi

; Allocate third page
call pmm_alloc_page
push edi
push ebx
mov edi, 0xb8000 + 160*12 + 20*2  ; Continue on same row
mov bl, 0x0F
call print_hex32_pm
pop ebx
pop edi

; Print header at row 3 (row 2 is empty separator)
mov esi, mem_header
mov edi, 0xb8000 + 160*3    ; Row 3: "Memory Regions: "
mov bl, 0x0F
call print_string_pm

; Get memory entry count and limit to 5
mov ebp, [memory_map_buffer]  ; Load count
test ebp, ebp
je .no_entries

; Start displaying entries at row 4
lea esi, [memory_map_buffer + 4]
mov edx, 4                   ; Start at row 4

.display_loop:
    ; Calculate video memory address for this row
    push eax
    push ebx
    push edx

    mov eax, edx             ; Current row number
    mov ebx, 160
    mul ebx                  ; eax = row * 160
    add eax, 0xb8000         ; eax = video memory address for this row
    mov edi, eax             ; Set destination
    mov bl, 0x0F             ; White text
    
    pop edx
    pop ebx
    pop eax
    
    ; Display base address
    push ebx
    mov eax, [esi + 0]       ; Low base addr 32 bit
    mov bl, 0x0F
    call print_hex32_pm
    pop ebx
    
    ; Space
    push ebx
    mov al, ' '
    mov bl, 0x0F
    call print_char_pm
    pop ebx
    
    ; Display length
    push ebx
    mov eax, [esi + 8]      ; low length 32
    mov bl, 0x0F
    call print_hex32_pm
    pop ebx
    
    ; Space
    push ebx
    mov al, ' '
    mov bl, 0x0F
    call print_char_pm
    pop ebx
    
    ; Display type
    push ebx
    mov eax, [esi + 16]      ; type
    mov bl, 0x0F
    call print_hex32_pm
    pop ebx
    
    ; Move to next entry
    add esi, 24              ; Each entry is 24 bytes
    inc edx                  ; Next row
    dec ebp
    jnz .display_loop

.no_entries:
; After printing the memory map, show a compact PMM free/used summary.
; This keeps the summary from being overwritten by the last E820 entry.
call pmm_dump_region_usage

; Draw a separator line before console (row 13, after PMM test)
mov edi, 0xb8000 + 160*13
mov bl, 0x07
mov ecx, 80
mov al, '-'
.draw_separator:
    call print_char_pm
    loop .draw_separator

; Print the initial prompt before accepting input
call print_prompt
mov eax, [cons_cursor]
mov [line_start], eax
call update_hw_cursor_from_ptr

console_loop:
  call kbd_buffer_get
  cmp eax, -1
  je .idle

  cmp al, 10
  je .handle_enter

  cmp al, 8
  je .handle_backspace

  ; Arrow keys: up=0x80, down=0x81 (special codes from keyboard handler)
  cmp al, 0x80
  je .handle_history_up
  cmp al, 0x81
  je .handle_history_down

  cmp al, 9
  je .handle_tab

  ; Any non-Tab key resets the tab cycling state
  call tab_reset

  mov ebx, [cmd_pos]
  cmp ebx, 63
  jge console_loop

  mov [command_buffer + ebx], al
  inc dword [cmd_pos]

  mov edi, [cons_cursor]
  mov bl, 0x0F
  call print_char_pm
  add dword [cons_cursor], 2

  ; Update blinking hardware cursor to new position
  mov eax, [cons_cursor]
  call update_hw_cursor_from_ptr

  jmp console_loop

; ---------------------------------------------------------------------------
; Tab completion
; ---------------------------------------------------------------------------
.handle_tab:
  ; NUL-terminate current buffer content
  mov ebx, [cmd_pos]
  mov byte [command_buffer + ebx], 0

  ; tab_complete(command_buffer, cmd_pos) → returns new cmd_pos in EAX
  push dword [cmd_pos]
  push command_buffer
  call tab_complete
  add esp, 8

  ; If cmd_pos unchanged, nothing to redraw
  cmp eax, [cmd_pos]
  je console_loop

  ; Save new cmd_pos from tab_complete
  mov [cmd_pos], eax

  ; Erase visible text on screen (without touching command_buffer)
  mov eax, [cons_cursor]
  sub eax, [line_start]
  shr eax, 1
  mov ecx, eax
  test ecx, ecx
  jz .tab_redraw
  mov edi, [line_start]
  mov [cons_cursor], edi
.tab_erase:
  mov byte [edi], ' '
  mov byte [edi + 1], 0x07
  add edi, 2
  dec ecx
  jnz .tab_erase
  mov edi, [line_start]
  mov [cons_cursor], edi

.tab_redraw:
  ; Print the completed buffer on screen
  mov esi, command_buffer
  mov edi, [cons_cursor]
  mov bl, 0x0F
  call print_string_pm
  mov [cons_cursor], edi

  mov eax, [cons_cursor]
  call update_hw_cursor_from_ptr
  jmp console_loop

.newline:
  ; Scroll-safe newline (matches .handle_enter fix)
  call newline_from_cursor
  mov eax, [cons_cursor]
  call update_hw_cursor_from_ptr
  jmp console_loop

.handle_enter:
  mov ebx, [cmd_pos]
  mov byte [command_buffer + ebx], 0

  ; Save to command history (only if non-empty)
  cmp dword [cmd_pos], 0
  je .skip_history_save
  call history_save
.skip_history_save:

  ; Move to next line -- uses scroll-aware helper that scrolls
  ; the screen up when reaching the bottom (row 25).
  call newline_from_cursor

  ; Only parse if non-empty
  cmp dword [cmd_pos], 0
  je .skip_parse
  ; Run command handlers atomically in kernel context.
  ; This avoids timer-preemption/context-switch while parsing/executing
  ; shell commands (which can corrupt command-local state).
  call arch_disable_interrupts
  call parse_command
  call arch_enable_interrupts
.skip_parse:

  mov dword [cmd_pos], 0
  ; Reset history browse position
  mov eax, [history_count]
  mov [history_browse], eax

  ; Print prompt for the next command
  call print_prompt
  ; Record the start of the new input line (after prompt)
  mov eax, [cons_cursor]
  mov [line_start], eax
  call update_hw_cursor_from_ptr
  jmp console_loop

.handle_backspace:
  ; Don't backspace if command buffer is empty
  mov ebx, [cmd_pos]
  cmp ebx, 0
  je console_loop

  ; Don't backspace past the start of the current input line
  mov eax, [cons_cursor]
  cmp eax, [line_start]
  jle console_loop

  ; Remove last character from command buffer
  dec dword [cmd_pos]
  mov ebx, [cmd_pos]
  mov byte [command_buffer + ebx], 0

  ; Move screen cursor back and erase the character
  sub dword [cons_cursor], 2
  mov edi, [cons_cursor]
  mov al, ' '
  mov bl, 0x0F
  call print_char_pm

  ; Update hardware cursor to the new position
  mov eax, [cons_cursor]
  call update_hw_cursor_from_ptr

  jmp console_loop

; ---------------------------------------------------------------------------
; History Up (recall previous command)
; ---------------------------------------------------------------------------
.handle_history_up:
  mov eax, [history_browse]
  cmp eax, 0
  je console_loop              ; already at oldest entry
  dec eax
  mov [history_browse], eax
  call clear_input_line
  call history_recall
  jmp console_loop

; ---------------------------------------------------------------------------
; History Down (recall newer command)
; ---------------------------------------------------------------------------
.handle_history_down:
  mov eax, [history_browse]
  cmp eax, [history_count]
  jge .history_down_clear      ; past newest -> clear line
  inc eax
  mov [history_browse], eax
  cmp eax, [history_count]
  je .history_down_clear       ; at end -> clear line
  call clear_input_line
  call history_recall
  jmp console_loop
.history_down_clear:
  mov eax, [history_count]
  mov [history_browse], eax
  call clear_input_line
  jmp console_loop

.idle:
  call arch_idle
  jmp console_loop

; ---------------------------------------------------------------------------
; print_prompt: Prints "admin@MyOS:/path> " or "user@MyOS:/path> " in green
; ---------------------------------------------------------------------------
global print_prompt
print_prompt:
  push eax
  push ebx
  push esi
  push edi

  call process_get_uid
  cmp eax, 0
  je .is_root
  mov esi, prompt_user
  jmp .print_prefix
.is_root:
  mov esi, prompt_root
.print_prefix:
  mov edi, [cons_cursor]
  mov bl, 0x0A
  call print_string_pm
  mov [cons_cursor], edi

  ; Print ':' then current path from C
  mov edi, [cons_cursor]
  mov al, ':'
  mov bl, 0x0A
  call print_char_pm
  add edi, 2
  mov [cons_cursor], edi

  call vfs_get_cwd_path
  mov esi, eax
  mov edi, [cons_cursor]
  mov bl, 0x0A
  call print_string_pm
  mov [cons_cursor], edi

  ; Print "> "
  mov esi, prompt_tail
  mov edi, [cons_cursor]
  mov bl, 0x0A
  call print_string_pm
  mov [cons_cursor], edi

  pop edi
  pop esi
  pop ebx
  pop eax
  ret

; ---------------------------------------------------------------------------
; history_save: Save command_buffer into history ring
; ---------------------------------------------------------------------------
history_save:
  push eax
  push ecx
  push esi
  push edi

  ; Destination slot = history_count % HISTORY_MAX
  mov eax, [history_count]
  cmp eax, HISTORY_MAX
  jl .hist_no_wrap
  ; Ring buffer: shift all entries up by one
  mov esi, history_ring + 64       ; source = slot 1
  mov edi, history_ring            ; dest   = slot 0
  mov ecx, (HISTORY_MAX - 1) * 16  ; copy N-1 entries (dwords)
  rep movsd
  mov eax, HISTORY_MAX - 1         ; write into last slot
  jmp .hist_do_copy

.hist_no_wrap:
  ; eax = index of next free slot

.hist_do_copy:
  ; Calculate destination address: history_ring + eax*64
  mov ecx, 64
  push eax
  mul ecx
  mov edi, history_ring
  add edi, eax
  pop eax

  ; Copy command_buffer to the slot (64 bytes)
  mov esi, command_buffer
  mov ecx, 16                 ; 64 bytes / 4 = 16 dwords
  rep movsd

  ; Update count (capped at HISTORY_MAX)
  mov eax, [history_count]
  cmp eax, HISTORY_MAX
  jge .hist_save_done
  inc dword [history_count]

.hist_save_done:
  pop edi
  pop esi
  pop ecx
  pop eax
  ret

; ---------------------------------------------------------------------------
; history_recall: Copy history entry [history_browse] into command_buffer
;                 and display it on screen.
; ---------------------------------------------------------------------------
history_recall:
  push eax
  push ecx
  push esi
  push edi

  ; Source = history_ring + history_browse * 64
  mov eax, [history_browse]
  mov ecx, 64
  mul ecx
  mov esi, history_ring
  add esi, eax

  ; Copy 64 bytes to command_buffer
  mov edi, command_buffer
  mov ecx, 16
  rep movsd

  ; Count string length for cmd_pos
  mov esi, command_buffer
  xor ecx, ecx
.hist_len:
  cmp byte [esi + ecx], 0
  je .hist_len_done
  inc ecx
  cmp ecx, 63
  jge .hist_len_done
  jmp .hist_len
.hist_len_done:
  mov [cmd_pos], ecx

  ; Print the string on screen
  mov esi, command_buffer
  mov edi, [cons_cursor]
  mov bl, 0x0F
  call print_string_pm
  mov [cons_cursor], edi

  ; Update hardware cursor
  mov eax, [cons_cursor]
  call update_hw_cursor_from_ptr

  pop edi
  pop esi
  pop ecx
  pop eax
  ret

; ---------------------------------------------------------------------------
; clear_input_line: Erase current input from screen, reset cmd_pos
; ---------------------------------------------------------------------------
clear_input_line:
  push eax
  push ecx
  push edi

  ; Calculate how many chars to erase (current pos - line_start)
  mov eax, [cons_cursor]
  sub eax, [line_start]
  shr eax, 1                  ; bytes -> character count
  mov ecx, eax
  test ecx, ecx
  jz .clear_done

  ; Move cursor back to line_start and fill with spaces
  mov edi, [line_start]
  mov [cons_cursor], edi
.clear_loop:
  mov byte [edi], ' '
  mov byte [edi + 1], 0x07
  add edi, 2
  dec ecx
  jnz .clear_loop

  ; Reset cursor position and command buffer
  mov dword [cmd_pos], 0
  mov byte [command_buffer], 0

  ; Update hardware cursor
  mov eax, [cons_cursor]
  call update_hw_cursor_from_ptr

.clear_done:
  pop edi
  pop ecx
  pop eax
  ret

kernel_main:
    ; Kernel main loop - halt forever (arch-specific)
    call arch_halt
    jmp kernel_main

section .data
global cons_cursor
global line_start

kernel_msg: db 'Kernel started successfully! Welcome to MyOS!', 0
paging_msg: db 'Paging enabled!', 0
debug_msg1: db 'Before STI', 0
debug_msg2: db 'After STI', 0
cons_cursor: dd 0xb8000 + 160*14
line_start:  dd 0xb8000 + 160*14   ; Start of current input line (backspace boundary)
memmap_kernel_msb: db 'Kernel memory map: ', 0
mem_header: db 'Memory Regions: ', 0
pmm_test_msg: db 'PMM Test - Allocated pages: ', 0
prompt_string: db 'MyOS> ', 0
prompt_root:   db 'admin@MyOS', 0
prompt_user:   db 'user@MyOS', 0
prompt_tail:   db '> ', 0

; History constants
HISTORY_MAX equ 8

; History state (in .data so they get initialized)
history_count:  dd 0               ; number of entries stored (max HISTORY_MAX)
history_browse: dd 0               ; current browse index (for up/down arrow)

section .bss
memory_map_buffer: resb 1024
; History ring buffer: HISTORY_MAX entries of 64 bytes each
history_ring: resb HISTORY_MAX * 64