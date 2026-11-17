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

void parser_destroy(parser_t *p) { token_free(&p->current); }

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
// expressions can start with {TT_IDENTIFIER, TT_INT_LITERAL, TT_DOUBLE_LITERAL,
// TT_STRING_LITERAL, TT_BOOLEAN_LITERAL, TT_UNIT_LITERAL, TT_LPAREN}, so will
// need an edit later
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

        return true;
    }

    return parse_statement(p);
}

// <body> -> <statement_list>
static bool parse_body(parser_t *p) {
    if (!parse_statement_list(p)) {
        return false;
    }

    return true;
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

// <variable_definition> -> KW_VAL TT_IDENTIFIER <variable_type> TT_ASSIGN
// <expression>
// <variable_definition> -> KW_VAR TT_IDENTIFIER <variable_type> TT_ASSIGN
// <expression>
static bool parse_variable_definition(parser_t *p) {
    if (!accept_kw(p, KW_VAL) && !accept_kw(p, KW_VAR)) {
        return false;
    }

    if (!expect(p, TT_IDENTIFIER)) {
        return false;
    }

    if (!parse_variable_type(p)) {
        return false;
    }

    if (!expect(p, TT_ASSIGN)) {
        return false;
    }

    if (!parse_expression(p)) {
        return false;
    }

    return true;
}

// <variable_type> -> TT_COLON <type>
// <variable_type> -> ε
static bool parse_variable_type(parser_t *p) {
    if (accept(p, TT_COLON)) {
        if (!parse_type(p)) {
            return false;
        }
    }

    return true;
}

// <function_return> -> KW_RETURN
// quite possibly wrong, probably needs KW_RETURN <expression> in the future,
// will require an editing the grammar rules and LL table
static bool parse_function_return(parser_t *p) {
    if (!expect_kw(p, KW_RETURN)) {
        return false;
    }

    return true;
}

// <if_statement> -> KW_IF <expression> KW_THEN TT_EOL TT_INDENT <body> TT_EOL
// TT_OUTDENT KW_ELSE TT_EOL TT_INDENT <body> TT_EOL TT_OUTDENT
static bool parse_if_statement(parser_t *p) {
    if (!expect_kw(p, KW_IF)) {
        return false;
    }

    if (!parse_expression(p)) {
        return false;
    }

    if (!expect_kw(p, KW_THEN)) {
        return false;
    }

    if (!expect(p, TT_EOL)) {
        return false;
    }

    if (!expect(p, TT_INDENT)) {
        return false;
    }

    if (!parse_body(p)) {
        return false;
    }

    if (!expect(p, TT_EOL)) {
        return false;
    }

    if (!expect(p, TT_DEDENT)) {
        return false;
    }

    if (!expect_kw(p, KW_ELSE)) {
        return false;
    }

    if (!expect(p, TT_EOL)) {
        return false;
    }

    if (!expect(p, TT_INDENT)) {
        return false;
    }

    if (!parse_body(p)) {
        return false;
    }

    if (!expect(p, TT_EOL)) {
        return false;
    }

    if (!expect(p, TT_DEDENT)) {
        return false;
    }

    return true;
}

// <while_statement> -> KW_WHILE <expression> KW_DO TT_EOL TT_INDENT <body>
// TT_EOL TT_OUTDENT
static bool parse_while_statement(parser_t *p) {
    if (!expect_kw(p, KW_WHILE)) {
        return false;
    }

    if (!parse_expression(p)) {
        return false;
    }

    if (!expect_kw(p, KW_DO)) {
        return false;
    }

    if (!expect(p, TT_EOL)) {
        return false;
    }

    if (!expect(p, TT_INDENT)) {
        return false;
    }

    if (!parse_body(p)) {
        return false;
    }

    if (!expect(p, TT_EOL)) {
        return false;
    }

    if (!expect(p, TT_DEDENT)) {
        return false;
    }

    return true;
}

// lookahead because of TT_IDENTIFIER
// <statement> -> <assignment> // lookahead
// <statement> -> <function_call> // lookahead
// <statement> -> <expression> // lookahead
static bool parse_identifier_statement(parser_t *p) {
    const token *next = lexer_peek(p->lx, 1);

    if (next == NULL) {
        return false;
    }

    if (next->type == TT_ASSIGN) {
        return parse_assignment(p);
    }

    if (next->type == TT_LPAREN) {
        return parse_function_call(p);
    }

    else {
        return parse_expression(p);
    }
}

// <assignment> -> TT_IDENTIFIER TT_ASSIGN <expression>
static bool parse_assignment(parser_t *p) {
    if (!expect(p, TT_IDENTIFIER)) {
        return false;
    }

    if (!expect(p, TT_ASSIGN)) {
        return false;
    }

    if (!parse_expression(p)) {
        return false;
    }

    return true;
}

// <function_call> -> TT_IDENTIFIER TT_LPAREN <argument_list> TT_RPAREN
static bool parse_function_call(parser_t *p) {
    if (!expect(p, TT_IDENTIFIER)) {
        return false;
    }

    if (!expect(p, TT_LPAREN)) {
        return false;
    }

    if (!parse_argument_list(p)) {
        return false;
    }

    if (!expect(p, TT_RPAREN)) {
        return false;
    }

    return true;
}

// <argument_list> -> <argument> <argument_next>
// <argument_list> -> ε
static bool parse_argument_list(parser_t *p) {
    if (p->current.type == TT_IDENTIFIER) {
        if (!parse_argument(p)) {
            return false;
        }

        if (!parse_argument_next(p)) {
            return false;
        }
    }

    return true;
}

// <argument> -> TT_IDENTIFIER
// argument should allow terms IDENTIFIER, INT_LITERAL, DOUBLE_LITERAL,
// STRING_LITERAL, BOOLEAN_LITERAL, UNIT_LITERAL, so this should be taken care
// of
static bool parse_argument(parser_t *p) {
    if (!expect(p, TT_IDENTIFIER)) {
        return false;
    }

    return true;
}

// <argument_next> TT_COMMA TT_IDENTIFIER <argument_next>
// <argument_next> -> ε
static bool parse_argument_next(parser_t *p) {
    if (accept(p, TT_COMMA)) {
        if (!expect(p, TT_IDENTIFIER)) {
            return false;
        }

        if (!parse_argument_next(p)) {
            return false;
        }
    }

    return true;
}

// parse_expression() -> in expr.c
