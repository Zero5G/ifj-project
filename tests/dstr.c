#include "dstr.h"
#include <criterion/criterion.h>
#include <stdbool.h>

#define LEN_5 5

// dstr_create
Test(dstr_create, create_valid) {
    char *text = "Hello";
    unsigned len = LEN_5;
    dstr dstring = dstr_create(text);

    cr_assert_not_null(dstring);
    cr_assert_not_null(dstr_cstr(dstring));
    cr_assert_eq(dstr_len(dstring), len);
    // cr_assert_geq(dstring->cap, dstr_len(dstring));
    cr_assert_str_eq(dstr_cstr(dstring), text);

    dstr_delete(dstring);
}

Test(dstr_create, create_empty) {
    char *text = "";
    dstr dstring = dstr_create(text);

    cr_assert_not_null(dstring);
    cr_assert_eq(dstr_len(dstring), 0);
    cr_assert_str_eq(dstr_cstr(dstring), "");

    dstr_delete(dstring);
}

// dstr_concat_raw
Test(dstr_concat_raw, concat_raw_valid) {
    dstr str = dstr_create("Hello");
    bool result = dstr_concat_raw(str, " World");

    cr_assert_eq(result, true);
    cr_assert_str_eq(dstr_cstr(str), "Hello World");
    cr_assert_eq(dstr_len(str), 11);

    dstr_delete(str);
}

// dstr_concat
Test(dstr_concat, concat_valid) {
    dstr str_a = dstr_create("Foo");
    dstr str_b = dstr_create("Bar");
    bool result = dstr_concat(str_a, str_b);

    cr_assert_eq(result, true);
    cr_assert_str_eq(dstr_cstr(str_a), "FooBar");
    cr_assert_eq(dstr_len(str_a), 6);

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
    cr_assert_str_eq(dstr_cstr(str), "AB");
    cr_assert_eq(dstr_len(str), 2);

    dstr_delete(str);
}

// dstr_delete_char
Test(dstr_delete_char, delete_char_valid) {
    dstr str = dstr_create("Word");
    dstr_delete_char(str, 2);

    cr_assert_str_eq(dstr_cstr(str), "Wod");
    cr_assert_eq(dstr_len(str), 3);

    dstr_delete(str);
}

// dstr_clear
Test(dstr_clear, clear_valid) {
    dstr str = dstr_create("Data");
    dstr_clear(str);

    cr_assert_eq(dstr_len(str), 0);
    cr_assert_str_eq(dstr_cstr(str), "");

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
    cr_assert_neq(dstr_cstr(original), dstr_cstr(copy));
    cr_assert_str_eq(dstr_cstr(copy), dstr_cstr(original));
    cr_assert_eq(dstr_len(copy), dstr_len(original));

    dstr_delete(original);
    dstr_delete(copy);
}

Test(dstr_dup, dup_empty) {
    dstr original = dstr_create("");
    dstr copy = dstr_dup(original);

    cr_assert_not_null(copy);
    cr_assert_str_eq(dstr_cstr(copy), "");
    cr_assert_eq(dstr_len(copy), 0);

    dstr_delete(original);
    dstr_delete(copy);
}
