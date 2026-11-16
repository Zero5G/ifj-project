/**
 * @file sematic.h
 *
 * @author Jakub Manoušek
 * @brief Modul pro semantickou analýzu
 *
 * @details
 */

#ifndef SEMATIC_H
#define SEMATIC_H

#include "ast.h"
#include "symtable.h"

void semantic_analyze(ast_prog_t *prog, symtable *stab);

#endif
