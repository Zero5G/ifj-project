/**
 * @file symtable.c
 *
 * @author Jakub Manoušek
 * @brief Implementace modulu pro tabulku symbolů
 *
 * @details
 */

#define HTAB_BASE_SIZE 12

#include "symtable.h"
#include "htab.h"
#include <stdlib.h>

typedef struct scope {
    hash_table vars;
    struct scope *parent;
} scope;

struct symtable {
    hash_table funcs;
    scope *current;
    unsigned next_var_id;
};

symtable *symtable_create() {
    symtable *symtable = calloc(1, sizeof(struct symtable));
    if (symtable == nullptr) {
        return nullptr;
    }
    if (!hashtable_init(&symtable->funcs, HTAB_BASE_SIZE)) {
        free(symtable);
        return nullptr;
    }
    symtable->current = nullptr;
    symtable->next_var_id = 0;

    return symtable;
}

void symtable_destroy(symtable *stab) {
    if (stab == nullptr) {
        return;
    }
    while (stab->current != nullptr) {
        scope *parent = stab->current->parent;
        hashtable_dispose(&stab->current->vars);
        free(stab->current);
        stab->current = parent;
    }
    hashtable_dispose(&stab->funcs);
    free(stab);
}

void symtable_enter_scope(symtable *stab) {
    scope *scp = calloc(1, sizeof(struct scope));
    if (scp == nullptr) {
        return;
    }
    hashtable_init(&scp->vars, HTAB_BASE_SIZE);
    scp->parent = stab->current;
    stab->current = scp;
}

void symtable_leave_scope(symtable *stab) {
    scope *child = stab->current;
    stab->current = child->parent;
    hashtable_dispose(&child->vars);
    free(child);
}
