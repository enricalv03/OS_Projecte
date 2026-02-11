[bits 32]

section .data

; Command names (Catalan)
global cmd_alloc_ca, cmd_memstat_ca, cmd_help_ca, cmd_free_ca, cmd_clear_ca, cmd_language_ca
global cmd_ls_ca, cmd_cat_ca, cmd_disk_ca
global cmd_echo_ca, cmd_uptime_ca, cmd_ps_ca, cmd_write_ca

cmd_alloc_ca:    db 'assigna', 0
cmd_memstat_ca:  db 'memoria', 0
cmd_help_ca:     db 'ajuda', 0
cmd_free_ca:     db 'allibera', 0
cmd_clear_ca:    db 'neteja', 0
cmd_language_ca: db 'idioma', 0
cmd_ls_ca:       db 'llista', 0
cmd_cat_ca:      db 'mostra', 0
cmd_disk_ca:     db 'disc', 0
cmd_echo_ca:     db 'eco', 0
cmd_uptime_ca:   db 'temps', 0
cmd_ps_ca:       db 'processos', 0
cmd_write_ca:    db 'escriu', 0

; Messages (Catalan)
global unknown_cmd_ca, help_msg_ca
global memstat_free_msg_ca, memstat_used_msg_ca
global alloc_success_msg_ca, alloc_oom_msg_ca
global free_ok_msg_ca, free_fail_msg_ca, free_usage_msg_ca
global language_current_ca, language_available_ca, language_set_ca
global file_not_found_ca
global disk_error_ca, disk_usage_ca, disk_reading_ca
global uptime_msg_ca, uptime_sec_ca
global ps_header_ca
global write_ok_ca, write_usage_ca, write_fail_ca

unknown_cmd_ca:      db 'Comanda desconeguda', 0
help_msg_ca:         db 'Comandes: ajuda neteja eco llista mostra escriu assigna allibera memoria disc processos temps idioma', 0
memstat_free_msg_ca: db 'Pagines lliures: ', 0
memstat_used_msg_ca: db 'Pagines usades: ', 0
alloc_success_msg_ca: db 'Pagina assignada: 0x', 0
alloc_oom_msg_ca:     db 'Memoria fisica esgotada', 0
free_ok_msg_ca:       db 'Pagina alliberada correctament', 0
free_fail_msg_ca:     db 'Pagina invalida o ja lliure', 0
free_usage_msg_ca:    db 'Us: allibera <adreca_hex>', 0
language_current_ca:  db 'Idioma actual: Catala', 0
language_available_ca: db 'Disponibles: lang/language/idioma [en/eng/es/esp/ca/cat]', 0
language_set_ca:      db 'Idioma canviat a: ', 0
file_not_found_ca:    db 'Fitxer no trobat', 0
disk_error_ca:        db 'Error llegint sector del disc', 0
disk_usage_ca:        db 'Us: disc <sector>', 0
disk_reading_ca:      db 'Sector 0x', 0
uptime_msg_ca:        db 'Temps actiu: ', 0
uptime_sec_ca:        db ' s)', 0
ps_header_ca:         db 'PID   ESTAT    PRI   NOM', 0
write_ok_ca:          db 'Fitxer creat', 0
write_usage_ca:       db 'Us: escriu <fitxer> <contingut>', 0
write_fail_ca:        db 'Error: sistema de fitxers ple o invalid', 0

