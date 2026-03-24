[bits 32]

section .data

; Command names (English)
global cmd_alloc_en, cmd_memstat_en, cmd_help_en, cmd_free_en, cmd_clear_en, cmd_language_en
global cmd_ls_en, cmd_cat_en, cmd_disk_en
global cmd_echo_en, cmd_uptime_en, cmd_ps_en, cmd_write_en, cmd_go_en, cmd_become_en, cmd_bc_en
global cmd_rm_en
global cmd_mkdir_en
global cmd_cp_en
global cmd_mv_en
global cmd_find_en
global cmd_panic_en
global cmd_spawn_en
global cmd_whoami_en
global cmd_pwd_en
global cmd_touch_en
global cmd_sysinfo_en
global cmd_hostname_en
global cmd_head_en
global cmd_wc_en
global cmd_ring3_en

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
cmd_go_en:       db 'go', 0
cmd_become_en:   db 'become', 0
cmd_bc_en:       db 'bc', 0
cmd_rm_en:       db 'rm', 0
cmd_mkdir_en:    db 'mkdir', 0
cmd_cp_en:       db 'cp', 0
cmd_mv_en:       db 'mv', 0
cmd_find_en:     db 'find', 0
cmd_panic_en:    db 'panic', 0
cmd_spawn_en:    db 'spawn', 0
cmd_whoami_en:   db 'whoami', 0
cmd_pwd_en:      db 'pwd', 0
cmd_touch_en:    db 'touch', 0
cmd_sysinfo_en:  db 'sysinfo', 0
cmd_hostname_en: db 'hostname', 0
cmd_head_en:     db 'head', 0
cmd_wc_en:       db 'wc', 0
cmd_ring3_en:    db 'ring3', 0

; Messages (English)
global unknown_cmd_en, help_msg_en, help_all_en
global memstat_free_msg_en, memstat_used_msg_en
global alloc_success_msg_en, alloc_oom_msg_en
global free_ok_msg_en, free_fail_msg_en, free_usage_msg_en
global language_current_en, language_available_en, language_set_en
global file_not_found_en
global disk_error_en, disk_usage_en, disk_reading_en
global uptime_msg_en, uptime_sec_en
global ps_header_en
global write_ok_en, write_usage_en, write_fail_en
global go_usage_en, go_ok_admin_en, go_ok_user_en, go_denied_en
global go_not_found_en, go_not_dir_en
global become_usage_en, become_ok_admin_en, become_ok_user_en, become_denied_en
global become_prompt_en, become_wrong_pw_en
global rm_usage_en, rm_not_found_en, rm_ok_en, rm_cancelled_en, rm_confirm_en, rm_yn_en, rm_not_empty_en, rm_cannot_root_en
global mkdir_usage_en, mkdir_ok_en, mkdir_fail_en, mkdir_exists_en
global cp_usage_en, cp_ok_en, cp_fail_en, cp_exists_en
global mv_usage_en, mv_ok_en, mv_fail_en, mv_exists_en
global find_usage_en, find_not_found_en
global whoami_root_en, whoami_user_en
global touch_ok_en, touch_usage_en, touch_fail_en
global sysinfo_arch_en, sysinfo_mem_en, sysinfo_proc_en, sysinfo_fs_en, sysinfo_lang_en
global hostname_msg_en
global head_usage_en
global wc_lines_en, wc_usage_en

unknown_cmd_en:      db 'Unknown command', 0
help_msg_en:         db 'ls  cat  write  mkdir  cp  mv  rm  find  go  clear  echo', 10, 'whoami  pwd  touch  sysinfo  head  wc  hostname', 10, 'Type "help all" for full command reference', 0
help_all_en:
  db '--- Files ---', 0
  db '  ls [path]            List directory contents', 0
  db '  cat <file>           Show file content', 0
  db '  write <file> <text>  Create a file', 0
  db '  mkdir <path>         Create a directory', 0
  db '  cp <src> <dest>      Copy a file', 0
  db '  mv <src> <dest>      Move or rename', 0
  db '  rm <path>            Delete file or directory', 0
  db '  find [path] <name>   Search by name', 0
  db '  go <path>            Change directory (go ..)', 0
  db '--- System ---', 0
  db '  clear                Clear the screen', 0
  db '  echo <text>          Print text', 0
  db '  ps                   List processes', 0
  db '  uptime               System uptime', 0
  db '  memory               Memory statistics', 0
  db '  alloc                Allocate a page', 0
  db '  free <hex>           Free a page', 0
  db '  disk <sector>        Read a disk sector', 0
  db '--- Info ---', 0
  db '  whoami               Show current user', 0
  db '  pwd                  Print working directory', 0
  db '  hostname             Show system hostname', 0
  db '  sysinfo              Show system information', 0
  db '  head <file>          Show first 5 lines of file', 0
  db '  wc <file>            Count lines and bytes', 0
  db '  touch <file>         Create an empty file', 0
  db '--- User ---', 0
  db '  become admin|user    Switch user (shortcut: bc)', 0
  db '  language [en|ca|es]  Change language (lang/idioma)', 0
  db '--- Advanced ---', 0
  db '  ring3                Test ring-3 user mode (no return)', 0
  db 0
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
go_usage_en:          db 'Usage: go <path>', 0
go_ok_admin_en:       db 'Now you are root (admin)', 0
go_ok_user_en:        db 'Now you are normal user', 0
go_denied_en:         db 'Only root can switch user', 0
go_not_found_en:      db 'No such path', 0
go_not_dir_en:        db 'Not a directory', 0
become_usage_en:      db 'Usage: become admin | become user  (or bc admin | bc user)', 0
become_ok_admin_en:   db 'Now you are root (admin)', 0
become_ok_user_en:    db 'Now you are normal user', 0
become_denied_en:     db 'Only root can switch user', 0
become_prompt_en:     db 'Password for admin: ', 0
become_wrong_pw_en:   db 'Wrong password', 0
rm_usage_en:         db 'Usage: rm <path>', 0
rm_not_found_en:     db 'No such file or directory', 0
rm_ok_en:            db 'Removed', 0
rm_cancelled_en:     db 'Cancelled', 0
rm_confirm_en:       db 'Are you sure you want to delete the directory ', 0
rm_yn_en:            db '? y/n?', 0
rm_not_empty_en:     db 'Directory not empty', 0
rm_cannot_root_en:   db 'Cannot remove root', 0
mkdir_usage_en:      db 'Usage: mkdir <path>', 0
mkdir_ok_en:         db 'Directory created', 0
mkdir_fail_en:       db 'Cannot create directory', 0
mkdir_exists_en:     db 'File or directory already exists', 0
cp_usage_en:         db 'Usage: cp <source> <dest>', 0
cp_ok_en:            db 'File copied', 0
cp_fail_en:          db 'Cannot copy: source not found or not a file', 0
cp_exists_en:        db 'Cannot copy: destination already exists', 0
mv_usage_en:         db 'Usage: mv <source> <dest>', 0
mv_ok_en:            db 'Moved', 0
mv_fail_en:          db 'Cannot move: source not found or dest parent invalid', 0
mv_exists_en:        db 'Cannot move: destination already exists', 0
find_usage_en:       db 'Usage: find [path] <name>', 0
find_not_found_en:   db 'No matches found', 0
whoami_root_en:      db 'admin (root)', 0
whoami_user_en:      db 'user', 0
touch_ok_en:         db 'File created (empty)', 0
touch_usage_en:      db 'Usage: touch <filename>', 0
touch_fail_en:       db 'Cannot create file', 0
sysinfo_arch_en:     db 'Architecture: x86 (i686)', 0
sysinfo_mem_en:      db 'Free pages: ', 0
sysinfo_proc_en:     db 'Active processes: ', 0
sysinfo_fs_en:       db 'Root filesystem: RAMFS', 0
sysinfo_lang_en:     db 'Language: ', 0
hostname_msg_en:     db 'MyOS', 0
head_usage_en:       db 'Usage: head <file>', 0
wc_lines_en:         db ' lines, ', 0
wc_usage_en:         db 'Usage: wc <file>', 0

global ring3_msg_en
ring3_msg_en:        db 'Switching to ring-3 user mode...', 0
