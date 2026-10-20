#include "dstr.h"
#include <criterion/criterion.h>
#include <stdbool.h>
#include <string.h>

// expand
Test(expand, expand_valid) {
    dstr str = dstr_create("Test");
    bool result = expand(str, 20);

    cr_assert_eq(result, true);
    cr_assert_geq(str->cap, 21);

    dstr_delete(str);
}

// dstr_create
Test(dstr_create, create_valid) {
    char *text = "Hello";
    unsigned len = 5;
    dstr dstring = dstr_create(text);

    cr_assert_not_null(dstring);
    cr_assert_not_null(dstring->raw);
    cr_assert_eq(dstring->len, len);
    cr_assert_geq(dstring->cap, dstring->len);
    cr_assert_str_eq(dstring->raw, text);

    dstr_delete(dstring);
}

Test(dstr_create, create_empty) {
    char *text = "";
    dstr dstring = dstr_create(text);

    cr_assert_not_null(dstring);
    cr_assert_eq(dstring->len, 0);
    cr_assert_str_eq(dstring->raw, "");

    dstr_delete(dstring);
}

// dstr_concat_raw
Test(dstr_concat_raw, concat_raw_valid) {
    dstr str = dstr_create("Hello");
    bool result = dstr_concat_raw(str, " World");

    cr_assert_eq(result, true);
    cr_assert_str_eq(str->raw, "Hello World");
    cr_assert_eq(str->len, 11);

    dstr_delete(str);
}

// dstr_concat
Test(dstr_concat, concat_valid) {
    dstr str_a = dstr_create("Foo");
    dstr str_b = dstr_create("Bar");
    bool result = dstr_concat(str_a, str_b);

    cr_assert_eq(result, true);
    cr_assert_str_eq(str_a->raw, "FooBar");
    cr_assert_eq(str_a->len, 6);

    dstr_delete(str_a);
    dstr_delete(str_b);
}

// dstr_cmp
Test(dstr_cmp, cmp_equal) {
    dstr str_a = dstr_create("Match");
    dstr str_b = dstr_create("Match");
    int result = dstr_cmp(str_a, str_b);

    cr_assert_eq(result, 0);

    dstr_delete(str_a);
    dstr_delete(str_b);
}

// dstr_cmp_raw
Test(dstr_cmp_raw, cmp_raw_equal) {
    dstr str_a = dstr_create("Match");
    int result = dstr_cmp_raw(str_a, "Match");

    cr_assert_eq(result, 0);

    dstr_delete(str_a);
}

// dstr_push_char
Test(dstr_push_char, push_char_valid) {
    dstr str = dstr_create("A");
    bool result = dstr_push_char(str, 'B');

    cr_assert_eq(result, true);
    cr_assert_str_eq(str->raw, "AB");
    cr_assert_eq(str->len, 2);

    dstr_delete(str);
}

// dstr_delete_char
Test(dstr_delete_char, delete_char_valid) {
    dstr str = dstr_create("Word");
    dstr_delete_char(str, 2);

    cr_assert_str_eq(str->raw, "Wod");
    cr_assert_eq(str->len, 3);

    dstr_delete(str);
}

// dstr_clear
Test(dstr_clear, clear_valid) {
    dstr str = dstr_create("Data");
    dstr_clear(str);

    cr_assert_eq(str->len, 0);
    cr_assert_str_eq(str->raw, "");

    dstr_delete(str);
}

// dstr_len
Test(dstr_len, len_valid) {
    dstr str = dstr_create("LengthTest");
    unsigned len = dstr_len(str);

    cr_assert_eq(len, 10);

    dstr_delete(str);
}

// dstr_dup
Test(dstr_dup, dup_valid) {
    dstr original = dstr_create("DuplicateMe");
    dstr copy = dstr_dup(original);

    cr_assert_not_null(copy);
    cr_assert_neq(original, copy);
    cr_assert_neq(original->raw, copy->raw);
    cr_assert_str_eq(copy->raw, original->raw);
    cr_assert_eq(copy->len, original->len);

    dstr_delete(original);
    dstr_delete(copy);
}

Test(dstr_dup, dup_empty) {
    dstr original = dstr_create("");
    dstr copy = dstr_dup(original);

    cr_assert_not_null(copy);
    cr_assert_str_eq(copy->raw, "");
    cr_assert_eq(copy->len, 0);

    dstr_delete(original);
    dstr_delete(copy);
}
