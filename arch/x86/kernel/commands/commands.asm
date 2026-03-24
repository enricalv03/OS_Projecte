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
extern vfs_lookup
extern vfs_get_root
extern vfs_get_cwd
extern vfs_set_cwd
extern vfs_dir_entry_get_type
extern ramfs_lookup_root

; C functions for new commands (CDECL)
extern pit_get_ticks
extern process_get_current
extern process_get_by_pid
extern process_set_uid
extern process_get_uid
extern ramfs_add_ram_file
extern ramfs_remove_node
extern ramfs_remove_node_recursive
extern ramfs_count_recursive
extern kbd_buffer_get
extern kernel_panic
extern process_demo_spawn
extern print_prompt
extern update_hw_cursor_from_ptr

; Language strings (English by default)
extern cmd_alloc_en, cmd_memstat_en, cmd_help_en, cmd_free_en, cmd_clear_en, cmd_language_en
extern cmd_ls_en, cmd_cat_en, cmd_disk_en
extern cmd_echo_en, cmd_uptime_en, cmd_ps_en, cmd_write_en, cmd_go_en, cmd_become_en, cmd_bc_en, cmd_rm_en
extern cmd_mkdir_en
extern cmd_cp_en
extern cmd_mv_en
extern cmd_find_en, cmd_panic_en, cmd_spawn_en
extern cmd_whoami_en, cmd_pwd_en, cmd_touch_en, cmd_sysinfo_en, cmd_hostname_en
extern cmd_head_en, cmd_wc_en, cmd_ring3_en
extern unknown_cmd_en, help_msg_en, help_all_en
extern memstat_free_msg_en, memstat_used_msg_en
extern alloc_success_msg_en, alloc_oom_msg_en
extern free_ok_msg_en, free_fail_msg_en, free_usage_msg_en
extern language_current_en, language_available_en, language_set_en
extern file_not_found_en
extern disk_error_en, disk_usage_en, disk_reading_en
extern uptime_msg_en, uptime_sec_en
extern ps_header_en
extern write_ok_en, write_usage_en, write_fail_en
extern go_usage_en, go_ok_admin_en, go_ok_user_en, go_denied_en
extern go_not_found_en, go_not_dir_en
extern become_usage_en, become_ok_admin_en, become_ok_user_en, become_denied_en
extern become_prompt_en, become_wrong_pw_en
extern rm_usage_en, rm_not_found_en, rm_ok_en, rm_cancelled_en, rm_confirm_en, rm_yn_en, rm_not_empty_en, rm_cannot_root_en
extern mkdir_usage_en, mkdir_ok_en, mkdir_fail_en, mkdir_exists_en
extern cp_usage_en, cp_ok_en, cp_fail_en, cp_exists_en
extern mv_usage_en, mv_ok_en, mv_fail_en, mv_exists_en
extern find_usage_en, find_not_found_en
extern whoami_root_en, whoami_user_en
extern touch_ok_en, touch_usage_en, touch_fail_en
extern sysinfo_arch_en, sysinfo_mem_en, sysinfo_proc_en, sysinfo_fs_en, sysinfo_lang_en
extern hostname_msg_en
extern head_usage_en
extern wc_lines_en, wc_usage_en
extern ring3_msg_en

; Language strings (Catalan)
extern cmd_alloc_ca, cmd_memstat_ca, cmd_help_ca, cmd_free_ca, cmd_clear_ca, cmd_language_ca
extern cmd_ls_ca, cmd_cat_ca, cmd_disk_ca
extern cmd_echo_ca, cmd_uptime_ca, cmd_ps_ca, cmd_write_ca, cmd_go_ca, cmd_become_ca, cmd_bc_ca, cmd_rm_ca
extern cmd_mkdir_ca
extern cmd_cp_ca
extern cmd_mv_ca
extern cmd_find_ca, cmd_panic_ca, cmd_spawn_ca
extern cmd_whoami_ca, cmd_pwd_ca, cmd_touch_ca, cmd_sysinfo_ca, cmd_hostname_ca
extern cmd_head_ca, cmd_wc_ca, cmd_ring3_ca
extern unknown_cmd_ca, help_msg_ca, help_all_ca
extern memstat_free_msg_ca, memstat_used_msg_ca
extern alloc_success_msg_ca, alloc_oom_msg_ca
extern free_ok_msg_ca, free_fail_msg_ca, free_usage_msg_ca
extern language_current_ca, language_available_ca, language_set_ca
extern file_not_found_ca
extern disk_error_ca, disk_usage_ca, disk_reading_ca
extern uptime_msg_ca, uptime_sec_ca
extern ps_header_ca
extern write_ok_ca, write_usage_ca, write_fail_ca
extern go_usage_ca, go_ok_admin_ca, go_ok_user_ca, go_denied_ca
extern go_not_found_ca, go_not_dir_ca
extern become_usage_ca, become_ok_admin_ca, become_ok_user_ca, become_denied_ca
extern become_prompt_ca, become_wrong_pw_ca
extern rm_usage_ca, rm_not_found_ca, rm_ok_ca, rm_cancelled_ca, rm_confirm_ca, rm_yn_ca, rm_not_empty_ca, rm_cannot_root_ca
extern mkdir_usage_ca, mkdir_ok_ca, mkdir_fail_ca, mkdir_exists_ca
extern cp_usage_ca, cp_ok_ca, cp_fail_ca, cp_exists_ca
extern mv_usage_ca, mv_ok_ca, mv_fail_ca, mv_exists_ca
extern find_usage_ca, find_not_found_ca
extern whoami_root_ca, whoami_user_ca
extern touch_ok_ca, touch_usage_ca, touch_fail_ca
extern sysinfo_arch_ca, sysinfo_mem_ca, sysinfo_proc_ca, sysinfo_fs_ca, sysinfo_lang_ca
extern hostname_msg_ca
extern head_usage_ca
extern wc_lines_ca, wc_usage_ca
extern ring3_msg_ca

; Language strings (Spanish)
extern cmd_alloc_es, cmd_memstat_es, cmd_help_es, cmd_free_es, cmd_clear_es, cmd_language_es
extern cmd_ls_es, cmd_cat_es, cmd_disk_es
extern cmd_echo_es, cmd_uptime_es, cmd_ps_es, cmd_write_es, cmd_go_es, cmd_become_es, cmd_bc_es, cmd_rm_es
extern cmd_mkdir_es
extern cmd_cp_es
extern cmd_mv_es
extern cmd_find_es, cmd_panic_es, cmd_spawn_es
extern cmd_whoami_es, cmd_pwd_es, cmd_touch_es, cmd_sysinfo_es, cmd_hostname_es
extern cmd_head_es, cmd_wc_es, cmd_ring3_es
;extern cmd_rm_es_alias
extern cmd_rm_ca_alias
extern unknown_cmd_es, help_msg_es, help_all_es
extern memstat_free_msg_es, memstat_used_msg_es
extern alloc_success_msg_es, alloc_oom_msg_es
extern free_ok_msg_es, free_fail_msg_es, free_usage_msg_es
extern language_current_es, language_available_es, language_set_es
extern file_not_found_es
extern disk_error_es, disk_usage_es, disk_reading_es
extern uptime_msg_es, uptime_sec_es
extern ps_header_es
extern write_ok_es, write_usage_es, write_fail_es
extern go_usage_es, go_ok_admin_es, go_ok_user_es, go_denied_es
extern go_not_found_es, go_not_dir_es
extern become_usage_es, become_ok_admin_es, become_ok_user_es, become_denied_es
extern become_prompt_es, become_wrong_pw_es
extern rm_usage_es, rm_not_found_es, rm_ok_es, rm_cancelled_es, rm_confirm_es, rm_yn_es, rm_not_empty_es, rm_cannot_root_es
extern mkdir_usage_es, mkdir_ok_es, mkdir_fail_es, mkdir_exists_es
extern cp_usage_es, cp_ok_es, cp_fail_es, cp_exists_es
extern mv_usage_es, mv_ok_es, mv_fail_es, mv_exists_es
extern find_usage_es, find_not_found_es
extern whoami_root_es, whoami_user_es
extern touch_ok_es, touch_usage_es, touch_fail_es
extern sysinfo_arch_es, sysinfo_mem_es, sysinfo_proc_es, sysinfo_fs_es, sysinfo_lang_es
extern hostname_msg_es
extern head_usage_es
extern wc_lines_es, wc_usage_es
extern ring3_msg_es
extern ramfs_add_ram_file_at_path
extern cmd_ring3_test
extern ramfs_mkdir_at_path
extern ramfs_copy_file
extern ramfs_move_node
extern ramfs_find
extern ramfs_find_get_result

; C function from ata_test.c (CDECL)
extern ata_test_read_sector

extern vfs_set_cwd_with_path
extern vfs_get_cwd_path
extern vfs_chdir_parent

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

; ---------------------------------------------------------------------------
; handle_help: Short help (full command names only) or "help all" (detailed usage).
; ---------------------------------------------------------------------------
handle_help:
  push eax
  push ebx
  push edi
  push esi

  mov esi, command_buffer
  call skip_to_argument
  cmp byte [esi], 0
  je .help_short

  ; Check for "all" (same in every language)
  mov al, [esi]
  cmp al, 'a'
  jne .help_short
  cmp byte [esi + 1], 'l'
  jne .help_short
  cmp byte [esi + 2], 'l'
  jne .help_short
  mov al, [esi + 3]
  cmp al, ' '
  je .help_all
  cmp al, 0
  je .help_all

.help_short:
  ; Print the short help (may contain 0x0A for a second line)
  mov esi, [current_help_msg]
  call print_multiline
  jmp .help_done

.help_all:
  ; Walk the packed null-separated help block; stop at double-null
  mov esi, [current_help_all]
.help_all_loop:
  cmp byte [esi], 0
  je .help_done
  call print_line
  jmp .help_all_loop

.help_done:
  pop esi
  pop edi
  pop ebx
  pop eax
  ret

; Helper: print a string that may contain 0x0A (newline) characters.
; Splits at each 0x0A and calls print_line for each segment.
print_multiline:
  push eax
  push ebx
  push edi

  mov edi, [cons_cursor]
  mov bl, 0x0F

.pml_loop:
  lodsb
  cmp al, 0
  je .pml_done
  cmp al, 10
  je .pml_newline
  mov [edi], al
  mov [edi + 1], bl
  add edi, 2
  jmp .pml_loop

.pml_newline:
  mov [cons_cursor], edi
  call newline_from_cursor
  mov edi, [cons_cursor]
  jmp .pml_loop

.pml_done:
  mov [cons_cursor], edi
  call newline_from_cursor

  pop edi
  pop ebx
  pop eax
  ret

handle_clear:
  call clear_screen
  mov dword [cons_cursor], 0xb8000
  mov dword [line_start], 0xb8000
  ret

handle_panic:
  call kernel_panic
  ; never returns

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

; VFS_NODE_DIR = 2 (from vfs.h) -- directories shown in blue
VFS_NODE_FILE equ 1
VFS_NODE_DIR  equ 2

; ---------------------------------------------------------------------------
; handle_ls: List current directory, or list a given path if argument provided.
;   ls          -> list CWD
;   ls /sistem  -> list /sistem
; ---------------------------------------------------------------------------
handle_ls:
  push eax
  push ebx
  push ecx
  push edx
  push esi
  push edi

  ; Check for optional path argument
  mov esi, command_buffer
  call skip_to_argument
  cmp byte [esi], 0
  je .ls_use_cwd

  ; Build absolute path in go_path_buf using CWD for relative paths
  mov edi, go_path_buf
  call build_cwd_path

  push go_path_buf
  call vfs_lookup
  add esp, 4
  cmp eax, 0
  je .ls_not_found
  cmp byte [eax + 32], VFS_NODE_DIR
  jne .ls_not_dir
  mov ebx, eax
  xor ecx, ecx
  jmp .ls_next

.ls_not_found:
  mov esi, [current_go_not_found]
  call print_line
  jmp .ls_done

.ls_not_dir:
  mov esi, [current_go_not_dir]
  call print_line
  jmp .ls_done

.ls_use_cwd:
  ; List current working directory based on the same textual cwd used by prompt
  ; and path-building commands. This prevents "prompt says /X but ls lists /Y".
  call vfs_get_cwd_path
  push eax
  call vfs_lookup
  add esp, 4
  cmp eax, 0
  jne .ls_have_dir
  ; Fallback to vnode-based cwd if lookup from path failed
  call vfs_get_cwd
  cmp eax, 0
  jne .ls_have_dir
  ; Last resort: root
  call vfs_get_root
.ls_have_dir:
  mov ebx, eax          ; ebx = directory node pointer
  xor ecx, ecx          ; ecx = directory entry index

.ls_next:
  push ecx
  push dir_entry
  push ecx
  push ebx
  call vfs_readdir
  add esp, 12
  pop ecx

  cmp eax, 0
  jne .ls_done

  ; Wrap to new line if we're past column 46 (so name + "  " fits in 80)
  push ebx
  mov eax, [cons_cursor]
  sub eax, 0xb8000
  mov ebx, 160
  xor edx, edx
  div ebx               ; edx = offset in row (bytes)
  pop ebx
  cmp edx, 94           ; 47 chars * 2 = 94 bytes; if >= 47 chars, wrap
  jb .ls_no_wrap
  call newline_from_cursor

.ls_no_wrap:
  ; Save ebx (directory node) so we can use bl for color without corrupting the pointer
  push ebx
  ; Get type from C so we don't depend on struct layout: 1 = file, 2 = directory
  push dir_entry
  call vfs_dir_entry_get_type
  add esp, 4
  cmp eax, 2             ; VFS_NODE_DIR = 2
  jne .ls_white
  mov bl, 0x0D           ; pink (light magenta) for directories
  jmp .ls_print_name
.ls_white:
  mov bl, 0x0F           ; white for files

.ls_print_name:
  mov edi, [cons_cursor]
  mov esi, dir_entry
  call print_string_pm
  mov [cons_cursor], edi

  ; Two spaces between entries (white)
  mov al, ' '
  mov bl, 0x0F
  call print_char_pm
  add edi, 2
  mov al, ' '
  call print_char_pm
  add edi, 2
  mov [cons_cursor], edi

  pop ebx               ; restore directory node pointer for next vfs_readdir
  inc ecx
  jmp .ls_next

.ls_done:
  ; One newline after the whole listing
  call newline_from_cursor

  pop edi
  pop esi
  pop edx
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
  ; Build absolute path in cat_path_buf using CWD for relative paths
  mov edi, cat_path_buf
  call build_cwd_path

  push cat_path_buf
  call vfs_lookup
  add esp, 4
  mov [filename_node], eax
  cmp eax, 0
  je .cat_not_found

  ; Read file into buffer
  mov eax, [filename_node]   ; node*
  push cat_buffer            ; buffer
  push 255                   ; size (leave space for 0)
  push 0                     ; offset
  push eax                   ; node
  call vfs_read
  add esp, 16

  cmp eax, 0
  jle .cat_not_found

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
  jmp .ps_next

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
; handle_spawn: Temporarily disabled kernel-thread demo.
; For now, just print a short message and return so the shell never blocks.
; ---------------------------------------------------------------------------
handle_spawn:
  push esi
  mov esi, spawn_disabled_msg
  call print_line
  pop esi
  ret

section .data
spawn_disabled_msg: db 'spawn: kernel thread demo temporarily disabled', 0

; ---------------------------------------------------------------------------
; handle_ring3: Test ring-3 user mode transition.
; Prints a message and calls the C function cmd_ring3_test() which
; does tss_set_kernel_stack + enter_user_mode (does not return).
; ---------------------------------------------------------------------------
handle_ring3:
  push esi
  mov esi, [current_ring3_msg]
  call print_line
  call cmd_ring3_test
  pop esi
  ret

; ---------------------------------------------------------------------------
; handle_whoami: Print current user (admin or user).
; ---------------------------------------------------------------------------
handle_whoami:
  push eax
  push esi

  call process_get_uid
  cmp eax, 0
  je .whoami_root
  mov esi, [current_whoami_user]
  jmp .whoami_print
.whoami_root:
  mov esi, [current_whoami_root]
.whoami_print:
  call print_line

  pop esi
  pop eax
  ret

; ---------------------------------------------------------------------------
; handle_pwd: Print current working directory path.
; ---------------------------------------------------------------------------
handle_pwd:
  push eax
  push esi

  call vfs_get_cwd_path
  mov esi, eax
  call print_line

  pop esi
  pop eax
  ret

; ---------------------------------------------------------------------------
; handle_touch: Create an empty file.  Usage: touch <filename>
; ---------------------------------------------------------------------------
handle_touch:
  push eax
  push ebx
  push ecx
  push esi
  push edi

  mov esi, command_buffer
  call skip_to_argument
  cmp byte [esi], 0
  je .touch_usage

  mov edi, cat_path_buf
  call build_cwd_path

  ; Create empty file (data=0, size=0)
  push 0                    ; size = 0
  push 0                    ; data = NULL
  push cat_path_buf         ; path
  call ramfs_add_ram_file_at_path
  add esp, 12

  cmp eax, 0
  jne .touch_fail

  mov esi, [current_touch_ok]
  call print_line
  jmp .touch_done

.touch_usage:
  mov esi, [current_touch_usage]
  call print_line
  jmp .touch_done

.touch_fail:
  mov esi, [current_touch_fail]
  call print_line

.touch_done:
  pop edi
  pop esi
  pop ecx
  pop ebx
  pop eax
  ret

; ---------------------------------------------------------------------------
; handle_hostname: Print the system hostname.
; ---------------------------------------------------------------------------
handle_hostname:
  push esi
  mov esi, [current_hostname_msg]
  call print_line
  pop esi
  ret

; ---------------------------------------------------------------------------
; handle_sysinfo: Print system information summary.
; ---------------------------------------------------------------------------
handle_sysinfo:
  push eax
  push ebx
  push ecx
  push esi
  push edi

  ; Line 1: Architecture
  mov esi, [current_sysinfo_arch]
  call print_line

  ; Line 2: Free memory pages
  mov esi, [current_sysinfo_mem]
  call print_string_no_newline
  call pmm_get_free_pages
  call print_decimal
  call newline_from_cursor

  ; Line 3: Active processes (count PIDs 0-15 that exist)
  mov esi, [current_sysinfo_proc]
  call print_string_no_newline
  xor ecx, ecx                ; PID counter
  xor ebx, ebx                ; active count
.si_count:
  push ecx
  push ecx
  call process_get_by_pid
  add esp, 4
  pop ecx
  test eax, eax
  jz .si_skip
  inc ebx
.si_skip:
  inc ecx
  cmp ecx, 16
  jl .si_count
  mov eax, ebx
  call print_decimal
  call newline_from_cursor

  ; Line 4: Filesystem
  mov esi, [current_sysinfo_fs]
  call print_line

  ; Line 5: Current language
  mov esi, [current_sysinfo_lang]
  call print_string_no_newline
  mov esi, [current_language_current_msg]
  call print_line

  pop edi
  pop esi
  pop ecx
  pop ebx
  pop eax
  ret

; ---------------------------------------------------------------------------
; handle_head: Show first 5 lines of a file.  Usage: head <file>
; ---------------------------------------------------------------------------
handle_head:
  push eax
  push ebx
  push ecx
  push edx
  push esi
  push edi

  mov esi, command_buffer
  call skip_to_argument
  cmp byte [esi], 0
  je .head_usage

  mov edi, cat_path_buf
  call build_cwd_path

  push cat_path_buf
  call vfs_lookup
  add esp, 4
  cmp eax, 0
  je .head_not_found

  ; Read up to 255 bytes
  push cat_buffer
  push 255
  push 0
  push eax
  call vfs_read
  add esp, 16
  cmp eax, 0
  jle .head_not_found

  ; EAX = bytes read. Print up to 5 lines.
  mov ebx, eax               ; EBX = total bytes
  mov byte [cat_buffer + ebx], 0
  mov esi, cat_buffer
  mov ecx, 5                  ; line limit
  mov edi, [cons_cursor]

.head_char:
  cmp ecx, 0
  je .head_done_print
  mov al, [esi]
  cmp al, 0
  je .head_done_print
  cmp al, 10
  je .head_newline
  mov bl, 0x0F
  call print_char_pm
  inc esi
  jmp .head_char

.head_newline:
  mov [cons_cursor], edi
  call newline_from_cursor
  mov edi, [cons_cursor]
  dec ecx
  inc esi
  jmp .head_char

.head_done_print:
  mov [cons_cursor], edi
  call newline_from_cursor
  jmp .head_done

.head_not_found:
  mov esi, [current_file_not_found_msg]
  call print_line
  jmp .head_done

.head_usage:
  mov esi, [current_head_usage]
  call print_line

.head_done:
  pop edi
  pop esi
  pop edx
  pop ecx
  pop ebx
  pop eax
  ret

; ---------------------------------------------------------------------------
; handle_wc: Count lines and bytes of a file.  Usage: wc <file>
;   Output: "N lines, M bytes  filename"
; ---------------------------------------------------------------------------
handle_wc:
  push eax
  push ebx
  push ecx
  push edx
  push esi
  push edi

  mov esi, command_buffer
  call skip_to_argument
  cmp byte [esi], 0
  je .wc_usage

  mov edi, cat_path_buf
  call build_cwd_path

  push cat_path_buf
  call vfs_lookup
  add esp, 4
  cmp eax, 0
  je .wc_not_found

  ; Read up to 255 bytes
  push cat_buffer
  push 255
  push 0
  push eax
  call vfs_read
  add esp, 16
  cmp eax, 0
  jle .wc_not_found

  ; EAX = bytes read. Count newlines.
  mov ebx, eax               ; EBX = byte count
  mov ecx, 0                 ; ECX = line count
  mov edx, 0                 ; index
.wc_scan:
  cmp edx, ebx
  jge .wc_print
  cmp byte [cat_buffer + edx], 10
  jne .wc_next
  inc ecx
.wc_next:
  inc edx
  jmp .wc_scan

.wc_print:
  ; If last byte isn't newline, count the final line
  cmp ebx, 0
  je .wc_print_line
  cmp byte [cat_buffer + ebx - 1], 10
  je .wc_print_line
  inc ecx

.wc_print_line:
  ; Print: "N lines, M bytes"
  mov eax, ecx
  call print_decimal
  mov esi, [current_wc_lines]
  call print_string_no_newline
  mov eax, ebx
  call print_decimal
  mov esi, wc_bytes_str
  call print_string_no_newline
  ; Print filename
  mov esi, cat_path_buf
  call print_line
  jmp .wc_done

.wc_not_found:
  mov esi, [current_file_not_found_msg]
  call print_line
  jmp .wc_done

.wc_usage:
  mov esi, [current_wc_usage]
  call print_line

.wc_done:
  pop edi
  pop esi
  pop edx
  pop ecx
  pop ebx
  pop eax
  ret

; ---------------------------------------------------------------------------
;   handle_write: Create a file in RAMFS.
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

  ; Build full path: if filename starts with '/', use it as absolute path;
  ; otherwise prepend CWD path so file is created in the current directory.
  push ecx
  push ebx
  mov eax, edi                ; EAX = filename pointer
  mov esi, cat_path_buf
  cmp byte [eax], '/'
  je .write_path_abs

  ; Relative path: copy CWD first, then '/', then filename
  push eax
  call vfs_get_cwd_path       ; returns pointer in EAX
  mov edx, eax                ; EDX = CWD string
  pop eax                     ; EAX = filename again
.write_cwd_copy:
  mov cl, [edx]
  cmp cl, 0
  je .write_cwd_done
  mov [esi], cl
  inc edx
  inc esi
  jmp .write_cwd_copy
.write_cwd_done:
  ; Add '/' separator unless CWD already ends with '/'
  cmp esi, cat_path_buf
  je .write_add_slash
  cmp byte [esi - 1], '/'
  je .write_path_abs_copy
.write_add_slash:
  mov byte [esi], '/'
  inc esi
  jmp .write_path_abs_copy

.write_path_abs:
  ; Absolute path: just copy filename as-is
.write_path_abs_copy:
  mov dl, [eax]
  mov [esi], dl
  inc eax
  inc esi
  cmp dl, 0
  jne .write_path_abs_copy
  pop ebx
  pop ecx

  mov eax, [write_pool_idx]
  inc eax
  mov [write_pool_idx], eax

  push ecx
  push ebx
  push cat_path_buf
  call ramfs_add_ram_file_at_path
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

; ---------------------------------------------------------------------------
; handle_rm: Remove file (no confirm) or directory (confirm with count, or -r for recursive).
;   rm <path>      -> file: delete at once; directory: "Are you sure ... ? (N entries) y/n?"
;   rm -r <path>   -> recursive delete, no confirmation
; ---------------------------------------------------------------------------
handle_rm:
  push eax
  push ebx
  push ecx
  push edx
  push esi
  push edi

  mov esi, command_buffer
  call skip_to_argument
  cmp byte [esi], 0
  je .rm_usage

  ; Check for -r / -R flag (recursive, no confirmation)
  xor edx, edx                  ; EDX = 0 (not recursive)
  cmp byte [esi], '-'
  jne .rm_have_path
  cmp byte [esi + 1], 'r'
  je .rm_check_recursive
  cmp byte [esi + 1], 'R'
  jne .rm_have_path
.rm_check_recursive:
  cmp byte [esi + 2], 0
  je .rm_advance_arg
  cmp byte [esi + 2], ' '
  jne .rm_have_path
.rm_advance_arg:
  mov edx, 1                    ; EDX = 1 (recursive)
  add esi, 2                    ; skip "-r"
.rm_skip_spaces:
  cmp byte [esi], ' '
  jne .rm_have_path
  inc esi
  jmp .rm_skip_spaces

.rm_have_path:
  cmp byte [esi], 0
  je .rm_usage

  ; Build absolute path in go_path_buf using CWD for relative paths
  mov edi, go_path_buf
  call build_cwd_path

  push go_path_buf
  call vfs_lookup
  add esp, 4
  cmp eax, 0
  je .rm_not_found

  ; EAX = node. Check type: 1 = file, 2 = directory
  cmp byte [eax + 32], VFS_NODE_DIR
  jne .rm_is_file

  ; Directory
  test edx, edx
  jnz .rm_recursive_del         ; -r: delete recursively without confirm

  mov ebx, eax                  ; save node for later

  ; Ask C to count how many entries live under this directory
  push eax
  call ramfs_count_recursive
  add esp, 4
  mov ecx, eax                  ; ECX = total entries

  ; Print "Are you sure you want to delete the directory "
  mov esi, [current_rm_confirm]
  call print_string_no_newline
  mov esi, go_path_buf
  call print_string_no_newline

  ; Print " (" <count> " entries) "
  mov al, ' '
  mov bl, 0x0F
  call print_char_pm
  mov al, '('
  mov bl, 0x0F
  call print_char_pm

  mov eax, ecx
  call print_decimal

  mov al, ' '
  mov bl, 0x0F
  call print_char_pm
  mov al, 'e'
  mov bl, 0x0F
  call print_char_pm
  mov al, 'n'
  mov bl, 0x0F
  call print_char_pm
  mov al, 't'
  mov bl, 0x0F
  call print_char_pm
  mov al, 'r'
  mov bl, 0x0F
  call print_char_pm
  mov al, 'i'
  mov bl, 0x0F
  call print_char_pm
  mov al, 'e'
  mov bl, 0x0F
  call print_char_pm
  mov al, 's'
  mov bl, 0x0F
  call print_char_pm
  mov al, ')'
  mov bl, 0x0F
  call print_char_pm
  mov al, ' '
  mov bl, 0x0F
  call print_char_pm

  mov esi, [current_rm_yn]
  call print_line

  sti                        ; re-enable IRQs for keyboard input
.rm_wait_key:
  hlt
  call kbd_buffer_get
  cmp eax, -1
  je .rm_wait_key
  cli                        ; restore IRQs-off state
  and eax, 0xFF
  cmp al, 'y'
  je .rm_do_del
  cmp al, 'Y'
  je .rm_do_del
  mov esi, [current_rm_cancelled]
  call print_line
  jmp .rm_done

.rm_do_del:
  push ebx
  call ramfs_remove_node
  add esp, 4
  jmp .rm_check_result

.rm_recursive_del:
  push eax
  call ramfs_remove_node_recursive
  add esp, 4
  jmp .rm_check_result

.rm_is_file:
  ; File: remove immediately
  push eax
  call ramfs_remove_node
  add esp, 4

.rm_check_result:
  cmp eax, 0
  je .rm_ok
  cmp eax, -1
  je .rm_cannot_root
  cmp eax, -2
  je .rm_not_empty
  mov esi, [current_rm_not_found]
  call print_line
  jmp .rm_done

.rm_not_empty:
  ; Directory not empty (non-recursive rm on dir with contents)
  mov esi, [current_rm_not_found]
  call print_line
  jmp .rm_done

.rm_usage:
  mov esi, [current_rm_usage]
  call print_line
  jmp .rm_done
.rm_not_found:
  mov esi, [current_rm_not_found]
  call print_line
  jmp .rm_done
.rm_ok:
  mov esi, [current_rm_ok]
  call print_line
  jmp .rm_done
.rm_cannot_root:
  mov esi, [current_rm_cannot_root]
  call print_line

.rm_done:
  pop edi
  pop esi
  pop edx
  pop ecx
  pop ebx
  pop eax
  ret

; ---------------------------------------------------------------------------
; handle_mkdir: Create a directory. Usage: mkdir <path>
;   e.g. mkdir /tmp/foo  or  mkdir sistem/config
; ---------------------------------------------------------------------------
handle_mkdir:
  push eax
  push ebx
  push esi
  push edi

  mov esi, command_buffer
  call skip_to_argument
  cmp byte [esi], 0
  je .mkdir_usage

  ; NUL-terminate the argument at the first space
  mov edi, esi
.mkdir_find_end:
  mov al, [edi]
  cmp al, ' '
  je .mkdir_term
  cmp al, 0
  je .mkdir_build
  inc edi
  jmp .mkdir_find_end
.mkdir_term:
  mov byte [edi], 0
.mkdir_build:
  ; Build CWD-relative path into go_path_buf
  mov edi, go_path_buf
  call build_cwd_path

  push go_path_buf
  call ramfs_mkdir_at_path
  add esp, 4

  cmp eax, 0
  je .mkdir_ok
  cmp eax, -2
  je .mkdir_exists
  ; -1: fail
  mov esi, [current_mkdir_fail]
  call print_line
  jmp .mkdir_done

.mkdir_usage:
  mov esi, [current_mkdir_usage]
  call print_line
  jmp .mkdir_done
.mkdir_ok:
  mov esi, [current_mkdir_ok]
  call print_line
  jmp .mkdir_done
.mkdir_exists:
  mov esi, [current_mkdir_exists]
  call print_line

.mkdir_done:
  pop edi
  pop esi
  pop ebx
  pop eax
  ret

; ---------------------------------------------------------------------------
; build_cwd_path: Resolve a path relative to CWD into a buffer.
;   Input : ESI = input path string (NUL-terminated)
;           EDI = destination buffer (must be >= 64 bytes)
;   If the path starts with '/', it is copied as-is (absolute).
;   Otherwise CWD + '/' + path is built (relative).
;   Clobbers: EAX, ECX, EDX
; ---------------------------------------------------------------------------
build_cwd_path:
  cmp byte [esi], '/'
  je .bcp_abs

  push esi
  call vfs_get_cwd_path
  mov edx, eax
  pop esi
.bcp_cwd:
  mov cl, [edx]
  cmp cl, 0
  je .bcp_sep
  mov [edi], cl
  inc edx
  inc edi
  jmp .bcp_cwd
.bcp_sep:
  cmp byte [edi - 1], '/'
  je .bcp_copy
  mov byte [edi], '/'
  inc edi
  jmp .bcp_copy

.bcp_abs:
.bcp_copy:
  mov cl, [esi]
  mov [edi], cl
  inc esi
  inc edi
  cmp cl, 0
  jne .bcp_copy
  ret

; ---------------------------------------------------------------------------
; handle_cp: Copy a file.  Usage: cp <source> <dest>
; ---------------------------------------------------------------------------
handle_cp:
  push eax
  push ebx
  push ecx
  push edx
  push esi
  push edi

  mov esi, command_buffer
  call skip_to_argument
  cmp byte [esi], 0
  je .cp_usage

  ; ESI = source start. Find end of source (space or NUL).
  mov edi, esi
.cp_find_space:
  mov al, [esi]
  cmp al, ' '
  je .cp_got_src
  cmp al, 0
  je .cp_usage
  inc esi
  jmp .cp_find_space

.cp_got_src:
  mov byte [esi], 0
  inc esi

.cp_skip_space:
  cmp byte [esi], ' '
  jne .cp_have_dest
  inc esi
  jmp .cp_skip_space

.cp_have_dest:
  cmp byte [esi], 0
  je .cp_usage

  ; EDI = source string, ESI = dest string
  push esi

  ; Build source full path in cat_path_buf
  mov esi, edi
  mov edi, cat_path_buf
  call build_cwd_path

  ; Build dest full path in go_path_buf
  pop esi
  mov edi, go_path_buf
  call build_cwd_path

  ; ramfs_copy_file(cat_path_buf, go_path_buf)
  push go_path_buf
  push cat_path_buf
  call ramfs_copy_file
  add esp, 8

  cmp eax, 0
  je .cp_ok
  cmp eax, -2
  je .cp_exists
  ; -1 or -3: generic fail
  mov esi, [current_cp_fail]
  call print_line
  jmp .cp_done

.cp_usage:
  mov esi, [current_cp_usage]
  call print_line
  jmp .cp_done
.cp_ok:
  mov esi, [current_cp_ok]
  call print_line
  jmp .cp_done
.cp_exists:
  mov esi, [current_cp_exists]
  call print_line

.cp_done:
  pop edi
  pop esi
  pop edx
  pop ecx
  pop ebx
  pop eax
  ret

; ---------------------------------------------------------------------------
; handle_mv: Move/rename a file or directory.  Usage: mv <source> <dest>
;   Relinks the parent pointer — no data copy (like a real filesystem rename).
; ---------------------------------------------------------------------------
handle_mv:
  push eax
  push ebx
  push ecx
  push edx
  push esi
  push edi

  mov esi, command_buffer
  call skip_to_argument
  cmp byte [esi], 0
  je .mv_usage

  mov edi, esi
.mv_find_space:
  mov al, [esi]
  cmp al, ' '
  je .mv_got_src
  cmp al, 0
  je .mv_usage
  inc esi
  jmp .mv_find_space

.mv_got_src:
  mov byte [esi], 0
  inc esi

.mv_skip_space:
  cmp byte [esi], ' '
  jne .mv_have_dest
  inc esi
  jmp .mv_skip_space

.mv_have_dest:
  cmp byte [esi], 0
  je .mv_usage

  push esi

  ; Build source full path in cat_path_buf
  mov esi, edi
  mov edi, cat_path_buf
  call build_cwd_path

  ; Build dest full path in go_path_buf
  pop esi
  mov edi, go_path_buf
  call build_cwd_path

  ; Check if dest is an existing directory (Unix: mv file dir → dir/file)
  push go_path_buf
  call vfs_lookup
  add esp, 4
  cmp eax, 0
  je .mv_do_move              ; dest doesn't exist → simple rename/move
  cmp byte [eax + 32], VFS_NODE_DIR
  jne .mv_exists              ; dest is an existing file → error

  ; Dest is a directory: append "/" + basename(src) to go_path_buf
  mov edi, go_path_buf
.mv_find_end:
  cmp byte [edi], 0
  je .mv_at_end
  inc edi
  jmp .mv_find_end
.mv_at_end:
  cmp byte [edi - 1], '/'
  je .mv_has_slash
  mov byte [edi], '/'
  inc edi
.mv_has_slash:
  ; Find basename of source (char after last '/')
  mov esi, cat_path_buf
  mov edx, esi
.mv_find_base:
  cmp byte [esi], 0
  je .mv_copy_base
  cmp byte [esi], '/'
  jne .mv_next_bc
  lea edx, [esi + 1]
.mv_next_bc:
  inc esi
  jmp .mv_find_base
.mv_copy_base:
  mov esi, edx
.mv_copy_bn:
  mov al, [esi]
  mov [edi], al
  inc esi
  inc edi
  cmp al, 0
  jne .mv_copy_bn

.mv_do_move:
  push go_path_buf
  push cat_path_buf
  call ramfs_move_node
  add esp, 8

  cmp eax, 0
  je .mv_ok
  cmp eax, -2
  je .mv_exists
  mov esi, [current_mv_fail]
  call print_line
  jmp .mv_done

.mv_usage:
  mov esi, [current_mv_usage]
  call print_line
  jmp .mv_done
.mv_ok:
  mov esi, [current_mv_ok]
  call print_line
  jmp .mv_done
.mv_exists:
  mov esi, [current_mv_exists]
  call print_line

.mv_done:
  pop edi
  pop esi
  pop edx
  pop ecx
  pop ebx
  pop eax
  ret

; ---------------------------------------------------------------------------
; handle_find: Recursive search.  Usage: find <name>  or  find <path> <name>
;   find <name>            — searches from / (entire filesystem)
;   find <path> <name>     — searches under <path>
; ---------------------------------------------------------------------------
handle_find:
  push eax
  push ebx
  push ecx
  push edx
  push esi
  push edi

  mov esi, command_buffer
  call skip_to_argument
  cmp byte [esi], 0
  je .find_usage

  ; ESI = first argument. Check if there's a second argument.
  mov edi, esi                ; EDI = start of arg1
.find_scan_arg1:
  mov al, [esi]
  cmp al, ' '
  je .find_two_args
  cmp al, 0
  je .find_one_arg
  inc esi
  jmp .find_scan_arg1

.find_one_arg:
  ; Build absolute candidate path in go_path_buf:
  ; if arg starts '/', use it as-is; else prefix with "/".
  mov esi, edi
  mov edi, go_path_buf
  cmp byte [esi], '/'
  je .find_copy_one
  mov byte [edi], '/'
  inc edi
.find_copy_one:
  xor ecx, ecx
.find_copy_one_loop:
  mov al, [esi]
  mov [edi], al
  cmp al, 0
  je .find_lookup_candidate
  inc esi
  inc edi
  inc ecx
  cmp ecx, 62
  jb .find_copy_one_loop
  mov byte [edi], 0
  jmp .find_lookup_candidate

.find_two_args:
  ; Split args: arg1 = base path, arg2 = name
  mov byte [esi], 0           ; terminate arg1
  inc esi
.find_skip_sp:
  cmp byte [esi], ' '
  jne .find_have_two
  inc esi
  jmp .find_skip_sp
.find_have_two:
  cmp byte [esi], 0
  je .find_usage

  ; Save arg2 pointer in EDX
  mov edx, esi

  ; Build absolute base path from arg1 into cat_path_buf
  mov esi, edi
  mov edi, cat_path_buf
  call build_cwd_path

  ; Build candidate "base/name" into go_path_buf
  mov esi, cat_path_buf
  mov edi, go_path_buf
  xor ecx, ecx
.find_copy_base:
  mov al, [esi]
  mov [edi], al
  cmp al, 0
  je .find_base_done
  inc esi
  inc edi
  inc ecx
  cmp ecx, 62
  jb .find_copy_base
  mov byte [edi], 0
  jmp .find_lookup_candidate

.find_base_done:
  ; If base doesn't end with '/', add it.
  cmp edi, go_path_buf
  je .find_add_sep
  cmp byte [edi - 1], '/'
  je .find_append_name
.find_add_sep:
  mov byte [edi], '/'
  inc edi

.find_append_name:
  mov esi, edx
  xor ecx, ecx
.find_copy_name:
  mov al, [esi]
  mov [edi], al
  cmp al, 0
  je .find_lookup_candidate
  inc esi
  inc edi
  inc ecx
  cmp ecx, 31
  jb .find_copy_name
  mov byte [edi], 0

.find_lookup_candidate:
  push go_path_buf
  call vfs_lookup
  add esp, 4
  cmp eax, 0
  je .find_none

  ; Found one exact match -> print resolved path
  mov esi, go_path_buf
  call print_line
  jmp .find_done

.find_none:
  mov esi, [current_find_not_found]
  call print_line
  jmp .find_done

.find_usage:
  mov esi, [current_find_usage]
  call print_line

.find_done:
  pop edi
  pop esi
  pop edx
  pop ecx
  pop ebx
  pop eax
  ret

; ---------------------------------------------------------------------------
; handle_go: Change directory (cd). Usage: go <path>  or  go ..
;   go /sistem, go /, go /home/normal,  go ..  (parent directory)
; ---------------------------------------------------------------------------
handle_go:
  push eax
  push ebx
  push ecx
  push esi
  push edi

  mov esi, command_buffer
  call skip_to_argument
  cmp byte [esi], 0
  je .go_usage

  ; Special case: ".." -> go to parent directory
  mov al, [esi]
  cmp al, '.'
  jne .go_normal
  cmp byte [esi + 1], '.'
  jne .go_normal
  mov al, [esi + 2]
  cmp al, ' '
  je .go_parent
  cmp al, 0
  je .go_parent

.go_normal:
  ; Build absolute path in go_path_buf using CWD for relative paths
  mov edi, go_path_buf
  call build_cwd_path

  push go_path_buf
  call vfs_lookup
  add esp, 4
  cmp eax, 0
  je .go_not_found

  ; Check it's a directory (type at offset 32 in vfs_node_t)
  cmp byte [eax + 32], VFS_NODE_DIR
  jne .go_not_dir

  push go_path_buf
  push eax
  call vfs_set_cwd_with_path
  add esp, 8
  jmp .go_done

.go_parent:
  call vfs_chdir_parent
  jmp .go_done

.go_usage:
  mov esi, [current_go_usage]
  call print_line
  jmp .go_done
.go_not_found:
  mov esi, [current_go_not_found]
  call print_line
  jmp .go_done
.go_not_dir:
  mov esi, [current_go_not_dir]
  call print_line

.go_done:
  pop edi
  pop esi
  pop ecx
  pop ebx
  pop eax
  ret

; ---------------------------------------------------------------------------
; handle_become: Switch user. "become admin" / "bc admin" -> root, "become user" / "bc user" -> normal user.
;   Only root can switch. Usage: become admin | become user  (or bc admin | bc user)
; ---------------------------------------------------------------------------
handle_become:
  push eax
  push ebx
  push esi
  push edi
  ;
  ; Parse argument: admin or user
  ;
  mov esi, command_buffer
  call skip_to_argument
  cmp byte [esi], 0
  je .become_usage

  mov al, [esi]
  cmp al, 'a'
  jne .become_try_user
  cmp byte [esi + 1], 'd'
  jne .become_usage
  cmp byte [esi + 2], 'm'
  jne .become_usage
  cmp byte [esi + 3], 'i'
  jne .become_usage
  cmp byte [esi + 4], 'n'
  jne .become_usage
  mov al, [esi + 5]
  cmp al, ' '
  je .become_do_admin
  cmp al, 0
  je .become_do_admin
  jmp .become_usage

.become_do_admin:
  ; Prompt for password
  mov esi, [current_become_prompt]
  call print_string_no_newline

  ; Read password into temp buffer (reuse command_buffer end)
  mov edi, command_buffer
  add edi, 128              ; offset inside buffer for password
  mov ecx, 31               ; max password length
  sti                        ; re-enable IRQs so keyboard handler can fill buffer
.become_pw_read_loop:
  hlt                        ; sleep until next interrupt (saves CPU)
  call kbd_buffer_get
  cmp eax, -1
  je .become_pw_read_loop
  and eax, 0xFF

  cmp al, 10                ; Enter
  je .become_pw_done

  cmp al, 8                 ; Backspace
  je .become_pw_backspace

  cmp ecx, 0
  je .become_pw_read_loop   ; ignore extra chars

  mov [edi], al
  inc edi
  dec ecx
  jmp .become_pw_read_loop

.become_pw_backspace:
  cmp edi, command_buffer + 128
  jle .become_pw_read_loop
  dec edi
  inc ecx
  jmp .become_pw_read_loop

.become_pw_done:
  cli                        ; re-disable IRQs (parse_command runs with IRQs off)
  mov byte [edi], 0

  ; Compare with expected password (same literal in all languages for now)
  mov esi, command_buffer
  add esi, 128
  mov edi, admin_password
.become_pw_cmp_loop:
  mov al, [esi]
  mov bl, [edi]
  cmp al, bl
  jne .become_bad_pw
  cmp al, 0
  je .become_pw_ok
  inc esi
  inc edi
  jmp .become_pw_cmp_loop

.become_pw_ok:
  push 0
  call process_set_uid
  add esp, 4
  mov esi, [current_become_ok_admin]
  call print_line
  jmp .become_done

.become_bad_pw:
  mov esi, [current_become_wrong_pw]
  call print_line
  jmp .become_done

.become_try_user:
  cmp al, 'u'
  jne .become_usage
  cmp byte [esi + 1], 's'
  jne .become_usage
  cmp byte [esi + 2], 'e'
  jne .become_usage
  cmp byte [esi + 3], 'r'
  jne .become_usage
  mov al, [esi + 4]
  cmp al, ' '
  je .become_do_user
  cmp al, 0
  je .become_do_user
  jmp .become_usage

.become_do_user:
  push 1
  call process_set_uid
  add esp, 4
  mov esi, [current_become_ok_user]
  call print_line
  jmp .become_done

.become_denied:
  mov esi, [current_become_denied]
  call print_line
  jmp .become_done

.become_usage:
  mov esi, [current_become_usage]
  call print_line

.become_done:
  pop edi
  pop esi
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
  mov dword [current_help_all], help_all_en
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
  mov dword [current_go_usage], go_usage_en
  mov dword [current_go_ok_admin], go_ok_admin_en
  mov dword [current_go_ok_user], go_ok_user_en
  mov dword [current_go_denied], go_denied_en
  mov dword [current_go_not_found], go_not_found_en
  mov dword [current_go_not_dir], go_not_dir_en
  mov dword [current_become_usage], become_usage_en
  mov dword [current_become_ok_admin], become_ok_admin_en
  mov dword [current_become_ok_user], become_ok_user_en
  mov dword [current_become_denied], become_denied_en
  mov dword [current_become_prompt], become_prompt_en
  mov dword [current_become_wrong_pw], become_wrong_pw_en
  mov dword [current_rm_usage], rm_usage_en
  mov dword [current_rm_not_found], rm_not_found_en
  mov dword [current_rm_ok], rm_ok_en
  mov dword [current_rm_cancelled], rm_cancelled_en
  mov dword [current_rm_confirm], rm_confirm_en
  mov dword [current_rm_yn], rm_yn_en
  mov dword [current_rm_not_empty], rm_not_empty_en
  mov dword [current_rm_cannot_root], rm_cannot_root_en
  mov dword [current_mkdir_usage], mkdir_usage_en
  mov dword [current_mkdir_ok], mkdir_ok_en
  mov dword [current_mkdir_fail], mkdir_fail_en
  mov dword [current_mkdir_exists], mkdir_exists_en
  mov dword [current_cp_usage], cp_usage_en
  mov dword [current_cp_ok], cp_ok_en
  mov dword [current_cp_fail], cp_fail_en
  mov dword [current_cp_exists], cp_exists_en
  mov dword [current_mv_usage], mv_usage_en
  mov dword [current_mv_ok], mv_ok_en
  mov dword [current_mv_fail], mv_fail_en
  mov dword [current_mv_exists], mv_exists_en
  mov dword [current_find_usage], find_usage_en
  mov dword [current_find_not_found], find_not_found_en
  mov dword [current_whoami_root], whoami_root_en
  mov dword [current_whoami_user], whoami_user_en
  mov dword [current_touch_ok], touch_ok_en
  mov dword [current_touch_usage], touch_usage_en
  mov dword [current_touch_fail], touch_fail_en
  mov dword [current_sysinfo_arch], sysinfo_arch_en
  mov dword [current_sysinfo_mem], sysinfo_mem_en
  mov dword [current_sysinfo_proc], sysinfo_proc_en
  mov dword [current_sysinfo_fs], sysinfo_fs_en
  mov dword [current_sysinfo_lang], sysinfo_lang_en
  mov dword [current_hostname_msg], hostname_msg_en
  mov dword [current_head_usage], head_usage_en
  mov dword [current_wc_lines], wc_lines_en
  mov dword [current_wc_usage], wc_usage_en
  mov dword [current_ring3_msg], ring3_msg_en
  mov esi, language_set_en
  call print_string_no_newline
  mov esi, lang_en_name
  call print_line
  ret

set_language_ca:
  mov dword [current_command_table], command_table_ca
  mov dword [current_unknown_cmd], unknown_cmd_ca
  mov dword [current_help_msg], help_msg_ca
  mov dword [current_help_all], help_all_ca
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
  mov dword [current_go_usage], go_usage_ca
  mov dword [current_go_ok_admin], go_ok_admin_ca
  mov dword [current_go_ok_user], go_ok_user_ca
  mov dword [current_go_denied], go_denied_ca
  mov dword [current_go_not_found], go_not_found_ca
  mov dword [current_go_not_dir], go_not_dir_ca
  mov dword [current_become_usage], become_usage_ca
  mov dword [current_become_ok_admin], become_ok_admin_ca
  mov dword [current_become_ok_user], become_ok_user_ca
  mov dword [current_become_denied], become_denied_ca
  mov dword [current_become_prompt], become_prompt_ca
  mov dword [current_become_wrong_pw], become_wrong_pw_ca
  mov dword [current_rm_usage], rm_usage_ca
  mov dword [current_rm_not_found], rm_not_found_ca
  mov dword [current_rm_ok], rm_ok_ca
  mov dword [current_rm_cancelled], rm_cancelled_ca
  mov dword [current_rm_confirm], rm_confirm_ca
  mov dword [current_rm_yn], rm_yn_ca
  mov dword [current_rm_not_empty], rm_not_empty_ca
  mov dword [current_rm_cannot_root], rm_cannot_root_ca
  mov dword [current_mkdir_usage], mkdir_usage_ca
  mov dword [current_mkdir_ok], mkdir_ok_ca
  mov dword [current_mkdir_fail], mkdir_fail_ca
  mov dword [current_mkdir_exists], mkdir_exists_ca
  mov dword [current_cp_usage], cp_usage_ca
  mov dword [current_cp_ok], cp_ok_ca
  mov dword [current_cp_fail], cp_fail_ca
  mov dword [current_cp_exists], cp_exists_ca
  mov dword [current_mv_usage], mv_usage_ca
  mov dword [current_mv_ok], mv_ok_ca
  mov dword [current_mv_fail], mv_fail_ca
  mov dword [current_mv_exists], mv_exists_ca
  mov dword [current_find_usage], find_usage_ca
  mov dword [current_find_not_found], find_not_found_ca
  mov dword [current_whoami_root], whoami_root_ca
  mov dword [current_whoami_user], whoami_user_ca
  mov dword [current_touch_ok], touch_ok_ca
  mov dword [current_touch_usage], touch_usage_ca
  mov dword [current_touch_fail], touch_fail_ca
  mov dword [current_sysinfo_arch], sysinfo_arch_ca
  mov dword [current_sysinfo_mem], sysinfo_mem_ca
  mov dword [current_sysinfo_proc], sysinfo_proc_ca
  mov dword [current_sysinfo_fs], sysinfo_fs_ca
  mov dword [current_sysinfo_lang], sysinfo_lang_ca
  mov dword [current_hostname_msg], hostname_msg_ca
  mov dword [current_head_usage], head_usage_ca
  mov dword [current_wc_lines], wc_lines_ca
  mov dword [current_wc_usage], wc_usage_ca
  mov dword [current_ring3_msg], ring3_msg_ca
  mov esi, language_set_ca
  call print_string_no_newline
  mov esi, lang_ca_name
  call print_line
  ret

set_language_es:
  mov dword [current_command_table], command_table_es
  mov dword [current_unknown_cmd], unknown_cmd_es
  mov dword [current_help_msg], help_msg_es
  mov dword [current_help_all], help_all_es
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
  mov dword [current_go_usage], go_usage_es
  mov dword [current_go_ok_admin], go_ok_admin_es
  mov dword [current_go_ok_user], go_ok_user_es
  mov dword [current_go_denied], go_denied_es
  mov dword [current_go_not_found], go_not_found_es
  mov dword [current_go_not_dir], go_not_dir_es
  mov dword [current_become_usage], become_usage_es
  mov dword [current_become_ok_admin], become_ok_admin_es
  mov dword [current_become_ok_user], become_ok_user_es
  mov dword [current_become_denied], become_denied_es
  mov dword [current_become_prompt], become_prompt_es
  mov dword [current_become_wrong_pw], become_wrong_pw_es
  mov dword [current_rm_usage], rm_usage_es
  mov dword [current_rm_not_found], rm_not_found_es
  mov dword [current_rm_ok], rm_ok_es
  mov dword [current_rm_cancelled], rm_cancelled_es
  mov dword [current_rm_confirm], rm_confirm_es
  mov dword [current_rm_yn], rm_yn_es
  mov dword [current_rm_not_empty], rm_not_empty_es
  mov dword [current_rm_cannot_root], rm_cannot_root_es
  mov dword [current_mkdir_usage], mkdir_usage_es
  mov dword [current_mkdir_ok], mkdir_ok_es
  mov dword [current_mkdir_fail], mkdir_fail_es
  mov dword [current_mkdir_exists], mkdir_exists_es
  mov dword [current_cp_usage], cp_usage_es
  mov dword [current_cp_ok], cp_ok_es
  mov dword [current_cp_fail], cp_fail_es
  mov dword [current_cp_exists], cp_exists_es
  mov dword [current_mv_usage], mv_usage_es
  mov dword [current_mv_ok], mv_ok_es
  mov dword [current_mv_fail], mv_fail_es
  mov dword [current_mv_exists], mv_exists_es
  mov dword [current_find_usage], find_usage_es
  mov dword [current_find_not_found], find_not_found_es
  mov dword [current_whoami_root], whoami_root_es
  mov dword [current_whoami_user], whoami_user_es
  mov dword [current_touch_ok], touch_ok_es
  mov dword [current_touch_usage], touch_usage_es
  mov dword [current_touch_fail], touch_fail_es
  mov dword [current_sysinfo_arch], sysinfo_arch_es
  mov dword [current_sysinfo_mem], sysinfo_mem_es
  mov dword [current_sysinfo_proc], sysinfo_proc_es
  mov dword [current_sysinfo_fs], sysinfo_fs_es
  mov dword [current_sysinfo_lang], sysinfo_lang_es
  mov dword [current_hostname_msg], hostname_msg_es
  mov dword [current_head_usage], head_usage_es
  mov dword [current_wc_lines], wc_lines_es
  mov dword [current_wc_usage], wc_usage_es
  mov dword [current_ring3_msg], ring3_msg_es
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
  dd cmd_panic_en, handle_panic
  dd cmd_language_en, handle_language
  dd cmd_ls_en, handle_ls
  dd cmd_cat_en, handle_cat
  dd cmd_disk_en, handle_disk
  dd cmd_echo_en, handle_echo
  dd cmd_uptime_en, handle_uptime
  dd cmd_ps_en, handle_ps
  dd cmd_spawn_en, handle_spawn
  dd cmd_write_en, handle_write
  dd cmd_go_en, handle_go
  dd cmd_become_en, handle_become
  dd cmd_bc_en, handle_become
  dd cmd_rm_en, handle_rm
  dd cmd_mkdir_en, handle_mkdir
  dd cmd_cp_en, handle_cp
  dd cmd_mv_en, handle_mv
  dd cmd_find_en, handle_find
  dd cmd_whoami_en, handle_whoami
  dd cmd_pwd_en, handle_pwd
  dd cmd_touch_en, handle_touch
  dd cmd_sysinfo_en, handle_sysinfo
  dd cmd_hostname_en, handle_hostname
  dd cmd_head_en, handle_head
  dd cmd_wc_en, handle_wc
  dd cmd_ring3_en, handle_ring3
  dd 0, 0

command_table_ca:
  dd cmd_alloc_ca, handle_alloc
  dd cmd_help_ca, handle_help
  dd cmd_memstat_ca, handle_memstat
  dd cmd_free_ca, handle_free
  dd cmd_clear_ca, handle_clear
  dd cmd_panic_ca, handle_panic
  dd cmd_language_ca, handle_language
  dd cmd_ls_ca, handle_ls
  dd cmd_cat_ca, handle_cat
  dd cmd_disk_ca, handle_disk
  dd cmd_echo_ca, handle_echo
  dd cmd_uptime_ca, handle_uptime
  dd cmd_ps_ca, handle_ps
  dd cmd_spawn_ca, handle_spawn
  dd cmd_write_ca, handle_write
  dd cmd_go_ca, handle_go
  dd cmd_become_ca, handle_become
  dd cmd_bc_ca, handle_become
  dd cmd_rm_ca, handle_rm
  dd cmd_mkdir_ca, handle_mkdir
  dd cmd_cp_ca, handle_cp
  dd cmd_mv_ca, handle_mv
  dd cmd_find_ca, handle_find
  dd cmd_whoami_ca, handle_whoami
  dd cmd_pwd_ca, handle_pwd
  dd cmd_touch_ca, handle_touch
  dd cmd_sysinfo_ca, handle_sysinfo
  dd cmd_hostname_ca, handle_hostname
  dd cmd_head_ca, handle_head
  dd cmd_wc_ca, handle_wc
  dd cmd_ring3_ca, handle_ring3
  dd 0, 0

command_table_es:
  dd cmd_alloc_es, handle_alloc
  dd cmd_help_es, handle_help
  dd cmd_memstat_es, handle_memstat
  dd cmd_free_es, handle_free
  dd cmd_clear_es, handle_clear
  dd cmd_panic_es, handle_panic
  dd cmd_language_es, handle_language
  dd cmd_ls_es, handle_ls
  dd cmd_cat_es, handle_cat
  dd cmd_disk_es, handle_disk
  dd cmd_echo_es, handle_echo
  dd cmd_uptime_es, handle_uptime
  dd cmd_ps_es, handle_ps
  dd cmd_spawn_es, handle_spawn
  dd cmd_write_es, handle_write
  dd cmd_go_es, handle_go
  dd cmd_become_es, handle_become
  dd cmd_bc_es, handle_become
  dd cmd_rm_es, handle_rm
  dd cmd_mkdir_es, handle_mkdir
  dd cmd_cp_es, handle_cp
  dd cmd_mv_es, handle_mv
  dd cmd_find_es, handle_find
  dd cmd_whoami_es, handle_whoami
  dd cmd_pwd_es, handle_pwd
  dd cmd_touch_es, handle_touch
  dd cmd_sysinfo_es, handle_sysinfo
  dd cmd_hostname_es, handle_hostname
  dd cmd_head_es, handle_head
  dd cmd_wc_es, handle_wc
  dd cmd_ring3_es, handle_ring3
;  dd cmd_rm_es_alias, handle_rm
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
cat_path_buf:   resb 64      ; path for vfs_lookup (e.g. /readme.txt or /sistem/file)
go_path_buf:    resb 64     ; path for go (cd) command

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
current_help_all: dd help_all_en
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
wc_bytes_str: db ' bytes  ', 0
find_root_path: db '/', 0
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
current_go_usage:     dd go_usage_en
current_go_ok_admin:  dd go_ok_admin_en
current_go_ok_user:   dd go_ok_user_en
current_go_denied:    dd go_denied_en
current_go_not_found: dd go_not_found_en
current_go_not_dir:   dd go_not_dir_en
current_become_usage:   dd become_usage_en
current_become_ok_admin: dd become_ok_admin_en
current_become_ok_user:  dd become_ok_user_en
current_become_denied:   dd become_denied_en
current_become_prompt:   dd become_prompt_en
current_become_wrong_pw: dd become_wrong_pw_en
current_rm_usage:       dd rm_usage_en
current_rm_not_found:   dd rm_not_found_en
current_rm_ok:          dd rm_ok_en
current_rm_cancelled:   dd rm_cancelled_en
current_rm_confirm:     dd rm_confirm_en
current_rm_yn:          dd rm_yn_en
current_rm_not_empty:   dd rm_not_empty_en
current_rm_cannot_root: dd rm_cannot_root_en
current_mkdir_usage:    dd mkdir_usage_en
current_mkdir_ok:        dd mkdir_ok_en
current_mkdir_fail:      dd mkdir_fail_en
current_mkdir_exists:    dd mkdir_exists_en
current_cp_usage:       dd cp_usage_en
current_cp_ok:          dd cp_ok_en
current_cp_fail:        dd cp_fail_en
current_cp_exists:      dd cp_exists_en
current_mv_usage:       dd mv_usage_en
current_mv_ok:          dd mv_ok_en
current_mv_fail:        dd mv_fail_en
current_mv_exists:      dd mv_exists_en
current_find_usage:     dd find_usage_en
current_find_not_found: dd find_not_found_en

; New command message pointers (Phase 6)
current_whoami_root:    dd whoami_root_en
current_whoami_user:    dd whoami_user_en
current_touch_ok:       dd touch_ok_en
current_touch_usage:    dd touch_usage_en
current_touch_fail:     dd touch_fail_en
current_sysinfo_arch:   dd sysinfo_arch_en
current_sysinfo_mem:    dd sysinfo_mem_en
current_sysinfo_proc:   dd sysinfo_proc_en
current_sysinfo_fs:     dd sysinfo_fs_en
current_sysinfo_lang:   dd sysinfo_lang_en
current_hostname_msg:   dd hostname_msg_en
current_head_usage:     dd head_usage_en
current_wc_lines:       dd wc_lines_en
current_wc_usage:       dd wc_usage_en
current_ring3_msg:      dd ring3_msg_en

; Static admin password (same literal in all languages for now)
admin_password: db 'admin', 0
