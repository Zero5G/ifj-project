// NOLINTBEGIN(bugprone-reserved-identifier, cert-dcl37-c, cert-dcl51-cpp,
// readability-magic-numbers)

#define _POSIX_C_SOURCE 200'809L /* open_memstream */

#include <criterion/criterion.h>

#include "ast.h"

#include <stdio.h>
#include <stdlib.h>

/* Any type != TYPE_UNKNOWN. Swap for a real one from types.h (e.g. TYPE_INT).
 */
#define NON_UNKNOWN_TYPE ((data_type)(TYPE_UNKNOWN + 1))

/*
 * Create/append tests free nodes with plain free() on purpose: they test only
 * create + append, not ast_prog_free. Only the ast_free suite uses
 * ast_prog_free.
 */

/* ---------- helpers ---------- */

static char *dump_to_string(const ast_prog_t *prog) {
    char *buf = nullptr;
    size_t len = 0;
    FILE *out = open_memstream(&buf, &len);
    cr_assert_not_null(out);
    ast_prog_dump(prog, out);
    fclose(out); // NOLINT
    return buf;
}

/* ---------- program ---------- */

Test(ast_prog, create_is_empty) {
    ast_prog_t *prog = ast_prog_create();
    cr_assert_not_null(prog);
    cr_assert_null(prog->first);
    cr_assert_null(prog->last);
    cr_assert_eq(prog->count, 0);
    free(prog);
}

Test(ast_prog, append_one_func) {
    ast_prog_t *prog = ast_prog_create();
    ast_func_t *func = ast_func_create(1);

    cr_assert(ast_prog_append(prog, func));
    cr_assert_eq(prog->first, func);
    cr_assert_eq(prog->last, func);
    cr_assert_eq(prog->count, 1);

    free(func);
    free(prog);
}

Test(ast_prog, append_two_funcs_keeps_order) {
    ast_prog_t *prog = ast_prog_create();
    ast_func_t *func1 = ast_func_create(1);
    ast_func_t *func2 = ast_func_create(2);

    cr_assert(ast_prog_append(prog, func1));
    cr_assert(ast_prog_append(prog, func2));
    cr_assert_eq(prog->first, func1);
    cr_assert_eq(prog->last, func2);
    cr_assert_eq(func1->next, func2);
    cr_assert_null(func2->next);
    cr_assert_eq(prog->count, 2);

    free(func1);
    free(func2);
    free(prog);
}

Test(ast_prog, append_null_args_fail) {
    ast_prog_t *prog = ast_prog_create();
    ast_func_t *func = ast_func_create(1);

    cr_assert_not(ast_prog_append(nullptr, func));
    cr_assert_not(ast_prog_append(prog, nullptr));
    cr_assert_eq(prog->count, 0);

    free(func);
    free(prog);
}

/* ---------- function ---------- */

Test(ast_func, create_defaults) {
    ast_func_t *func = ast_func_create(42);
    cr_assert_not_null(func);
    cr_assert_eq(func->line, 42);
    cr_assert_null(func->next);
    cr_assert_null(func->params);
    cr_assert_eq(func->param_count, 0);
    cr_assert_not(func->is_main);
    cr_assert_not(func->has_ret_type);
    cr_assert_eq(func->ret_type, TYPE_UNKNOWN);
    cr_assert_null(func->body.first);
    cr_assert_null(func->body.last);
    cr_assert_eq(func->body.count, 0);

    free(func);
}

/* ---------- statement ---------- */

Test(ast_stmt, create_sets_fields) {
    ast_stmt_t *stmt = ast_stmt_create(7, STMT_RET);
    cr_assert_not_null(stmt);
    cr_assert_eq(stmt->line, 7);
    cr_assert_eq(stmt->kind, STMT_RET);
    cr_assert_null(stmt->next);

    free(stmt);
}

/* ---------- block ---------- */

Test(ast_block, append_one_stmt) {
    ast_block_t blk = {};
    ast_stmt_t *stmt = ast_stmt_create(2, STMT_EXPR);

    cr_assert(ast_block_append(&blk, stmt));
    cr_assert_eq(blk.first, stmt);
    cr_assert_eq(blk.last, stmt);
    cr_assert_eq(blk.count, 1);

    free(stmt);
}

Test(ast_block, append_two_stmts_keeps_order) {
    ast_block_t blk = {};
    ast_stmt_t *stmt1 = ast_stmt_create(2, STMT_EXPR);
    ast_stmt_t *stmt2 = ast_stmt_create(3, STMT_RET);

    cr_assert(ast_block_append(&blk, stmt1));
    cr_assert(ast_block_append(&blk, stmt2));
    cr_assert_eq(blk.first, stmt1);
    cr_assert_eq(blk.last, stmt2);
    cr_assert_eq(stmt1->next, stmt2);
    cr_assert_null(stmt2->next);
    cr_assert_eq(blk.count, 2);

    free(stmt1);
    free(stmt2);
}

Test(ast_block, append_null_args_fail) {
    ast_block_t blk = {};
    ast_stmt_t *stmt = ast_stmt_create(1, STMT_EXPR);

    cr_assert_not(ast_block_append(nullptr, stmt));
    cr_assert_not(ast_block_append(&blk, nullptr));
    cr_assert_eq(blk.count, 0);

    free(stmt);
}

/* ---------- expressions ---------- */

Test(ast_expr, create_sets_fields) {
    ast_expr_t *expr = ast_expr_create(5, E_INT_LIT);
    cr_assert_not_null(expr);
    cr_assert_eq(expr->line, 5);
    cr_assert_eq(expr->kind, E_INT_LIT);
    cr_assert_eq(expr->data_type, TYPE_UNKNOWN);

    free(expr);
}

Test(ast_expr, unary_takes_operand_type) {
    ast_expr_t *operand = ast_expr_create(1, E_INT_LIT);
    operand->data_type = NON_UNKNOWN_TYPE;

    ast_expr_t *expr = ast_expr_unary_create(1, OPU_NEG, operand);
    cr_assert_not_null(expr);
    cr_assert_eq(expr->kind, E_UNARY);
    cr_assert_eq(expr->as.unary.op, OPU_NEG);
    cr_assert_eq(expr->as.unary.operand, operand);
    cr_assert_eq(expr->data_type, NON_UNKNOWN_TYPE);

    free(operand);
    free(expr);
}

Test(ast_expr, unary_null_operand_is_unknown) {
    ast_expr_t *expr = ast_expr_unary_create(1, OPU_NOT, nullptr);
    cr_assert_not_null(expr);
    cr_assert_null(expr->as.unary.operand);
    cr_assert_eq(expr->data_type, TYPE_UNKNOWN);

    free(expr);
}

Test(ast_expr, binary_sets_fields) {
    ast_expr_t *left = ast_expr_create(1, E_INT_LIT);
    ast_expr_t *right = ast_expr_create(1, E_INT_LIT);

    ast_expr_t *expr = ast_expr_binary_create(3, OPB_ADD, left, right);
    cr_assert_not_null(expr);
    cr_assert_eq(expr->line, 3);
    cr_assert_eq(expr->kind, E_BINARY);
    cr_assert_eq(expr->as.binary.op, OPB_ADD);
    cr_assert_eq(expr->as.binary.left, left);
    cr_assert_eq(expr->as.binary.right, right);

    free(left);
    free(right);
    free(expr);
}

Test(ast_expr, binary_takes_left_type) {
    ast_expr_t *left = ast_expr_create(1, E_INT_LIT);
    ast_expr_t *right = ast_expr_create(1, E_INT_LIT);
    left->data_type = NON_UNKNOWN_TYPE;

    ast_expr_t *expr = ast_expr_binary_create(1, OPB_MUL, left, right);
    cr_assert_eq(expr->data_type, NON_UNKNOWN_TYPE);

    free(left);
    free(right);
    free(expr);
}

Test(ast_expr, binary_null_left_takes_right_type) {
    ast_expr_t *right = ast_expr_create(1, E_INT_LIT);
    right->data_type = NON_UNKNOWN_TYPE;

    ast_expr_t *expr = ast_expr_binary_create(1, OPB_SUB, nullptr, right);
    cr_assert_eq(expr->data_type, NON_UNKNOWN_TYPE);

    free(right);
    free(expr);
}

Test(ast_expr, binary_both_null_is_unknown) {
    ast_expr_t *expr = ast_expr_binary_create(1, OPB_EQ, nullptr, nullptr);
    cr_assert_not_null(expr);
    cr_assert_eq(expr->data_type, TYPE_UNKNOWN);

    free(expr);
}

/* ---------- free ---------- */

Test(ast_free, nested_tree_no_crash) {
    /* run under valgrind/ASan to catch leaks + double frees */
    ast_expr_t *left = ast_expr_create(1, E_INT_LIT);
    left->as.int_val = 1;
    ast_expr_t *right = ast_expr_create(1, E_INT_LIT);
    right->as.int_val = 2;
    ast_expr_t *sum = ast_expr_binary_create(1, OPB_ADD, left, right);
    ast_expr_t *neg = ast_expr_unary_create(1, OPU_NEG, sum);

    ast_stmt_t *ret = ast_stmt_create(1, STMT_RET);
    ret->as.ret_val = neg;

    ast_func_t *func = ast_func_create(1);
    ast_block_append(&func->body, ret);

    ast_prog_t *prog = ast_prog_create();
    ast_prog_append(prog, func);
    ast_prog_free(prog);
}

/* ---------- dump ---------- */

Test(ast_dump, empty_program_prints_nothing) {
    ast_prog_t *prog = ast_prog_create();

    char *stmt = dump_to_string(prog);
    cr_assert_str_eq(stmt, "");

    free(stmt);
    free(prog);
}

// NOLINTEND(bugprone-reserved-identifier, cert-dcl37-c, cert-dcl51-cpp,
// readability-magic-numbers)
