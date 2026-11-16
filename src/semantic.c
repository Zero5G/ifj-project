/**
 * @file sematic.c
 *
 * @author Jakub Manoušek
 * @brief Implementace modulu pro semantickou analýzu
 *
 * @details
 */

#include "ast.h"
#include "symtable.h"

typedef struct sematic_ctx {
    ast_prog_t *prog;
    symtable *stab;
} sematic_ctx;
