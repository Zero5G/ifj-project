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

typedef struct ast_stmnt ast_stmnt_t;
typedef struct ast_func ast_func_t;

typedef enum stmnt_type {
    STMNT_VAR_DEF,
    STMNT_FUNC,
    STMNT_EXPR,
    STMNT_RET,
    STMNT_ASSIGN,
    STMNT_IF,
    STMNT_WHILE,
} stmnt_type;

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

struct ast_stmnt {
    unsigned line;
    ast_stmnt_t *next;
    stmnt_type type;
    union {} as;
};

typedef struct ast_block {
    ast_stmnt_t *first;
    ast_stmnt_t *last;
    size_t count;
} ast_block_t;

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
