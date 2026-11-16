#include <criterion/criterion.h>

#include <stdarg.h>
#include <stdlib.h>

#include "ast.h"
#include "dstr.h"
#include "semantic.h" /* ADAPT: header that declares semantic_analyze() */
#include "symtable.h"
#include "types.h"

/*
 * Errors are not reported yet, so error cases can't assert on messages.
 * They assert what is observable instead:
 *   - expr->data_type == TYPE_UNKNOWN
 *   - var_id still == NO_ID (sentinel set by the builders below)
 *   - no crash / no hang
 * Once error reporting exists, add error-count asserts to those tests.
 */

/* ---------- ADAPT to your project ---------- */

#define HAVE_VAR_COUNT 1 /* set 0 if ast_func has no var_count field yet */

/* build a dstr from char*; replace body with your dstr API */
static dstr mk_name(const char *s) {
    return dstr_create(s);
}

/* TYPE_INT, TYPE_DOUBLE, TYPE_STRING, TYPE_BOOL, TYPE_UNIT, TYPE_UNKNOWN
 * assumed from types.h; rename if yours differ */

#define NO_ID 0xFFFFFFFFu

/* ---------- fixture ---------- */

static ast_prog_t *prog;
static symtable *stab;

static void setup(void) {
    prog = ast_prog_create();
    cr_assert_not_null(prog);
    stab = symtable_create();
    cr_assert_not_null(stab);
}

static void teardown(void) {
    ast_prog_free(prog);
    symtable_destroy(stab);
    prog = nullptr;
    stab = nullptr;
}

TestSuite(sema, .init = setup, .fini = teardown);

static void run(void) {
    semantic_analyze(prog, stab);
}

static symbol *fsym(const char *name) {
    symbol *s = symtable_find_func(stab, name);
    cr_assert_not_null(s);
    return s;
}

/* ---------- expression builders ---------- */

static ast_expr_t *int_lit(int v) {
    ast_expr_t *e = ast_expr_create(1, E_INT_LIT);
    cr_assert_not_null(e);
    e->as.int_val = v;
    return e;
}

static ast_expr_t *dbl_lit(double v) {
    ast_expr_t *e = ast_expr_create(1, E_DOUBLE_LIT);
    cr_assert_not_null(e);
    e->as.double_val = v;
    return e;
}

static ast_expr_t *bool_lit(bool v) {
    ast_expr_t *e = ast_expr_create(1, E_BOOLEAN_LIT);
    cr_assert_not_null(e);
    e->as.bool_val = v;
    return e;
}

static ast_expr_t *str_lit(const char *s) {
    ast_expr_t *e = ast_expr_create(1, E_STRING_LIT);
    cr_assert_not_null(e);
    e->as.str = mk_name(s);
    return e;
}

static ast_expr_t *unit_lit(void) {
    ast_expr_t *e = ast_expr_create(1, E_UNIT_LIT);
    cr_assert_not_null(e);
    return e;
}

static ast_expr_t *var(const char *name) {
    ast_expr_t *e = ast_expr_create(1, E_VAR);
    cr_assert_not_null(e);
    e->as.var.name = mk_name(name);
    e->as.var.var_id = NO_ID;
    return e;
}

static ast_expr_t *call(const char *name, size_t argc, ...) {
    ast_expr_t *e = ast_expr_create(1, E_FUNC);
    cr_assert_not_null(e);
    e->as.func.name = mk_name(name);
    e->as.func.argc = argc;
    e->as.func.args = nullptr;
    e->as.func.target = nullptr;
    if (argc > 0) {
        e->as.func.args = malloc(argc * sizeof(ast_expr_t *));
        cr_assert_not_null(e->as.func.args);
        va_list ap;
        va_start(ap, argc);
        for (size_t i = 0; i < argc; i++) {
            e->as.func.args[i] = va_arg(ap, ast_expr_t *);
        }
        va_end(ap);
    }
    return e;
}

static ast_expr_t *un(op_unary op, ast_expr_t *operand) {
    ast_expr_t *e = ast_expr_unary_create(1, op, operand);
    cr_assert_not_null(e);
    return e;
}

static ast_expr_t *bin(op_binary op, ast_expr_t *l, ast_expr_t *r) {
    ast_expr_t *e = ast_expr_binary_create(1, op, l, r);
    cr_assert_not_null(e);
    return e;
}

/* ---------- statement builders ---------- */

static ast_stmt_t *let_stmt(const char *name, bool is_mut, data_type type,
                            ast_expr_t *init) {
    ast_stmt_t *s = ast_stmt_create(1, STMT_VAR_DEF);
    cr_assert_not_null(s);
    s->as.var_def.name = mk_name(name);
    s->as.var_def.var_id = NO_ID;
    s->as.var_def.is_mut = is_mut;
    s->as.var_def.type = type;
    s->as.var_def.init = init;
    return s;
}

static ast_stmt_t *assign_stmt(const char *name, ast_expr_t *val) {
    ast_stmt_t *s = ast_stmt_create(1, STMT_ASSIGN);
    cr_assert_not_null(s);
    s->as.assign.name = mk_name(name);
    s->as.assign.var_id = NO_ID;
    s->as.assign.val = val;
    return s;
}

static ast_stmt_t *ret_stmt(ast_expr_t *val) {
    ast_stmt_t *s = ast_stmt_create(1, STMT_RET);
    cr_assert_not_null(s);
    s->as.ret_val = val; /* nullptr = bare return */
    return s;
}

static ast_stmt_t *expr_stmt(ast_expr_t *e) {
    ast_stmt_t *s = ast_stmt_create(1, STMT_EXPR);
    cr_assert_not_null(s);
    s->as.expr = e;
    return s;
}

/* then_s / else_s: single stmt or nullptr (empty block) */
static ast_stmt_t *if_stmt(ast_expr_t *cond, ast_stmt_t *then_s,
                           ast_stmt_t *else_s) {
    ast_stmt_t *s = ast_stmt_create(1, STMT_IF);
    cr_assert_not_null(s);
    s->as.if_stmt.cond = cond;
    s->as.if_stmt.then_blk = (ast_block_t){};
    s->as.if_stmt.else_blk = (ast_block_t){};
    s->as.if_stmt.has_else = else_s != nullptr;
    if (then_s != nullptr) {
        cr_assert(ast_block_append(&s->as.if_stmt.then_blk, then_s));
    }
    if (else_s != nullptr) {
        cr_assert(ast_block_append(&s->as.if_stmt.else_blk, else_s));
    }
    return s;
}

static ast_stmt_t *while_stmt(ast_expr_t *cond, ast_stmt_t *body_s) {
    ast_stmt_t *s = ast_stmt_create(1, STMT_WHILE);
    cr_assert_not_null(s);
    s->as.while_stmt.cond = cond;
    s->as.while_stmt.block = (ast_block_t){};
    if (body_s != nullptr) {
        cr_assert(ast_block_append(&s->as.while_stmt.block, body_s));
    }
    return s;
}

/* ---------- function builders ---------- */

/* creates func and appends it to prog (order = declaration order) */
static ast_func_t *mk_func(const char *name, bool has_ret, data_type ret) {
    ast_func_t *f = ast_func_create(1);
    cr_assert_not_null(f);
    f->name = mk_name(name);
    f->has_ret_type = has_ret;
    f->ret_type = ret;
    f->params = nullptr;
    f->param_count = 0;
    cr_assert(ast_prog_append(prog, f));
    return f;
}

static ast_func_t *mk_main(void) {
    ast_func_t *f = mk_func("main", false, TYPE_UNKNOWN);
    f->is_main = true;
    return f;
}

static void add_param(ast_func_t *f, const char *name, data_type t) {
    ast_func_param_t *p =
        realloc(f->params, (f->param_count + 1) * sizeof(*p));
    cr_assert_not_null(p);
    f->params = p;
    f->params[f->param_count].name = mk_name(name);
    f->params[f->param_count].type = t;
    f->param_count++;
}

/* append stmt to func body, return it for later inspection */
static ast_stmt_t *add(ast_func_t *f, ast_stmt_t *s) {
    cr_assert(ast_block_append(&f->body, s));
    return s;
}

/* wrap expr in expr statement, append, return expr for inspection */
static ast_expr_t *probe(ast_func_t *f, ast_expr_t *e) {
    add(f, expr_stmt(e));
    return e;
}

/* ===================== program level ===================== */

Test(sema, null_args_no_crash) {
    semantic_analyze(nullptr, stab);
    semantic_analyze(prog, nullptr);
}

Test(sema, empty_program) {
    run();
}

Test(sema, empty_function) {
    mk_main();
    run();
    cr_assert_not_null(symtable_find_func(stab, "main"));
}

/* ===================== literals ===================== */

Test(sema, literal_types) {
    ast_func_t *m = mk_main();
    ast_expr_t *u = probe(m, unit_lit());
    ast_expr_t *i = probe(m, int_lit(1));
    ast_expr_t *d = probe(m, dbl_lit(1.5));
    ast_expr_t *s = probe(m, str_lit("s"));
    ast_expr_t *b = probe(m, bool_lit(true));
    run();
    cr_assert_eq(u->data_type, TYPE_UNIT);
    cr_assert_eq(i->data_type, TYPE_INT);
    cr_assert_eq(d->data_type, TYPE_DOUBLE);
    cr_assert_eq(s->data_type, TYPE_STRING);
    cr_assert_eq(b->data_type, TYPE_BOOL);
}

/* ===================== variables ===================== */

Test(sema, var_def_infers_type) {
    ast_func_t *m = mk_main();
    ast_stmt_t *d = add(m, let_stmt("x", false, TYPE_UNKNOWN, int_lit(1)));
    run();
    cr_assert_eq(d->as.var_def.type, TYPE_INT);
    cr_assert_eq(d->as.var_def.var_id, 0u);
}

Test(sema, var_def_explicit_type_kept) {
    ast_func_t *m = mk_main();
    ast_stmt_t *d = add(m, let_stmt("x", false, TYPE_DOUBLE, dbl_lit(1.0)));
    run();
    cr_assert_eq(d->as.var_def.type, TYPE_DOUBLE);
}

Test(sema, var_def_declared_type_wins_on_mismatch) {
    ast_func_t *m = mk_main();
    ast_stmt_t *d = add(m, let_stmt("x", false, TYPE_INT, str_lit("s")));
    ast_expr_t *use = probe(m, var("x"));
    run();
    cr_assert_eq(d->as.var_def.type, TYPE_INT);
    cr_assert_eq(use->data_type, TYPE_INT);
}

Test(sema, var_use_gets_type_and_id) {
    ast_func_t *m = mk_main();
    ast_stmt_t *a = add(m, let_stmt("a", false, TYPE_UNKNOWN, int_lit(1)));
    ast_stmt_t *b = add(m, let_stmt("b", false, TYPE_UNKNOWN, dbl_lit(2.0)));
    ast_expr_t *use = probe(m, var("b"));
    run();
    cr_assert_neq(a->as.var_def.var_id, b->as.var_def.var_id);
    cr_assert_eq(use->data_type, TYPE_DOUBLE);
    cr_assert_eq(use->as.var.var_id, b->as.var_def.var_id);
}

Test(sema, undefined_var) {
    ast_func_t *m = mk_main();
    ast_expr_t *use = probe(m, var("nope"));
    run();
    cr_assert_eq(use->data_type, TYPE_UNKNOWN);
    cr_assert_eq(use->as.var.var_id, NO_ID);
}

Test(sema, var_self_reference_is_undefined) {
    /* let x = x : init analyzed before x is declared */
    ast_func_t *m = mk_main();
    ast_expr_t *init = var("x");
    add(m, let_stmt("x", false, TYPE_UNKNOWN, init));
    run();
    cr_assert_eq(init->data_type, TYPE_UNKNOWN);
}

Test(sema, var_redeclaration_keeps_first) {
    ast_func_t *m = mk_main();
    add(m, let_stmt("x", false, TYPE_UNKNOWN, int_lit(1)));
    add(m, let_stmt("x", false, TYPE_UNKNOWN, dbl_lit(2.0)));
    ast_expr_t *use = probe(m, var("x"));
    run();
    cr_assert_eq(use->data_type, TYPE_INT);
}

Test(sema, var_ids_restart_per_func) {
    ast_func_t *f = mk_func("f", false, TYPE_UNKNOWN);
    ast_func_t *g = mk_func("g", false, TYPE_UNKNOWN);
    ast_stmt_t *fx = add(f, let_stmt("x", false, TYPE_UNKNOWN, int_lit(1)));
    ast_stmt_t *gx = add(g, let_stmt("x", false, TYPE_UNKNOWN, int_lit(1)));
    run();
    cr_assert_eq(fx->as.var_def.var_id, 0u);
    cr_assert_eq(gx->as.var_def.var_id, 0u);
}

/* ===================== assignment ===================== */

Test(sema, assign_mutable_ok) {
    ast_func_t *m = mk_main();
    add(m, let_stmt("a", false, TYPE_UNKNOWN, int_lit(0)));
    ast_stmt_t *b = add(m, let_stmt("b", true, TYPE_UNKNOWN, int_lit(1)));
    ast_stmt_t *as = add(m, assign_stmt("b", int_lit(2)));
    run();
    cr_assert_eq(as->as.assign.val->data_type, TYPE_INT);
    cr_assert_eq(as->as.assign.var_id, b->as.var_def.var_id);
    cr_assert_neq(as->as.assign.var_id, NO_ID);
}

Test(sema, assign_immutable_rejected) {
    ast_func_t *m = mk_main();
    add(m, let_stmt("b", false, TYPE_UNKNOWN, int_lit(1)));
    ast_stmt_t *as = add(m, assign_stmt("b", int_lit(2)));
    run();
    cr_assert_eq(as->as.assign.var_id, NO_ID);
}

Test(sema, assign_undefined_rejected) {
    ast_func_t *m = mk_main();
    ast_stmt_t *as = add(m, assign_stmt("x", int_lit(1)));
    run();
    cr_assert_eq(as->as.assign.var_id, NO_ID);
}

Test(sema, assign_type_mismatch_rejected) {
    ast_func_t *m = mk_main();
    add(m, let_stmt("b", true, TYPE_INT, int_lit(1)));
    ast_stmt_t *as = add(m, assign_stmt("b", str_lit("s")));
    run();
    cr_assert_eq(as->as.assign.var_id, NO_ID);
}

Test(sema, assign_to_function_name_rejected) {
    mk_func("f", false, TYPE_UNKNOWN);
    ast_func_t *m = mk_main();
    ast_stmt_t *as = add(m, assign_stmt("f", int_lit(1)));
    run();
    cr_assert_eq(as->as.assign.var_id, NO_ID);
}

/* ===================== unary ===================== */

Test(sema, unary_types) {
    ast_func_t *m = mk_main();
    ast_expr_t *neg_i = probe(m, un(OPU_NEG, int_lit(1)));
    ast_expr_t *neg_d = probe(m, un(OPU_NEG, dbl_lit(1.0)));
    ast_expr_t *plus_i = probe(m, un(OPU_PLUS, int_lit(1)));
    ast_expr_t *not_b = probe(m, un(OPU_NOT, bool_lit(true)));
    run();
    cr_assert_eq(neg_i->data_type, TYPE_INT);
    cr_assert_eq(neg_d->data_type, TYPE_DOUBLE);
    cr_assert_eq(plus_i->data_type, TYPE_INT);
    cr_assert_eq(not_b->data_type, TYPE_BOOL);
}

Test(sema, unary_bad_operand) {
    ast_func_t *m = mk_main();
    ast_expr_t *not_i = probe(m, un(OPU_NOT, int_lit(1)));
    ast_expr_t *neg_b = probe(m, un(OPU_NEG, bool_lit(true)));
    ast_expr_t *neg_s = probe(m, un(OPU_NEG, str_lit("s")));
    run();
    cr_assert_eq(not_i->data_type, TYPE_UNKNOWN);
    cr_assert_eq(neg_b->data_type, TYPE_UNKNOWN);
    cr_assert_eq(neg_s->data_type, TYPE_UNKNOWN);
}

Test(sema, unary_unknown_operand_no_cascade) {
    ast_func_t *m = mk_main();
    ast_expr_t *e = probe(m, un(OPU_NEG, var("nope")));
    run();
    cr_assert_eq(e->data_type, TYPE_UNKNOWN);
}

/* ===================== binary ===================== */

Test(sema, binary_arith) {
    ast_func_t *m = mk_main();
    op_binary ops[] = {OPB_ADD, OPB_SUB, OPB_MUL, OPB_DIV};
    size_t n = sizeof ops / sizeof ops[0];
    ast_expr_t *ei[4];
    ast_expr_t *ed[4];
    for (size_t i = 0; i < n; i++) {
        ei[i] = probe(m, bin(ops[i], int_lit(6), int_lit(3)));
        ed[i] = probe(m, bin(ops[i], dbl_lit(6.0), dbl_lit(3.0)));
    }
    run();
    for (size_t i = 0; i < n; i++) {
        cr_assert_eq(ei[i]->data_type, TYPE_INT);
        cr_assert_eq(ed[i]->data_type, TYPE_DOUBLE);
    }
}

Test(sema, binary_compare_yields_bool) {
    /* ctor copies left type (int); analyzer must overwrite with bool */
    ast_func_t *m = mk_main();
    op_binary ops[] = {OPB_LT, OPB_LTE, OPB_GT, OPB_GTE};
    size_t n = sizeof ops / sizeof ops[0];
    ast_expr_t *e[4];
    for (size_t i = 0; i < n; i++) {
        e[i] = probe(m, bin(ops[i], int_lit(1), int_lit(2)));
    }
    run();
    for (size_t i = 0; i < n; i++) {
        cr_assert_eq(e[i]->data_type, TYPE_BOOL);
    }
}

Test(sema, binary_equality_any_same_type) {
    ast_func_t *m = mk_main();
    ast_expr_t *eq_i = probe(m, bin(OPB_EQ, int_lit(1), int_lit(1)));
    ast_expr_t *ne_s = probe(m, bin(OPB_NE, str_lit("a"), str_lit("b")));
    ast_expr_t *eq_b = probe(m, bin(OPB_EQ, bool_lit(true), bool_lit(false)));
    run();
    cr_assert_eq(eq_i->data_type, TYPE_BOOL);
    cr_assert_eq(ne_s->data_type, TYPE_BOOL);
    cr_assert_eq(eq_b->data_type, TYPE_BOOL);
}

Test(sema, binary_logic) {
    ast_func_t *m = mk_main();
    ast_expr_t *and_b = probe(m, bin(OPB_AND, bool_lit(true), bool_lit(false)));
    ast_expr_t *or_b = probe(m, bin(OPB_OR, bool_lit(true), bool_lit(false)));
    ast_expr_t *and_i = probe(m, bin(OPB_AND, int_lit(1), int_lit(0)));
    run();
    cr_assert_eq(and_b->data_type, TYPE_BOOL);
    cr_assert_eq(or_b->data_type, TYPE_BOOL);
    cr_assert_eq(and_i->data_type, TYPE_UNKNOWN);
}

Test(sema, binary_mixed_types_rejected) {
    /* strict policy. If you add int->double promotion, change this test */
    ast_func_t *m = mk_main();
    ast_expr_t *add_e = probe(m, bin(OPB_ADD, int_lit(1), dbl_lit(2.0)));
    ast_expr_t *lt_e = probe(m, bin(OPB_LT, int_lit(1), dbl_lit(2.0)));
    run();
    cr_assert_eq(add_e->data_type, TYPE_UNKNOWN);
    cr_assert_eq(lt_e->data_type, TYPE_UNKNOWN);
}

Test(sema, binary_string_arith_policy) {
    /* policy: strings not numeric. Change if + means concat */
    ast_func_t *m = mk_main();
    ast_expr_t *e = probe(m, bin(OPB_ADD, str_lit("a"), str_lit("b")));
    run();
    cr_assert_eq(e->data_type, TYPE_UNKNOWN);
}

Test(sema, binary_unknown_operand_no_cascade) {
    ast_func_t *m = mk_main();
    ast_expr_t *inner = bin(OPB_ADD, var("nope"), int_lit(1));
    ast_expr_t *outer = probe(m, bin(OPB_MUL, inner, int_lit(2)));
    run();
    cr_assert_eq(inner->data_type, TYPE_UNKNOWN);
    cr_assert_eq(outer->data_type, TYPE_UNKNOWN);
}

Test(sema, binary_nested) {
    ast_func_t *m = mk_main();
    ast_expr_t *inner = bin(OPB_ADD, int_lit(1), int_lit(2));
    ast_expr_t *outer = probe(m, bin(OPB_LT, inner, int_lit(4)));
    run();
    cr_assert_eq(inner->data_type, TYPE_INT);
    cr_assert_eq(outer->data_type, TYPE_BOOL);
}

Test(sema, binary_with_vars) {
    ast_func_t *m = mk_main();
    add(m, let_stmt("a", false, TYPE_UNKNOWN, int_lit(1)));
    add(m, let_stmt("b", false, TYPE_UNKNOWN, int_lit(2)));
    ast_expr_t *e = probe(m, bin(OPB_ADD, var("a"), var("b")));
    run();
    cr_assert_eq(e->data_type, TYPE_INT);
}

/* ===================== if / while / scopes ===================== */

Test(sema, if_cond_analyzed) {
    ast_func_t *m = mk_main();
    ast_expr_t *cond = bin(OPB_LT, int_lit(1), int_lit(2));
    add(m, if_stmt(cond, nullptr, nullptr));
    run();
    cr_assert_eq(cond->data_type, TYPE_BOOL);
}

Test(sema, if_var_scoped_to_block) {
    ast_func_t *m = mk_main();
    add(m, if_stmt(bool_lit(true),
                   let_stmt("x", false, TYPE_UNKNOWN, int_lit(1)), nullptr));
    ast_expr_t *use = probe(m, var("x"));
    run();
    cr_assert_eq(use->data_type, TYPE_UNKNOWN);
    cr_assert_eq(use->as.var.var_id, NO_ID);
}

Test(sema, if_block_sees_outer_var) {
    ast_func_t *m = mk_main();
    add(m, let_stmt("a", false, TYPE_UNKNOWN, int_lit(1)));
    ast_stmt_t *b = add(m, let_stmt("b", false, TYPE_UNKNOWN, dbl_lit(2.0)));
    ast_expr_t *use = var("b");
    add(m, if_stmt(bool_lit(true), expr_stmt(use), nullptr));
    run();
    cr_assert_eq(use->data_type, TYPE_DOUBLE);
    cr_assert_eq(use->as.var.var_id, b->as.var_def.var_id);
}

Test(sema, else_block_analyzed) {
    ast_func_t *m = mk_main();
    add(m, let_stmt("a", false, TYPE_UNKNOWN, int_lit(1)));
    ast_expr_t *use = var("a");
    add(m, if_stmt(bool_lit(true), nullptr, expr_stmt(use)));
    run();
    cr_assert_eq(use->data_type, TYPE_INT);
}

Test(sema, then_and_else_scopes_independent) {
    ast_func_t *m = mk_main();
    ast_stmt_t *then_x = let_stmt("x", false, TYPE_UNKNOWN, int_lit(1));
    ast_stmt_t *else_x = let_stmt("x", false, TYPE_UNKNOWN, dbl_lit(1.0));
    add(m, if_stmt(bool_lit(true), then_x, else_x));
    run();
    cr_assert_eq(then_x->as.var_def.type, TYPE_INT);
    cr_assert_eq(else_x->as.var_def.type, TYPE_DOUBLE); /* no SYM_EXISTS */
}

Test(sema, if_non_bool_cond_still_analyzes_block) {
    ast_func_t *m = mk_main();
    add(m, let_stmt("a", false, TYPE_UNKNOWN, int_lit(1)));
    ast_expr_t *use = var("a");
    ast_expr_t *cond = int_lit(1);
    add(m, if_stmt(cond, expr_stmt(use), nullptr));
    run();
    cr_assert_eq(cond->data_type, TYPE_INT);
    cr_assert_eq(use->data_type, TYPE_INT);
}

Test(sema, while_cond_and_block_analyzed) {
    ast_func_t *m = mk_main();
    add(m, let_stmt("a", false, TYPE_UNKNOWN, int_lit(1)));
    ast_expr_t *cond = bool_lit(true);
    ast_expr_t *use = var("a");
    add(m, while_stmt(cond, expr_stmt(use)));
    run();
    cr_assert_eq(cond->data_type, TYPE_BOOL);
    cr_assert_eq(use->data_type, TYPE_INT);
}

Test(sema, while_var_scoped_to_block) {
    ast_func_t *m = mk_main();
    add(m, while_stmt(bool_lit(true),
                      let_stmt("x", false, TYPE_UNKNOWN, int_lit(1))));
    ast_expr_t *use = probe(m, var("x"));
    run();
    cr_assert_eq(use->data_type, TYPE_UNKNOWN);
}

Test(sema, scope_balanced_after_bad_cond) {
    /* bad cond must not leave scope stack unbalanced:
     * later let in same func must still get a sane id */
    ast_func_t *m = mk_main();
    add(m, if_stmt(int_lit(1), nullptr, nullptr));
    ast_stmt_t *d = add(m, let_stmt("x", false, TYPE_UNKNOWN, int_lit(1)));
    ast_expr_t *use = probe(m, var("x"));
    run();
    cr_assert_eq(d->as.var_def.var_id, 0u);
    cr_assert_eq(use->data_type, TYPE_INT);
}

/* ===================== functions: return types ===================== */

Test(sema, explicit_ret_type) {
    ast_func_t *f = mk_func("f", true, TYPE_INT);
    add(f, ret_stmt(int_lit(1)));
    run();
    cr_assert_eq(fsym("f")->type, TYPE_INT);
    cr_assert_eq(fsym("f")->ret_state, RET_EXPLICIT);
}

Test(sema, explicit_ret_mismatch_keeps_declared) {
    ast_func_t *f = mk_func("f", true, TYPE_INT);
    add(f, ret_stmt(str_lit("s")));
    run();
    cr_assert_eq(fsym("f")->type, TYPE_INT);
}

Test(sema, infer_ret_from_return) {
    ast_func_t *f = mk_func("f", false, TYPE_UNKNOWN);
    add(f, ret_stmt(int_lit(1)));
    run();
    cr_assert_eq(fsym("f")->type, TYPE_INT);
    cr_assert_eq(fsym("f")->ret_state, RET_INFERRED);
}

Test(sema, infer_ret_no_return_is_unit) {
    mk_func("f", false, TYPE_UNKNOWN);
    run();
    cr_assert_eq(fsym("f")->type, TYPE_UNIT);
    cr_assert_eq(fsym("f")->ret_state, RET_INFERRED);
}

Test(sema, infer_ret_bare_return_is_unit) {
    ast_func_t *f = mk_func("f", false, TYPE_UNKNOWN);
    add(f, ret_stmt(nullptr));
    run();
    cr_assert_eq(fsym("f")->type, TYPE_UNIT);
}

Test(sema, infer_ret_first_return_wins) {
    ast_func_t *f = mk_func("f", false, TYPE_UNKNOWN);
    add(f, ret_stmt(int_lit(1)));
    add(f, ret_stmt(str_lit("s")));
    run();
    cr_assert_eq(fsym("f")->type, TYPE_INT);
}

Test(sema, infer_ret_from_expression) {
    ast_func_t *f = mk_func("f", false, TYPE_UNKNOWN);
    add(f, let_stmt("x", false, TYPE_UNKNOWN, dbl_lit(1.0)));
    add(f, ret_stmt(bin(OPB_MUL, var("x"), dbl_lit(2.0))));
    run();
    cr_assert_eq(fsym("f")->type, TYPE_DOUBLE);
}

Test(sema, return_expr_analyzed) {
    ast_func_t *f = mk_func("f", true, TYPE_INT);
    ast_expr_t *e = int_lit(1);
    add(f, ret_stmt(e));
    run();
    cr_assert_eq(e->data_type, TYPE_INT);
}

/* ===================== functions: params ===================== */

Test(sema, params_visible_in_body) {
    ast_func_t *f = mk_func("f", true, TYPE_INT);
    add_param(f, "a", TYPE_INT);
    add_param(f, "b", TYPE_DOUBLE);
    ast_expr_t *ua = probe(f, var("a"));
    ast_expr_t *ub = probe(f, var("b"));
    run();
    cr_assert_eq(ua->data_type, TYPE_INT);
    cr_assert_eq(ub->data_type, TYPE_DOUBLE);
    cr_assert_neq(ua->as.var.var_id, ub->as.var.var_id);
}

Test(sema, params_not_visible_in_other_func) {
    ast_func_t *f = mk_func("f", false, TYPE_UNKNOWN);
    add_param(f, "a", TYPE_INT);
    ast_func_t *g = mk_func("g", false, TYPE_UNKNOWN);
    ast_expr_t *use = probe(g, var("a"));
    run();
    cr_assert_eq(use->data_type, TYPE_UNKNOWN);
}

Test(sema, locals_not_visible_in_other_func) {
    ast_func_t *f = mk_func("f", false, TYPE_UNKNOWN);
    add(f, let_stmt("x", false, TYPE_UNKNOWN, int_lit(1)));
    ast_func_t *g = mk_func("g", false, TYPE_UNKNOWN);
    ast_expr_t *use = probe(g, var("x"));
    run();
    cr_assert_eq(use->data_type, TYPE_UNKNOWN);
}

#if HAVE_VAR_COUNT
Test(sema, func_var_count_includes_params_and_locals) {
    ast_func_t *f = mk_func("f", false, TYPE_UNKNOWN);
    add_param(f, "a", TYPE_INT);
    add(f, let_stmt("b", false, TYPE_UNKNOWN, int_lit(1)));
    add(f, let_stmt("c", false, TYPE_UNKNOWN, int_lit(2)));
    run();
    cr_assert_eq(f->var_count, 3u);
}
#endif

/* ===================== calls ===================== */

Test(sema, call_explicit_func) {
    ast_func_t *f = mk_func("f", true, TYPE_INT);
    add(f, ret_stmt(int_lit(1)));
    ast_func_t *m = mk_main();
    ast_expr_t *c = probe(m, call("f", 0));
    run();
    cr_assert_eq(c->data_type, TYPE_INT);
    cr_assert_eq(c->as.func.target, f);
}

Test(sema, call_forward_inferred_func) {
    /* main analyzed before g; g's type must be inferred on demand */
    ast_func_t *m = mk_main();
    ast_expr_t *c = probe(m, call("g", 0));
    ast_func_t *g = mk_func("g", false, TYPE_UNKNOWN);
    add(g, ret_stmt(int_lit(1)));
    run();
    cr_assert_eq(c->data_type, TYPE_INT);
    cr_assert_eq(c->as.func.target, g);
    cr_assert_eq(fsym("g")->ret_state, RET_INFERRED);
}

Test(sema, call_backward_inferred_func) {
    ast_func_t *g = mk_func("g", false, TYPE_UNKNOWN);
    add(g, ret_stmt(dbl_lit(1.0)));
    ast_func_t *m = mk_main();
    ast_expr_t *c = probe(m, call("g", 0));
    run();
    cr_assert_eq(c->data_type, TYPE_DOUBLE);
}

Test(sema, call_chain_inferred) {
    /* main -> a -> b, a and b untyped, declared after main */
    ast_func_t *m = mk_main();
    ast_expr_t *c = probe(m, call("a", 0));
    ast_func_t *a = mk_func("a", false, TYPE_UNKNOWN);
    add(a, ret_stmt(call("b", 0)));
    ast_func_t *b = mk_func("b", false, TYPE_UNKNOWN);
    add(b, ret_stmt(str_lit("s")));
    run();
    cr_assert_eq(c->data_type, TYPE_STRING);
    cr_assert_eq(fsym("a")->type, TYPE_STRING);
    cr_assert_eq(fsym("b")->type, TYPE_STRING);
}

Test(sema, call_undefined_func) {
    ast_func_t *m = mk_main();
    ast_expr_t *c = probe(m, call("nope", 0));
    run();
    cr_assert_eq(c->data_type, TYPE_UNKNOWN);
}

Test(sema, call_args_analyzed) {
    ast_func_t *f = mk_func("f", true, TYPE_INT);
    add_param(f, "a", TYPE_INT);
    add(f, ret_stmt(var("a")));
    ast_func_t *m = mk_main();
    ast_expr_t *arg = int_lit(1);
    probe(m, call("f", 1, arg));
    run();
    cr_assert_eq(arg->data_type, TYPE_INT);
}

Test(sema, call_wrong_argc_still_typed) {
    /* arg errors must not cascade: result type still resolved */
    ast_func_t *f = mk_func("f", true, TYPE_INT);
    add_param(f, "a", TYPE_INT);
    add(f, ret_stmt(var("a")));
    ast_func_t *m = mk_main();
    ast_expr_t *few = probe(m, call("f", 0));
    ast_expr_t *many = probe(m, call("f", 2, int_lit(1), int_lit(2)));
    run();
    cr_assert_eq(few->data_type, TYPE_INT);
    cr_assert_eq(many->data_type, TYPE_INT);
}

Test(sema, call_wrong_arg_type_still_typed) {
    ast_func_t *f = mk_func("f", true, TYPE_INT);
    add_param(f, "a", TYPE_INT);
    add(f, ret_stmt(var("a")));
    ast_func_t *m = mk_main();
    ast_expr_t *c = probe(m, call("f", 1, str_lit("s")));
    run();
    cr_assert_eq(c->data_type, TYPE_INT);
}

Test(sema, call_as_operand) {
    ast_func_t *f = mk_func("f", true, TYPE_INT);
    add(f, ret_stmt(int_lit(1)));
    ast_func_t *m = mk_main();
    ast_expr_t *e = probe(m, bin(OPB_ADD, call("f", 0), int_lit(1)));
    run();
    cr_assert_eq(e->data_type, TYPE_INT);
}

Test(sema, call_result_in_let) {
    ast_func_t *f = mk_func("f", true, TYPE_DOUBLE);
    add(f, ret_stmt(dbl_lit(1.0)));
    ast_func_t *m = mk_main();
    ast_stmt_t *d = add(m, let_stmt("x", false, TYPE_UNKNOWN, call("f", 0)));
    run();
    cr_assert_eq(d->as.var_def.type, TYPE_DOUBLE);
}

Test(sema, ondemand_inference_keeps_caller_var_ids) {
    /* g analyzed in the middle of main's `let b = g()`.
     * main ids must continue a=0 b=1 d=2; g's own x starts at 0 */
    ast_func_t *m = mk_main();
    ast_func_t *g = mk_func("g", false, TYPE_UNKNOWN);
    ast_stmt_t *gx = add(g, let_stmt("x", false, TYPE_UNKNOWN, int_lit(1)));
    add(g, ret_stmt(var("x")));

    ast_stmt_t *a = add(m, let_stmt("a", false, TYPE_UNKNOWN, int_lit(1)));
    ast_stmt_t *b = add(m, let_stmt("b", false, TYPE_UNKNOWN, call("g", 0)));
    ast_stmt_t *d = add(m, let_stmt("d", false, TYPE_UNKNOWN, int_lit(2)));
    run();
    cr_assert_eq(gx->as.var_def.var_id, 0u);
    cr_assert_eq(a->as.var_def.var_id, 0u);
    cr_assert_eq(b->as.var_def.var_id, 1u);
    cr_assert_eq(d->as.var_def.var_id, 2u);
    cr_assert_eq(b->as.var_def.type, TYPE_INT);
}

Test(sema, ondemand_inference_keeps_caller_scope) {
    /* caller local must still resolve after callee analysis */
    ast_func_t *m = mk_main();
    ast_func_t *g = mk_func("g", false, TYPE_UNKNOWN);
    add(g, ret_stmt(int_lit(1)));
    add(m, let_stmt("a", false, TYPE_UNKNOWN, dbl_lit(1.0)));
    probe(m, call("g", 0));
    ast_expr_t *use = probe(m, var("a"));
    run();
    cr_assert_eq(use->data_type, TYPE_DOUBLE);
}

/* ===================== recursion ===================== */

Test(sema, recursion_explicit_ret) {
    ast_func_t *f = mk_func("f", true, TYPE_INT);
    add_param(f, "n", TYPE_INT);
    ast_expr_t *c = call("f", 1, var("n"));
    add(f, ret_stmt(c));
    run();
    cr_assert_eq(c->data_type, TYPE_INT);
}

Test(sema, recursion_unannotated_terminates) {
    /* type not yet known at the call -> unknown, but no hang/crash */
    ast_func_t *f = mk_func("f", false, TYPE_UNKNOWN);
    ast_expr_t *c = probe(f, call("f", 0));
    run();
    cr_assert_eq(c->data_type, TYPE_UNKNOWN);
    cr_assert_eq(fsym("f")->ret_state, RET_INFERRED);
}

Test(sema, recursion_after_return_ok) {
    /* first return fixes type, recursive call afterwards is typed */
    ast_func_t *f = mk_func("f", false, TYPE_UNKNOWN);
    add(f, ret_stmt(int_lit(1)));
    ast_expr_t *c = call("f", 0);
    add(f, ret_stmt(c));
    run();
    cr_assert_eq(c->data_type, TYPE_INT);
    cr_assert_eq(fsym("f")->type, TYPE_INT);
}

Test(sema, mutual_recursion_terminates) {
    ast_func_t *a = mk_func("a", false, TYPE_UNKNOWN);
    ast_func_t *b = mk_func("b", false, TYPE_UNKNOWN);
    probe(a, call("b", 0));
    probe(b, call("a", 0));
    run();
    cr_assert_eq(fsym("a")->ret_state, RET_INFERRED);
    cr_assert_eq(fsym("b")->ret_state, RET_INFERRED);
}

/* ===================== duplicates ===================== */

Test(sema, duplicate_function_keeps_first) {
    ast_func_t *first = mk_func("f", true, TYPE_INT);
    add(first, ret_stmt(int_lit(1)));
    ast_func_t *second = mk_func("f", true, TYPE_DOUBLE);
    add(second, ret_stmt(dbl_lit(1.0)));
    run();
    cr_assert_eq(fsym("f")->decl, first);
    cr_assert_eq(fsym("f")->type, TYPE_INT);
}

Test(sema, duplicate_function_calls_still_resolve) {
    mk_func("f", true, TYPE_INT);
    mk_func("f", true, TYPE_DOUBLE);
    ast_func_t *main = mk_main();
    ast_expr_t *cal = probe(main, call("f", 0));
    run();
    cr_assert_eq(cal->data_type, TYPE_INT);
}
