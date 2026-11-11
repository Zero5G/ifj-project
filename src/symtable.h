/**
 * @file symtable.h
 *
 * @author Jakub Manoušek
 * @brief Modul pro tabulku symbolů
 *
 * @details
 */
#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "ast.h"
#include "types.h"
#include <stddef.h>

typedef enum sym_kind {
    SYM_VAR,
    SYM_FUNC,
} sym_kind;

/**
 * @brief Struktura symbolu
 */
typedef struct symbol {
    sym_kind kind;
    char *name; /** vlastní je hašovací tabulka */
    data_type type;

    bool is_mut;
    unsigned var_id; /**< unikátní v rámci funkce, např: LF@x$3 */

    data_type *param_types; /**< pole typů */
    size_t param_count;
    bool is_buildin;
    bool ret_explicit; /**< jestli decl má definovaný typ, nebo ne (`TYPE_UNKNOWN`) */
    bool infer_in_progress; /**< typ co vrací funkce se hledá */
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
 * @brief Přidá funkci do tabulky symbolů
 *
 * @param stab Ukazatel na tabulku symbolů
 * @param name Jméno funkce pro přidání
 *
 * @return Ukazatel na přidaný symbol, `nullptr` pokud již existuje
 */
symbol *symtable_add_func(symtable *stab, const char *name);

/**
 * @brief Přidá funkci do tabulky symbolů
 *
 * @param stab Ukazatel na tabulku symbolů
 * @param name Jméno funkce co vyhledat
 *
 * @return Ukazatel na symbol, `nullptr` pokud nenalezen
 */
symbol *symtable_find_func(const symtable *stab, const char *name);

/**
 * @brief Přejít do dalšího rozsahu platnosti
 *
 * @param[in,out] stab Ukazatel na tabulku symbolů
 */
void symtable_enter_scope(symtable *stab);

/**
 * @brief Přejít do předchozího rozsahu platnosti
 *
 * @param[in,out] stab Ukazatel na tabulku symbolů
 */
void symtable_leave_scope(symtable *stab);

/**
 * @brief Přidá proměnnou do tabulky symbolů
 *
 * @param stab Ukazatel na tabulku symbolů
 * @param name Jméno proměnné co vyhledat
 *
 * @return Ukazatel na symbol, `nullptr` pokud již existuje
 */
symbol *symtable_add_var(symtable *stab, const char *name);

/**
 * @brief Přidá proměnnou do tabulky symbolů
 *
 * @param stab Ukazatel na tabulku symbolů
 * @param name Jméno proměnné co vyhledat
 *
 * @return Ukazatel na symbol, `nullptr` pokud nenalezen
 */
symbol *symtable_find_var(const symtable *stab, const char *name);

#endif
