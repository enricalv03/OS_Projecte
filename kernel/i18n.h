#ifndef KERNEL_I18N_H
#define KERNEL_I18N_H

/* =============================================================================
 * kernel/i18n.h  —  Language context and i18n catalogue
 * =============================================================================
 * The OS treats language as a first-class per-process concept:
 *   - Each process has a language_context_t that records the active language
 *     and character encoding.
 *   - Kernel error messages and shell output are looked up from the
 *     per-language catalogue below, so they automatically follow the
 *     process's current language.
 *   - The sys_set_language(lang_id) syscall updates the calling process's
 *     language_context and the global default for new processes.
 *
 * The assembly command tables in arch/x86/kernel/commands/languages/ remain
 * the "front-end" for rich shell output; the C catalogue here is the single
 * source of truth for kernel-level messages shared across subsystems.
 * =========================================================================== */

/* ---- Language IDs --------------------------------------------------------- */

typedef enum {
    LANG_EN = 0,   /* English  (default) */
    LANG_ES = 1,   /* Spanish  */
    LANG_CA = 2,   /* Catalan  */
    LANG_COUNT
} language_id_t;

/* ---- Character encoding IDs ---------------------------------------------- */

typedef enum {
    ENC_ASCII  = 0,   /* 7-bit ASCII  (default, subset of all below) */
    ENC_LATIN1 = 1,   /* ISO 8859-1   (Western European)              */
    ENC_UTF8   = 2    /* UTF-8        (future)                        */
} encoding_t;

/* ---- Kernel message IDs -------------------------------------------------- */

typedef enum {
    /* Generic errors */
    MSG_UNKNOWN_CMD = 0,
    MSG_FILE_NOT_FOUND,
    MSG_PERMISSION_DENIED,
    MSG_OUT_OF_MEMORY,
    MSG_INVALID_ARG,

    /* Language system */
    MSG_LANG_CURRENT,
    MSG_LANG_AVAILABLE,
    MSG_LANG_SET,

    /* Memory / process */
    MSG_MEMSTAT_FREE,
    MSG_MEMSTAT_USED,
    MSG_ALLOC_SUCCESS,
    MSG_ALLOC_OOM,
    MSG_FREE_OK,
    MSG_FREE_FAIL,

    /* I/O */
    MSG_DISK_ERROR,
    MSG_WRITE_OK,
    MSG_WRITE_FAIL,

    /* Process */
    MSG_PS_HEADER,
    MSG_UPTIME,

    MSG_COUNT   /* sentinel — keep last */
} msg_id_t;

/* ---- Per-process language context ---------------------------------------- */

typedef struct {
    language_id_t  lang_id;    /* active language                       */
    encoding_t     encoding;   /* active encoding                       */
} language_context_t;

/* ---- API ------------------------------------------------------------------ */

/* Return the catalogue string for msg_id in the given language.
 * Falls back to English if the requested language has no entry. */
const char* i18n_get_msg(language_id_t lang, msg_id_t id);

/* Global default language for new processes (init: LANG_EN). */
language_id_t i18n_get_default_lang(void);
void          i18n_set_default_lang(language_id_t lang);

/* Convenience: look up a message in the calling process's language.
 * Requires process_get_current() to be valid; falls back to the global default
 * if no current process is set yet. */
const char* i18n_msg(msg_id_t id);

/* Name string for a language ID (e.g. "English", "Español", "Català"). */
const char* i18n_lang_name(language_id_t lang);

#endif
