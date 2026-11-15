/**
* @file codegen.c
*
* @author Jakub Machac
* @brief Modulu generátoru kódu
*
* @details
*/

#include "codegen.h"
#include "ast.h"

typedef struct {
    FILE *out;
    unsigned label_id;
    ast_func_t *current_func;
} codegen_ctx_t;