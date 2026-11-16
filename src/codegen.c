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
#include "dstr.h"
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
            switch (expr->as.unary.op) {
                case OPU_PLUS:
                    break;
                case OPU_NOT:
                    fputs("NOTS\n", ctx->out);
                    break;
                case OPU_NEG:
                    if (expr->data_type == TYPE_INT) {
                        fputs("PUSHS int@-1\n", ctx->out);
                    }
                    else {
                        fputs("PUSHS float@-0x1p+0\n", ctx->out);
                    }
                    fputs("MULS\n", ctx->out);
                    break;
            }
            break;
        case E_BINARY:
            gen_expr(ctx, expr->as.binary.left);
            gen_expr(ctx, expr->as.binary.right);
            switch (expr->as.binary.op) {
                case OPB_ADD:
                    fputs("ADDS\n", ctx->out);
                    break;
                case OPB_SUB:
                    fputs("SUBS\n", ctx->out);
                    break;
                case OPB_DIV:
                    fputs("DIVS\n", ctx->out);
                    fputs("IDIVS\n", ctx->out);
                    break;
                case OPB_MUL:
                    fputs("MULS\n", ctx->out);
                    break;
                case OPB_EQ:
                    fputs("EQS\n", ctx->out);
                    break;
                case OPB_GT:
                    fputs("GTS\n", ctx->out);
                    break;
                case OPB_GTE:
                    fputs("LTS\nNOTS\n", ctx->out);
                    break;
                case OPB_LT:
                    fputs("LTS\n", ctx->out);
                    break; 
                case OPB_LTE:
                    fputs("GTS\nNOTS\n", ctx->out);
                    break;
                case OPB_AND:
                    fputs("ANDS\n", ctx->out);
                    break;
                case OPB_OR:
                    fputs("ORS\n", ctx->out);
                    break;
                case OPB_NE:
                    fputs("EQS\nNOTS\n", ctx->out);
            }
            break;
        case E_FUNC:
            if (expr->as.func.target == nullptr) {
                gen_expr(ctx, expr->as.func.args[0]);
                fputs("PUSHS GF@tmp\n", ctx->out);
                fputs("POPS GF@tmp\n", ctx->out);
                fputs("WRITE GF@tmp\n", ctx->out);
            }
            break;
        case E_VAR:
            fprintf(ctx->out, "PUSH LF@v_%u\n", expr->as.var.var_id);
            break;
        case E_UNIT_LIT:
            break;
        case E_DOUBLE_LIT:
            fprintf(ctx->out, "PUSHS float@%a\n", expr->as.double_val);
            break;
        case E_STRING_LIT:
            fputs("PUSH", ctx->out);
            print_string(ctx->out, dstr_cstr(expr->as.str));
            fputc('\n', ctx->out);
            break;
        case E_BOOLEAN_LIT:
            fprintf(ctx->out, "PUSH bool@%s\n", expr->as.bool_val ? "true" : "false");
            break;
        default:
            break;
    }
}

static void gen_block(codegen_ctx_t *ctx, const ast_block_t *block);

static void gen_stmt(codegen_ctx_t *ctx, const ast_stmt_t *stmt) {
    switch (stmt->kind) {
    case STMT_VAR_DEF:
        gen_expr(ctx, stmt->as.var_def.init);
        fprintf(ctx->out, "POPS LF@v_%u\n", stmt->as.var_def.var_id);
        break;
    case STMT_EXPR:
        gen_expr(ctx, stmt->as.expr);
        fputs("POPS GF@tmp\n", ctx->out);
        break;
    case STMT_RET:
        if (stmt->as.ret_val != nullptr) {
            gen_expr(ctx, stmt->as.ret_val);
        }
        fprintf(ctx->out, "RETURN\n");
        break;
    case STMT_ASSIGN:
        gen_expr(ctx, stmt->as.assign.val);
        fprintf(ctx->out, "POPS LF@v_%u/n", stmt->as.assign.var_id);
        break;
    case STMT_IF:
        unsigned if_id = ++ctx->label_id;
        gen_expr(ctx, stmt->as.if_stmt.cond);
        fputs("POPS GF@tmp\n", ctx->out);
        fprintf(ctx->out, "JUMPIFEQ $if_else_%u GF@tmp bool@false\n", if_id);

        gen_block(ctx, &stmt->as.if_stmt.then_blk);
        fprintf(ctx->out, "JUMP $if_end_%u\n", if_id);

        fprintf(ctx->out, "LABEL $if_else_%u\n", if_id);
        if (stmt->as.if_stmt.has_else) {
            gen_block(ctx, &stmt->as.if_stmt.else_blk);
        }
        fprintf(ctx->out, "LABEL &if_end_%u\n", if_id);
        break;
    case STMT_WHILE:
        unsigned while_id = ++ctx->label_id;
        fprintf(ctx->out, "LABEL $while_start_%u\n", while_id);

        gen_expr(ctx, stmt->as.while_stmt.cond);
        fputs("POPS GF@tmp\n", ctx->out);

        fprintf(ctx->out, "JUMPIFEQ $while_end_%u GF@tmp bool@false\n", while_id);

        gen_block(ctx, &stmt->as.while_stmt.block);

        fprintf(ctx->out, "JUMP &while_start_%u\n", while_id);
        fprintf(ctx->out, "LABEL &while_end_%u\n", while_id);
        break;
    }
}

static void gen_block(codegen_ctx_t *ctx, const ast_block_t *block) {
    for (const ast_stmt_t *stmt = block->first; stmt != nullptr; stmt = stmt->next ) {
        gen_stmt(ctx, stmt);
    }
}

static void de_var_block(codegen_ctx_t *ctx, const ast_block_t *block) {
    for (const ast_stmt_t *stmt = block->first; stmt != nullptr; stmt = stmt->next) {
        if (stmt->kind == STMT_VAR_DEF) {
            fprintf(ctx->out, "DEFVAR LF@v_%u\n", stmt->as.var_def.var_id);
        }
        else if (stmt->kind == STMT_IF) {
            de_var_block(ctx, &stmt->as.if_stmt.then_blk);
            if (stmt->as.if_stmt.has_else) {
                de_var_block(ctx, &stmt->as.if_stmt.else_blk);
            } 
        }
        else if (stmt->kind == STMT_WHILE) {
            de_var_block(ctx, &stmt->as.while_stmt.block);
        }
    };
}

static void gen_func(codegen_ctx_t *ctx, const ast_func_t *func) {
    fprintf(ctx->out, "LABEL &fn_%s\n",dstr_cstr(func->name));
    de_var_block(ctx, &func->body);
    gen_block(ctx, &func->body);
    fputs("POPFRAME\n", ctx->out);
    fputs("RETURN\n", ctx->out);
}

void codegen(const ast_prog_t *prog, FILE *out) {
    codegen_ctx_t ctx = {.out = out, .label_id = 0};

    fputs(".IFJcode26\n", out);
    fputs("DEFVAR GF@tmp\n", out);
    fputs("CREATEFRAME\n", out);
    fputs("PUSHFRAME\n", out);

    for (const ast_func_t *func = prog->first; func != nullptr; func = func->next) {
        if(func->is_main) {
            fprintf(out, "CALL &fn_%s\n", dstr_cstr(func->name));
            break;
        }
    }
    fputs("EXIT int@0\n", out);
    
    for (const ast_func_t *func = prog->first; func != nullptr; func = func->next) {
        gen_func(&ctx, func);
    }
}

// NOLINTEND(cert-err33-c)