/**
* @file codegen.c
*
* @author Jakub Machac
* @brief odulu generátoru kódu
*
* @details
*/
 
#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include <stdio.h>

/**
* @brief Vygeneruje cilovy kod v jazyce IFjcode26 podle AST programu
* @param prog Ukazatel na korenopvy uzel AST
* @param out Vystup, kam se zapise vysledny kod
*/

void codegen(const ast_prog_t *prog, FILE *out);

#endif