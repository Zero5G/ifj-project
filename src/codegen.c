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
#include <stdio.h>

#define SPECIAL_CHAR_LAST 32

typedef struct {
    FILE *out;
    unsigned label_id;
    ast_func_t *current_func;
} codegen_ctx_t;

// NOLINTBEGIN(cert-err33-c)

static void print_string(FILE *out, const char *str) {
    fputs("string@", out);
    for (const unsigned char *ptr = (const unsigned char *)str; *ptr != '\0'; ptr++) {
        if (*ptr <= SPECIAL_CHAR_LAST || *ptr == '#' || *ptr == '\\') {
            fprintf(out, "\\%03u", *ptr);
        }
        else {
            fputc(*ptr, out);
        }
    }
}

static void gen_expr(codegen_ctx_t *ctx, const ast_expr_t *expr) {
    switch (expr->kind) {   
        case E_INT_LIT:
            fprintf(ctx->out, "PUSHS int@%d\n", expr->as.int_val);
            break;
        case E_UNARY:
            break;
        case E_BINARY:
            break;
        case E_FUNC:
            break;
        case E_VAR:
            break;
        case E_UNIT_LIT:
            break;
        case E_DOUBLE_LIT:
            break;
        case E_STRING_LIT:
            break;
        case E_BOOLEAN_LIT:
            break;
    }
}
// NOLINTEND(cert-err33-c)