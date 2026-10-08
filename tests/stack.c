#include "stack.h"
#include <criterion/criterion.h>
#include <criterion/internal/assert.h>
#include <criterion/logging.h>
#include <signal.h>
#include <stdlib.h>

#define MULTIPLE_COUNT 50

Test(stack, is_empty) {
    stack *stk = malloc(sizeof(struct stack_t));
    stack_init(stk);

    cr_assert_not_null(stk);
    cr_expect(stack_is_empty(stk) == true);

    free(stk);
}

Test(stack, not_empty) {
    stack *stk = malloc(sizeof(struct stack_t));
    stack_init(stk);

    token tkn = {};
    stack_push(stk, tkn);

    cr_assert_not_null(stk);
    cr_expect(stack_is_empty(stk) == false);

    free(stk);
}

Test(stack, is_empty_null, .signal = SIGSEGV) {
    stack_is_empty(nullptr);
}

Test(stack, init) {
    stack *stk = malloc(sizeof(struct stack_t));
    stack_init(stk);
    cr_expect_null(stk->top);
    free(stk);
}

Test(stack, no_init) {
    stack *stk = malloc(sizeof(struct stack_t));
    cr_expect_not_null(stk->top);
    free(stk);
}

Test(stack, push_once) {
    stack *stk = malloc(sizeof(struct stack_t));
    stack_init(stk);
    cr_assert_not_null(stk);

    token tkn = {};
    stack_push(stk, tkn);

    cr_expect_not_null(stk->top);

    free(stk);
}

Test(stack, push_multiple) {
    stack *stk = malloc(sizeof(struct stack_t));
    stack_init(stk);
    cr_assert_not_null(stk);

    int expected = MULTIPLE_COUNT;
    token tkn = {};

    for (int i = 0; i < expected; i++) {
        stack_push(stk, tkn);
    }

    cr_expect_not_null(stk->top);

    int count = 1;
    stack_item next = stk->top;
    while (next->next != nullptr) {
        count++;
        next = next->next;
    }
    cr_log_info("expected: %d, count: %d", expected, count);
    cr_expect_eq(expected, count);

    free(stk);
}

Test(stack, delete_empty) {
    stack *stk = malloc(sizeof(struct stack_t));
    stack_init(stk);
    cr_assert_not_null(stk);

    cr_expect(stack_delete(&stk) == true);
    cr_expect_null(stk);

    free(stk);
}

Test(stack, delete) {
    stack *stk = malloc(sizeof(struct stack_t));
    stack_init(stk);
    cr_assert_not_null(stk);

    token tkn = {};
    stack_push(stk, tkn);

    cr_expect(stack_delete(&stk) == false);
    cr_expect_not_null(stk);

    free(stk);
}

Test(stack, create) {
    cr_expect_not_null(stack_create());
}
