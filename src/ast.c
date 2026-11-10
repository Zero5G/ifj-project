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
#include <stdlib.h>

ast_prog_t *ast_prog_create() {
    ast_prog_t *prog = malloc(sizeof(ast_prog_t));
    if (prog == nullptr) {
        return nullptr;
    }

    prog->count = 0;
    prog->first = nullptr;
    prog->last = nullptr;

    return prog;
}

ast_func_t *ast_func_create(unsigned line) {
    ast_func_t *func = malloc(sizeof(ast_func_t));
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
    ast_stmt_t *stmt = malloc(sizeof(ast_stmt_t));
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
    ast_expr_t *expr = malloc(sizeof(ast_expr_t));
    if (expr == nullptr) {
        return nullptr;
    }

    expr->kind = kind;
    expr->data_type = TYPE_UNKNOWN;
    expr->line = line;

    return expr;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
ast_expr_t *ast_expr_unary_create(unsigned line, op_unary opu, ast_expr_t *operand) {
    ast_expr_t *expr = malloc(sizeof(ast_expr_t));
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
ast_expr_t *ast_expr_binary_create(unsigned line, op_binary opb, ast_expr_t *left, ast_expr_t *right) {
    ast_expr_t *expr = malloc(sizeof(ast_expr_t));
    if (expr == nullptr) {
        return nullptr;
    }

    expr->kind = E_UNARY;
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
            free((ast_expr_t*)expr->as.func.args);
        case E_STRING_LIT:
            dstr_delete(expr->as.str);
        case E_UNIT_LIT:
        case E_INT_LIT:
        case E_DOUBLE_LIT:
        case E_BOOLEAN_LIT:
        break;
    }
}

static void block_free(ast_block_t *block) {
    if (block->first != nullptr) {
        stmt_free(block->first);
    }
    ast_stmt_t *next = block->first;
    ast_stmt_t *prev = block->first;
    while (block->count < 0) {
        prev = next;
        next = next->next;
        stmt_free(prev);
        block->count--;
    }
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
            block_free(stmt->as.if_stmt.then_blk);
            block_free(stmt->as.if_stmt.else_blk);
        case STMT_WHILE:
            expr_free(stmt->as.while_stmt.cond);
            block_free(stmt->as.while_stmt.block);
        break;
    }
    free(stmt);
}

void ast_prog_free(ast_prog_t *program) {
    ast_func_t *next = program->first;
    ast_func_t *prev = program->first;
    while (program->count > 0) {
        prev = next;
        block_free(&prev->body);
        next = next->next;
        free(prev->params);
        free(prev);
        program->count--;
    }
}
