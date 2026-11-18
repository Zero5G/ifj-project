/**
 * @file main.c
 * @brief Main entry point of application
 */

#include "ast.h"
#include "lexer.h"
#include "error.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

static lexer *lex = nullptr;
static ast_prog_t *ast = nullptr;
static parser_ctx parser;

void cleanup() {
    lexer_destroy(lex);
    ast_prog_free(ast);
    parser_destroy(&parser);
}

int main(void) {
    error_fatal_set_cleanup(&cleanup);

    lex = lexer_create(stdin);
    if (lex == nullptr) {
        error_fatal(ERR_OTHER);
    }

    ast = ast_prog_create();
    if (ast == nullptr) {
        lexer_destroy(lex);
        error_fatal(ERR_OTHER);
    }

    parser_init(&parser, lex);



    return EXIT_SUCCESS;
}
