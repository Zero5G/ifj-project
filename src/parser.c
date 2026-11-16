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

static bool accept_kw(parser_t *p, keyword kw) {
    if (!token_is_kw(&p->current, kw)) {
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

// First(<statement>) = {KW_VAL, KW_VAR, TT_IDENTIFIER, KW_RETURN, KW_IF,
// KW_WHILE}
static bool is_statement(parser_t *p) {
    if (token_is_kw(&p->current, KW_VAL)) {
        return true;
    }

    if (token_is_kw(&p->current, KW_VAR)) {
        return true;
    }

    if (p->current.type == TT_IDENTIFIER) {
        return true;
    }

    if (token_is_kw(&p->current, KW_RETURN)) {
        return true;
    }

    if (token_is_kw(&p->current, KW_IF)) {
        return true;
    }

    if (token_is_kw(&p->current, KW_WHILE)) {
        return true;
    }

    return false;
}

bool parser_run(parser_t *p) { return parse_prog(p); }

// <prog> -> <prolog> <function_list>
static bool parse_prog(parser_t *p) {
    return parse_prolog(p) && parse_function_list(p) && expect(p, TT_EOF);
}

// <prolog> -> KW_IMPORT ifj26 TT_DOT TT_STAR
static bool parse_prolog(parser_t *p) {
    if (!expect_kw(p, KW_IMPORT)) {
        return false;
    }

    if (!expect(p, TT_IDENTIFIER)) {
        return false;
    }

    if (!expect(p, TT_DOT)) {
        return false;
    }

    if (!expect(p, TT_STAR)) {
        return false;
    }

    return true;
}

// <function_list> -> <function> <function_list>
// <function_list> -> ε
static bool parse_function_list(parser_t *p) {
    while (token_is_kw(&p->current, KW_DEF)) {
        if (!parse_function(p)) {
            return false;
        }
    }

    return true;
}

// <function> -> KW_DEF TT_IDENTIFIER TT_LPAREN <parameters> TT_RPAREN
// <return_type> TT_ASSIGN <block>
static bool parse_function(parser_t *p) {
    if (!expect_kw(p, KW_DEF)) {
        return false;
    }

    if (!expect(p, TT_IDENTIFIER)) {
        return false;
    }

    if (!expect(p, TT_LPAREN)) {
        return false;
    }

    if (!parse_parameters(p)) {
        return false;
    }

    if (!expect(p, TT_RPAREN)) {
        return false;
    }

    if (!parse_return_type(p)) {
        return false;
    }

    if (!expect(p, TT_ASSIGN)) {
        return false;
    }

    if (!parse_block(p)) {
        return false;
    }

    return true;
}

// <parameters> -> <parameter> <parameter_next>
// <parameters> -> ε
static bool parse_parameters(parser_t *p) {
    if (p->current.type == TT_IDENTIFIER) {
        if (!parse_parameter(p)) {
            return false;
        }

        if (!parse_parameter_next(p)) {
            return false;
        }
    }

    return true;
}

// <parameter> -> TT_IDENTIFIER TT_COLON <type>
static bool parse_parameter(parser_t *p) {
    if (!expect(p, TT_IDENTIFIER)) {
        return false;
    }

    if (!expect(p, TT_COLON)) {
        return false;
    }

    if (!parse_type(p)) {
        return false;
    }

    return true;
}

// <parameter_next> -> TT_COMMA TT_IDENTIFIER TT_COLON <type> <parameter_next>
// <parameter_next> -> ε
static bool parse_parameter_next(parser_t *p) {
    if (accept(p, TT_COMMA)) {
        if (!expect(p, TT_IDENTIFIER)) {
            return false;
        }

        if (!expect(p, TT_COLON)) {
            return false;
        }

        if (!parse_type(p)) {
            return false;
        }

        if (!parse_parameter_next(p)) {
            return false;
        }
    }

    return true;
}

// <return_type> -> TT_COLON <type>
// <return_type> -> ε
static bool parse_return_type(parser_t *p) {
    if (accept(p, TT_COLON)) {
        if (!parse_type(p)) {
            return false;
        }
    }

    return true;
}

// <type> -> KW_INT
// <type> -> KW_DOUBLE
// <type> -> KW_STRING
// <type> -> KW_BOOLEAN
// <type> -> KW_UNIT
// <type> -> KW_LIST
static bool parse_type(parser_t *p) {
    if (accept_kw(p, KW_INT)) {
        return true;
    }

    if (accept_kw(p, KW_DOUBLE)) {
        return true;
    }

    if (accept_kw(p, KW_STRING)) {
        return true;
    }

    if (accept_kw(p, KW_BOOLEAN)) {
        return true;
    }

    if (accept_kw(p, KW_UNIT)) {
        return true;
    }

    if (accept_kw(p, KW_LIST)) {
        return true;
    }

    return false;
}

// <block> -> <statement>
// <block> -> TT_EOL TT_INDENT <body> TT_OUTDENT
static bool parse_block(parser_t *p) {
    if (accept(p, TT_EOL)) {
        if (!expect(p, TT_INDENT)) {
            return false;
        }

        if (!parse_body(p)) {
            return false;
        }

        if (!expect(p, TT_DEDENT)) {
            return false;
        }
    }

    return parse_statement(p);
}

// <body> -> <statement_list>
static bool parse_body(parser_t *p) {
    if (!parse_statement_list(p)) {
        return false;
    }
}

// <statement_list> -> <statement> <statement_list>
// <statement_list> -> ε
static bool parse_statement_list(parser_t *p) {
    while (is_statement(p)) {
        if (!parse_statement(p)) {
            return false;
        }
    }

    return true;
}

// <statement> -> <variable_definition>
// <statement> -> <assignment> // lookahead
// <statement> -> <function_call> // lookahead
// <statement> -> <function_return>
// <statement> -> <if_statement>
// <statement> -> <while_statement>
// <statement> -> <expression> // lookahead
static bool parse_statement(parser_t *p) {
    if (token_is_kw(&p->current, KW_VAL)) {
        return parse_variable_definition(p);
    }

    if (token_is_kw(&p->current, KW_VAR)) {
        return parse_variable_definition(p);
    }

    if (token_is_kw(&p->current, KW_RETURN)) {
        return parse_function_return(p);
    }

    if (token_is_kw(&p->current, KW_IF)) {
        return parse_if_statement(p);
    }

    if (token_is_kw(&p->current, KW_WHILE)) {
        return parse_while_statement(p);
    }

    if (p->current.type == TT_IDENTIFIER) {
        return parse_identifier_statement(p);
    }

    return false;
}
