#include "dstr.h"
#include <criterion/criterion.h>
#include <stdbool.h>
#include <string.h>

// expand
Test(expand, expand_valid) {
    dstr str = dstr_create("Test");
    bool result = expand(str, 20);

    cr_assert_eq(result, true,
                 "Error: expand returned false! Expected: true (1), Got: %d",
                 result);
    cr_assert_geq(str->cap, 21,
                  "Error: Capacity was not expanded properly! Expected at "
                  "least: 21, Got: %u",
                  str->cap);

    dstr_delete(str);
}

// dstr_create
Test(dstr_create, create_valid) {
    char *text = "Hello";
    unsigned len = 5;
    dstr dstring = dstr_create(text);

    cr_assert_not_null(dstring, "Error: dstr_create returned NULL!");
    cr_assert_not_null(dstring->raw, "Error: Internal raw pointer is NULL!");
    cr_assert_eq(dstring->len, len,
                 "Error: Length mismatch! Expected: %u, Got: %u", len,
                 dstring->len);
    cr_assert_geq(
        dstring->cap, dstring->len,
        "Error: Capacity (cap) is smaller than length (len)! Cap: %u, Len: %u",
        dstring->cap, dstring->len);
    cr_assert_str_eq(
        dstring->raw, text,
        "Error: Content in raw does not match! Expected: '%s', Got: '%s'", text,
        dstring->raw);

    dstr_delete(dstring);
}

Test(dstr_create, create_empty) {
    char *text = "";
    dstr dstring = dstr_create(text);

    cr_assert_not_null(dstring,
                       "Error: dstr_create returned NULL for empty string!");
    cr_assert_eq(dstring->len, 0,
                 "Error: For empty string, len must be 0! Got: %u",
                 dstring->len);
    cr_assert_str_eq(dstring->raw, "",
                     "Error: Internal raw string should be empty! Got: '%s'",
                     dstring->raw);

    dstr_delete(dstring);
}

// dstr_concat_raw
Test(dstr_concat_raw, concat_raw_valid) {
    dstr str = dstr_create("Hello");
    bool result = dstr_concat_raw(str, " World");

    cr_assert_eq(
        result, true,
        "Error: dstr_concat_raw returned false! Expected: true (1), Got: %d",
        result);
    cr_assert_str_eq(str->raw, "Hello World",
                     "Error: String content is incorrect after concat! "
                     "Expected: 'Hello World', Got: '%s'",
                     str->raw);
    cr_assert_eq(
        str->len, 11,
        "Error: Length is incorrect after concat! Expected: 11, Got: %u",
        str->len);

    dstr_delete(str);
}

// dstr_concat
Test(dstr_concat, concat_valid) {
    dstr str_a = dstr_create("Foo");
    dstr str_b = dstr_create("Bar");
    bool result = dstr_concat(str_a, str_b);

    cr_assert_eq(
        result, true,
        "Error: dstr_concat returned false! Expected: true (1), Got: %d",
        result);
    cr_assert_str_eq(str_a->raw, "FooBar",
                     "Error: String content is incorrect after concat! "
                     "Expected: 'FooBar', Got: '%s'",
                     str_a->raw);
    cr_assert_eq(
        str_a->len, 6,
        "Error: Length is incorrect after concat! Expected: 6, Got: %u",
        str_a->len);

    dstr_delete(str_a);
    dstr_delete(str_b);
}

// dstr_cmp
Test(dstr_cmp, cmp_equal) {
    dstr str_a = dstr_create("Match");
    dstr str_b = dstr_create("Match");
    int result = dstr_cmp(str_a, str_b);
    cr_assert_eq(result, 0,
                 "Error: dstr_cmp should return 0 for equal strings! Expected: "
                 "0, Got: %d",
                 result);

    dstr_delete(str_a);
    dstr_delete(str_b);
}

// dstr_cmp_raw
Test(dstr_cmp_raw, cmp_raw_equal) {
    dstr str_a = dstr_create("Match");
    int result = dstr_cmp_raw(str_a, "Match");
    cr_assert_eq(result, 0,
                 "Error: dstr_cmp_raw should return 0 for equal strings! "
                 "Expected: 0, Got: %d",
                 result);

    dstr_delete(str_a);
}

// dstr_push_char
Test(dstr_push_char, push_char_valid) {
    dstr str = dstr_create("A");
    bool result = dstr_push_char(str, 'B');

    cr_assert_eq(
        result, true,
        "Error: dstr_push_char returned false! Expected: true (1), Got: %d",
        result);
    cr_assert_str_eq(str->raw, "AB",
                     "Error: Character was not appended correctly! Expected: "
                     "'AB', Got: '%s'",
                     str->raw);
    cr_assert_eq(
        str->len, 2,
        "Error: Length did not increase properly! Expected: 2, Got: %u",
        str->len);
    dstr_delete(str);
}

// dstr_delete_char
Test(dstr_delete_char, delete_char_valid) {
    dstr str = dstr_create("Word");
    dstr_delete_char(str, 2);

    cr_assert_str_eq(str->raw, "Wod",
                     "Error: Character was not deleted correctly! Expected: "
                     "'Wod', Got: '%s'",
                     str->raw);
    cr_assert_eq(
        str->len, 3,
        "Error: Length did not decrease properly! Expected: 3, Got: %u",
        str->len);

    dstr_delete(str);
}

// dstr_clear
Test(dstr_clear, clear_valid) {
    dstr str = dstr_create("Data");
    dstr_clear(str);

    cr_assert_eq(str->len, 0,
                 "Error: Length should be 0 after clear! Expected: 0, Got: %u",
                 str->len);
    cr_assert_str_eq(
        str->raw, "",
        "Error: String should be empty after clear! Expected: '', Got: '%s'",
        str->raw);

    dstr_delete(str);
}

// dstr_len
Test(dstr_len, len_valid) {
    dstr str = dstr_create("LengthTest");
    unsigned len = dstr_len(str);

    cr_assert_eq(
        len, 10,
        "Error: dstr_len returned incorrect length! Expected: 10, Got: %u",
        len);

    dstr_delete(str);
}

// dstr_dup
Test(dstr_dup, dup_valid) {
    dstr original = dstr_create("DuplicateMe");
    dstr copy = dstr_dup(original);

    cr_assert_not_null(copy, "Error: dstr_dup returned NULL!");
    cr_assert_neq(
        original, copy,
        "Error: The duplicated dstr pointer is the same as the original!");
    cr_assert_neq(
        original->raw, copy->raw,
        "Error: The raw string pointer is the same! Deep copy is required.");
    cr_assert_str_eq(
        copy->raw, original->raw,
        "Error: String content incorrect! Expected: '%s', Got: '%s'",
        original->raw, copy->raw);
    cr_assert_eq(copy->len, original->len,
                 "Error: Length is incorrect! Expected: %u, Got: %u",
                 original->len, copy->len);

    dstr_delete(original);
    dstr_delete(copy);
}

Test(dstr_dup, dup_empty) {
    dstr original = dstr_create("");
    dstr copy = dstr_dup(original);

    cr_assert_not_null(copy, "Error: dstr_dup returned NULL for empty string!");
    cr_assert_str_eq(
        copy->raw, "",
        "Error: String content incorrect! Expected empty string '', Got: '%s'",
        copy->raw);
    cr_assert_eq(copy->len, 0,
                 "Error: Length is incorrect! Expected: 0, Got: %u", copy->len);

    dstr_delete(original);
    dstr_delete(copy);
}
