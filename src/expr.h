/**
 * @file expr.h
 *
 * @author Matěj Krňák
 * @brief Modul pro zpracování výrazů precedenční syntaktickou analýzou
 *
 * @details
 */

#ifndef EXPR_H
#define EXPR_H

#include "ast.h"
#include "parser.h"

/**
 * @brief Funkce, ktera zacne parsovat expression
 * @param p Ukazatel na struct, kde je ukazatel na vnitrni stav lexeru a
 * aktualni token predavany lexerem
 */
ast_expr_t *parse_expression(parser_t *p);

#endif
