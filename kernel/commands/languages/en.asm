[bits 32]

section .data

; Command names (English)
global cmd_alloc_en, cmd_memstat_en, cmd_help_en, cmd_free_en, cmd_clear_en, cmd_language_en
global cmd_ls_en, cmd_cat_en, cmd_disk_en
global cmd_echo_en, cmd_uptime_en, cmd_ps_en, cmd_write_en

cmd_alloc_en:    db 'alloc', 0
cmd_memstat_en:  db 'memory', 0
cmd_help_en:     db 'help', 0
cmd_free_en:     db 'free', 0
cmd_clear_en:    db 'clear', 0
cmd_language_en: db 'language', 0
cmd_ls_en:       db 'ls', 0
cmd_cat_en:      db 'cat', 0
cmd_disk_en:     db 'disk', 0
cmd_echo_en:     db 'echo', 0
cmd_uptime_en:   db 'uptime', 0
cmd_ps_en:       db 'ps', 0
cmd_write_en:    db 'write', 0

; Messages (English)
global unknown_cmd_en, help_msg_en
global memstat_free_msg_en, memstat_used_msg_en
global alloc_success_msg_en, alloc_oom_msg_en
global free_ok_msg_en, free_fail_msg_en, free_usage_msg_en
global language_current_en, language_available_en, language_set_en
global file_not_found_en
global disk_error_en, disk_usage_en, disk_reading_en
global uptime_msg_en, uptime_sec_en
global ps_header_en
global write_ok_en, write_usage_en, write_fail_en

unknown_cmd_en:      db 'Unknown command', 0
help_msg_en:         db 'Commands: help clear echo ls cat write alloc free memory disk ps uptime language', 0
memstat_free_msg_en: db 'Free pages: ', 0
memstat_used_msg_en: db 'Used pages: ', 0
alloc_success_msg_en: db 'Allocated page: 0x', 0
alloc_oom_msg_en:     db 'Out of physical memory', 0
free_ok_msg_en:       db 'Page freed successfully', 0
free_fail_msg_en:     db 'Invalid page or already free', 0
free_usage_msg_en:    db 'Usage: free <hexaddr>', 0
language_current_en:  db 'Current language: English', 0
language_available_en: db 'Available: lang/language/idioma [en/eng/es/esp/ca/cat]', 0
language_set_en:      db 'Language set to: ', 0
file_not_found_en:    db 'File not found', 0
disk_error_en:        db 'Error reading disk sector', 0
disk_usage_en:        db 'Usage: disk <sector>', 0
disk_reading_en:      db 'Sector 0x', 0
uptime_msg_en:        db 'Uptime: ', 0
uptime_sec_en:        db ' s)', 0
ps_header_en:         db 'PID   STATE    PRI   NAME', 0
write_ok_en:          db 'File created', 0
write_usage_en:       db 'Usage: write <filename> <content>', 0
write_fail_en:        db 'Error: filesystem full or invalid', 0

