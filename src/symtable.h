/**
 * @file symtable.h
 *
 * @author Jakub Manoušek (xmanouj00)
 * @brief Modul pro tabulku symbolů
 *
 * @details
 */
#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "ast.h"
#include "types.h"
#include <stddef.h>

typedef struct ast_func ast_func_t;

typedef enum sym_kind {
    SYM_VAR,
    SYM_FUNC,
} sym_kind;

typedef enum {
    SYM_OK,
    SYM_EXISTS,
    SYM_NOMEM,
} sym_result;

typedef enum {
    RET_EXPLICIT,
    RET_UNRESOLVED,
    RET_INFERRING,
    RET_INFERRED,
} return_state;

/**
 * @brief Struktura symbolu
 */
typedef struct symbol {
    sym_kind kind;
    data_type type;

    bool is_mut;
    unsigned var_id; /**< unikátní v rámci funkce, např: LF@x$3 */

    const data_type *param_types; /**< pole typů */
    size_t param_count;
    bool is_builtin;
    return_state ret_state;
    ast_func_t *decl; /**< funkce, kterou vytvořil uživatel */
} symbol;

typedef struct symtable symtable;

/**
 * @brief Vytvoří tabulku symbolů
 *
 * @return Ukazatel na tabulku symbolů
 */
symtable *symtable_create();

/**
 * @brief Zruší tabulku symbolů
 *
 * @param stab Ukazatel na tabulku symbolů
 */
void symtable_destroy(symtable *stab);

/**
 * @brief Přejít do dalšího rozsahu platnosti
 *
 * @param[in,out] stab Ukazatel na tabulku symbolů
 */
bool symtable_enter_scope(symtable *stab);

/**
 * @brief Přejít do předchozího rozsahu platnosti
 *
 * @param[in,out] stab Ukazatel na tabulku symbolů
 */
bool symtable_leave_scope(symtable *stab);

/**
 * @brief Přidá proměnnou do tabulky symbolů
 *
 * @param stab[in,out] Ukazatel na tabulku symbolů
 * @param name Jméno proměnné co přidat
 * @param out[out] Ukazatel na nový symbol proměnné
 *
 * @return Vrací `sym_result`
 */
sym_result symtable_add_var(symtable *stab, const char *name, symbol **out);

/**
 * @brief Vyhledá proměnnou v tabulce symbolů
 *
 * @param stab Ukazatel na tabulku symbolů
 * @param name Jméno proměnné co vyhledat
 *
 * @return Ukazatel na symbol, `nullptr` pokud již existuje
 */
symbol *symtable_find_var(const symtable *stab, const char *name);

/**
 * @brief Přejít do rozsahu platnosti funkce
 *
 * @param stab Ukazatel na tabulku symbolů
 *
 * @return Pokud se vstup povedl
 */
bool symtable_enter_func(symtable *stab);

/**
 * @brief Odejít z rozsahu platnosti funkce
 *
 * @param stab Ukazatel na tabulku symbolů
 *
 * @return ast_func.var_count
 */
unsigned symtable_leave_func(symtable *stab);

/**
 * @brief Přidá funkci do tabulky symbolů
 *
 * @param stab[in,out] Ukazatel na tabulku symbolů
 * @param name Jméno funkce pro přidání
 * @param out[out] Ukazatel na nový symbol funkce
 *
 * @return Vrací `sym_result`
 */
sym_result symtable_add_func(symtable *stab, const char *name, symbol **out);

/**
 * @brief Vyhledá funkci v tabulce symbolů
 *
 * @param stab Ukazatel na tabulku symbolů
 * @param name Jméno funkce co vyhledat
 *
 * @return Ukazatel na symbol, `nullptr` pokud nenalezen
 */
symbol *symtable_find_func(const symtable *stab, const char *name);

#endif
