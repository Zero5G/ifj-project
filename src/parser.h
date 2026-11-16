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

bool parser_init(parser_t *p, lexer *lx);

bool parser_run(parser_t *p);

void parser_destroy(parser_t *p);

#endif
