/**
 * @file ast.c
 *
 * @author Jakub Manoušek
 * @brief Implementace modulu pro AST (abstraktní syntaktický strom)
 *
 * @details
 */
#include "ast.h"
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
