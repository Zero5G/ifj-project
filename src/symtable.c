/**
 * @file symtable.c
 *
 * @author Jakub Manoušek
 * @brief Implementace modulu pro ADT tabulku symbolů
 *
 * @details
 */

#include "ast.h"
#include <string.h>
#define HTAB_BASE_SIZE 12

#include "htab.h"
#include "symtable.h"
#include <stdlib.h>

/** @brief Interní typ pro obor platnosti funkce */
typedef struct scope {
    hash_table vars;
    struct scope *parent;
    bool is_func_parent;
    unsigned saved_next_var_id;
} scope_t;

struct symtable {
    hash_table funcs;
    scope_t *current;
    unsigned next_var_id;
};

static sym_result symtable_insert(hash_table *htab, const char *name,
                                  sym_kind kind, symbol **out) {
    symbol *sym = calloc(1, sizeof(struct symbol));
    if (sym == nullptr) {
        return SYM_NOMEM;
    }

    char *name_dup = strdup(name);
    if (name_dup == nullptr) {
        free(sym);
        return SYM_NOMEM;
    }

    hash_item *item = hashtable_insert(htab, name_dup);
    if (item == nullptr) {
        free(name_dup);
        free(sym);
        return SYM_NOMEM;
    }

    sym->kind = kind;
    sym->type = TYPE_UNKNOWN;
    sym->ret_state = RET_UNRESOLVED;
    item->symbol = sym;

    *out = sym;
    return SYM_OK;
}

symtable_t *symtable_create() {
    symtable_t *symtable = calloc(1, sizeof(struct symtable));
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

void symtable_destroy(symtable_t *stab) {
    if (stab == nullptr) {
        return;
    }

    while (stab->current != nullptr) {
        scope_t *parent = stab->current->parent;
        hashtable_dispose(&stab->current->vars);
        free(stab->current);
        stab->current = parent;
    }

    hashtable_dispose(&stab->funcs);
    free(stab);
}

bool symtable_enter_scope(symtable_t *stab) {
    if (stab == nullptr) {
        return false;
    }
    scope_t *scp = calloc(1, sizeof(struct scope));
    if (scp == nullptr) {
        return false;
    }

    if (!hashtable_init(&scp->vars, HTAB_BASE_SIZE)) {
        free(scp);
        return false;
    }

    scp->parent = stab->current;
    stab->current = scp;

    return true;
}

bool symtable_leave_scope(symtable_t *stab) {
    if (stab == nullptr) {
        return false;
    }
    if (stab->current == nullptr) {
        return false;
    }

    scope_t *child = stab->current;
    stab->current = child->parent;
    hashtable_dispose(&child->vars);
    free(child);

    return true;
}

sym_result symtable_add_var(symtable_t *stab, const char *name, symbol **out) {
    symbol *existing = symtable_find_var(stab, name);
    if (existing != nullptr) {
        *out = existing;
        return SYM_EXISTS;
    }

    sym_result res = symtable_insert(&stab->current->vars, name, SYM_VAR, out);
    if (res == SYM_OK) {
        (*out)->var_id = stab->next_var_id++;
    }

    return res;
}

symbol *symtable_find_var(const symtable_t *stab, const char *name) {
    for (scope_t *scp = stab->current; scp != nullptr; scp = scp->parent) {
        hash_item *item = hashtable_search(&scp->vars, name);
        if (item != nullptr) {
            return item->symbol;
        }
        if (scp->is_func_parent) {
            break;
        }
    }

    return nullptr;
}

bool symtable_enter_func(symtable_t *stab) {
    unsigned saved = stab->next_var_id;
    if (!symtable_enter_scope(stab)) {
        return false;
    }

    stab->current->is_func_parent = true;
    stab->current->saved_next_var_id = saved;
    stab->next_var_id = 0;

    return true;
}

unsigned symtable_leave_func(symtable_t *stab) {
    unsigned count = stab->next_var_id;
    unsigned saved = stab->current->saved_next_var_id;
    symtable_leave_scope(stab);

    stab->next_var_id = saved;

    return count; /**< se rovná ast_func.var_count */
}

sym_result symtable_add_func(symtable_t *stab, const char *name, symbol **out) {
    symbol *existing = symtable_find_func(stab, name);
    if (existing != nullptr) {
        *out = existing;
        return SYM_EXISTS;
    }

    return symtable_insert(&stab->funcs, name, SYM_FUNC, out);
}

symbol *symtable_find_func(const symtable_t *stab, const char *name) {
    hash_item *item = hashtable_search(&stab->funcs, name);
    return item != nullptr ? item->symbol : nullptr;
}
