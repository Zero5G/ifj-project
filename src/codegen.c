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

typedef struct {
    FILE *out;
    unsigned label_id;
    ast_func_t *current_func;
} codegen_ctx_t;

static void print_string(FILE *out, const char *s) {
    fputs("string@", out);
    for (const unsigned char *p = (const unsigned char *)s; *p != '\0'; p++) {
        if (*p <= 32 || *p == '#' || *p == '\\') {
            fprintf(out, "\\%03u", *p);
        }
        else {
            fputc(*p, out);
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