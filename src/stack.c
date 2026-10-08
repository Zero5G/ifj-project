/**
 * @file stack.c
 *
 * @author Jakub Manoušek
 * @brief Implementace modul pro datovou strukturu ADT TStack určenou pro tokeny
 *
 * @details
 */

#include "stack.h"
#include <stdlib.h>

stack *stack_create() {
    stack *stk = malloc(sizeof(struct stack_t));
    if (!stk) {
        return nullptr;
    }
    return stk;
}

bool stack_delete(stack *stk) {
    if (!stack_is_empty(stk)) {
        return false;
    }

    free(stk);
    stk = nullptr;

    return true;
}

bool stack_is_empty(stack *stk) {
    return stk->top == nullptr;
}

void stack_init(stack *stk) {
    stk->top = nullptr;
}

bool stack_push(stack *stk, token data) {
    if (!stk) {
        return false;
    }

    stack_item item = malloc(sizeof(struct stack_item_t));
    if (!item) {
        return false;
    }

    token *ndata = malloc(sizeof(token));
    if (!ndata) {
        return false;
    }
    *ndata = data;

    item->next = stk->top;
    item->data = ndata;
    stk->top = item;

    return true;
}

token *stack_pop(stack *stk) {
    token *data = stack_top(stk);
    if (!data) {
        return nullptr;
    }

    stack_item ntop = stk->top->next;
    free(stk->top);
    stk->top = ntop;

    return data;
}

token* stack_top(stack *stk) {
    if (!stack_is_empty(stk)) {
        return nullptr;
    }
    return stk->top->data;
}
