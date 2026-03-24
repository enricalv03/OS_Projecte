/* =============================================================================
 * kernel/i18n.c  —  Per-language message catalogue (C-side)
 * =============================================================================
 * Centralises all user-visible kernel messages so they can be shared by
 * kernel subsystems (syscall errors, panic messages, etc.) in addition to
 * the shell.  The assembly language tables in commands/languages/ remain
 * authoritative for the rich shell output; this C catalogue covers the
 * messages that kernel C code needs to emit directly.
 * =========================================================================== */

#include "i18n.h"
#include "sched/process.h"

/* ---- English catalogue --------------------------------------------------- */
static const char* const en_msgs[MSG_COUNT] = {
    /* MSG_UNKNOWN_CMD        */ "Unknown command",
    /* MSG_FILE_NOT_FOUND     */ "File not found",
    /* MSG_PERMISSION_DENIED  */ "Permission denied",
    /* MSG_OUT_OF_MEMORY      */ "Out of memory",
    /* MSG_INVALID_ARG        */ "Invalid argument",
    /* MSG_LANG_CURRENT       */ "Current language: English",
    /* MSG_LANG_AVAILABLE     */ "Available: en  es  ca",
    /* MSG_LANG_SET           */ "Language set to: ",
    /* MSG_MEMSTAT_FREE       */ "Free pages: ",
    /* MSG_MEMSTAT_USED       */ "Used pages: ",
    /* MSG_ALLOC_SUCCESS      */ "Allocated page: 0x",
    /* MSG_ALLOC_OOM          */ "Out of physical memory",
    /* MSG_FREE_OK            */ "Page freed",
    /* MSG_FREE_FAIL          */ "Invalid page or already free",
    /* MSG_DISK_ERROR         */ "Error reading disk sector",
    /* MSG_WRITE_OK           */ "File created",
    /* MSG_WRITE_FAIL         */ "Filesystem full or invalid path",
    /* MSG_PS_HEADER          */ "PID   STATE    PRI   NAME",
    /* MSG_UPTIME             */ "Uptime: ",
};

/* ---- Spanish catalogue --------------------------------------------------- */
static const char* const es_msgs[MSG_COUNT] = {
    /* MSG_UNKNOWN_CMD        */ "Comando desconocido",
    /* MSG_FILE_NOT_FOUND     */ "Fichero no encontrado",
    /* MSG_PERMISSION_DENIED  */ "Permiso denegado",
    /* MSG_OUT_OF_MEMORY      */ "Sin memoria",
    /* MSG_INVALID_ARG        */ "Argumento invalido",
    /* MSG_LANG_CURRENT       */ "Idioma actual: Espanol",
    /* MSG_LANG_AVAILABLE     */ "Disponibles: en  es  ca",
    /* MSG_LANG_SET           */ "Idioma cambiado a: ",
    /* MSG_MEMSTAT_FREE       */ "Paginas libres: ",
    /* MSG_MEMSTAT_USED       */ "Paginas usadas: ",
    /* MSG_ALLOC_SUCCESS      */ "Pagina asignada: 0x",
    /* MSG_ALLOC_OOM          */ "Sin memoria fisica",
    /* MSG_FREE_OK            */ "Pagina liberada",
    /* MSG_FREE_FAIL          */ "Pagina invalida o ya libre",
    /* MSG_DISK_ERROR         */ "Error al leer sector",
    /* MSG_WRITE_OK           */ "Fichero creado",
    /* MSG_WRITE_FAIL         */ "Sistema de ficheros lleno o ruta invalida",
    /* MSG_PS_HEADER          */ "PID   ESTADO   PRI   NOMBRE",
    /* MSG_UPTIME             */ "Tiempo activo: ",
};

/* ---- Catalan catalogue --------------------------------------------------- */
static const char* const ca_msgs[MSG_COUNT] = {
    /* MSG_UNKNOWN_CMD        */ "Ordre desconeguda",
    /* MSG_FILE_NOT_FOUND     */ "Fitxer no trobat",
    /* MSG_PERMISSION_DENIED  */ "Permisos insuficients",
    /* MSG_OUT_OF_MEMORY      */ "Sense memoria",
    /* MSG_INVALID_ARG        */ "Argument invalid",
    /* MSG_LANG_CURRENT       */ "Idioma actual: Catala",
    /* MSG_LANG_AVAILABLE     */ "Disponibles: en  es  ca",
    /* MSG_LANG_SET           */ "Idioma canviat a: ",
    /* MSG_MEMSTAT_FREE       */ "Pagines lliures: ",
    /* MSG_MEMSTAT_USED       */ "Pagines usades: ",
    /* MSG_ALLOC_SUCCESS      */ "Pagina assignada: 0x",
    /* MSG_ALLOC_OOM          */ "Sense memoria fisica",
    /* MSG_FREE_OK            */ "Pagina alliberada",
    /* MSG_FREE_FAIL          */ "Pagina invalida o ja lliure",
    /* MSG_DISK_ERROR         */ "Error en llegir el sector",
    /* MSG_WRITE_OK           */ "Fitxer creat",
    /* MSG_WRITE_FAIL         */ "Sistema de fitxers ple o ruta invalida",
    /* MSG_PS_HEADER          */ "PID   ESTAT    PRI   NOM",
    /* MSG_UPTIME             */ "Temps actiu: ",
};

/* ---- Language name strings ----------------------------------------------- */
static const char* const lang_names[LANG_COUNT] = {
    /* LANG_EN */ "English",
    /* LANG_ES */ "Espanol",
    /* LANG_CA */ "Catala",
};

/* ---- Global pointer table ------------------------------------------------ */
static const char* const * const catalogues[LANG_COUNT] = {
    en_msgs,
    es_msgs,
    ca_msgs,
};

/* ---- Global default language --------------------------------------------- */
static language_id_t default_lang = LANG_EN;

/* ---- API ------------------------------------------------------------------ */

const char* i18n_get_msg(language_id_t lang, msg_id_t id) {
    if ((unsigned int)lang  >= (unsigned int)LANG_COUNT) lang = LANG_EN;
    if ((unsigned int)id    >= (unsigned int)MSG_COUNT)  id   = MSG_UNKNOWN_CMD;
    return catalogues[lang][id];
}

language_id_t i18n_get_default_lang(void) {
    return default_lang;
}

void i18n_set_default_lang(language_id_t lang) {
    if ((unsigned int)lang < (unsigned int)LANG_COUNT)
        default_lang = lang;
}

const char* i18n_msg(msg_id_t id) {
    language_id_t lang = default_lang;

    /* Use the calling process's language if one is running. */
    pcb_t* cur = process_get_current();
    if (cur) lang = cur->language_id;

    return i18n_get_msg(lang, id);
}

const char* i18n_lang_name(language_id_t lang) {
    if ((unsigned int)lang >= (unsigned int)LANG_COUNT) return "Unknown";
    return lang_names[lang];
}
