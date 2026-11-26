/**
 * @file codegen.c
 *
 * @author Jakub Machac
 * @brief implementace modulu generátoru kódu
 *
 * @details
 */

#include "codegen.h"
#include "ast.h"
#include "dstr.h"
#include "error.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define SPECIAL_CHAR_LAST 32

typedef struct {
    FILE *out;
    unsigned label_id;
    const ast_func_t *current_func;
} codegen_ctx_t;

// NOLINTBEGIN(cert-err33-c)

/** * @brief Převede řetězec na konstantu string@... . */
static void print_string(FILE *out, const char *str) {
    fputs("string@", out);
    for (const unsigned char *ptr = (const unsigned char *)str; *ptr != '\0';
         ptr++) {
        if (*ptr <= SPECIAL_CHAR_LAST || *ptr == '#' || *ptr == '\\') {
            fprintf(out, "\\%03u", *ptr);
        } else {
            fputc(*ptr, out);
        }
    }
}

static void gen_func_call(codegen_ctx_t *ctx, const ast_expr_t *expr);

/** * @brief Rekurzivne vyhodnoti uzel výrazu tak, aby hodnota zůstala uložena
 * na vrcholu zásobníku. */
static void gen_expr(codegen_ctx_t *ctx, const ast_expr_t *expr) { // NOLINT
    switch (expr->kind) {
    case E_INT_LIT:
        fprintf(ctx->out, "PUSHS int@%d\n", expr->as.int_val);
        break;
    case E_UNARY:
        gen_expr(ctx, expr->as.unary.operand);
        switch (expr->as.unary.op) {
        case OPU_PLUS:
            break;
        case OPU_NOT:
            fputs("NOTS\n", ctx->out);
            break;
        case OPU_INT2FLOAT:
            fputs("INT2FLOATS\n", ctx->out);
            break;
        case OPU_NEG:
            if (expr->data_type == TYPE_INT) {
                fputs("PUSHS int@-1\n", ctx->out);
            } else {
                fputs("PUSHS float@-0x1p+0\n", ctx->out);
            }
            fputs("MULS\n", ctx->out);
            break;
        }
        break;
    case E_BINARY:
        // &&
        if (expr->as.binary.op == OPB_AND) {
            unsigned idx = ++ctx->label_id;
            gen_expr(ctx, expr->as.binary.left);
            fputs("POPS LF@tmp1\n", ctx->out);
            fprintf(ctx->out, "JUMPIFEQ &and_false_%u LF@tmp1 bool@false\n",
                    idx);
            gen_expr(ctx, expr->as.binary.right);
            fprintf(ctx->out, "JUMP &and_end_%u\n", idx);
            fprintf(ctx->out, "LABEL &and_false_%u\n", idx);
            fputs("PUSHS bool@false\n", ctx->out);
            fprintf(ctx->out, "LABEL &and_end_%u\n", idx);
            break;
        }
        // ||
        if (expr->as.binary.op == OPB_OR) {
            unsigned idx = ++ctx->label_id;
            gen_expr(ctx, expr->as.binary.left);
            fputs("POPS LF@tmp1\n", ctx->out);
            fprintf(ctx->out, "JUMPIFEQ &or_false_%u LF@tmp1 bool@true\n", idx);
            gen_expr(ctx, expr->as.binary.right);
            fprintf(ctx->out, "JUMP &or_end_%u\n", idx);
            fprintf(ctx->out, "LABEL &or_false_%u\n", idx);
            fputs("PUSHS bool@true\n", ctx->out);
            fprintf(ctx->out, "LABEL &or_end_%u\n", idx);
            break;
        }
        gen_expr(ctx, expr->as.binary.left);
        gen_expr(ctx, expr->as.binary.right);
        switch (expr->as.binary.op) {
        // +
        case OPB_ADD:
            if (expr->data_type == TYPE_STRING) {
                fputs("POPS LF@tmp2\n", ctx->out);
                fputs("POPS LF@tmp1\n", ctx->out);
                fputs("CONCAT LF@tmp1 LF@tmp1 LF@tmp2\n", ctx->out);
                fputs("PUSHS LF@tmp1\n", ctx->out);
            } else {
                fputs("ADDS\n", ctx->out);
            }
            break;
        // -
        case OPB_SUB:
            fputs("SUBS\n", ctx->out);
            break;
        // /
        case OPB_DIV:
            if (expr->data_type == TYPE_INT) {
                fputs("IDIVS\n", ctx->out);
            } else {
                fputs("DIVS\n", ctx->out);
            }
            break;
        // *
        case OPB_MUL:
            fputs("MULS\n", ctx->out);
            break;
        // ==
        case OPB_EQ:
            fputs("EQS\n", ctx->out);
            break;
        // >
        case OPB_GT:
            fputs("GTS\n", ctx->out);
            break;
        // =>
        case OPB_GTE:
            fputs("LTS\nNOTS\n", ctx->out);
            break;
        // <
        case OPB_LT:
            fputs("LTS\n", ctx->out);
            break;
        // =<
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
            break;
        default:
            break;
        }
        break;

    case E_FUNC:
        if (expr->as.func.target == nullptr) {
            gen_func_call(ctx, expr);
        } else {
            for (size_t idx = 0; idx < expr->as.func.argc; idx++) {
                gen_expr(ctx, expr->as.func.args[idx]);
            }
            fputs("CREATEFRAME\n", ctx->out);
            fputs("PUSHFRAME\n", ctx->out);
            fprintf(ctx->out, "CALL &fn_%s\n",
                    dstr_cstr(expr->as.func.target->name));
        }
        break;
    case E_VAR:
        fprintf(ctx->out, "PUSHS LF@v_%u\n", expr->as.var.var_id);
        break;
    case E_UNIT_LIT:
        fputs("PUSHS nil@nil\n",
              ctx->out); // musim dat na zasobnik, jinak by se mohl stat ze u
                         // POPS vznikne chyba kvuly prazdnemu zasobniku
        break;
    case E_DOUBLE_LIT:
        fprintf(ctx->out, "PUSHS float@%a\n", expr->as.double_val);
        break;
    case E_STRING_LIT:
        fputs("PUSHS ", ctx->out);
        print_string(ctx->out, dstr_cstr(expr->as.str));
        fputs("\n", ctx->out);
        break;
    case E_BOOLEAN_LIT:
        fprintf(ctx->out, "PUSHS bool@%s\n",
                expr->as.bool_val ? "true" : "false");
        break;
    default:
        break;
    }
}

/** * @brief Vygeneruje instrukce pro volání funkcí. */
static void gen_func_call(codegen_ctx_t *ctx, const ast_expr_t *expr) {
    const char *func_name = dstr_cstr(expr->as.func.name);

    // print(value: Any): Unit
    if (strcmp(func_name, "print") == 0) {
        gen_expr(ctx, expr->as.func.args[0]);
        fputs("POPS LF@tmp1\n", ctx->out);
        if (expr->as.func.args[0]->data_type == TYPE_UNIT) {
            fputs("WRITE string@()\n", ctx->out);
        } else {
            fputs("WRITE LF@tmp1\n", ctx->out);
        }
        fputs("PUSHS nil@nil\n", ctx->out);
    }

    // readLine(): String
    else if (strcmp(func_name, "readLine") == 0) {
        fputs("READ LF@tmp1 string\n", ctx->out);
        fputs("PUSHS LF@tmp1\n", ctx->out);

    }

    // readInt(): Int (pri chybe/EOF vraci nil, potrebujeme 0)
    else if (strcmp(func_name, "readInt") == 0) {
        unsigned idx = ++ctx->label_id;
        fputs("READ LF@tmp1 int\n", ctx->out);
        fprintf(ctx->out, "JUMPIFEQ &ri_zero_%u LF@tmp1 nil@nil\n", idx);
        // nejmenší hodnota u 32bit čísla
        fprintf(ctx->out, "LT LF@tmp2 LF@tmp1 int@%d\n", INT32_MIN);
        fprintf(ctx->out, "JUMPIFEQ &ri_zero_%u LF@tmp2 bool@true\n", idx);
        // nejvetší hodnota u 32bit čísla
        fprintf(ctx->out, "GT LF@tmp2 LF@tmp1 int@%d\n", INT32_MAX);
        fprintf(ctx->out, "JUMPIFEQ &ri_zero_%u LF@tmp2 bool@true\n", idx);
        fprintf(ctx->out, "JUMP &ri_ok_%u\n", idx);
        fprintf(ctx->out, "LABEL &ri_zero_%u\n", idx);
        fputs("MOVE LF@tmp1 int@0\n", ctx->out);
        fprintf(ctx->out, "LABEL &ri_ok_%u\n", idx);
        fputs("PUSHS LF@tmp1\n", ctx->out);
    }

    // readDouble(): Double (chyba 0.0)
    else if (strcmp(func_name, "readDouble") == 0) {
        unsigned idx = ++ctx->label_id;
        fputs("READ LF@tmp1 float\n", ctx->out);
        fprintf(ctx->out, "JUMPIFNEQ &rd_ok_%u LF@tmp1 nil@nil\n", idx);
        fputs("MOVE LF@tmp1 float@0x0p+0\n", ctx->out);
        fprintf(ctx->out, "LABEL &rd_ok_%u\n", idx);
        fputs("PUSHS LF@tmp1\n", ctx->out);
    }

    // toStr(value: Any): String
    else if (strcmp(func_name, "toStr") == 0) {
        gen_expr(ctx, expr->as.func.args[0]);
        switch (expr->as.func.args[0]->data_type) {
        case TYPE_INT:
            fputs("INT2STRS\n", ctx->out);
            break;
        case TYPE_UNKNOWN: // chyba
            error_fatalf_cleanup(ERR_SEM_TYPE, "toStr UNKNOWN TYPE");
            break;
        case TYPE_DOUBLE:
            fputs("FLOAT2STRS\n", ctx->out);
            break;
        case TYPE_STRING:
            break;
        case TYPE_UNIT:
            fputs("POPS LF@tmp1\n", ctx->out);
            fputs("PUSHS string@()\n", ctx->out);
            break;
        case TYPE_BOOL:
            unsigned idx = ++ctx->label_id;
            fputs("POPS LF@tmp1\n", ctx->out);
            fprintf(ctx->out, "JUMPIFEQ &bstr_t_%u LF@tmp1 bool@true\n", idx);
            fputs("PUSHS string@false\n", ctx->out);
            fprintf(ctx->out, "JUMP &bstr_end_%u\n", idx);
            fprintf(ctx->out, "LABEL &bstr_t_%u\n", idx);
            fputs("PUSHS string@true\n", ctx->out);
            fprintf(ctx->out, "LABEL &bstr_end_%u\n", idx);
            break;
        case TYPE_ANY: // chyba
            error_fatalf_cleanup(ERR_SEM_TYPE, "toStr ANY TYPE");
            break;
        }
    }

    // toInt(value: Double): Int
    else if (strcmp(func_name, "toInt") == 0) {
        gen_expr(ctx, expr->as.func.args[0]);
        fputs("FLOAT2INTS\n", ctx->out);
    }

    // strLength(value: String): Int
    else if (strcmp(func_name, "strLength") == 0) {
        gen_expr(ctx, expr->as.func.args[0]);
        fputs("POPS LF@tmp1\n", ctx->out);
        fputs("STRLEN LF@tmp1 LF@tmp1\n", ctx->out);
        fputs("PUSHS LF@tmp1\n", ctx->out);
    }

    // subStr(value: String, start: Int, end: Int): String
    else if (strcmp(func_name, "subStr") == 0) {
        unsigned idx = ++ctx->label_id;
        gen_expr(ctx, expr->as.func.args[0]);
        gen_expr(ctx, expr->as.func.args[1]);
        gen_expr(ctx, expr->as.func.args[2]);
        fputs("POPS LF@tmp3\n", ctx->out);
        fputs("POPS LF@tmp2\n", ctx->out);
        fputs("POPS LF@tmp1\n", ctx->out);
        fputs("STRLEN LF@tmp4 LF@tmp1\n", ctx->out);

        fputs("LT LF@tmp5 LF@tmp2 int@0\n", ctx->out);
        fprintf(ctx->out, "JUMPIFEQ &sub_err_%u LF@tmp5 bool@true\n", idx);
        fputs("LT LF@tmp5 LF@tmp3 int@0\n", ctx->out);
        fprintf(ctx->out, "JUMPIFEQ &sub_err_%u LF@tmp5 bool@true\n", idx);
        fputs("GT LF@tmp5 LF@tmp2 LF@tmp3\n", ctx->out);
        fprintf(ctx->out, "JUMPIFEQ &sub_err_%u LF@tmp5 bool@true\n", idx);
        fputs("LT LF@tmp5 LF@tmp2 LF@tm4\n", ctx->out);
        fprintf(ctx->out, "JUMPIFEQ &sub_err_%u LF@tmp5 bool@false\n", idx);
        
        fputs("MOVE LF@tmp4 string@\n", ctx->out);
        fprintf(ctx->out, "LABEL &sub_loop_%u\n", idx);
        fprintf(ctx->out, "JUMPIFEQ &sub_done_%u LF@tmp2 LF@tmp3\n", idx);
        fputs("GETCHAR LF@tmp5 LF@tmp1 LF@tmp2\n", ctx->out);
        fputs("CONCAT LF@tmp4 LF@tmp4 LF@tmp5\n", ctx->out);
        fputs("ADD LF@tmp2 LF@tmp2 int@1\n", ctx->out);
        fprintf(ctx->out, "JUMP &sub_loop_%u\n", idx);

        fprintf(ctx->out, "LABEL &sub_err_%u\n", idx);
        fputs("EXIT int@40\n", ctx->out);

        fprintf(ctx->out, "LABEL &sub_done_%u\n", idx);
        fputs("PUSHS LF@tmp4\n", ctx->out);

    }

    // ord(value: String, index: Int): Int
    else if (strcmp(func_name, "ord") == 0) {
        unsigned idx = ++ctx->label_id;
        gen_expr(ctx, expr->as.func.args[0]);
        gen_expr(ctx, expr->as.func.args[1]);
        fputs("POPS LF@tmp2\n", ctx->out);
        fputs("POPS LF@tmp1\n", ctx->out);

        // index < 0
        fputs("LT LF@tmp3 LF@tmp2 int@0\n", ctx->out);
        fprintf(ctx->out, "JUMPIFEQ &ord_err_%u LF@tmp3 bool@true\n", idx);

        // idx >= strLenght
        fputs("STRLEN LF@tmp3 LF@tmp1\n", ctx->out);
        fputs("LT LF@tmp3 LF@tmp2 LF@tmp3\n", ctx->out);
        fprintf(ctx->out, "JUMPIFEQ &ord_err_%u LF@tmp3 bool@false\n", idx);

        fputs("STRI2INT LF@tmp1 LF@tmp1 LF@tmp2\n", ctx->out);
        fputs("PUSHS LF@tmp1\n", ctx->out);
        fprintf(ctx->out, "JUMP &ord_end_%u\n", idx);

        fprintf(ctx->out, "LABEL &ord_err_%u\n", idx);
        fputs("PUSHS int@-1\n", ctx->out);
        fprintf(ctx->out, "LABEL &ord_end_%u\n", idx);
    }

    // chr(code: Int): String
    else if (strcmp(func_name, "chr") == 0) {
        gen_expr(ctx, expr->as.func.args[0]);
        fputs("INT2CHARS\n", ctx->out);
    }

    // readBoolean()
    else if (strcmp(func_name, "readBoolean") == 0) {
        unsigned idx = ++ctx->label_id;
        fputs("READ LF@tmp1 bool\n", ctx->out);
        fprintf(ctx->out, "JUMPIFNEQ &rb_ok_%u LF@tmp1 nil@nil\n", idx);
        fputs("MOVE LF@tmp1 bool@false\n", ctx->out);
        fprintf(ctx->out, "LABEL &rb_ok_%u\n", idx);
        fputs("PUSHS LF@tmp1\n", ctx->out);
    }
}

static void gen_block(codegen_ctx_t *ctx, const ast_block_t *block);

/** * @brief Příkaz z AST převede do instrukce IFJcode26. */
static void gen_stmt(codegen_ctx_t *ctx, const ast_stmt_t *stmt) {
    switch (stmt->kind) {
    case STMT_VAR_DEF:
        gen_expr(ctx, stmt->as.var_def.init);
        fprintf(ctx->out, "POPS LF@v_%u\n", stmt->as.var_def.var_id);
        break;
    case STMT_EXPR:
        gen_expr(ctx, stmt->as.expr);
        fputs("POPS LF@tmp1\n", ctx->out);
        break;
    case STMT_RET:
        if (stmt->as.ret_val != nullptr) {
            gen_expr(ctx, stmt->as.ret_val);
            if (ctx->current_func->ret_type == TYPE_UNIT) {
                fputs("POPS LF@tmp1\n", ctx->out);
                fputs("PUSHS nil@nil\n", ctx->out);
            }
        } else {
            fputs("PUSHS nil@nil\n", ctx->out);
        }

        fprintf(ctx->out, "JUMP &fn_epilog_%s\n",
                dstr_cstr(ctx->current_func->name));

        break;
    case STMT_ASSIGN:
        gen_expr(ctx, stmt->as.assign.val);
        fprintf(ctx->out, "POPS LF@v_%u\n", stmt->as.assign.var_id);
        break;
    case STMT_IF: {
        unsigned if_id = ++ctx->label_id;
        gen_expr(ctx, stmt->as.if_stmt.cond);
        fputs("POPS LF@tmp1\n", ctx->out);
        fprintf(ctx->out, "JUMPIFEQ &if_else_%u LF@tmp1 bool@false\n", if_id);

        gen_block(ctx, &stmt->as.if_stmt.then_blk);
        fprintf(ctx->out, "JUMP &if_end_%u\n", if_id);

        fprintf(ctx->out, "LABEL &if_else_%u\n", if_id);
        if (stmt->as.if_stmt.has_else) {
            gen_block(ctx, &stmt->as.if_stmt.else_blk);
        }
        fprintf(ctx->out, "LABEL &if_end_%u\n", if_id);
        break;
    }
    case STMT_WHILE: {
        unsigned while_id = ++ctx->label_id;
        fprintf(ctx->out, "LABEL &while_start_%u\n", while_id);

        gen_expr(ctx, stmt->as.while_stmt.cond);
        fputs("POPS LF@tmp1\n", ctx->out);

        fprintf(ctx->out, "JUMPIFEQ &while_end_%u LF@tmp1 bool@false\n",
                while_id);

        gen_block(ctx, &stmt->as.while_stmt.block);

        fprintf(ctx->out, "JUMP &while_start_%u\n", while_id);
        fprintf(ctx->out, "LABEL &while_end_%u\n", while_id);
        break;
    }
    }
}

/** * @brief Projde seznam příkazů a udělá jeho překlad. */
static void gen_block(codegen_ctx_t *ctx, const ast_block_t *block) {
    for (const ast_stmt_t *stmt = block->first; stmt != nullptr;
         stmt = stmt->next) {
        gen_stmt(ctx, stmt);
    }
}

/** * @brief Rekurzivne projde bloky příkazů a vypíše instrukce DEVAR pro
 * lokální proměnné. */
static void de_var_block(codegen_ctx_t *ctx, const ast_block_t *block) {
    for (const ast_stmt_t *stmt = block->first; stmt != nullptr;
         stmt = stmt->next) {
        if (stmt->kind == STMT_VAR_DEF) {
            fprintf(ctx->out, "DEFVAR LF@v_%u\n", stmt->as.var_def.var_id);
        } else if (stmt->kind == STMT_IF) {
            de_var_block(ctx, &stmt->as.if_stmt.then_blk);
            if (stmt->as.if_stmt.has_else) {
                de_var_block(ctx, &stmt->as.if_stmt.else_blk);
            }
        } else if (stmt->kind == STMT_WHILE) {
            de_var_block(ctx, &stmt->as.while_stmt.block);
        }
    }
}

/** * @brief Vygeneruje hlavičku funkce, deklarace pomocných proměnných,
 * nadeklaruje parametry a vezme jejich hodnoty z konce sásobníku. Přeloží tělo
 * a uzavře ji. */
static void gen_func(codegen_ctx_t *ctx, const ast_func_t *func) {
    ctx->current_func = func;
    fprintf(ctx->out, "\nLABEL &fn_%s\n", dstr_cstr(func->name));

    fputs("DEFVAR LF@tmp1\n", ctx->out);
    fputs("DEFVAR LF@tmp2\n", ctx->out);
    fputs("DEFVAR LF@tmp3\n", ctx->out);
    fputs("DEFVAR LF@tmp4\n", ctx->out);
    fputs("DEFVAR LF@tmp5\n", ctx->out);

    for (size_t i = 0; i < func->param_count; i++) {
        fprintf(ctx->out, "DEFVAR LF@v_%u\n", func->params[i].var_id);
    }
    for (size_t i = func->param_count; i > 0; i--) {
        fprintf(ctx->out, "POPS LF@v_%u\n", func->params[i - 1].var_id);
    }
    de_var_block(ctx, &func->body);
    // osetreni funkce bez return
    const ast_stmt_t *stmt = func->body.first;
    while (stmt != nullptr && stmt != func->body.last) {
        gen_stmt(ctx, stmt);
        stmt = stmt->next;
    }
    if (stmt != nullptr && stmt->kind == STMT_EXPR &&
        func->ret_type != TYPE_UNIT) {
        gen_expr(ctx, stmt->as.expr);
    } else {
        if (stmt != nullptr) {
            gen_stmt(ctx, stmt);
        }
        fputs("PUSHS nil@nil\n", ctx->out);
    }
    fprintf(ctx->out, "LABEL &fn_epilog_%s\n", dstr_cstr(func->name));
    fputs("POPFRAME\n", ctx->out);
    fputs("RETURN\n", ctx->out);
}

void codegen(const ast_prog_t *prog, FILE *out) {
    codegen_ctx_t ctx = {.out = out, .label_id = 0};

    fputs(".IFJcode26\n", out);
    fputs("CREATEFRAME\n", out);
    fputs("PUSHFRAME\n", out);

    for (const ast_func_t *func = prog->first; func != nullptr;
         func = func->next) {
        if (func->is_main) {
            fprintf(out, "CALL &fn_%s\n", dstr_cstr(func->name));
            break;
        }
    }
    fputs("EXIT int@0\n", out);

    for (const ast_func_t *func = prog->first; func != nullptr;
         func = func->next) {
        gen_func(&ctx, func);
    }
}

// NOLINTEND(cert-err33-c)
