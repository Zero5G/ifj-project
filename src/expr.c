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

typedef enum {
    PS_PLUS,
    PS_MINUS,
    PS_STAR,
    PS_SLASH,
    PS_EQ,
    PS_NEQ,
    PS_LT,
    PS_LTE,
    PS_GT,
    PS_GTE,
    PS_LPAREN,
    PS_RPAREN,
    PS_OPERAND,
    PS_DOLLAR
} prec_symbol_t;

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
