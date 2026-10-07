/**
 * @file stack.h
 *
 * @author Jakub Manoušek
 * @brief Modul pro datovou strukturu ADT TStack
 *
 * @details
 */

union stack_data_t {
    int as_int;
    double as_double;
};

typedef union stack_data_t stack_data, *stack_data_p;

struct stack_item_t {
    stack_data_p data;
    struct stack_item_t *next;
};

struct stack_t {
    struct stack_item_t *top;
};

typedef struct stack_t stack;

stack *stack_create();

stack *stack_delete(stack *stack);

void stack_empty(stack *stack);

bool stack_is_empty(stack *stack);

void stack_init(stack *stack);

void stack_push(stack *stack, stack_data data);

stack_data stack_pop(stack *stack);

stack_data stack_top(stack *stack);
