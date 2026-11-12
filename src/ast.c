/**
 * @file ast.c
 *
 * @author Jakub Manoušek
 * @brief Implementace modulu pro AST (abstraktní syntaktický strom)
 *
 * @details
 */
#include "ast.h"
#include "dstr.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

ast_prog_t *ast_prog_create() {
    ast_prog_t *prog = calloc(1, sizeof(ast_prog_t));
    if (prog == nullptr) {
        return nullptr;
    }

    prog->count = 0;
    prog->first = nullptr;
    prog->last = nullptr;

    return prog;
}

ast_func_t *ast_func_create(unsigned line) {
    ast_func_t *func = calloc(1, sizeof(ast_func_t));
    if (func == nullptr) {
        return nullptr;
    }

    func->name = nullptr;
    func->is_main = false;
    func->params = nullptr;
    func->param_count = 0;
    func->has_ret_type = false;
    func->ret_type = TYPE_UNKNOWN;
    func->line = line;
    func->body = (ast_block_t){0}; /* zero out */
    func->next = nullptr;

    return func;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
ast_stmt_t *ast_stmt_create(unsigned line, stmt_kind kind) {
    ast_stmt_t *stmt = calloc(1, sizeof(ast_stmt_t));
    if (stmt == nullptr) {
        return nullptr;
    }

    stmt->kind = kind;
    stmt->next = nullptr;
    stmt->line = line;

    return stmt;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
ast_expr_t *ast_expr_create(unsigned line, expr_kind kind) {
    ast_expr_t *expr = calloc(1, sizeof(ast_expr_t));
    if (expr == nullptr) {
        return nullptr;
    }

    expr->kind = kind;
    expr->data_type = TYPE_UNKNOWN;
    expr->line = line;

    return expr;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
ast_expr_t *ast_expr_unary_create(unsigned line, op_unary opu,
                                  ast_expr_t *operand) {
    ast_expr_t *expr = calloc(1, sizeof(ast_expr_t));
    if (expr == nullptr) {
        return nullptr;
    }

    expr->kind = E_UNARY;
    if (operand != nullptr) {
        expr->data_type = operand->data_type;
    } else {
        expr->data_type = TYPE_UNKNOWN;
    }
    expr->line = line;
    expr->as.unary.op = opu;
    expr->as.unary.operand = operand;

    return expr;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
ast_expr_t *ast_expr_binary_create(unsigned line, op_binary opb,
                                   ast_expr_t *left, ast_expr_t *right) {
    ast_expr_t *expr = calloc(1, sizeof(ast_expr_t));
    if (expr == nullptr) {
        return nullptr;
    }

    expr->kind = E_BINARY;
    if (left != nullptr) {
        expr->data_type = left->data_type;
    } else if (right != nullptr) {
        expr->data_type = right->data_type;
    } else {
        expr->data_type = TYPE_UNKNOWN;
    }
    expr->line = line;
    expr->as.binary.op = opb;
    expr->as.binary.left = left;
    expr->as.binary.right = right;

    return expr;
}

bool ast_block_append(ast_block_t *block, ast_stmt_t *stmt) {
    if (block == nullptr || stmt == nullptr) {
        return false;
    }

    if (block->last != nullptr) {
        block->last->next = stmt;
    } else {
        block->first = stmt;
    }
    block->last = stmt;
    block->count++;

    return true;
}

bool ast_prog_append(ast_prog_t *program, ast_func_t *func) {
    if (program == nullptr || func == nullptr) {
        return false;
    }

    if (program->last != nullptr) {
        program->last->next = func;
    } else {
        program->first = func;
    }
    program->last = func;
    program->count++;

    return true;
}

static void stmt_free(ast_stmt_t *stmt);
static void expr_free(ast_expr_t *expr);
static void block_free(ast_block_t *block);

static void expr_free(ast_expr_t *expr) {
    switch (expr->kind) {
    case E_UNARY:
        expr_free(expr->as.unary.operand);
        break;
    case E_BINARY:
        expr_free(expr->as.binary.left);
        expr_free(expr->as.binary.right);
        break;
    case E_FUNC:
        for (size_t i = 0; i < expr->as.func.argc; i++) {
            expr_free(expr->as.func.args[i]);
        }
        free((ast_expr_t *)expr->as.func.args);
        break;
    case E_STRING_LIT:
        dstr_delete(expr->as.str);
        break;
    case E_VAR:
        dstr_delete(expr->as.var.name);
        break;
    case E_UNIT_LIT:
    case E_INT_LIT:
    case E_DOUBLE_LIT:
    case E_BOOLEAN_LIT:
        break;
    }
    free(expr);
}

static void block_free(ast_block_t *block) {
    if (block == nullptr) {
        return;
    }
    for (ast_stmt_t *stmt = block->first; stmt != nullptr;) {
        ast_stmt_t *next = stmt->next;
        stmt_free(stmt);
        stmt = next;
    }
    block->first = nullptr;
    block->last = nullptr;
    block->count = 0;
}

static void stmt_free(ast_stmt_t *stmt) {
    if (stmt == nullptr) {
        return;
    }
    switch (stmt->kind) {
    case STMT_VAR_DEF:
        dstr_delete(stmt->as.var_def.name);
        expr_free(stmt->as.var_def.init);
        break;
    case STMT_EXPR:
        expr_free(stmt->as.expr);
        break;
    case STMT_RET:
        expr_free(stmt->as.ret_val);
        break;
    case STMT_ASSIGN:
        dstr_delete(stmt->as.assign.name);
        expr_free(stmt->as.assign.val);
        break;
    case STMT_IF:
        expr_free(stmt->as.if_stmt.cond);
        block_free(&stmt->as.if_stmt.then_blk);
        if (stmt->as.if_stmt.has_else) {
            block_free(&stmt->as.if_stmt.else_blk);
        }
        break;
    case STMT_WHILE:
        expr_free(stmt->as.while_stmt.cond);
        block_free(&stmt->as.while_stmt.block);
        break;
    }
    free(stmt);
}

void ast_prog_free(ast_prog_t *program) {
    ast_func_t *func = program->first;
    while (func != nullptr) {
        ast_func_t *next = func->next;
        dstr_delete(func->name);
        for (size_t i = 0; i < func->param_count; i++) {
            dstr_delete(func->params[i].name);
        }
        free(func->params);
        block_free(&func->body);
        free(func);
        func = next;
    }
    program->first = nullptr;
    program->last = nullptr;
    program->count = 0;
}

#define COUNT_OF(a) (sizeof(a) / sizeof((a)[0]))
#define NAME_OF(tbl, i) name_lookup((tbl), COUNT_OF(tbl), (size_t)(i))

static const char *const type_names[] = {
    [TYPE_UNKNOWN] = "unknown", [TYPE_INT] = "int",   [TYPE_DOUBLE] = "double",
    [TYPE_STRING] = "string",   [TYPE_UNIT] = "unit", [TYPE_BOOL] = "bool",
    [TYPE_ANY] = "any",
};

static const char *const unary_names[] = {
    [OPU_NEG] = "neg",
    [OPU_PLUS] = "plus",
    [OPU_NOT] = "not",
};

static const char *const binary_names[] = {
    [OPB_ADD] = "add", [OPB_SUB] = "sub", [OPB_MUL] = "mul", [OPB_DIV] = "div",
    [OPB_AND] = "and", [OPB_OR] = "or",   [OPB_LT] = "lt",   [OPB_LTE] = "lte",
    [OPB_GT] = "gt",   [OPB_GTE] = "gte", [OPB_EQ] = "eq",   [OPB_NE] = "ne",
};

static const char *name_lookup(const char *const *tbl, size_t n, size_t idx) {
    return (idx < n && tbl[idx] != nullptr) ? tbl[idx] : "?";
}

// NOLINTBEGIN(cert-err33-c)

static void indent(unsigned depth, FILE *out) {
    for (unsigned i = 0; i < depth; i++) {
        fputc('\t', out);
    }
}

static void expr_dump(const ast_expr_t *expr, FILE *out);
static void stmt_dump(const ast_stmt_t *stmt, unsigned depth, FILE *out);
static void block_dump(const char *label, const ast_block_t *block,
                       unsigned depth, FILE *out);

static void expr_dump(const ast_expr_t *expr, FILE *out) {
    if (expr == nullptr) {
        fputs("EXPR: NULL", out);
        return;
    }
    fprintf(out, "EXPR: TYPE(%s): ", NAME_OF(type_names, expr->data_type));
    switch (expr->kind) {
    case E_UNARY:
        fprintf(out, "UNARY(%s): ", NAME_OF(unary_names, expr->as.unary.op));
        expr_dump(expr->as.unary.operand, out);
        break;
    case E_BINARY:
        fputs("BINARY(left(", out);
        expr_dump(expr->as.binary.left, out);
        fprintf(out, ") %s right(", NAME_OF(binary_names, expr->as.binary.op));
        expr_dump(expr->as.binary.right, out);
        fputs("))", out);
        break;
    case E_FUNC:
        fprintf(out, "FUNC: %s(", dstr_cstr(expr->as.func.name));
        for (size_t i = 0; i < expr->as.func.argc; i++) {
            if (i > 0) {
                fputc(',', out);
            }
            expr_dump(expr->as.func.args[i], out);
        }
        fputc(')', out);
        break;
    case E_UNIT_LIT:
        fputs("UNIT", out);
        break;
    case E_INT_LIT:
        fprintf(out, "INT(%d)", expr->as.int_val);
        break;
    case E_DOUBLE_LIT:
        fprintf(out, "DOUBLE(%f)", expr->as.double_val);
        break;
    case E_STRING_LIT:
        fprintf(out, "STRING(%s)", dstr_cstr(expr->as.str));
        break;
    case E_BOOLEAN_LIT:
        fputs(expr->as.bool_val ? "BOOL(true)" : "BOOL(false)", out);
        break;
    case E_VAR:
        fprintf(out, "VAR(%s:%u)", dstr_cstr(expr->as.var.name), expr->as.var.var_id);
        break;
    }
}

static void block_dump(const char *label, const ast_block_t *block,
                       unsigned depth, FILE *out) {
    if (block == nullptr) {
        return;
    }
    indent(depth, out);
    fprintf(out, "%s(%zu):\n", label, block->count);
    for (const ast_stmt_t *stmt = block->first; stmt != nullptr;
         stmt = stmt->next) {
        stmt_dump(stmt, depth + 1, out);
    }
}

static void stmt_dump(const ast_stmt_t *stmt, unsigned depth, FILE *out) {
    indent(depth, out);
    fputs("STMT: ", out);
    switch (stmt->kind) {
    case STMT_VAR_DEF:
        fprintf(out, "VARDEF: MUT(%s) TYPE(%s) %s(id: %u) INIT: ",
                stmt->as.var_def.is_mut ? "true" : "false",
                NAME_OF(type_names, stmt->as.var_def.type),
                dstr_cstr(stmt->as.var_def.name), stmt->as.var_def.var_id);
        expr_dump(stmt->as.var_def.init, out);
        break;
    case STMT_EXPR:
        expr_dump(stmt->as.expr, out);
        break;
    case STMT_RET:
        fputs("RET: ", out);
        expr_dump(stmt->as.ret_val, out);
        break;
    case STMT_ASSIGN:
        fprintf(out, "ASSIGN: %s(%u) = ", dstr_cstr(stmt->as.assign.name),
                stmt->as.assign.var_id);
        expr_dump(stmt->as.assign.val, out);
        break;
    case STMT_IF:
        fputs("IF: COND: ", out);
        expr_dump(stmt->as.if_stmt.cond, out);
        fputc('\n', out);
        block_dump("THEN", &stmt->as.if_stmt.then_blk, depth + 1, out);
        if (stmt->as.if_stmt.has_else) {
            block_dump("ELSE", &stmt->as.if_stmt.else_blk, depth + 1, out);
        }
        return;
    case STMT_WHILE:
        fputs("WHILE: COND: ", out);
        expr_dump(stmt->as.while_stmt.cond, out);
        fputc('\n', out);
        block_dump("BLOCK", &stmt->as.while_stmt.block, depth + 1, out);
        return;
    }
    fputc('\n', out);
}

void ast_prog_dump(const ast_prog_t *program, FILE *out) {
    for (const ast_func_t *func = program->first; func != nullptr;
         func = func->next) {
        fprintf(out, "FUNC%s: %s(", func->is_main ? " [main]" : "",
                dstr_cstr(func->name));
        for (size_t i = 0; i < func->param_count; i++) {
            if (i > 0) {
                fputs(", ", out);
            }
            fprintf(out, "%s: %s", dstr_cstr(func->params[i].name),
                    NAME_OF(type_names, func->params[i].type));
        }
        fputc(')', out);
        if (func->has_ret_type) {
            fprintf(out, " -> %s", NAME_OF(type_names, func->ret_type));
        }
        fputc('\n', out);
        block_dump("BODY", &func->body, 1, out);
    }
}

// NOLINTEND(cert-err33-c)
