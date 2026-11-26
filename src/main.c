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
#include <stdio.h>
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

    fprintf(stderr, "lexer start\n"); // NOLINT
    lex = lexer_create(stdin);
    if (lex == nullptr) {
        error_fatal_cleanup(ERR_OTHER);
    }
    fprintf(stderr, "lexer done\n"); // NOLINT

    ast = ast_prog_create();
    if (ast == nullptr) {
        error_fatal_cleanup(ERR_OTHER);
    }
    fprintf(stderr, "ast allocated\n"); // NOLINT

    fprintf(stderr, "parser init\n"); // NOLINT
    parser.prog = ast;
    if (!parser_init(&parser, lex)) {
        error_fatal_cleanup(ERR_OTHER);
    }
    fprintf(stderr, "parser run\n"); // NOLINT
    if (!parser_run(&parser)) {
        error_fatal_cleanup(ERR_OTHER);
    }
    fprintf(stderr, "parser done\n"); // NOLINT

    lexer_destroy(&lex);
    parser_destroy(&parser);

    stab = symtable_create();
    if (stab == nullptr) {
        error_fatal_cleanup(ERR_OTHER);
    }
    fprintf(stderr, "symtable allocated\n"); // NOLINT

    fprintf(stderr, "analysis start\n"); // NOLINT
    semantic_analyze(ast, stab);
    fprintf(stderr, "analysis done\n"); // NOLINT
    symtable_destroy(stab);

    fprintf(stderr, "codegen start\n"); // NOLINT
    codegen(ast, stdout);
    fprintf(stderr, "codegen done\n"); // NOLINT
    ast_prog_free(ast);

    return EXIT_SUCCESS;
}
