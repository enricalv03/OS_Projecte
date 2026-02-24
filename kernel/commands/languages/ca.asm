[bits 32]

section .data

; Command names (Catalan)
global cmd_alloc_ca, cmd_memstat_ca, cmd_help_ca, cmd_free_ca, cmd_clear_ca, cmd_language_ca
global cmd_ls_ca, cmd_cat_ca, cmd_disk_ca
global cmd_echo_ca, cmd_uptime_ca, cmd_ps_ca, cmd_write_ca, cmd_go_ca, cmd_become_ca, cmd_bc_ca
global cmd_rm_ca
global cmd_mkdir_ca
global cmd_cp_ca
global cmd_mv_ca
global cmd_find_ca

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
cmd_go_ca:       db 'ves', 0
cmd_become_ca:   db 'canviausuari', 0
cmd_bc_ca:       db 'cu', 0
cmd_rm_ca:       db 'esborrar', 0
cmd_rm_ca_alias: db 'rm', 0
cmd_mkdir_ca:    db 'creadir', 0
cmd_cp_ca:       db 'copia', 0
cmd_mv_ca:       db 'mou', 0
cmd_find_ca:     db 'cerca', 0

; Messages (Catalan)
global unknown_cmd_ca, help_msg_ca, help_all_ca
global memstat_free_msg_ca, memstat_used_msg_ca
global alloc_success_msg_ca, alloc_oom_msg_ca
global free_ok_msg_ca, free_fail_msg_ca, free_usage_msg_ca
global language_current_ca, language_available_ca, language_set_ca
global file_not_found_ca
global disk_error_ca, disk_usage_ca, disk_reading_ca
global uptime_msg_ca, uptime_sec_ca
global ps_header_ca
global write_ok_ca, write_usage_ca, write_fail_ca
global go_usage_ca, go_ok_admin_ca, go_ok_user_ca, go_denied_ca
global go_not_found_ca, go_not_dir_ca
global become_usage_ca, become_ok_admin_ca, become_ok_user_ca, become_denied_ca
global rm_usage_ca, rm_not_found_ca, rm_ok_ca, rm_cancelled_ca, rm_confirm_ca, rm_yn_ca, rm_not_empty_ca, rm_cannot_root_ca
global mkdir_usage_ca, mkdir_ok_ca, mkdir_fail_ca, mkdir_exists_ca
global cp_usage_ca, cp_ok_ca, cp_fail_ca, cp_exists_ca
global mv_usage_ca, mv_ok_ca, mv_fail_ca, mv_exists_ca
global find_usage_ca, find_not_found_ca

unknown_cmd_ca:      db 'Comanda desconeguda', 0
help_msg_ca:         db 'llista  mostra  escriu  creadir  copia  mou  esborrar  cerca  ves  neteja  eco', 10, 'Escriu "ajuda all" per veure totes les comandes', 0
help_all_ca:
  db '--- Fitxers ---', 0
  db '  llista [cami]          Llista el contingut', 0
  db '  mostra <fitxer>        Mostra contingut del fitxer', 0
  db '  escriu <fitxer> <text> Crea un fitxer', 0
  db '  creadir <cami>         Crea un directori', 0
  db '  copia <org> <dest>     Copia un fitxer', 0
  db '  mou <org> <dest>       Mou o reanomena', 0
  db '  esborrar <cami>        Elimina fitxer o directori', 0
  db '  cerca [cami] <nom>     Cerca per nom', 0
  db '  ves <cami>             Canvia de directori (ves ..)', 0
  db '--- Sistema ---', 0
  db '  neteja                 Neteja la pantalla', 0
  db '  eco <text>             Imprimeix text', 0
  db '  processos              Llista processos', 0
  db '  temps                  Temps actiu del sistema', 0
  db '  memoria                Estadistiques de memoria', 0
  db '  assigna                Assigna una pagina', 0
  db '  allibera <hex>         Allibera una pagina', 0
  db '  disc <sector>          Llegeix sector del disc', 0
  db '--- Usuari ---', 0
  db '  canviausuari admin|user  Canvia usuari (drecera: cu)', 0
  db '  idioma [en|ca|es]      Canvia idioma (lang/language)', 0
  db 0
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
go_usage_ca:          db 'Us: ves <camí>', 0
go_ok_admin_ca:       db 'Ara ets root (admin)', 0
go_ok_user_ca:        db 'Ara ets usuari normal', 0
go_denied_ca:         db "Només root pot canviar d'usuari", 0
go_not_found_ca:      db 'Camí no trobat', 0
go_not_dir_ca:        db 'No és un directori', 0
become_usage_ca:      db 'Us: canviausuari admin | canviausuari usuari  (o cu admin | cu usuari)', 0
become_ok_admin_ca:  db 'Ara ets root (admin)', 0
become_ok_user_ca:    db 'Ara ets usuari normal', 0
become_denied_ca:     db "Només root pot canviar d'usuari", 0
rm_usage_ca:         db 'Us: rm <camí>', 0
rm_not_found_ca:     db 'Fitxer o directori no trobat', 0
rm_ok_ca:            db 'Eliminat', 0
rm_cancelled_ca:     db 'Cancel·lat', 0
rm_confirm_ca:       db 'Segur que vols eliminar el directori ', 0
rm_yn_ca:            db ' (pot contenir fitxers)? s/n?', 0
rm_not_empty_ca:     db 'El directori no està buit', 0
rm_cannot_root_ca:   db "No es pot eliminar l'arrel", 0
mkdir_usage_ca:      db 'Us: creadir <camí>', 0
mkdir_ok_ca:         db 'Directori creat', 0
mkdir_fail_ca:       db 'No es pot crear el directori', 0
mkdir_exists_ca:     db 'El fitxer o directori ja existeix', 0
cp_usage_ca:         db "Us: copia <origen> <desti>", 0
cp_ok_ca:            db 'Fitxer copiat', 0
cp_fail_ca:          db "No es pot copiar: origen no trobat o no es un fitxer", 0
cp_exists_ca:        db "No es pot copiar: el desti ja existeix", 0
mv_usage_ca:         db "Us: mou <origen> <desti>", 0
mv_ok_ca:            db 'Mogut', 0
mv_fail_ca:          db "No es pot moure: origen no trobat o desti invalid", 0
mv_exists_ca:        db "No es pot moure: el desti ja existeix", 0
find_usage_ca:       db "Us: cerca [cami] <nom>", 0
find_not_found_ca:   db "Cap coincidencia trobada", 0

