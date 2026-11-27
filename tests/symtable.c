#include <criterion/criterion.h>

#include "htab.h"
#include "symtable.h"

/* ---------- lifecycle (no fixture) ---------- */

TestSuite(symtab_life);

Test(symtab_life, create_destroy) {
    symtable *st = symtable_create();
    cr_assert_not_null(st);
    symtable_destroy(st);
}

Test(symtab_life, destroy_null_is_noop) { symtable_destroy(nullptr); }

Test(symtab_life, scope_null_stab) {
    cr_assert_not(symtable_enter_scope(nullptr));
    cr_assert_not(symtable_leave_scope(nullptr));
}

Test(symtab_life, destroy_with_open_scopes) {
    symtable *st = symtable_create();
    cr_assert_not_null(st);
    cr_assert(symtable_enter_scope(st));
    cr_assert(symtable_enter_scope(st));
    symtable_destroy(st); /* must free open scopes, no leak/crash */
}

/* ---------- fixture: table with one open (global) scope ---------- */

static symtable *st;

static void setup(void) {
    st = symtable_create();
    cr_assert_not_null(st);
    cr_assert(symtable_enter_scope(st));
}

static void teardown(void) {
    symtable_destroy(st);
    st = nullptr;
}

TestSuite(symtab, .init = setup, .fini = teardown);

static symbol *add_var_ok(const char *name) {
    symbol *sym = nullptr;
    cr_assert_eq(symtable_add_var(st, name, &sym), SYM_OK);
    cr_assert_not_null(sym);
    return sym;
}

static symbol *add_func_ok(const char *name) {
    symbol *sym = nullptr;
    cr_assert_eq(symtable_add_func(st, name, &sym), SYM_OK);
    cr_assert_not_null(sym);
    return sym;
}

/* ---------- variables ---------- */

Test(symtab, add_var_basic) {
    symbol *x = add_var_ok("x");
    cr_assert_eq(x->kind, SYM_VAR);
    cr_assert_eq(x->type, TYPE_UNKNOWN);
}

Test(symtab, add_var_duplicate) {
    symbol *first = add_var_ok("x");
    symbol *again = nullptr;
    cr_assert_eq(symtable_add_var(st, "x", &again), SYM_EXISTS);
    cr_assert_eq(again, first);
}

Test(symtab, add_var_distinct_names) {
    symbol *x = add_var_ok("x");
    symbol *y = add_var_ok("y");
    cr_assert_neq(x, y);
}

Test(symtab, var_ids_unique_in_func) {
    cr_assert(symtable_enter_func(st));
    symbol *a = add_var_ok("a");
    symbol *b = add_var_ok("b");
    symbol *c = add_var_ok("c");
    cr_assert_neq(a->var_id, b->var_id);
    cr_assert_neq(b->var_id, c->var_id);
    cr_assert_neq(a->var_id, c->var_id);
    symtable_leave_func(st);
}

Test(symtab, find_var_found) {
    symbol *x = add_var_ok("x");
    cr_assert_eq(symtable_find_var(st, "x"), x);
}

Test(symtab, find_var_missing) {
    add_var_ok("x");
    cr_assert_null(symtable_find_var(st, "nope"));
}

/* ---------- scopes ---------- */

Test(symtab, inner_scope_sees_outer_var) {
    symbol *x = add_var_ok("x");
    cr_assert(symtable_enter_scope(st));
    cr_assert_eq(symtable_find_var(st, "x"), x);
    cr_assert(symtable_leave_scope(st));
}

Test(symtab, inner_var_gone_after_leave) {
    cr_assert(symtable_enter_scope(st));
    add_var_ok("inner");
    cr_assert_not_null(symtable_find_var(st, "inner"));
    cr_assert(symtable_leave_scope(st));
    cr_assert_null(symtable_find_var(st, "inner"));
}

Test(symtab, nested_scopes_three_deep) {
    symbol *a = add_var_ok("a");
    cr_assert(symtable_enter_scope(st));
    symbol *b = add_var_ok("b");
    cr_assert(symtable_enter_scope(st));
    symbol *c = add_var_ok("c");

    cr_assert_eq(symtable_find_var(st, "a"), a);
    cr_assert_eq(symtable_find_var(st, "b"), b);
    cr_assert_eq(symtable_find_var(st, "c"), c);

    cr_assert(symtable_leave_scope(st));
    cr_assert_null(symtable_find_var(st, "c"));
    cr_assert_not_null(symtable_find_var(st, "b"));

    cr_assert(symtable_leave_scope(st));
    cr_assert_null(symtable_find_var(st, "b"));
    cr_assert_not_null(symtable_find_var(st, "a"));
}

/* ---------- function scope ---------- */

Test(symtab, func_scope_hides_outer_vars) {
    add_var_ok("global_var");
    cr_assert(symtable_enter_func(st));
    cr_assert_null(symtable_find_var(st, "global_var"));
    symtable_leave_func(st);
    cr_assert_not_null(symtable_find_var(st, "global_var"));
}

Test(symtab, func_inner_block_sees_func_vars) {
    cr_assert(symtable_enter_func(st));
    symbol *p = add_var_ok("param");
    cr_assert(symtable_enter_scope(st));
    cr_assert_eq(symtable_find_var(st, "param"), p);
    cr_assert(symtable_leave_scope(st));
    symtable_leave_func(st);
}

Test(symtab, leave_func_returns_var_count) {
    cr_assert(symtable_enter_func(st));
    add_var_ok("a");
    add_var_ok("b");
    add_var_ok("c");
    cr_assert_eq(symtable_leave_func(st), 3u);
}

Test(symtab, leave_func_zero_vars) {
    cr_assert(symtable_enter_func(st));
    cr_assert_eq(symtable_leave_func(st), 0u);
}

Test(symtab, var_count_resets_per_func) {
    add_var_ok("g"); /* outside any func, must not leak into counts */

    cr_assert(symtable_enter_func(st));
    add_var_ok("a");
    add_var_ok("b");
    cr_assert_eq(symtable_leave_func(st), 2u);

    cr_assert(symtable_enter_func(st));
    add_var_ok("c");
    cr_assert_eq(symtable_leave_func(st), 1u);
}

Test(symtab, var_ids_restart_per_func) {
    cr_assert(symtable_enter_func(st));
    symbol *a = add_var_ok("a");
    unsigned first_id = a->var_id;
    symtable_leave_func(st);

    cr_assert(symtable_enter_func(st));
    symbol *b = add_var_ok("a");
    cr_assert_eq(b->var_id, first_id);
    symtable_leave_func(st);
}

Test(symtab, same_var_name_in_different_funcs) {
    cr_assert(symtable_enter_func(st));
    add_var_ok("x");
    symtable_leave_func(st);

    cr_assert(symtable_enter_func(st));
    add_var_ok("x"); /* new func, no SYM_EXISTS */
    symtable_leave_func(st);
}

/* ---------- functions ---------- */

Test(symtab, add_func_basic) {
    symbol *f = add_func_ok("foo");
    cr_assert_eq(f->kind, SYM_FUNC);
}

Test(symtab, add_func_duplicate) {
    symbol *first = add_func_ok("foo");
    symbol *again = nullptr;
    cr_assert_eq(symtable_add_func(st, "foo", &again), SYM_EXISTS);
    cr_assert_eq(again, first);
}

Test(symtab, find_func_found) {
    symbol *f = add_func_ok("foo");
    cr_assert_eq(symtable_find_func(st, "foo"), f);
}

Test(symtab, find_func_missing) {
    add_func_ok("foo");
    cr_assert_null(symtable_find_func(st, "bar"));
}

Test(symtab, find_func_empty_table) {
    cr_assert_null(symtable_find_func(st, "foo"));
}

Test(symtab, func_visible_from_inside_func_scope) {
    symbol *f = add_func_ok("foo");
    cr_assert(symtable_enter_func(st));
    cr_assert_eq(symtable_find_func(st, "foo"), f);
    symtable_leave_func(st);
}

Test(symtab, var_and_func_namespaces_separate) {
    symbol *v = add_var_ok("x");
    symbol *f = add_func_ok("x");
    cr_assert_neq(v, f);
    cr_assert_eq(symtable_find_var(st, "x"), v);
    cr_assert_eq(symtable_find_func(st, "x"), f);
}

Test(symtab, find_var_does_not_return_func) {
    add_func_ok("foo");
    cr_assert_null(symtable_find_var(st, "foo"));
}
