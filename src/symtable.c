/**
 * @file symtable.c
 *
 * @author Jakub Manoušek
 * @brief Implementace modulu pro tabulku symbolů
 *
 * @details
 */

#include "htab.h"

typedef struct scope {
    hash_table vars;
    struct scope *parent;
} scope;

struct symtable {
    hash_table funcs;
    scope *current;
    unsigned next_var_id;
};
