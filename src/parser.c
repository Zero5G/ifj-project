/**
 * @file parser.c
 *
 * @author Matěj Krňák
 * @brief Implementace modulu pro syntaktickou analýzu
 *
 * @details
 */

#include "parser.h"
#include "ast.h"
#include "dstr.h"
#include "expr.h"

#include <stdlib.h>

static bool parse_prog(parser_ctx *ctx);
static bool parse_prolog(parser_ctx *ctx);
static bool parse_function_list(parser_ctx *ctx);
static bool parse_parameters(parser_ctx *ctx, ast_func_t *func);
static bool parse_parameter(parser_ctx *ctx, ast_func_t *func);
static bool parse_parameter_next(parser_ctx *ctx, ast_func_t *func);
static bool parse_function(parser_ctx *ctx);
static bool parse_return_type(parser_ctx *ctx, ast_func_t *func);
static bool parse_type(parser_ctx *ctx, data_type *out);
static bool parse_block(parser_ctx *ctx, ast_block_t *block);
static bool parse_indented_block(parser_ctx *ctx, ast_block_t *block);
static bool parse_body(parser_ctx *ctx, ast_block_t *block);
static bool parse_statement_list(parser_ctx *ctx, ast_block_t *block);
static ast_stmt_t *parse_statement(parser_ctx *ctx);
static ast_stmt_t *parse_variable_definition(parser_ctx *ctx);
static ast_stmt_t *parse_function_return(parser_ctx *ctx);
static ast_stmt_t *parse_if_statement(parser_ctx *ctx);
static ast_stmt_t *parse_while_statement(parser_ctx *ctx);
static bool parse_identifier_statement(parser_ctx *ctx);
static bool parse_variable_type(parser_ctx *ctx, data_type *out);
static bool parse_assignment(parser_ctx *ctx);
static bool parse_function_call(parser_ctx *ctx);
static bool parse_argument_list(parser_ctx *ctx);
static bool parse_argument(parser_ctx *ctx);
static bool parse_argument_next(parser_ctx *ctx);

bool parser_init(parser_ctx *ctx, lexer *lx) {
    ctx->lx = lx;
    ctx->prog = ast_prog_create();
    if (ctx->prog == nullptr) {
        return false;
    }

    ctx->current = get_next_token(lx);

    return true;
}

ast_prog_t *parser_take_prog(parser_ctx *ctx) {
    ast_prog_t *prog = ctx->prog;
    ctx->prog = nullptr;
    return prog;
}

void parser_destroy(parser_ctx *ctx) {
    token_free(&ctx->current);

    if (ctx->prog != nullptr) {
        ast_prog_free(ctx->prog);
        free(ctx->prog);
        ctx->prog = nullptr;
    }
}

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

static unsigned current_line(const parser_ctx *ctx) {
    return (unsigned)ctx->current.line;
}

static dstr take_identifier(parser_ctx *ctx) {
    if (ctx->current.type != TT_IDENTIFIER) {
        return nullptr;
    }

    dstr name = ctx->current.attr.as_str;

    return name;
}

static dstr take_function_name(parser_ctx *ctx) {
    if (ctx->current.type == TT_IDENTIFIER) {
        return take_identifier(ctx);
    }

    dstr name = nullptr;
    if (token_is_kw(&ctx->current, KW_MAIN)) {
        name = dstr_create("main");
    } else {
        return nullptr;
    }

    if (name != nullptr) {
        advance(ctx);
    }

    return name;
}

static bool is_stmt_end(const parser_ctx *ctx) {
    return ctx->current.type == TT_EOL || ctx->current.type == TT_DEDENT ||
           ctx->current.type == TT_EOF;
}

static bool expect_stmt_end(parser_ctx *ctx) {
    if (accept(ctx, TT_EOL)) {
        return true;
    }

    return ctx->current.type == TT_DEDENT || ctx->current.type == TT_EOF;
}

static bool add_parameter(ast_func_t *func, dstr name) {
    ast_func_param_t *params =
        realloc(func->params, (func->param_count + 1) * sizeof(*params));
    if (params == nullptr) {
        return false;
    }

    func->params = params;
    func->params[func->param_count] = (ast_func_param_t){
        .name = name,
        .type = TYPE_UNKNOWN,
        .var_id = 0,
    };
    func->param_count++;

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

bool parser_run(parser_ctx *ctx) {
    if (ctx->prog == nullptr) {
        return false;
    }

    return parse_prog(ctx);
}

// <prog> -> <prolog> <function_list>
static bool parse_prog(parser_ctx *ctx) {
    if (!parse_prolog(ctx)) {
        return false;
    }

    if (!parse_function_list(ctx)) {
        return false;
    }

    return expect(ctx, TT_EOF);
}

// <prolog> -> KW_IMPORT ifj26 TT_DOT TT_STAR
static bool parse_prolog(parser_ctx *ctx) {
    if (!expect_kw(ctx, KW_IMPORT)) {
        return false;
    }

    if (ctx->current.type != TT_IDENTIFIER ||
        dstr_cmp_raw(ctx->current.attr.as_str, "ifj26") != 0) {
        return false;
    }
    advance(ctx);

    if (!expect(ctx, TT_DOT)) {
        return false;
    }

    if (!expect(ctx, TT_STAR)) {
        return false;
    }

    return accept(ctx, TT_EOL) || ctx->current.type == TT_EOF;
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
    unsigned line = current_line(ctx);

    if (!expect_kw(ctx, KW_DEF)) {
        return false;
    }

    dstr name = take_function_name(ctx);
    if (name == nullptr) {
        return false;
    }

    ast_func_t *func = ast_func_create(line);
    if (func == nullptr) {
        dstr_delete(name);
        return false;
    }

    func->name = name;
    if (dstr_cmp_raw(name, "main") == 0) {
        func->is_main = name;
    }

    ast_prog_append(ctx->prog, func);

    if (!expect(ctx, TT_LPAREN)) {
        return false;
    }

    if (!parse_parameters(ctx, func)) {
        return false;
    }

    if (!expect(ctx, TT_RPAREN)) {
        return false;
    }

    if (!parse_return_type(ctx, func)) {
        return false;
    }

    if (!expect(ctx, TT_ASSIGN)) {
        return false;
    }

    return parse_block(ctx, &func->body);
}

// <parameters> -> <parameter> <parameter_next>
// <parameters> -> ε
static bool parse_parameters(parser_ctx *ctx, ast_func_t *func) {
    if (ctx->current.type == TT_IDENTIFIER) {
        if (!parse_parameter(ctx, func)) {
            return false;
        }

        if (!parse_parameter_next(ctx, func)) {
            return false;
        }
    }

    return true;
}

// <parameter> -> TT_IDENTIFIER TT_COLON <type>
static bool parse_parameter(parser_ctx *ctx, ast_func_t *func) {
    dstr name = take_identifier(ctx);
    if (name == nullptr) {
        return false;
    }

    if (!add_parameter(func, name)) {
        dstr_delete(name);
        return false;
    }

    if (!expect(ctx, TT_COLON)) {
        return false;
    }

    return parse_type(ctx, &func->params[func->param_count - 1].type);
}

// <parameter_next> -> TT_COMMA TT_IDENTIFIER TT_COLON <type> <parameter_next>
// <parameter_next> -> ε
static bool parse_parameter_next(parser_ctx *ctx, ast_func_t *func) {
    while (accept(ctx, TT_COMMA)) {
        if (!parse_parameter(ctx, func)) {
            return false;
        }
    }

    return true;
}

// <return_type> -> TT_COLON <type>
// <return_type> -> ε
static bool parse_return_type(parser_ctx *ctx, ast_func_t *func) {
    if (!accept(ctx, TT_COLON)) {
        func->has_ret_type = false;
        func->ret_type = TYPE_UNKNOWN;
        return true;
    }

    func->has_ret_type = true;
    return parse_type(ctx, &func->ret_type);
}

// <type> -> KW_INT
// <type> -> KW_DOUBLE
// <type> -> KW_STRING
// <type> -> KW_BOOLEAN
// <type> -> KW_UNIT
// <type> -> KW_LIST
static bool parse_type(parser_ctx *ctx, data_type *out) {
    if (ctx->current.type != TT_KEYWORD) {
        return false;
    }

    data_type type = TYPE_UNKNOWN;

    switch (ctx->current.attr.as_keyword) {
    case KW_INT:
        type = TYPE_INT;
        break;
    case KW_DOUBLE:
        type = TYPE_DOUBLE;
        break;
    case KW_STRING:
        type = TYPE_STRING;
        break;
    case KW_BOOLEAN:
        type = TYPE_BOOL;
        break;
    case KW_UNIT:
        type = TYPE_UNIT;
        break;
    case KW_LIST:
        type = TYPE_UNKNOWN; // no list type in types.h so idk what to put here
        break;
    default:
        return false;
    }

    advance(ctx);
    *out = type;

    return true;
}

// <block> -> <statement>
// <block> -> TT_EOL TT_INDENT <body> TT_OUTDENT
static bool parse_block(parser_ctx *ctx, ast_block_t *block) {
    if (ctx->current.type == TT_EOL) {
        return parse_indented_block(ctx, block);
    }

    return parse_block_statement(ctx, block);
}

static bool parse_indented_block(parser_ctx *ctx, ast_block_t *block) {
    if (!expext(ctx, TT_EOL)) {
        return false;
    }

    if (!expect(ctx, TT_INDENT)) {
        return false;
    }

    if (!parse_body(ctx, block)) {
        return false;
    }

    if (!expect(ctx, TT_DEDENT)) {
        return false;
    }

    return true;
}

// <body> -> <statement_list>
static bool parse_body(parser_ctx *ctx, ast_block_t *block) {
    return parse_statement_list(ctx, block);
}

// <statement_list> -> <statement> <statement_list>
// <statement_list> -> ε
static bool parse_statement_list(parser_ctx *ctx, ast_block_t *block) {
    while (is_statement(ctx)) {
        if (!parse_block_statement(ctx, block)) {
            return false;
        }
    }

    return true;
}

static bool parse_block_statement(parser_ctx *ctx, ast_block_t *block) {
    ast_stmt_t *stmt = parse_statement(ctx);
    if (stmt == nullptr) {
        return false;
    }

    ast_block_append(block, stmt);

    if (stmt->kind == STMT_IF || stmt->kind == STMT_WHILE) {
        return true;
    }

    return expect_stmt_end(ctx);
}

// <statement> -> <variable_definition>
// <statement> -> <assignment> // lookahead
// <statement> -> <function_call> // lookahead
// <statement> -> <function_return>
// <statement> -> <if_statement>
// <statement> -> <while_statement>
// <statement> -> <expression> // lookahead
static ast_stmt_t *parse_statement(parser_ctx *ctx) {
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
static ast_stmt_t *parse_variable_definition(parser_ctx *ctx) {
    unsigned line = current_line(ctx);
    bool is_mut = false;

    if (accept_kw(ctx, KW_VAR)) {
        is_mut = true;
    } else if (!accept_kw(ctx, KW_VAL)) {
        return nullptr;
    }

    dstr name = take_identifier(ctx);
    if (name == nullptr) {
        return nullptr;
    }

    data_type type = TYPE_UNKNOWN;
    if (!parse_variable_type(ctx, &type)) {
        dstr_delete(name);
        return nullptr;
    }

    if (!expect(ctx, TT_ASSIGN)) {
        dstr_delete(name);
        return nullptr;
    }

    ast_expr_t *init = parse_expression(ctx);
    if (init == nullptr) {
        dstr_delete(name);
        return nullptr;
    }

    ast_stmt_t *stmt = ast_stmt_create(line, STMT_VAR_DEF);
    if (stmt == nullptr) {
        dstr_delete(name);
        return nullptr;
    }

    stmt->as.var_def.name = name;
    stmt->as.var_def.var_id = 0;
    stmt->as.var_def.is_mut = is_mut;
    stmt->as.var_def.type = type;
    stmt->as.var_def.init = init;

    return stmt;
}

// <variable_type> -> TT_COLON <type>
// <variable_type> -> ε
static bool parse_variable_type(parser_ctx *ctx, data_type *out) {
    if (!accept(ctx, TT_COLON)) {
        *out = TYPE_UNKNOWN;
        return true;
    }

    return parse_type(ctx, out);
}

// <function_return> -> KW_RETURN <expression>
static ast_stmt_t *parse_function_return(parser_ctx *ctx) {
    unsigned line = current_line(ctx);

    if (!expect_kw(ctx, KW_RETURN)) {
        return nullptr;
    }

    ast_expr_t *val = nullptr;
    if (!is_stmt_end(ctx)) {
        val = parse_expression(ctx);
        if (val == nullptr) {
            return nullptr;
        }
    }

    ast_stmt_t *stmt = ast_stmt_create(line, STMT_RET);
    if (stmt == nullptr) {
        return nullptr;
    }

    stmt->as.ret_val = val;

    return stmt;
}

// <if_statement> -> KW_IF <expression> KW_THEN TT_EOL TT_INDENT <body> TT_EOL
// TT_OUTDENT KW_ELSE TT_EOL TT_INDENT <body> TT_EOL TT_OUTDENT
static ast_stmt_t *parse_if_statement(parser_ctx *ctx) {
    unsigned line = current_line(ctx);

    if (!expect_kw(ctx, KW_IF)) {
        return nullptr;
    }

    ast_expr_t *cond = parse_expression(ctx);
    if (cond == nullptr) {
        return nullptr;
    }

    if (!expect_kw(ctx, KW_THEN)) {
        return nullptr;
    }

    ast_stmt_t *stmt = ast_stmt_create(line, STMT_IF);
    if (stmt == nullptr) {
        return nullptr;
    }

    stmt->as.if_stmt.cond = cond;
    stmt->as.if_stmt.has_else = false;

    if (!parse_indented_block(ctx, &stmt->as.if_stmt.then_blk)) {
        return nullptr;
    }

    if (accept(ctx, KW_ELSE)) {
        stmt->as.if_stmt.has_else = true;

        if (!parse_indented_block(ctx, &stmt->as.if_stmt.else_blk)) {
            return nullptr;
        }
    }

    return stmt;
}

// <while_statement> -> KW_WHILE <expression> KW_DO TT_EOL TT_INDENT <body>
// TT_EOL TT_OUTDENT
static ast_stmt_t *parse_while_statement(parser_ctx *ctx) {
    unsigned line = current_line(ctx);

    if (!expect_kw(ctx, KW_WHILE)) {
        return nullptr;
    }

    ast_expr_t *cond = parse_expression(ctx);
    if (cond == nullptr) {
        return nullptr;
    }

    if (!expect_kw(ctx, KW_DO)) {
        return false;
    }

    ast_stmt_t *stmt = ast_stmt_create(line, STMT_WHILE);
    if (stmt == nullptr) {
        return nullptr;
    }

    stmt->as.while_stmt.cond = cond;

    if (!parse_indented_block(ctx, &stmt->as.while_stmt.block)) {
        return nullptr;
    }

    return stmt;
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
