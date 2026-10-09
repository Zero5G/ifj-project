#include "stack.h"
#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <signal.h>
#include <stdlib.h>

#define MULTIPLE_COUNT 50
#define NUM 11
#define EXPECTED_ACTUAL "expected: %d; actual: %d"

Test(stack, is_empty) {
    stack *stk = malloc(sizeof(struct stack_t));
    stk->top = nullptr;

    cr_assert_not_null(stk);
    cr_expect(stack_is_empty(stk) == true);

    free(stk);
}

Test(stack, not_empty) {
    stack *stk = malloc(sizeof(struct stack_t));
    stk->top = nullptr;

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
    stk->top = nullptr;
    cr_assert_not_null(stk);

    token tkn = {};
    stack_push(stk, tkn);

    cr_expect_not_null(stk->top);

    free(stk->top->data);
    free(stk->top);
    free(stk);
}

Test(stack, push_multiple) {
    stack *stk = malloc(sizeof(struct stack_t));
    stk->top = nullptr;
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
        stack_item tmp = next;
        next = next->next;
        count++;
        free(tmp->data);
        free(tmp);
    }

    cr_log_info(EXPECTED_ACTUAL, expected, count);
    cr_expect_eq(expected, count);

    free(stk);
}

Test(stack, pop_once) {
    stack *stk = malloc(sizeof(struct stack_t));
    cr_assert_not_null(stk);
    stk->top = nullptr;

    token tkn = {
        .attr.as_int = NUM,
    };
    stack_push(stk, tkn);
    token *pop = stack_pop(stk);

    cr_log_info(EXPECTED_ACTUAL, NUM, pop->attr.as_int);
    cr_expect_eq(NUM, pop->attr.as_int);

    cr_expect_null(stk->top);

    free(stk);
}

Test(stack, pop_multiple) {
    stack *stk = malloc(sizeof(struct stack_t));
    cr_assert_not_null(stk);
    stk->top = nullptr;

    int expected = MULTIPLE_COUNT;
    token tkn = {
        .attr.as_int = NUM,
    };

    for (int i = 0; i < expected; i++) {
        cr_assert(stack_push(stk, tkn) == true);
    }

    cr_expect_not_null(stk->top);

    int count = 0;
    while (stk->top != nullptr) {
        token *data = stack_pop(stk);
        cr_assert_not_null(data);

        if (data->attr.as_int == NUM){
            count++;
        }
        free(data);
    }
    cr_log_info(EXPECTED_ACTUAL, expected - 1, count);
    cr_expect_eq(expected, count);

    free(stk);
}

Test(stack, top) {
    stack *stk = malloc(sizeof(struct stack_t));
    cr_assert_not_null(stk);
    stk->top = nullptr;

    token tkn = {
        .attr.as_int = NUM,
    };
    stack_push(stk, tkn);
    token *top = stack_top(stk);

    cr_expect_not_null(stk->top);

    cr_log_info(EXPECTED_ACTUAL, NUM, top->attr.as_int);
    cr_expect_eq(NUM, top->attr.as_int);

    free(stk->top->data);
    free(stk->top);
    free(stk);
}

Test(stack, delete_empty) {
    stack *stk = malloc(sizeof(struct stack_t));
    cr_assert_not_null(stk);
    stk->top = nullptr;

    cr_expect(stack_delete(&stk) == true);
    cr_expect_null(stk);

    free(stk);
}

Test(stack, delete) {
    stack *stk = malloc(sizeof(struct stack_t));
    cr_assert_not_null(stk);
    stk->top = nullptr;

    token tkn = {};
    stack_push(stk, tkn);

    cr_expect(stack_delete(&stk) == false);
    cr_expect_not_null(stk);

    free(stk);
}

Test(stack, create) {
    cr_expect_not_null(stack_create());
}
