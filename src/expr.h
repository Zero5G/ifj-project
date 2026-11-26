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
 * @param p Ukazatel na kontext parseru
 */
ast_expr_t *parse_expression(parser_ctx *ctx);

/**
 * @brief Funkce, ktera zparsuje jeden term (identifikator nebo literal) a vrati
 * jeho uzel
 * @param p Ukazatel na kontext parseru
 */
ast_expr_t *parse_term(parser_ctx *ctx);

#endif
