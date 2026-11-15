/**
 * @file parser.c
 *
 * @author Matěj Krňák
 * @brief Implementace modulu pro syntaktickou analýzu
 *
 * @details
 */

#include "parser.h"

bool parser_init(parser_t *p, lexer *lx) {
    p->lx = lx;
    p->current = get_next_token(lx);

    return true;
}

static void advance(parser_t *p) {
    token_free(&p->current);
    p->current = get_next_token(p->lx);
}

static bool accept(parser_t *p, token_type type) {
    if (p->current.type != type) {
        return false;
    }

    advance(p);
    return true;
}

static bool expect(parser_t *p, token_type type) {
    if (p->current.type != type) {
        return false;
    }

    advance(p);
    return true;
}

static bool expect_kw(parser_t *p, keyword kw) {
    if (!token_is_kw(&p->current, kw)) {
        return false;
    }

    advance(p);
    return true;
}
