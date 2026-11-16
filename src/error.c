/**
 * @file error.c
 *
 * @author Jakub Manoušek
 * @brief Implementace modulu pro chybová hlašení a ukončení programu
 *
 * @details
 */
#include "error.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Statický ukazatel na cleanup funkci (nastaven pro každý soubor zvlášť)
 */
static void (*g_cleanup)(void) = NULL;

const char *error_string(error_code code) {
    switch (code) {
    case ERR_LEX:
        return "Chyba lexikální analýzy";
    case ERR_SYNT:
        return "Chyba syntaktické analýzy";
    case ERR_SEM_UNDEF:
        return "Chyba sematiky - nedefinovaný znak";
    case ERR_SEM_REDEF:
        return "Chyba sematiky - redefinovaný znak";
    case ERR_SEM_BADARGS:
        return "Chyba sematiky - špatné argumenty/nepovolené paramentry";
    case ERR_SEM_TYPE:
        return "Chyba sematiky - chyba typu";
    case ERR_SEM_ROAE:
        return "Chyba sematiky - zápis do read-only proměnné";
    case ERR_SEM_OTHER:
        return "Chyba sematiky - jiné";
    case ERR_OTHER:
        return "Ostatní - programové chyby, atd.";
    }
    return "Neznámí chyboví kód.";
}

void error_print(error_code code) {
    fprintf(stderr, "%s\n", error_string(code)); // NOLINT
}

void error_vprintf(error_code code, const char *fmt, va_list args) {
    fprintf(ERROR_OUT, "%s: ", error_string(code)); // NOLINT
    vfprintf(ERROR_OUT, fmt, args);                 // NOLINT
}

void error_printf(error_code code, char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    error_vprintf(code, fmt, args);
    va_end(args);
}

[[noreturn]] void error_fatal(error_code code) {
    error_print(code);
    exit((int)code);
}

[[noreturn]] void error_fatalf(error_code code, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    error_vprintf(code, fmt, args);
    va_end(args);
    exit((int)code);
}

void error_fatal_set_cleanup(void (*cleanup_func)(void)) {
    g_cleanup = cleanup_func;
}

[[noreturn]] static inline void die(error_code code) {
    void (*cleanup_hook)(void) = g_cleanup;
    g_cleanup = nullptr;
    if (cleanup_hook) {
        cleanup_hook();
    }
    exit((int)code);
}

[[noreturn]] void error_fatal_cleanup(error_code code) { die(code); }

[[noreturn]] void error_fatalf_cleanup(error_code code, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    error_vprintf(code, fmt, args);
    va_end(args);
    die(code);
}
