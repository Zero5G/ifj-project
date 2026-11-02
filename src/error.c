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

const char *error_string(error_code code) {
    switch (code) {
    case ERR_LEX:
        return "Chyba lexikální analýzy";
    case ERR_SYNT:
        return "Chyba syntaktické analýzy";
    case ERR_SEM_UNDEF:
        return "Chyba sematiky: nedefinovaný znak";
    case ERR_SEM_REDEF:
        return "Chyba sematiky: redefinovaný znak";
    case ERR_SEM_BADARGS:
        return "Chyba sematiky: špatné argumenty/nepovolené paramentry";
    case ERR_SEM_TYPE:
        return "Chyba sematiky: chyba typu";
    case ERR_SEM_ROAE:
        return "Chyba sematiky: zápis do read-only proměnné";
    case ERR_SEM_OTHER:
        return "Chyba sematiky: jiné";
    case ERR_SEM_RUNTIME:
        return "Nastala chyba sematiky za běhu";
    case ERR_OTHER:
        return "Ostatní: programové chyby, atd.";
    }
    return "Neznámí chyboví kód.";
}

void print_error(error_code code) {
    fprintf(stderr, "%s", error_string(code)); // NOLINT
}

void print_error_custom(error_code code, char *fmt, ...) {
    va_list args;

    fprintf(stderr, "%s: ", error_string(code)); // NOLINT

    va_start(args, fmt);
    vfprintf(stderr, fmt, args); // NOLINT
    va_end(args);
}

void error_exit(error_code code) {
    print_error(code);
    exit(code);
}

void error_exit_custom(error_code code, char *fmt, ...) {
    va_list args;

    fprintf(stderr, "%s: ", error_string(code)); // NOLINT

    va_start(args, fmt);
    vfprintf(stderr, fmt, args); // NOLINT
    va_end(args);

    exit(code);
}
