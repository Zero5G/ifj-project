/**
 * @file expr.c
 *
 * @author Matěj Krňák
 * @brief Implementace modulu pro zpracování výrazů precedenční syntaktickou
 * analýzou
 *
 * @details
 */

#include "expr.h"

#define STACK_SIZE 256

typedef enum {
    PS_PLUS,
    PS_MINUS,
    PS_STAR,
    PS_SLASH,
    PS_LT,
    PS_GT,
    PS_LTE,
    PS_GTE,
    PS_EQ,
    PS_NEQ,
    PS_LPAREN,
    PS_RPAREN,
    PS_OPERAND,
    PS_DOLLAR
} prec_symbol_t;

// no unary minus
static const char precedence_table[14][14] = {
    // +
    {'>', '>', '<', '<', '>', '>', '>', '>', '>', '>', '<', '>', '<', '>'},
    // -
    {'>', '>', '<', '<', '>', '>', '>', '>', '>', '>', '<', '>', '<', '>'},
    // *
    {'>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<', '>', '<', '>'},
    // /
    {'>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<', '>', '<', '>'},
    // <
    {'<', '<', '<', '<', ' ', ' ', ' ', ' ', '>', '>', '<', '>', '<', '>'},
    // >
    {'<', '<', '<', '<', ' ', ' ', ' ', ' ', '>', '>', '<', '>', '<', '>'},
    // <=
    {'<', '<', '<', '<', ' ', ' ', ' ', ' ', '>', '>', '<', '>', '<', '>'},
    // >=
    {'<', '<', '<', '<', ' ', ' ', ' ', ' ', '>', '>', '<', '>', '<', '>'},
    // ==
    {'<', '<', '<', '<', '<', '<', '<', '<', ' ', ' ', '<', '>', '<', '>'},
    // !=
    {'<', '<', '<', '<', '<', '<', '<', '<', ' ', ' ', '<', '>', '<', '>'},
    // (
    {'<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '=', '<', ' '},
    // )
    {'>', '>', '>', '>', '>', '>', '>', '>', '>', '>', ' ', '>', ' ', '>'},
    // i
    {'>', '>', '>', '>', '>', '>', '>', '>', '>', '>', ' ', '>', ' ', '>'},
    // $
    {'<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', ' ', '<', ' '},
};

static char precedence(prec_symbol_t stack, prec_symbol_t input) {
    return precedence_table[stack][input];
}

static prec_symbol_t token_to_symbol(token_type type) {
    switch (type) {
    case TT_PLUS:
        return PS_PLUS;
    case TT_MINUS:
        return PS_MINUS;
    case TT_STAR:
        return PS_STAR;
    case TT_SLASH:
        return PS_SLASH;
    case TT_EQ:
        return PS_EQ;
    case TT_NEQ:
        return PS_NEQ;
    case TT_LT:
        return PS_LT;
    case TT_LTE:
        return PS_LTE;
    case TT_GT:
        return PS_GT;
    case TT_GTE:
        return PS_GTE;
    case TT_LPAREN:
        return PS_LPAREN;
    case TT_RPAREN:
        return PS_RPAREN;

    case TT_INT_LITERAL:
    case TT_DOUBLE_LITERAL:
    case TT_STRING_LITERAL:
    case TT_BOOLEAN_LITERAL:
    case TT_UNIT_LITERAL:
    case TT_IDENTIFIER:
        return PS_OPERAND;

    default:
        return PS_DOLLAR;
    }
}

static op_binary token_to_binary(token_type type) {
    switch (type) {
    case TT_PLUS:
        return OPB_ADD;
    case TT_MINUS:
        return OPB_SUB;
    case TT_STAR:
        return OPB_MUL;
    case TT_SLASH:
        return OPB_DIV;
    case TT_EQ:
        return OPB_EQ;
    case TT_NEQ:
        return OPB_NE;
    case TT_LT:
        return OPB_LT;
    case TT_LTE:
        return OPB_LTE;
    case TT_GT:
        return OPB_GT;
    case TT_GTE:
        return OPB_GTE;

    default:
        return OPB_ADD;
    }
}

typedef enum { ITEM_SYMBOL, ITEM_EXPR, ITEM_MARKER } item_kind_t;

typedef struct {
    item_kind_t kind;
    prec_symbol_t symbol;
    ast_expr_t *expr;
} stack_item_t;

typedef struct {
    stack_item_t items[STACK_SIZE];
    int top;
} expr_stack_t;

static void stack_init(expr_stack_t *s) { s->top = -1; }

static void stack_push(expr_stack_t *s, stack_item_t item) {
    s->items[++s->top] = item;
}

static prec_symbol_t top_terminal(expr_stack_t *s) {
    for (int i = s->top; i >= 0; i--) {
        if (s->items[i].kind == ITEM_SYMBOL) {
            return s->items[i].symbol;
        }
    }

    return PS_DOLLAR;
}

static ast_expr_t *make_binary(token_type op, ast_expr_t *left,
                               ast_expr_t *right) {
    return ast_expr_binary_create(left->line, token_to_binary(op), left, right);
}

static stack_item_t make_expr(ast_expr_t *expr) {
    stack_item_t item;

    item.kind = ITEM_EXPR;
    item.symbol = PS_OPERAND;
    item.expr = expr;

    return item;
}

static bool is_expr_end(parser_ctx *ctx) {
    if (ctx->current.type == TT_EOL) {
        return true;
    }

    if (token_is_kw(&ctx->current, KW_THEN)) {
        return true;
    }

    if (token_is_kw(&ctx->current, KW_DO)) {
        return true;
    }

    if (ctx->current.type == TT_COMMA) {
        return true;
    }

    if (ctx->current.type == TT_RPAREN) {
        return true;
    }

    return false;
}

static stack_item_t make_marker(void) {
    stack_item_t item;

    item.kind = ITEM_MARKER;
    item.expr = nullptr;

    return item;
}

static void insert_marker(expr_stack_t *stack) {
    stack_push(stack, make_marker());
}

static int find_marker(expr_stack_t *stack) {
    for (int i = stack->top; i >= 0; i--) {
        if (stack->items[i].kind == ITEM_MARKER) {
            return i;
        }
    }

    return -1;
}

static void shift_symbol(expr_stack_t *stack, prec_symbol_t symbol) {
    stack_push(stack, (stack_item_t){.kind = ITEM_SYMBOL, .symbol = symbol});
}

ast_expr_t *parse_term(parser_ctx *ctx) {
    expr_kind kind;

    switch (ctx->current.type) {
    case TT_INT_LITERAL:
        kind = E_INT_LIT;
        break;
    case TT_DOUBLE_LITERAL:
        kind = E_DOUBLE_LIT;
        break;
    case TT_STRING_LITERAL:
        kind = E_STRING_LIT;
        break;
    case TT_BOOLEAN_LITERAL:
        kind = E_BOOLEAN_LIT;
        break;
    case TT_IDENTIFIER:
        kind = E_VAR;
        break;
    case TT_UNIT_LITERAL:
        kind = E_UNIT_LIT;
        break;
    default:
        return nullptr;
    }

    ast_expr_t *expr = ast_expr_create((unsigned)ctx->current.line, kind);
    if (expr == nullptr) {
        return nullptr;
    }

    switch (kind) {
    case E_INT_LIT:
        expr->as.int_val = ctx->current.attr.as_int;
        break;
    case E_DOUBLE_LIT:
        expr->as.double_val = ctx->current.attr.as_double;
        break;
    case E_STRING_LIT:
        expr->as.str = ctx->current.attr.as_str;
        ctx->current.attr.as_str = nullptr;
        break;
    case E_BOOLEAN_LIT:
        expr->as.bool_val = ctx->current.attr.as_bool;
        break;
    case E_VAR:
        expr->as.var.name = ctx->current.attr.as_str;
        ctx->current.attr.as_str = nullptr;
        break;
    default:
        break;
    }

    advance(ctx);

    return expr;
}

static ast_expr_t *parse_operand(parser_ctx *ctx) {
    if (ctx->current.type == TT_IDENTIFIER) {
        const token *next = lexer_peek(ctx->lx, 1);
        if (next != nullptr && next->type == TT_LPAREN) {
            return parse_function_call(ctx);
        }
    }

    return parse_term(ctx);
}

static bool shift_operand(expr_stack_t *stack, parser_ctx *ctx) {
    ast_expr_t *expr = parse_operand(ctx);
    if (expr == nullptr) {
        return false;
    }

    stack_push(stack, make_expr(expr));
    return true;
}

static bool reduce(expr_stack_t *stack) {
    int marker = find_marker(stack);
    if (marker < 0) {
        return false;
    }

    int count = stack->top - marker;

    if (count == 1) {
        stack_item_t expr = stack->items[marker + 1];

        if (expr.kind != ITEM_EXPR) {
            return false;
        }

        stack->top = marker - 1;

        stack_push(stack, expr);

        return true;
    }

    if (count == 3) {
        stack_item_t left = stack->items[marker + 1];
        stack_item_t op = stack->items[marker + 2];
        stack_item_t right = stack->items[marker + 3];

        // (E) - needs testing, may be wrong
        if (left.kind == ITEM_SYMBOL && left.symbol == PS_LPAREN &&
            op.kind == ITEM_EXPR && right.kind == ITEM_SYMBOL &&
            right.symbol == PS_RPAREN) {
            stack->top = marker - 1;

            stack_push(stack, op);

            return true;
        }

        if (left.kind != ITEM_EXPR) {
            return false;
        }
        if (right.kind != ITEM_EXPR) {
            return false;
        }
        if (op.kind != ITEM_SYMBOL) {
            return false;
        }

        token_type tok;

        switch (op.symbol) {
        case PS_PLUS:
            tok = TT_PLUS;
            break;
        case PS_MINUS:
            tok = TT_MINUS;
            break;
        case PS_STAR:
            tok = TT_STAR;
            break;
        case PS_SLASH:
            tok = TT_SLASH;
            break;
        case PS_LT:
            tok = TT_LT;
            break;
        case PS_GT:
            tok = TT_GT;
            break;
        case PS_LTE:
            tok = TT_LTE;
            break;
        case PS_GTE:
            tok = TT_GTE;
            break;
        case PS_EQ:
            tok = TT_EQ;
            break;
        case PS_NEQ:
            tok = TT_NEQ;
            break;

        default:
            return false;
        }

        ast_expr_t *expr = make_binary(tok, left.expr, right.expr);

        stack->top = marker - 1;

        stack_push(stack, make_expr(expr));

        return true;
    }

    return false;
}

ast_expr_t *parse_expression(parser_ctx *ctx) {
    expr_stack_t stack;

    stack_init(&stack);

    shift_symbol(&stack, PS_DOLLAR);

    while (true) {
        prec_symbol_t input =
            is_expr_end(ctx) ? PS_DOLLAR : token_to_symbol(ctx->current.type);

        prec_symbol_t top = top_terminal(&stack);

        if (top == PS_DOLLAR && input == PS_DOLLAR) {
            break;
        }

        char rel = precedence(top, input);

        if (rel == '<') {
            insert_marker(&stack);

            if (input == PS_OPERAND) {
                if (!shift_operand(&stack, ctx)) {
                    return nullptr;
                }
            } else {
                shift_symbol(&stack, input);
                advance(ctx);
            }

            continue;
        }

        if (rel == '=') {
            shift_symbol(&stack, input);
            advance(ctx);

            continue;
        }

        if (rel == '>') {
            if (!reduce(&stack)) {
                return nullptr;
            }

            continue;
        }

        if (rel != '<' && rel != '>' && rel != '=') {
            return nullptr;
        }
    }

    if (stack.top != 1) {
        return nullptr;
    }

    if (stack.items[1].kind != ITEM_EXPR) {
        return nullptr;
    }

    return stack.items[1].expr;
}
