/**
 * @file main.c
 * @brief Main entry point of application
 */

#include "ast.h"
#include "codegen.h"
#include "error.h"
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "symtable.h"
#include <stdlib.h>

static lexer *lex = nullptr;
static ast_prog_t *ast = nullptr;
static parser_ctx parser;
static symtable *stab = nullptr;

void cleanup() { // NOLINT
    lexer_destroy(&lex);
    ast_prog_free(ast);
    parser_destroy(&parser);
    symtable_destroy(stab);
}

int main(void) {
    error_fatal_set_cleanup(&cleanup);

    lex = lexer_create(stdin);
    if (lex == nullptr) {
        error_fatal_cleanup(ERR_OTHER);
    }

    ast = ast_prog_create();
    if (ast == nullptr) {
        error_fatal_cleanup(ERR_OTHER);
    }

    parser.prog = ast;
    if (!parser_init(&parser, lex)) {
        error_fatal_cleanup(ERR_OTHER);
    }
    if (!parser_run(&parser)) {
        error_fatal_cleanup(ERR_SYNT);
    }

    lexer_destroy(&lex);
    parser_destroy(&parser);

    stab = symtable_create();
    if (stab == nullptr) {
        error_fatal_cleanup(ERR_OTHER);
    }

    semantic_analyze(ast, stab);
    semantic_check_main(ast);
    symtable_destroy(stab);

    codegen(ast, stdout);
    ast_prog_free(ast);

    return EXIT_SUCCESS;
}
