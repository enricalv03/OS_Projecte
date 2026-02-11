[bits 32]

section .text
global parse_command
global newline_from_cursor

extern print_string_pm
extern print_char_pm
extern cons_cursor
extern line_start
extern pmm_alloc_page
extern print_hex32_pm
extern pmm_get_free_pages
extern pmm_free_page
extern total_pages_value
extern scroll_up_one_row
extern clear_screen
extern vfs_read
extern vfs_readdir
extern ramfs_lookup_root

; C functions for new commands (CDECL)
extern pit_get_ticks
extern process_get_current
extern process_get_by_pid
extern ramfs_add_ram_file

; Language strings (English by default)
extern cmd_alloc_en, cmd_memstat_en, cmd_help_en, cmd_free_en, cmd_clear_en, cmd_language_en
extern cmd_ls_en, cmd_cat_en, cmd_disk_en
extern cmd_echo_en, cmd_uptime_en, cmd_ps_en, cmd_write_en
extern unknown_cmd_en, help_msg_en
extern memstat_free_msg_en, memstat_used_msg_en
extern alloc_success_msg_en, alloc_oom_msg_en
extern free_ok_msg_en, free_fail_msg_en, free_usage_msg_en
extern language_current_en, language_available_en, language_set_en
extern file_not_found_en
extern disk_error_en, disk_usage_en, disk_reading_en
extern uptime_msg_en, uptime_sec_en
extern ps_header_en
extern write_ok_en, write_usage_en, write_fail_en

; Language strings (Catalan)
extern cmd_alloc_ca, cmd_memstat_ca, cmd_help_ca, cmd_free_ca, cmd_clear_ca, cmd_language_ca
extern cmd_ls_ca, cmd_cat_ca, cmd_disk_ca
extern cmd_echo_ca, cmd_uptime_ca, cmd_ps_ca, cmd_write_ca
extern unknown_cmd_ca, help_msg_ca
extern memstat_free_msg_ca, memstat_used_msg_ca
extern alloc_success_msg_ca, alloc_oom_msg_ca
extern free_ok_msg_ca, free_fail_msg_ca, free_usage_msg_ca
extern language_current_ca, language_available_ca, language_set_ca
extern file_not_found_ca
extern disk_error_ca, disk_usage_ca, disk_reading_ca
extern uptime_msg_ca, uptime_sec_ca
extern ps_header_ca
extern write_ok_ca, write_usage_ca, write_fail_ca

; Language strings (Spanish)
extern cmd_alloc_es, cmd_memstat_es, cmd_help_es, cmd_free_es, cmd_clear_es, cmd_language_es
extern cmd_ls_es, cmd_cat_es, cmd_disk_es
extern cmd_echo_es, cmd_uptime_es, cmd_ps_es, cmd_write_es
extern unknown_cmd_es, help_msg_es
extern memstat_free_msg_es, memstat_used_msg_es
extern alloc_success_msg_es, alloc_oom_msg_es
extern free_ok_msg_es, free_fail_msg_es, free_usage_msg_es
extern language_current_es, language_available_es, language_set_es
extern file_not_found_es
extern disk_error_es, disk_usage_es, disk_reading_es
extern uptime_msg_es, uptime_sec_es
extern ps_header_es
extern write_ok_es, write_usage_es, write_fail_es

; C function from ata_test.c (CDECL)
extern ata_test_read_sector

parse_command:
  push eax
  push ebx
  push esi
  push edi

  ; -----------------------------------------------------------------------
  ; PRIORITY CHECK: Always try language-switching commands in ALL languages
  ; first, so the user can ALWAYS switch back regardless of current language.
  ; This prevents the "stuck in wrong language" problem.
  ; -----------------------------------------------------------------------
  mov ebx, language_alias_table

.lang_alias_next:
  mov edi, [ebx]
  test edi, edi
  je .try_normal            ; No alias matched -> fall through to normal table

  mov esi, command_buffer
  call strcmp
  cmp eax, 1
  je .lang_alias_matched

  add ebx, 4
  jmp .lang_alias_next

.lang_alias_matched:
  call handle_language
  jmp .done

  ; -----------------------------------------------------------------------
  ; Normal command dispatch: check the current language's command table.
  ; -----------------------------------------------------------------------
.try_normal:
  mov ebx, [current_command_table]

.next_entry:
  mov edi, [ebx]
  test edi, edi
  je .unknown

  mov edx, [ebx + 4]

  mov esi, command_buffer
  call strcmp
  cmp eax, 1
  je .call_handler

  add ebx, 8
  jmp .next_entry

.call_handler:
  call edx
  jmp .done

.unknown:
  mov esi, [current_unknown_cmd]
  call print_line

.done:
  pop edi
  pop esi
  pop ebx
  pop eax
  ret

strcmp:
  push ebx

.compare_loop:
  mov al, [esi]
  mov bl, [edi]
  
  cmp al, ' '
  je .check_end
  cmp al, 0
  je .check_end
  
  cmp al, bl
  jne .not_equal
  
  cmp bl, 0
  je .check_buffer_end
  
  inc esi
  inc edi
  jmp .compare_loop

.check_end:
  cmp bl, 0
  je .equal
  jmp .not_equal

.check_buffer_end:
  cmp al, ' '
  je .equal
  cmp al, 0
  je .equal
  jmp .not_equal

.equal:
  mov eax, 1
  jmp .done

.not_equal:
  mov eax, 0 

.done:
  pop ebx
  ret

print_line:
  push eax
  push ebx
  push edi

  mov edi, [cons_cursor]
  mov bl, 0x0F
  call print_string_pm
  mov [cons_cursor], edi     ; Update cursor to end of printed string

  call newline_from_cursor   ; Now calculates newline from correct position

  pop edi
  pop ebx
  pop eax
  ret

print_string_no_newline:
  push eax
  push ebx
  push edi

  mov edi, [cons_cursor]
  mov bl, 0x0F
  call print_string_pm
  mov [cons_cursor], edi

  pop edi
  pop ebx
  pop eax
  ret

newline_from_cursor:
  push eax
  push ebx
  push edx

  mov eax, [cons_cursor]
  sub eax, 0xb8000
  mov ebx, 160
  xor edx, edx
  div ebx
  inc eax
  
  cmp eax, 25
  jl .no_scroll
  
  call scroll_up_one_row

  mov eax, 24
  mul ebx
  add eax, 0xb8000
  mov [cons_cursor], eax
  jmp .done

.no_scroll:
  mul ebx
  add eax, 0xb8000
  mov [cons_cursor], eax

.done:
  pop edx
  pop ebx
  pop eax
  ret

parse_hex_argument:
  push ebx
  push edx
  push ecx

  mov ecx, edx

.skip_prefix:
  test ecx, ecx
  je .skip_spaces
  inc esi
  dec ecx
  jmp .skip_prefix

.skip_spaces:
  mov bl, [esi]
  cmp bl, ' '
  jne .check_prefix
  inc esi
  jmp .skip_spaces

.check_prefix:
  mov bl, [esi]
  cmp bl, '0'
  jne .init
  mov bl, [esi + 1]
  cmp bl, 'x'
  jne .init
  add esi, 2

.init:
  xor eax, eax
  mov ecx, 0            ; digit count

.next_char:
  mov bl, [esi]
  cmp bl, 0
  je .done
  cmp bl, ' '
  je .done
  cmp bl, '0'
  jb .invalid
  cmp bl, '9'
  jbe .digit
  cmp bl, 'A'
  jb .check_lower
  cmp bl, 'F'
  jbe .upper
  jmp .check_lower

.upper:
  sub bl, 'A'
  add bl, 10
  jmp .accumulate

.check_lower:
  cmp bl, 'a'
  jb .invalid
  cmp bl, 'f'
  ja .invalid
  sub bl, 'a'
  add bl, 10
  jmp .accumulate

.digit:
  sub bl, '0'

.accumulate:
  shl eax, 4
  movzx edx, bl
  add eax, edx
  inc ecx
  inc esi
  jmp .next_char

.done:
  cmp ecx, 0
  je .invalid
  clc
  pop ecx
  pop edx
  pop ebx
  ret

.invalid:
  stc
  pop ecx
  pop edx
  pop ebx
  ret

handle_alloc:
  push eax
  push esi
  push edi

  call pmm_alloc_page
  cmp eax, 0
  je .oom

  mov esi, [current_alloc_success_msg]
  call print_string_no_newline

  mov edi, [cons_cursor]
  mov bl, 0x0F
  call print_hex32_pm

  call newline_from_cursor
  jmp .done

.oom:
  mov esi, [current_alloc_oom_msg]
  call print_line

.done:
  pop edi
  pop esi
  pop eax
  ret

handle_help:
  push esi
  mov esi, [current_help_msg]
  call print_line
  pop esi
  ret

handle_clear:
  call clear_screen
  mov dword [cons_cursor], 0xb8000
  mov dword [line_start], 0xb8000
  ret

handle_memstat:
  push eax
  push ebx
  push esi
  push edi

  call pmm_get_free_pages
  mov ebx, eax

  ; Free pages
  mov esi, [current_memstat_free_msg]
  call print_string_no_newline
  mov eax, ebx
  mov edi, [cons_cursor]
  mov bl, 0x0F
  call print_hex32_pm
  call newline_from_cursor

  ; Used pages
  mov eax, [total_pages_value]
  sub eax, ebx
  mov esi, [current_memstat_used_msg]
  call print_string_no_newline
  mov edi, [cons_cursor]
  mov bl, 0x0F
  call print_hex32_pm
  call newline_from_cursor

  pop edi
  pop esi
  pop ebx
  pop eax
  ret

; ---------------------------------------------------------------------------
; File system commands: ls, cat
; ---------------------------------------------------------------------------

handle_ls:
  push eax
  push ebx
  push ecx
  push esi
  push edi

  ; Get VFS root node (CDECL: result in eax; preserves ebx)
  call vfs_get_root
  mov ebx, eax          ; ebx = root node pointer (CDECL-safe)
  xor ecx, ecx          ; ecx = directory entry index

.ls_next:
  ; Save index -- vfs_readdir is CDECL and may clobber ecx
  push ecx

  push dir_entry        ; arg3: output buffer
  push ecx              ; arg2: entry index
  push ebx              ; arg1: directory node
  call vfs_readdir
  add esp, 12

  ; Restore index
  pop ecx

  cmp eax, 0
  jne .ls_done          ; non-zero -> no more entries / error

  ; Print the entry name.  print_line saves/restores ebx internally,
  ; so the root node pointer is safe across this call.
  push ecx              ; save index across print_line
  mov esi, dir_entry
  call print_line
  pop ecx               ; restore index

  inc ecx               ; advance to next directory entry
  jmp .ls_next

.ls_done:
  pop edi
  pop esi
  pop ecx
  pop ebx
  pop eax
  ret

handle_cat:
  push eax
  push ebx
  push esi
  push edi

  ; Find first space in command_buffer to locate filename
  mov esi, command_buffer
.cat_skip_cmd:
  mov al, [esi]
  cmp al, ' '
  je .cat_after_space
  cmp al, 0
  je .cat_no_arg
  inc esi
  jmp .cat_skip_cmd

.cat_after_space:
  inc esi
.cat_skip_spaces:
  mov al, [esi]
  cmp al, ' '
  jne .cat_have_name
  inc esi
  jmp .cat_skip_spaces

.cat_no_arg:
  ; Nothing to print
  jmp .cat_done

.cat_have_name:
  ; esi points to filename (0-terminated by command parser)
  push filename_node
  push esi
  call ramfs_lookup_root
  add esp, 8
  cmp eax, 0
  jne .cat_not_found

  ; Read file into buffer
  mov eax, [filename_node]   ; node*
  push cat_buffer            ; buffer
  push 255                   ; size (leave space for 0)
  push 0                     ; offset
  push eax                   ; node
  call vfs_read
  add esp, 16

  cmp eax, 0
  jle .cat_done

  ; eax = bytes read, zero-terminate
  mov ebx, eax
  mov byte [cat_buffer + ebx], 0

  ; Print buffer
  mov edi, [cons_cursor]
  mov bl, 0x0F
  mov esi, cat_buffer
  call print_string_pm
  call newline_from_cursor
  jmp .cat_done

.cat_not_found:
  mov esi, [current_file_not_found_msg]
  call print_line

.cat_done:
  pop edi
  pop esi
  pop ebx
  pop eax
  ret

handle_free:
  push eax
  push esi
  push edi
  push edx

  mov esi, command_buffer
  ; Get command length from current language
  mov edi, [current_cmd_free]
  push esi
  mov esi, edi
  call get_string_length
  mov edx, eax
  pop esi
  call parse_hex_argument
  jc .bad_input

  call pmm_free_page
  cmp eax, 1
  jne .free_failed

  mov esi, [current_free_ok_msg]
  call print_line
  jmp .done

.free_failed:
  mov esi, [current_free_fail_msg]
  call print_line
  jmp .done

.bad_input:
  mov esi, [current_free_usage_msg]
  call print_line

.done:
  pop edx
  pop edi
  pop esi
  pop eax
  ret

; ---------------------------------------------------------------------------
; Disk command: disk <sector>  (hex dump of a disk sector)
; ---------------------------------------------------------------------------
handle_disk:
  push eax
  push esi
  push edi
  push edx

  ; Find first space in command_buffer to locate argument
  mov esi, command_buffer
.disk_skip_cmd:
  mov al, [esi]
  cmp al, ' '
  je .disk_after_space
  cmp al, 0
  je .disk_default       ; No argument -> default sector 0
  inc esi
  jmp .disk_skip_cmd

.disk_after_space:
  inc esi
.disk_skip_spaces:
  mov al, [esi]
  cmp al, ' '
  jne .disk_check_arg
  inc esi
  jmp .disk_skip_spaces

.disk_check_arg:
  cmp byte [esi], 0
  je .disk_default       ; Empty argument -> default sector 0

  ; Parse hex sector number (reuse parse_hex_argument with edx=0)
  xor edx, edx
  call parse_hex_argument
  jc .disk_bad_input
  ; EAX = parsed sector number
  jmp .disk_do_read

.disk_default:
  xor eax, eax          ; sector 0

.disk_do_read:
  ; Call ata_test_read_sector(sector) - CDECL
  push eax
  call ata_test_read_sector
  add esp, 4

  ; Check return value (0 = success, -1 = error)
  cmp eax, 0
  jne .disk_error
  jmp .disk_done

.disk_error:
  mov esi, [current_disk_error_msg]
  call print_line
  jmp .disk_done

.disk_bad_input:
  mov esi, [current_disk_usage_msg]
  call print_line

.disk_done:
  pop edx
  pop edi
  pop esi
  pop eax
  ret

get_string_length:
  push edi
  mov edi, esi
  xor eax, eax
.loop:
  cmp byte [edi], 0
  je .done
  inc eax
  inc edi
  jmp .loop
.done:
  pop edi
  ret

; ---------------------------------------------------------------------------
; skip_to_argument: Advance ESI past the command name and spaces.
; On entry: ESI = command_buffer
; On exit:  ESI = pointer to first non-space char after command,
;           or to a NUL byte if no argument was given.
; ---------------------------------------------------------------------------
skip_to_argument:
  push eax
.skip_cmd:
  mov al, [esi]
  cmp al, ' '
  je .found_space
  cmp al, 0
  je .done
  inc esi
  jmp .skip_cmd
.found_space:
  inc esi
.skip_spaces:
  mov al, [esi]
  cmp al, ' '
  jne .done
  inc esi
  jmp .skip_spaces
.done:
  pop eax
  ret

; ---------------------------------------------------------------------------
; print_decimal: Print unsigned 32-bit integer in EAX as decimal.
;                Uses cons_cursor and white text (0x0F).
; ---------------------------------------------------------------------------
print_decimal:
  push eax
  push ebx
  push ecx
  push edx
  push edi

  mov ecx, 0               ; digit count on stack
  mov ebx, 10

.divide:
  xor edx, edx
  div ebx                  ; EAX = quotient, EDX = remainder
  push edx                 ; save digit
  inc ecx
  test eax, eax
  jnz .divide

  ; Print digits (they're on the stack in reverse order)
  mov edi, [cons_cursor]
.print_digit:
  pop eax
  add al, '0'
  mov bl, 0x0F
  call print_char_pm
  dec ecx
  jnz .print_digit

  mov [cons_cursor], edi

  pop edi
  pop edx
  pop ecx
  pop ebx
  pop eax
  ret

; ===========================================================================
; NEW COMMANDS: echo, uptime, ps, write
; ===========================================================================

; ---------------------------------------------------------------------------
; handle_echo: Print everything after the command name.
;   echo Hello World  ->  "Hello World"
; ---------------------------------------------------------------------------
handle_echo:
  push eax
  push esi

  mov esi, command_buffer
  call skip_to_argument

  ; If no argument, just print a blank line
  cmp byte [esi], 0
  je .blank

  call print_line
  jmp .done

.blank:
  call newline_from_cursor

.done:
  pop esi
  pop eax
  ret

; ---------------------------------------------------------------------------
; handle_uptime: Show ticks and seconds since boot.
;   "Uptime: 12345 ticks (123 s)"
; ---------------------------------------------------------------------------
handle_uptime:
  push eax
  push ebx
  push edx
  push esi

  ; Print label
  mov esi, [current_uptime_msg]
  call print_string_no_newline

  ; Get tick count from PIT (CDECL, returns in eax)
  call pit_get_ticks
  mov ebx, eax              ; save ticks in ebx

  ; Print ticks as decimal
  call print_decimal

  ; Print " ticks ("
  mov esi, uptime_ticks_str
  call print_string_no_newline

  ; Calculate seconds: ticks / 100  (PIT fires at 100 Hz)
  mov eax, ebx
  xor edx, edx
  push ebx
  mov ebx, 100
  div ebx
  pop ebx
  ; EAX = seconds
  call print_decimal

  ; Print " s)"
  mov esi, [current_uptime_sec]
  call print_line

  pop esi
  pop edx
  pop ebx
  pop eax
  ret

; ---------------------------------------------------------------------------
; handle_ps: List running processes.
;   PID  STATE  PRI  NAME
;   0    RUN    10   kernel
; ---------------------------------------------------------------------------
handle_ps:
  push eax
  push ebx
  push ecx
  push edx
  push esi
  push edi

  ; Print header
  mov esi, [current_ps_header]
  call print_line

  ; Iterate PIDs 0..15 (enough to show active processes)
  xor ecx, ecx              ; PID counter

.ps_next:
  push ecx
  push ecx                  ; arg: pid
  call process_get_by_pid   ; CDECL -> eax = pcb* or 0
  add esp, 4
  pop ecx

  test eax, eax
  jz .ps_skip               ; no process at this PID

  ; EAX = pcb_t*. Layout:
  ;   +0  pid, +8 state, +12 priority, +180 name[32]
  mov ebx, eax              ; save pcb pointer

  ; Print PID
  mov eax, [ebx + 0]        ; pid
  call print_decimal

  ; Tab space
  mov esi, ps_tab
  call print_string_no_newline

  ; Print state
  mov eax, [ebx + 8]        ; state
  cmp eax, 0
  je .state_run
  cmp eax, 1
  je .state_ready
  cmp eax, 2
  je .state_block
  cmp eax, 3
  je .state_zombie
  mov esi, ps_state_dead
  jmp .print_state
.state_run:
  mov esi, ps_state_run
  jmp .print_state
.state_ready:
  mov esi, ps_state_ready
  jmp .print_state
.state_block:
  mov esi, ps_state_block
  jmp .print_state
.state_zombie:
  mov esi, ps_state_zombie
.print_state:
  call print_string_no_newline

  ; Print priority
  mov eax, [ebx + 12]       ; priority
  call print_decimal

  ; Tab space
  mov esi, ps_tab
  call print_string_no_newline

  ; Print name (at offset +180 in the PCB: 4*45 bytes before name[32])
  ; PCB layout: pid(4) parent_pid(4) state(4) priority(4) base_priority(4)
  ; registers: eax,ebx,ecx,edx,esi,edi,ebp,esp,eip,eflags,cs,ds,es,fs,gs,ss (16*4=64)
  ; page_dir(4) heap_start(4) heap_end(4) stack_top(4) stack_bottom(4) = 20
  ; vruntime(4) timeslice(4) total_runtime(4) queue_level(4) queue_ticks(4) = 20
  ; name[32] = offset 20+64+20+20 = 124
  lea esi, [ebx + 124]      ; name field
  call print_string_no_newline

  call newline_from_cursor

.ps_skip:
  inc ecx
  cmp ecx, 16
  jl .ps_next

  pop edi
  pop esi
  pop edx
  pop ecx
  pop ebx
  pop eax
  ret

; ---------------------------------------------------------------------------
; handle_write: Create a file in RAMFS.
;   write filename.txt This is the content
; ---------------------------------------------------------------------------
handle_write:
  push eax
  push ebx
  push ecx
  push esi
  push edi

  ; Skip past "write" (or translated equivalent) to get filename
  mov esi, command_buffer
  call skip_to_argument
  cmp byte [esi], 0
  je .write_usage

  ; ESI = filename start. Find end of filename (next space or NUL).
  mov edi, esi               ; save filename start
.write_find_space:
  mov al, [esi]
  cmp al, ' '
  je .write_got_name
  cmp al, 0
  je .write_usage            ; filename only, no content
  inc esi
  jmp .write_find_space

.write_got_name:
  ; NUL-terminate the filename in-place
  mov byte [esi], 0
  inc esi

  ; Skip spaces to get content start
.write_skip:
  cmp byte [esi], ' '
  jne .write_have_content
  inc esi
  jmp .write_skip

.write_have_content:
  cmp byte [esi], 0
  je .write_usage            ; no content after filename

  ; Calculate content length
  push esi
  mov ebx, esi               ; save content start
  xor ecx, ecx
.write_len:
  cmp byte [esi], 0
  je .write_len_done
  inc ecx
  inc esi
  jmp .write_len
.write_len_done:
  pop esi
  ; ECX = content length, ESI = content start, EDI = filename

  ; Check if we have a free slot in the persistent pool
  mov eax, [write_pool_idx]
  cmp eax, WRITE_SLOTS
  jge .write_failed          ; pool full

  ; Calculate destination address: write_pool + (slot * WRITE_SLOT_SIZE)
  push ecx
  push esi
  mov ebx, WRITE_SLOT_SIZE
  mul ebx                    ; EAX = slot * WRITE_SLOT_SIZE (note: clobbers EDX)
  lea ebx, [write_pool + eax]  ; EBX = destination
  pop esi
  pop ecx

  ; Clamp content to WRITE_SLOT_SIZE - 1
  cmp ecx, WRITE_SLOT_SIZE - 1
  jle .write_copy
  mov ecx, WRITE_SLOT_SIZE - 1

.write_copy:
  ; Copy content from ESI to EBX, ECX bytes
  push ecx                   ; save length for ramfs call
  push edi                   ; save filename
  push ebx                   ; save dest
  xor edx, edx
.write_copy_loop:
  cmp edx, ecx
  je .write_copy_done
  mov al, [esi + edx]
  mov [ebx + edx], al
  inc edx
  jmp .write_copy_loop
.write_copy_done:
  mov byte [ebx + edx], 0   ; NUL-terminate
  pop ebx                    ; dest
  pop edi                    ; filename
  pop ecx                    ; length

  ; Advance pool index
  mov eax, [write_pool_idx]
  inc eax
  mov [write_pool_idx], eax

  ; Call ramfs_add_ram_file(name, data, size) -- CDECL
  push ecx                   ; arg3: size
  push ebx                   ; arg2: persistent data pointer
  push edi                   ; arg1: filename pointer
  call ramfs_add_ram_file
  add esp, 12

  cmp eax, 0
  jne .write_failed

  mov esi, [current_write_ok]
  call print_line
  jmp .write_done

.write_usage:
  mov esi, [current_write_usage]
  call print_line
  jmp .write_done

.write_failed:
  mov esi, [current_write_fail]
  call print_line

.write_done:
  pop edi
  pop esi
  pop ecx
  pop ebx
  pop eax
  ret

handle_language:
  push eax
  push esi
  push edi
  push ebx

  ; Find first space in command_buffer to locate argument
  mov esi, command_buffer

.skip_to_space:
  mov bl, [esi]
  cmp bl, ' '
  je .found_space
  cmp bl, 0
  je .show_current
  inc esi
  jmp .skip_to_space

.found_space:
  inc esi  ; Skip the space itself

  ; Skip any additional spaces
.skip_spaces:
  mov bl, [esi]
  cmp bl, ' '
  jne .check_arg
  inc esi
  jmp .skip_spaces

.check_arg:
  mov bl, [esi]
  cmp bl, 0
  je .show_current

  ; -------------------------------------------------------------------
  ; Parse language code: accept 2-letter (en, es, ca) or 3-letter
  ; (eng, esp, cat) codes for user convenience.
  ; -------------------------------------------------------------------
  mov al, [esi]
  mov ah, [esi + 1]
  
  ; Check for Catalan: "ca" or "cat"
  cmp al, 'c'
  jne .check_en
  cmp ah, 'a'
  jne .invalid
  call set_language_ca
  jmp .done

  ; Check for English: "en" or "eng"
.check_en:
  cmp al, 'e'
  jne .invalid
  cmp ah, 'n'
  jne .check_es
  call set_language_en
  jmp .done

  ; Check for Spanish: "es" or "esp"
.check_es:
  cmp ah, 's'
  jne .invalid
  call set_language_es
  jmp .done

.show_current:
  mov esi, [current_language_current_msg]
  call print_line
  mov esi, [current_language_available_msg]
  call print_line
  jmp .done

.invalid:
  mov esi, [current_language_available_msg]
  call print_line

.done:
  pop ebx
  pop edi
  pop esi
  pop eax
  ret

set_language_en:
  mov dword [current_command_table], command_table_en
  mov dword [current_unknown_cmd], unknown_cmd_en
  mov dword [current_help_msg], help_msg_en
  mov dword [current_memstat_free_msg], memstat_free_msg_en
  mov dword [current_memstat_used_msg], memstat_used_msg_en
  mov dword [current_alloc_success_msg], alloc_success_msg_en
  mov dword [current_alloc_oom_msg], alloc_oom_msg_en
  mov dword [current_free_ok_msg], free_ok_msg_en
  mov dword [current_free_fail_msg], free_fail_msg_en
  mov dword [current_free_usage_msg], free_usage_msg_en
  mov dword [current_cmd_free], cmd_free_en
  mov dword [current_cmd_language], cmd_language_en
  mov dword [current_language_current_msg], language_current_en
  mov dword [current_language_available_msg], language_available_en
  mov dword [current_language_set_msg], language_set_en
  mov dword [current_file_not_found_msg], file_not_found_en
  mov dword [current_disk_error_msg], disk_error_en
  mov dword [current_disk_usage_msg], disk_usage_en
  mov dword [current_disk_reading_msg], disk_reading_en
  mov dword [current_uptime_msg], uptime_msg_en
  mov dword [current_uptime_sec], uptime_sec_en
  mov dword [current_ps_header], ps_header_en
  mov dword [current_write_ok], write_ok_en
  mov dword [current_write_usage], write_usage_en
  mov dword [current_write_fail], write_fail_en
  mov esi, language_set_en
  call print_string_no_newline
  mov esi, lang_en_name
  call print_line
  ret

set_language_ca:
  mov dword [current_command_table], command_table_ca
  mov dword [current_unknown_cmd], unknown_cmd_ca
  mov dword [current_help_msg], help_msg_ca
  mov dword [current_memstat_free_msg], memstat_free_msg_ca
  mov dword [current_memstat_used_msg], memstat_used_msg_ca
  mov dword [current_alloc_success_msg], alloc_success_msg_ca
  mov dword [current_alloc_oom_msg], alloc_oom_msg_ca
  mov dword [current_free_ok_msg], free_ok_msg_ca
  mov dword [current_free_fail_msg], free_fail_msg_ca
  mov dword [current_free_usage_msg], free_usage_msg_ca
  mov dword [current_cmd_free], cmd_free_ca
  mov dword [current_cmd_language], cmd_language_ca
  mov dword [current_language_current_msg], language_current_ca
  mov dword [current_language_available_msg], language_available_ca
  mov dword [current_language_set_msg], language_set_ca
  mov dword [current_file_not_found_msg], file_not_found_ca
  mov dword [current_disk_error_msg], disk_error_ca
  mov dword [current_disk_usage_msg], disk_usage_ca
  mov dword [current_disk_reading_msg], disk_reading_ca
  mov dword [current_uptime_msg], uptime_msg_ca
  mov dword [current_uptime_sec], uptime_sec_ca
  mov dword [current_ps_header], ps_header_ca
  mov dword [current_write_ok], write_ok_ca
  mov dword [current_write_usage], write_usage_ca
  mov dword [current_write_fail], write_fail_ca
  mov esi, language_set_ca
  call print_string_no_newline
  mov esi, lang_ca_name
  call print_line
  ret

set_language_es:
  mov dword [current_command_table], command_table_es
  mov dword [current_unknown_cmd], unknown_cmd_es
  mov dword [current_help_msg], help_msg_es
  mov dword [current_memstat_free_msg], memstat_free_msg_es
  mov dword [current_memstat_used_msg], memstat_used_msg_es
  mov dword [current_alloc_success_msg], alloc_success_msg_es
  mov dword [current_alloc_oom_msg], alloc_oom_msg_es
  mov dword [current_free_ok_msg], free_ok_msg_es
  mov dword [current_free_fail_msg], free_fail_msg_es
  mov dword [current_free_usage_msg], free_usage_msg_es
  mov dword [current_cmd_free], cmd_free_es
  mov dword [current_cmd_language], cmd_language_es
  mov dword [current_language_current_msg], language_current_es
  mov dword [current_language_available_msg], language_available_es
  mov dword [current_language_set_msg], language_set_es
  mov dword [current_file_not_found_msg], file_not_found_es
  mov dword [current_disk_error_msg], disk_error_es
  mov dword [current_disk_usage_msg], disk_usage_es
  mov dword [current_disk_reading_msg], disk_reading_es
  mov dword [current_uptime_msg], uptime_msg_es
  mov dword [current_uptime_sec], uptime_sec_es
  mov dword [current_ps_header], ps_header_es
  mov dword [current_write_ok], write_ok_es
  mov dword [current_write_usage], write_usage_es
  mov dword [current_write_fail], write_fail_es
  mov esi, language_set_es
  call print_string_no_newline
  mov esi, lang_es_name
  call print_line
  ret

; ---------------------------------------------------------------------------
; Language alias table: ALL names for the language-switching command across
; every supported language. The parser checks this table FIRST so users
; can always switch languages even if they don't know the current language's
; command name. Each entry is a pointer to a command string; the list ends
; with a NULL sentinel.
; ---------------------------------------------------------------------------
language_alias_table:
  dd cmd_language_en    ; "language"  (English)
  dd cmd_language_ca    ; "idioma"    (Catalan)
  dd cmd_language_es    ; "idioma"    (Spanish)
  dd lang_alias_lang    ; "lang"      (universal shortcut)
  dd 0                  ; end sentinel

; Command tables for each language
command_table_en:
  dd cmd_alloc_en, handle_alloc
  dd cmd_help_en, handle_help
  dd cmd_memstat_en, handle_memstat
  dd cmd_free_en, handle_free
  dd cmd_clear_en, handle_clear
  dd cmd_language_en, handle_language
  dd cmd_ls_en, handle_ls
  dd cmd_cat_en, handle_cat
  dd cmd_disk_en, handle_disk
  dd cmd_echo_en, handle_echo
  dd cmd_uptime_en, handle_uptime
  dd cmd_ps_en, handle_ps
  dd cmd_write_en, handle_write
  dd 0, 0

command_table_ca:
  dd cmd_alloc_ca, handle_alloc
  dd cmd_help_ca, handle_help
  dd cmd_memstat_ca, handle_memstat
  dd cmd_free_ca, handle_free
  dd cmd_clear_ca, handle_clear
  dd cmd_language_ca, handle_language
  dd cmd_ls_ca, handle_ls
  dd cmd_cat_ca, handle_cat
  dd cmd_disk_ca, handle_disk
  dd cmd_echo_ca, handle_echo
  dd cmd_uptime_ca, handle_uptime
  dd cmd_ps_ca, handle_ps
  dd cmd_write_ca, handle_write
  dd 0, 0

command_table_es:
  dd cmd_alloc_es, handle_alloc
  dd cmd_help_es, handle_help
  dd cmd_memstat_es, handle_memstat
  dd cmd_free_es, handle_free
  dd cmd_clear_es, handle_clear
  dd cmd_language_es, handle_language
  dd cmd_ls_es, handle_ls
  dd cmd_cat_es, handle_cat
  dd cmd_disk_es, handle_disk
  dd cmd_echo_es, handle_echo
  dd cmd_uptime_es, handle_uptime
  dd cmd_ps_es, handle_ps
  dd cmd_write_es, handle_write
  dd 0, 0

section .bss
global command_buffer
global cmd_pos

command_buffer: resb 64
cmd_pos: resd 1

; Buffers used by file commands
dir_entry:      resb 36      ; 32 name + 4 type
filename_node:  resd 1
cat_buffer:     resb 256

; Persistent buffer pool for user-written files (write command)
; Each file gets a 64-byte slot to store its content persistently.
WRITE_SLOTS     equ 16
WRITE_SLOT_SIZE equ 64
write_pool:     resb WRITE_SLOTS * WRITE_SLOT_SIZE
write_pool_idx: resd 1       ; next free slot index

section .data
global current_command_table
global current_unknown_cmd
global current_help_msg
global current_memstat_free_msg
global current_memstat_used_msg
global current_alloc_success_msg
global current_alloc_oom_msg
global current_free_ok_msg
global current_free_fail_msg
global current_free_usage_msg
global current_cmd_free
global current_cmd_language
global current_language_current_msg
global current_language_available_msg
global current_language_set_msg
global current_file_not_found_msg
global current_disk_error_msg
global current_disk_usage_msg
global current_disk_reading_msg

; Pointers to current language strings (initialized to English)
current_command_table: dd command_table_en
current_unknown_cmd: dd unknown_cmd_en
current_help_msg: dd help_msg_en
current_memstat_free_msg: dd memstat_free_msg_en
current_memstat_used_msg: dd memstat_used_msg_en
current_alloc_success_msg: dd alloc_success_msg_en
current_alloc_oom_msg: dd alloc_oom_msg_en
current_free_ok_msg: dd free_ok_msg_en
current_free_fail_msg: dd free_fail_msg_en
current_free_usage_msg: dd free_usage_msg_en
current_cmd_free: dd cmd_free_en
current_cmd_language: dd cmd_language_en
current_language_current_msg: dd language_current_en
current_language_available_msg: dd language_available_en
current_language_set_msg: dd language_set_en
current_file_not_found_msg: dd file_not_found_en
current_disk_error_msg: dd disk_error_en
current_disk_usage_msg: dd disk_usage_en
current_disk_reading_msg: dd disk_reading_en

; VFS entry points
extern vfs_get_root

; Universal shortcut for language switching (always works)
lang_alias_lang: db 'lang', 0

; Language names for display
lang_en_name: db 'English', 0
lang_ca_name: db 'Catala', 0
lang_es_name: db 'Espanol', 0

; Shared formatting strings (language-independent)
uptime_ticks_str: db ' ticks (', 0
ps_tab:           db '    ', 0
ps_state_run:     db 'RUN     ', 0
ps_state_ready:   db 'READY   ', 0
ps_state_block:   db 'BLOCK   ', 0
ps_state_zombie:  db 'ZOMBIE  ', 0
ps_state_dead:    db 'DEAD    ', 0

; Pointers to new command messages (initialized to English)
current_uptime_msg:    dd uptime_msg_en
current_uptime_sec:    dd uptime_sec_en
current_ps_header:     dd ps_header_en
current_write_ok:      dd write_ok_en
current_write_usage:   dd write_usage_en
current_write_fail:    dd write_fail_en
