/**
 * @file main.c
 * @brief Main entry point of application
 */

#include "ast.h"
#include "codegen.h"
#include "semantic.h"
#include "lexer.h"
#include "error.h"
#include "parser.h"
#include "symtable.h"
#include <stdio.h>
#include <stdlib.h>

static lexer *lex = nullptr;
static ast_prog_t *ast = nullptr;
static parser_ctx parser;
static symtable *stab = nullptr;

void cleanup() { // NOLINT
    lexer_destroy(lex);
    ast_prog_free(ast);
    parser_destroy(&parser);
    symtable_destroy(stab);
}

int main(void) {
    error_fatal_set_cleanup(&cleanup);

    printf("lexer start\n");
    lex = lexer_create(stdin);
    if (lex == nullptr) {
        error_fatal_cleanup(ERR_OTHER);
    }
    printf("lexer done\n");

    ast = ast_prog_create();
    if (ast == nullptr) {
        error_fatal_cleanup(ERR_OTHER);
    }
    printf("ast allocated\n");

    printf("parser init\n");
    parser.prog = ast;
    if (!parser_init(&parser, lex)) {
        error_fatal_cleanup(ERR_OTHER);
    }
    printf("parser run\n");
    parser_run(&parser);
    printf("parser done\n");

    lexer_destroy(lex);
    parser_destroy(&parser);

    stab = symtable_create();
    if (stab == nullptr) {
        error_fatal_cleanup(ERR_OTHER);
    }
    printf("symtable allocated\n");

    printf("analysis start\n");
    semantic_analyze(ast, stab);
    printf("analysis done\n");
    symtable_destroy(stab);

    printf("codegen start\n");
    codegen(ast, stdout);
    printf("codegen done\n");
    ast_prog_free(ast);

    return EXIT_SUCCESS;
}
