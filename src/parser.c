/**
 * @file parser.c
 *
 * @author Matěj Krňák
 * @brief Implementace modulu pro syntaktickou analýzu
 *
 * @details
 */

#include "parser.h"
#include "expr.h"

static bool parse_prog(parser_ctx *ctx);
static bool parse_prolog(parser_ctx *ctx);
static bool parse_function_list(parser_ctx *ctx);
static bool parse_parameters(parser_ctx *ctx);
static bool parse_function(parser_ctx *ctx);
static bool parse_return_type(parser_ctx *ctx);
static bool parse_block(parser_ctx *ctx);
static bool parse_parameter(parser_ctx *ctx);
static bool parse_parameter_next(parser_ctx *ctx);
static bool parse_type(parser_ctx *ctx);
static bool parse_body(parser_ctx *ctx);
static bool parse_statement_list(parser_ctx *ctx);
static bool parse_statement(parser_ctx *ctx);
static bool parse_variable_definition(parser_ctx *ctx);
static bool parse_function_return(parser_ctx *ctx);
static bool parse_if_statement(parser_ctx *ctx);
static bool parse_while_statement(parser_ctx *ctx);
static bool parse_identifier_statement(parser_ctx *ctx);
static bool parse_variable_type(parser_ctx *ctx);
static bool parse_assignment(parser_ctx *ctx);
static bool parse_function_call(parser_ctx *ctx);
static bool parse_argument_list(parser_ctx *ctx);
static bool parse_argument(parser_ctx *ctx);
static bool parse_argument_next(parser_ctx *ctx);

bool parser_init(parser_ctx *ctx, lexer *lx) {
    ctx->lx = lx;
    ctx->current = get_next_token(lx);

    return true;
}

void parser_destroy(parser_ctx *ctx) { token_free(&ctx->current); }

void advance(parser_ctx *ctx) {
    token_free(&ctx->current);
    ctx->current = get_next_token(ctx->lx);
}

bool accept(parser_ctx *ctx, token_type type) {
    if (ctx->current.type != type) {
        return false;
    }

    advance(ctx);
    return true;
}

bool expect(parser_ctx *ctx, token_type type) {
    if (ctx->current.type != type) {
        return false;
    }

    advance(ctx);
    return true;
}

static bool accept_kw(parser_ctx *ctx, keyword kw) {
    if (!token_is_kw(&ctx->current, kw)) {
        return false;
    }

    advance(ctx);
    return true;
}

static bool expect_kw(parser_ctx *ctx, keyword kw) {
    if (!token_is_kw(&ctx->current, kw)) {
        return false;
    }

    advance(ctx);
    return true;
}

// First(<statement>) = {KW_VAL, KW_VAR, TT_IDENTIFIER, KW_RETURN, KW_IF,
// KW_WHILE}
// expressions can start with {TT_IDENTIFIER, TT_INT_LITERAL, TT_DOUBLE_LITERAL,
// TT_STRING_LITERAL, TT_BOOLEAN_LITERAL, TT_UNIT_LITERAL, TT_LPAREN}, so will
// need an edit later
static bool is_statement(parser_ctx *ctx) {
    if (token_is_kw(&ctx->current, KW_VAL)) {
        return true;
    }

    if (token_is_kw(&ctx->current, KW_VAR)) {
        return true;
    }

    if (ctx->current.type == TT_IDENTIFIER) {
        return true;
    }

    if (token_is_kw(&ctx->current, KW_RETURN)) {
        return true;
    }

    if (token_is_kw(&ctx->current, KW_IF)) {
        return true;
    }

    if (token_is_kw(&ctx->current, KW_WHILE)) {
        return true;
    }

    return false;
}

bool parser_run(parser_ctx *ctx) { return parse_prog(ctx); }

// <prog> -> <prolog> <function_list>
static bool parse_prog(parser_ctx *ctx) {
    return parse_prolog(ctx) && parse_function_list(ctx) && expect(ctx, TT_EOF);
}

// <prolog> -> KW_IMPORT ifj26 TT_DOT TT_STAR
static bool parse_prolog(parser_ctx *ctx) {
    if (!expect_kw(ctx, KW_IMPORT)) {
        return false;
    }

    if (!expect(ctx, TT_IDENTIFIER)) {
        return false;
    }

    if (!expect(ctx, TT_DOT)) {
        return false;
    }

    if (!expect(ctx, TT_STAR)) {
        return false;
    }

    return true;
}

// <function_list> -> <function> <function_list>
// <function_list> -> ε
static bool parse_function_list(parser_ctx *ctx) {
    while (token_is_kw(&ctx->current, KW_DEF)) {
        if (!parse_function(ctx)) {
            return false;
        }
    }

    return true;
}

// <function> -> KW_DEF TT_IDENTIFIER TT_LPAREN <parameters> TT_RPAREN
// <return_type> TT_ASSIGN <block>
static bool parse_function(parser_ctx *ctx) {
    if (!expect_kw(ctx, KW_DEF)) {
        return false;
    }

    if (!expect(ctx, TT_IDENTIFIER)) {
        return false;
    }

    if (!expect(ctx, TT_LPAREN)) {
        return false;
    }

    if (!parse_parameters(ctx)) {
        return false;
    }

    if (!expect(ctx, TT_RPAREN)) {
        return false;
    }

    if (!parse_return_type(ctx)) {
        return false;
    }

    if (!expect(ctx, TT_ASSIGN)) {
        return false;
    }

    if (!parse_block(ctx)) {
        return false;
    }

    return true;
}

// <parameters> -> <parameter> <parameter_next>
// <parameters> -> ε
static bool parse_parameters(parser_ctx *ctx) {
    if (ctx->current.type == TT_IDENTIFIER) {
        if (!parse_parameter(ctx)) {
            return false;
        }

        if (!parse_parameter_next(ctx)) {
            return false;
        }
    }

    return true;
}

// <parameter> -> TT_IDENTIFIER TT_COLON <type>
static bool parse_parameter(parser_ctx *ctx) {
    if (!expect(ctx, TT_IDENTIFIER)) {
        return false;
    }

    if (!expect(ctx, TT_COLON)) {
        return false;
    }

    if (!parse_type(ctx)) {
        return false;
    }

    return true;
}

// <parameter_next> -> TT_COMMA TT_IDENTIFIER TT_COLON <type> <parameter_next>
// <parameter_next> -> ε
static bool parse_parameter_next(parser_ctx *ctx) {
    if (accept(ctx, TT_COMMA)) {
        if (!expect(ctx, TT_IDENTIFIER)) {
            return false;
        }

        if (!expect(ctx, TT_COLON)) {
            return false;
        }

        if (!parse_type(ctx)) {
            return false;
        }

        if (!parse_parameter_next(ctx)) {
            return false;
        }
    }

    return true;
}

// <return_type> -> TT_COLON <type>
// <return_type> -> ε
static bool parse_return_type(parser_ctx *ctx) {
    if (accept(ctx, TT_COLON)) {
        if (!parse_type(ctx)) {
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
static bool parse_type(parser_ctx *ctx) {
    if (accept_kw(ctx, KW_INT)) {
        return true;
    }

    if (accept_kw(ctx, KW_DOUBLE)) {
        return true;
    }

    if (accept_kw(ctx, KW_STRING)) {
        return true;
    }

    if (accept_kw(ctx, KW_BOOLEAN)) {
        return true;
    }

    if (accept_kw(ctx, KW_UNIT)) {
        return true;
    }

    if (accept_kw(ctx, KW_LIST)) {
        return true;
    }

    return false;
}

// <block> -> <statement>
// <block> -> TT_EOL TT_INDENT <body> TT_OUTDENT
static bool parse_block(parser_ctx *ctx) {
    if (accept(ctx, TT_EOL)) {
        if (!expect(ctx, TT_INDENT)) {
            return false;
        }

        if (!parse_body(ctx)) {
            return false;
        }

        if (!expect(ctx, TT_DEDENT)) {
            return false;
        }

        return true;
    }

    return parse_statement(ctx);
}

// <body> -> <statement_list>
static bool parse_body(parser_ctx *ctx) {
    if (!parse_statement_list(ctx)) {
        return false;
    }

    return true;
}

// <statement_list> -> <statement> <statement_list>
// <statement_list> -> ε
static bool parse_statement_list(parser_ctx *ctx) {
    while (is_statement(ctx)) {
        if (!parse_statement(ctx)) {
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
static bool parse_statement(parser_ctx *ctx) {
    if (token_is_kw(&ctx->current, KW_VAL)) {
        return parse_variable_definition(ctx);
    }

    if (token_is_kw(&ctx->current, KW_VAR)) {
        return parse_variable_definition(ctx);
    }

    if (token_is_kw(&ctx->current, KW_RETURN)) {
        return parse_function_return(ctx);
    }

    if (token_is_kw(&ctx->current, KW_IF)) {
        return parse_if_statement(ctx);
    }

    if (token_is_kw(&ctx->current, KW_WHILE)) {
        return parse_while_statement(ctx);
    }

    if (ctx->current.type == TT_IDENTIFIER) {
        return parse_identifier_statement(ctx);
    }

    return false;
}

// <variable_definition> -> KW_VAL TT_IDENTIFIER <variable_type> TT_ASSIGN
// <expression>
// <variable_definition> -> KW_VAR TT_IDENTIFIER <variable_type> TT_ASSIGN
// <expression>
static bool parse_variable_definition(parser_ctx *ctx) {
    if (!accept_kw(ctx, KW_VAL) && !accept_kw(ctx, KW_VAR)) {
        return false;
    }

    if (!expect(ctx, TT_IDENTIFIER)) {
        return false;
    }

    if (!parse_variable_type(ctx)) {
        return false;
    }

    if (!expect(ctx, TT_ASSIGN)) {
        return false;
    }

    if (parse_expression(ctx) == nullptr) {
        return false;
    }

    return true;
}

// <variable_type> -> TT_COLON <type>
// <variable_type> -> ε
static bool parse_variable_type(parser_ctx *ctx) {
    if (accept(ctx, TT_COLON)) {
        if (!parse_type(ctx)) {
            return false;
        }
    }

    return true;
}

// <function_return> -> KW_RETURN
// quite possibly wrong, probably needs KW_RETURN <expression> in the future,
// will require an editing the grammar rules and LL table
static bool parse_function_return(parser_ctx *ctx) {
    if (!expect_kw(ctx, KW_RETURN)) {
        return false;
    }

    return true;
}

// <if_statement> -> KW_IF <expression> KW_THEN TT_EOL TT_INDENT <body> TT_EOL
// TT_OUTDENT KW_ELSE TT_EOL TT_INDENT <body> TT_EOL TT_OUTDENT
static bool parse_if_statement(parser_ctx *ctx) {
    if (!expect_kw(ctx, KW_IF)) {
        return false;
    }

    if (parse_expression(ctx) == nullptr) {
        return false;
    }

    if (!expect_kw(ctx, KW_THEN)) {
        return false;
    }

    if (!expect(ctx, TT_EOL)) {
        return false;
    }

    if (!expect(ctx, TT_INDENT)) {
        return false;
    }

    if (!parse_body(ctx)) {
        return false;
    }

    if (!expect(ctx, TT_EOL)) {
        return false;
    }

    if (!expect(ctx, TT_DEDENT)) {
        return false;
    }

    if (!expect_kw(ctx, KW_ELSE)) {
        return false;
    }

    if (!expect(ctx, TT_EOL)) {
        return false;
    }

    if (!expect(ctx, TT_INDENT)) {
        return false;
    }

    if (!parse_body(ctx)) {
        return false;
    }

    if (!expect(ctx, TT_EOL)) {
        return false;
    }

    if (!expect(ctx, TT_DEDENT)) {
        return false;
    }

    return true;
}

// <while_statement> -> KW_WHILE <expression> KW_DO TT_EOL TT_INDENT <body>
// TT_EOL TT_OUTDENT
static bool parse_while_statement(parser_ctx *ctx) {
    if (!expect_kw(ctx, KW_WHILE)) {
        return false;
    }

    if (parse_expression(ctx) == nullptr) {
        return false;
    }

    if (!expect_kw(ctx, KW_DO)) {
        return false;
    }

    if (!expect(ctx, TT_EOL)) {
        return false;
    }

    if (!expect(ctx, TT_INDENT)) {
        return false;
    }

    if (!parse_body(ctx)) {
        return false;
    }

    if (!expect(ctx, TT_EOL)) {
        return false;
    }

    if (!expect(ctx, TT_DEDENT)) {
        return false;
    }

    return true;
}

// lookahead because of TT_IDENTIFIER
// <statement> -> <assignment> // lookahead
// <statement> -> <function_call> // lookahead
// <statement> -> <expression> // lookahead
static bool parse_identifier_statement(parser_ctx *ctx) {
    const token *next = lexer_peek(ctx->lx, 1);

    if (next == nullptr) {
        return false;
    }

    if (next->type == TT_ASSIGN) {
        return parse_assignment(ctx);
    }

    if (next->type == TT_LPAREN) {
        return parse_function_call(ctx);
    }

    else {
        return parse_expression(ctx) != nullptr;
    }
}

// <assignment> -> TT_IDENTIFIER TT_ASSIGN <expression>
static bool parse_assignment(parser_ctx *ctx) {
    if (!expect(ctx, TT_IDENTIFIER)) {
        return false;
    }

    if (!expect(ctx, TT_ASSIGN)) {
        return false;
    }

    if (parse_expression(ctx) == nullptr) {
        return false;
    }

    return true;
}

// <function_call> -> TT_IDENTIFIER TT_LPAREN <argument_list> TT_RPAREN
static bool parse_function_call(parser_ctx *ctx) {
    if (!expect(ctx, TT_IDENTIFIER)) {
        return false;
    }

    if (!expect(ctx, TT_LPAREN)) {
        return false;
    }

    if (!parse_argument_list(ctx)) {
        return false;
    }

    if (!expect(ctx, TT_RPAREN)) {
        return false;
    }

    return true;
}

// <argument_list> -> <argument> <argument_next>
// <argument_list> -> ε
static bool parse_argument_list(parser_ctx *ctx) {
    if (ctx->current.type == TT_IDENTIFIER) {
        if (!parse_argument(ctx)) {
            return false;
        }

        if (!parse_argument_next(ctx)) {
            return false;
        }
    }

    return true;
}

// <argument> -> TT_IDENTIFIER
// argument should allow terms IDENTIFIER, INT_LITERAL, DOUBLE_LITERAL,
// STRING_LITERAL, BOOLEAN_LITERAL, UNIT_LITERAL, so this should be taken care
// of
static bool parse_argument(parser_ctx *ctx) {
    if (!expect(ctx, TT_IDENTIFIER)) {
        return false;
    }

    return true;
}

// <argument_next> TT_COMMA TT_IDENTIFIER <argument_next>
// <argument_next> -> ε
static bool parse_argument_next(parser_ctx *ctx) {
    if (accept(ctx, TT_COMMA)) {
        if (!expect(ctx, TT_IDENTIFIER)) {
            return false;
        }

        if (!parse_argument_next(ctx)) {
            return false;
        }
    }

    return true;
}

// parse_expression() -> in expr.c
