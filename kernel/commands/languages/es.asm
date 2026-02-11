[bits 32]

section .data

; Command names (Spanish)
global cmd_alloc_es, cmd_memstat_es, cmd_help_es, cmd_free_es, cmd_clear_es, cmd_language_es
global cmd_ls_es, cmd_cat_es, cmd_disk_es
global cmd_echo_es, cmd_uptime_es, cmd_ps_es, cmd_write_es

cmd_alloc_es:    db 'asignar', 0
cmd_memstat_es:  db 'memoria', 0
cmd_help_es:     db 'ayuda', 0
cmd_free_es:     db 'liberar', 0
cmd_clear_es:    db 'limpiar', 0
cmd_language_es: db 'idioma', 0
cmd_ls_es:       db 'listar', 0
cmd_cat_es:      db 'ver', 0
cmd_disk_es:     db 'disco', 0
cmd_echo_es:     db 'eco', 0
cmd_uptime_es:   db 'tiempo', 0
cmd_ps_es:       db 'procesos', 0
cmd_write_es:    db 'escribir', 0

; Messages (Spanish)
global unknown_cmd_es, help_msg_es
global memstat_free_msg_es, memstat_used_msg_es
global alloc_success_msg_es, alloc_oom_msg_es
global free_ok_msg_es, free_fail_msg_es, free_usage_msg_es
global language_current_es, language_available_es, language_set_es
global file_not_found_es
global disk_error_es, disk_usage_es, disk_reading_es
global uptime_msg_es, uptime_sec_es
global ps_header_es
global write_ok_es, write_usage_es, write_fail_es

unknown_cmd_es:      db 'Comando desconocido', 0
help_msg_es:         db 'Comandos: ayuda limpiar eco listar ver escribir asignar liberar memoria disco procesos tiempo idioma', 0
memstat_free_msg_es: db 'Paginas libres: ', 0
memstat_used_msg_es: db 'Paginas usadas: ', 0
alloc_success_msg_es: db 'Pagina asignada: 0x', 0
alloc_oom_msg_es:     db 'Memoria fisica agotada', 0
free_ok_msg_es:       db 'Pagina liberada correctamente', 0
free_fail_msg_es:     db 'Pagina invalida o ya libre', 0
free_usage_msg_es:    db 'Uso: liberar <direccion_hex>', 0
language_current_es:  db 'Idioma actual: Espanol', 0
language_available_es: db 'Disponibles: lang/language/idioma [en/eng/es/esp/ca/cat]', 0
language_set_es:      db 'Idioma cambiado a: ', 0
file_not_found_es:    db 'Archivo no encontrado', 0
disk_error_es:        db 'Error leyendo sector del disco', 0
disk_usage_es:        db 'Uso: disco <sector>', 0
disk_reading_es:      db 'Sector 0x', 0
uptime_msg_es:        db 'Tiempo activo: ', 0
uptime_sec_es:        db ' s)', 0
ps_header_es:         db 'PID   ESTADO   PRI   NOMBRE', 0
write_ok_es:          db 'Archivo creado', 0
write_usage_es:       db 'Uso: escribir <archivo> <contenido>', 0
write_fail_es:        db 'Error: sistema de archivos lleno o invalido', 0

