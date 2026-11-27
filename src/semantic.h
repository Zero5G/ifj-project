/**
 * @file sematic.h
 *
 * @author Jakub Manoušek (xmanouj00)
 * @brief Modul pro semantickou analýzu
 *
 * @details
 */

#ifndef SEMATIC_H
#define SEMATIC_H

#include "ast.h"
#include "symtable.h"

/**
 * @brief Spustí semanticou analýzu nad AST
 *
 * @param prog AST programu
 * @param stab Tabulka symbolů
 */
void semantic_analyze(ast_prog_t *prog, symtable *stab);

/**
 * @brief Zkontroluje zda program obsahuje správný počet @main
 * @param prog AST programu
 */
void semantic_check_main(const ast_prog_t *prog);

#endif
