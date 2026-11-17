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

typedef struct {
    lexer *lx;
    token current;
} parser_t;

/**
 * @brief Inicializuje parser, aby mohl zacit parsovat
 * @param p Ukazatel na struct, kde je ukazatel na vnitrni stav lexeru a
 * aktualni token predavany lexerem
 * @param lx Ukazatel na lexer
 */
bool parser_init(parser_t *p, lexer *lx);

/**
 * @brief Funkce, ktera zacina parsovani
 * @param p Ukazatel na struct, kde je ukazatel na vnitrni stav lexeru a
 * aktualni token predavany lexerem
 */
bool parser_run(parser_t *p);

/**
 * @brief Funkce, ktera znici parser, tj. uvolni pamet aktualniho tokenu
 * @param p Ukazatel na struct, kde je ukazatel na vnitrni stav lexeru a
 * aktualni token predavany lexerem
 */
void parser_destroy(parser_t *p);

#endif
