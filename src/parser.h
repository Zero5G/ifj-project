/**
 * @file parser.h
 *
 * @author Matěj Krňák
 * @brief Modul pro syntaktickou analýzu
 *
 * @details
 */

#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"
#include <stdbool.h>

typedef struct parser_ctx {
    lexer *lx;
    token current;
    ast_prog_t *prog;
} parser_ctx;

/**
 * @brief Inicializuje parser, aby mohl zacit parsovat
 * @param ctx Ukazatel na kontext parseru
 * @param lx Ukazatel na lexer
 */
bool parser_init(parser_ctx *ctx, lexer *lx);

/**
 * @brief Funkce, ktera zacina parsovani
 * @param ctx Ukazatel na kontext parseru
 */
bool parser_run(parser_ctx *ctx);

/**
 * @brief Funkce, ktera prevezme AST z parseru
 * @param ctx Ukazatel na kontext parseru
 * @return AST programu
 */
ast_prog_t *parser_take_prog(parser_ctx *ctx);

/**
 * @brief Funkce, ktera znici parser, tj. uvolni pamet aktualniho tokenu
 * @param ctx Ukazatel na kontext parseru
 */
void parser_destroy(parser_ctx *ctx);

/**
 * @brief Funkce, ktera uvolni aktualni token a zavola lexer pro dalsi
 * @param ctx Ukazatel na kontext parseru
 */
void advance(parser_ctx *ctx);

/**
 * @brief Funkce, ktera kontroluje, zda je pozadovany typ mozne prijmout pro
 * parsovani
 * @param ctx Ukazatel na kontext parseru
 * @param type Typ tokenu
 */
bool accept(parser_ctx *ctx, token_type type);

/**
 * @brief Funkce, ktera kontroluje, zda je pozadovany typ tim ocekavanym
 * @param ctx Ukazatel na kontext parseru
 * @param type Typ tokenu
 */
bool expect(parser_ctx *ctx, token_type type);

/**
 * @brief Funkce, ktera zparsuje volani funkce, a zapise ho do ast
 * @param ctx Ukazatel na kontext parseru
 */
ast_expr_t *parse_function_call(parser_ctx *ctx);

#endif
