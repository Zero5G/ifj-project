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

#include "lexer.h"
#include <stdbool.h>


typedef struct parser_ctx {
    lexer *lx;
    token current;
} parser_ctx;

/**
 * @brief Inicializuje parser, aby mohl zacit parsovat
 * @param p Ukazatel na struct, kde je ukazatel na vnitrni stav lexeru a
 * aktualni token predavany lexerem
 * @param lx Ukazatel na lexer
 */
bool parser_init(parser_ctx *ctx, lexer *lx);

/**
 * @brief Funkce, ktera zacina parsovani
 * @param p Ukazatel na struct, kde je ukazatel na vnitrni stav lexeru a
 * aktualni token predavany lexerem
 */
bool parser_run(parser_ctx *ctx);

/**
 * @brief Funkce, ktera znici parser, tj. uvolni pamet aktualniho tokenu
 * @param p Ukazatel na struct, kde je ukazatel na vnitrni stav lexeru a
 * aktualni token predavany lexerem
 */
void parser_destroy(parser_ctx *p);

/**
 * @brief Funkce, ktera uvolni aktualni token a zavola lexer pro dalsi
 * @param p Ukazatel na struct, kde je ukazatel na vnitrni stav lexeru a
 * aktualni token predavany lexerem
 */
void advance(parser_ctx *p);

/**
 * @brief Funkce, ktera kontroluje, zda je pozadovany typ mozne prijmout pro
 * parsovani
 * @param p Ukazatel na struct, kde je ukazatel na vnitrni stav lexeru a
 * aktualni token predavany lexerem
 * @param type Typ tokenu
 */
bool accept(parser_ctx *p, token_type type);

/**
 * @brief Funkce, ktera kontroluje, zda je pozadovany typ tim ocekavanym
 * @param p Ukazatel na struct, kde je ukazatel na vnitrni stav lexeru a
 * aktualni token predavany lexerem
 * @param type Typ tokenu
 */
bool expect(parser_ctx *p, token_type type);

#endif
