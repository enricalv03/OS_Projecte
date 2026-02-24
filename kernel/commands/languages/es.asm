[bits 32]

section .data

; Command names (Spanish)
global cmd_alloc_es, cmd_memstat_es, cmd_help_es, cmd_free_es, cmd_clear_es, cmd_language_es
global cmd_ls_es, cmd_cat_es, cmd_disk_es
global cmd_echo_es, cmd_uptime_es, cmd_ps_es, cmd_write_es, cmd_go_es, cmd_become_es, cmd_bc_es
global cmd_rm_es
global cmd_mkdir_es
global cmd_cp_es
global cmd_mv_es
global cmd_find_es

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
cmd_go_es:       db 'ir', 0
cmd_become_es:   db 'cambiarusuario', 0
cmd_bc_es:       db 'cu', 0
cmd_rm_es:       db 'borrar', 0
cmd_rm_es_alias: db 'rm', 0
cmd_mkdir_es:    db 'creadir', 0
cmd_cp_es:       db 'copiar', 0
cmd_mv_es:       db 'mover', 0
cmd_find_es:     db 'buscar', 0

; Messages (Spanish)
global unknown_cmd_es, help_msg_es, help_all_es
global memstat_free_msg_es, memstat_used_msg_es
global alloc_success_msg_es, alloc_oom_msg_es
global free_ok_msg_es, free_fail_msg_es, free_usage_msg_es
global language_current_es, language_available_es, language_set_es
global file_not_found_es
global disk_error_es, disk_usage_es, disk_reading_es
global uptime_msg_es, uptime_sec_es
global ps_header_es
global write_ok_es, write_usage_es, write_fail_es
global go_usage_es, go_ok_admin_es, go_ok_user_es, go_denied_es
global go_not_found_es, go_not_dir_es
global become_usage_es, become_ok_admin_es, become_ok_user_es, become_denied_es
global rm_usage_es, rm_not_found_es, rm_ok_es, rm_cancelled_es, rm_confirm_es, rm_yn_es, rm_not_empty_es, rm_cannot_root_es
global mkdir_usage_es, mkdir_ok_es, mkdir_fail_es, mkdir_exists_es
global cp_usage_es, cp_ok_es, cp_fail_es, cp_exists_es
global mv_usage_es, mv_ok_es, mv_fail_es, mv_exists_es
global find_usage_es, find_not_found_es

unknown_cmd_es:      db 'Comando desconocido', 0
help_msg_es:         db 'listar  ver  escribir  creadir  copiar  mover  borrar  buscar  ir  limpiar  eco', 10, 'Escribe "ayuda all" para ver todos los comandos', 0
help_all_es:
  db '--- Archivos ---', 0
  db '  listar [ruta]          Lista el contenido', 0
  db '  ver <archivo>          Muestra contenido', 0
  db '  escribir <arch> <txt>  Crea un archivo', 0
  db '  creadir <ruta>         Crea un directorio', 0
  db '  copiar <org> <dest>    Copia un archivo', 0
  db '  mover <org> <dest>     Mueve o renombra', 0
  db '  borrar <ruta>          Elimina archivo o directorio', 0
  db '  buscar [ruta] <nombre> Busca por nombre', 0
  db '  ir <ruta>              Cambia de directorio (ir ..)', 0
  db '--- Sistema ---', 0
  db '  limpiar                Limpia la pantalla', 0
  db '  eco <texto>            Imprime texto', 0
  db '  procesos               Lista procesos', 0
  db '  tiempo                 Tiempo activo del sistema', 0
  db '  memoria                Estadisticas de memoria', 0
  db '  asignar                Asigna una pagina', 0
  db '  liberar <hex>          Libera una pagina', 0
  db '  disco <sector>         Lee sector del disco', 0
  db '--- Usuario ---', 0
  db '  cambiarusuario admin|user  Cambia usuario (atajo: cu)', 0
  db '  idioma [en|ca|es]      Cambia idioma (lang/language)', 0
  db 0
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
go_usage_es:          db 'Uso: ir <ruta>', 0
go_ok_admin_es:       db 'Ahora eres root (admin)', 0
go_ok_user_es:        db 'Ahora eres usuario normal', 0
go_denied_es:         db 'Solo root puede cambiar de usuario', 0
go_not_found_es:      db 'Ruta no encontrada', 0
go_not_dir_es:        db 'No es un directorio', 0
become_usage_es:      db 'Uso: cambiarusuario admin | cambiarusuario usuario  (o cu admin | cu usuario)', 0
become_ok_admin_es:   db 'Ahora eres root (admin)', 0
become_ok_user_es:    db 'Ahora eres usuario normal', 0
become_denied_es:     db 'Solo root puede cambiar de usuario', 0
rm_usage_es:         db 'Uso: rm <ruta>', 0
rm_not_found_es:     db 'Archivo o directorio no encontrado', 0
rm_ok_es:            db 'Eliminado', 0
rm_cancelled_es:     db 'Cancelado', 0
rm_confirm_es:       db 'Seguro que quieres eliminar el directorio ', 0
rm_yn_es:            db ' (puede contener archivos)? s/n?', 0
rm_not_empty_es:     db 'El directorio no esta vacio', 0
rm_cannot_root_es:   db 'No se puede eliminar la raiz', 0
mkdir_usage_es:      db 'Uso: creadir <ruta>', 0
mkdir_ok_es:         db 'Directorio creado', 0
mkdir_fail_es:       db 'No se puede crear el directorio', 0
mkdir_exists_es:     db 'El archivo o directorio ya existe', 0
cp_usage_es:         db 'Uso: copiar <origen> <destino>', 0
cp_ok_es:            db 'Archivo copiado', 0
cp_fail_es:          db 'No se puede copiar: origen no encontrado o no es un archivo', 0
cp_exists_es:        db 'No se puede copiar: el destino ya existe', 0
mv_usage_es:         db 'Uso: mover <origen> <destino>', 0
mv_ok_es:            db 'Movido', 0
mv_fail_es:          db 'No se puede mover: origen no encontrado o destino invalido', 0
mv_exists_es:        db 'No se puede mover: el destino ya existe', 0
find_usage_es:       db 'Uso: buscar [ruta] <nombre>', 0
find_not_found_es:   db 'Ninguna coincidencia encontrada', 0

