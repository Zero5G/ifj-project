/**
 * @file stack.h
 *
 * @author Jakub Manoušek
 * @brief Modul pro datovou strukturu ADT TStack určenou pro tokeny
 *
 * @details
 */

#include "lexer.h"

struct stack_item_t {
    token *data;
    struct stack_item_t *next;
};

typedef struct stack_item_t *stack_item;

struct stack_t {
    stack_item top;
};

typedef struct stack_t stack;

stack *stack_create();

bool stack_delete(stack *stk);

bool stack_is_empty(stack *stk);

void stack_init(stack *stk);

void stack_push(stack *stk, token data);

token *stack_pop(stack *stk);

token* stack_top(stack *stk);
