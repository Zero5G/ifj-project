/**
 * @file ast.h
 *
 * @author Jakub Manoušek
 * @brief Modulu pro ast
 *
 * @details
 */
#include "dstr.h"
#include "types.h"

#include <stddef.h>
#include <stdio.h>

typedef struct ast_stmnt ast_stmnt_t;
typedef struct ast_func ast_func_t;
typedef struct ast_expr ast_expr_t;

typedef enum expr_type {
    E_UNARY,
    E_BINARY,
    E_FUNC,
    E_UNIT_LIT,
    E_INT_LIT,
    E_DOUBLE_LIT,
    E_STRING_LIT,
    E_BOOLEAN_LIT,
} expr_type;

typedef enum op_unary {
    OPU_NEG,
    OPU_PLUS,
    OPU_NOT,
} op_unary;

typedef enum op_binary {
    OPU_ADD,
    OPU_SUB,
    OPU_MUL,
    OPU_DIV,
    OPU_AND,
    OPU_OR,
    OPU_LT,
    OPU_LTE,
    OPU_GT,
    OPU_GTE,
    OPU_EQ,
    OPU_NE,
} op_binary;

typedef enum stmnt_type {
    STMNT_VAR_DEF,
    STMNT_EXPR,
    STMNT_RET,
    STMNT_ASSIGN,
    STMNT_IF,
    STMNT_WHILE,
} stmnt_type;

struct ast_expr {
    expr_type type;
    data_type data_type;
    unsigned line;
    union {
        dstr str;
        int int_val;
        double double_val;
        struct {
            op_binary op;
            ast_expr_t *right;
            ast_expr_t *left;
        } binary;
        struct {
            op_unary op;
            ast_expr_t *operand;
        } unary;
        struct {
            dstr name;
            unsigned var_id; /**< ID generované v sematické analýze */
        } var;
        struct {
            dstr name;
            ast_expr_t **args;
            size_t argc;
        } func;
    } as;
};

typedef struct ast_block {
    ast_stmnt_t *first;
    ast_stmnt_t *last;
    size_t count;
} ast_block_t;

struct ast_stmnt {
    unsigned line;
    ast_stmnt_t *next;
    stmnt_type type;
    union {
        struct {
            dstr name;
            unsigned var_id;
            bool is_mut;
            data_type type;
            ast_expr_t *init; /**< První hodnota */
        } var_def;
        struct {
            dstr name;
            unsigned var_id;
            ast_expr_t *val;
        } assign;
        ast_expr_t *expr;
        ast_expr_t *ret_val;
        struct {
            ast_expr_t *cond;
            ast_block_t *then_blk;
            ast_block_t *else_blk;
        } if_stmnt;
        struct {
            ast_expr_t *cond;
            ast_block_t *block;
        } while_stmnt;
    } as;
};

typedef struct ast_func_param {
    dstr name;
    data_type type;
} ast_func_param_t;

struct ast_func {
    dstr name;
    bool is_main;
    ast_func_param_t *params;
    size_t param_count;
    bool has_ret_type;
    data_type ret_type;
    unsigned line;
    ast_block_t body;
    ast_func_t *next;
};

typedef struct ast_prog {
    ast_stmnt_t *first;
    ast_stmnt_t *last;
    size_t count;
} ast_prog_t;

ast_prog_t *ast_prog_create();
ast_expr_t *ast_func_create(unsigned line);
ast_stmnt_t *ast_stmnt_create(unsigned line, stmnt_type type);
ast_expr_t *ast_expr_create(unsigned line, expr_type type);
ast_expr_t *ast_expr_unary_create(unsigned line, op_unary opu, ast_expr_t *operand);
ast_expr_t *ast_expr_binary_create(unsigned line, op_binary opb, ast_expr_t *left, ast_expr_t *right);

void ast_prog_free(ast_prog_t *program);

void ast_prog_dump(const ast_prog_t *program, FILE *out);
