/**
 * @file ast.h
 *
 * @author Jakub Manoušek
 * @brief Modulu pro AST (abstraktní syntaktický strom)
 *
 * @details
 */

#ifndef AST_H
#define AST_H

#include "dstr.h"
#include "types.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct ast_stmt ast_stmt_t;
typedef struct ast_func ast_func_t;
typedef struct ast_expr ast_expr_t;

typedef enum expr_kind {
    E_UNARY,
    E_BINARY,
    E_FUNC,
    E_VAR,
    E_UNIT_LIT,
    E_INT_LIT,
    E_DOUBLE_LIT,
    E_STRING_LIT,
    E_BOOLEAN_LIT,
} expr_kind;

typedef enum op_unary {
    OPU_NEG,
    OPU_PLUS,
    OPU_NOT,
} op_unary;

typedef enum op_binary {
    OPB_ADD,
    OPB_SUB,
    OPB_MUL,
    OPB_DIV,
    OPB_AND,
    OPB_OR,
    OPB_LT,
    OPB_LTE,
    OPB_GT,
    OPB_GTE,
    OPB_EQ,
    OPB_NE,
} op_binary;

typedef enum stmt_kind : uint8_t {
    STMT_VAR_DEF,
    STMT_EXPR,
    STMT_RET,
    STMT_ASSIGN,
    STMT_IF,
    STMT_WHILE,
} stmt_kind;

struct ast_expr {
    data_type data_type;
    unsigned line;
    expr_kind kind;
    union {
        dstr str;
        int int_val;
        double double_val;
        bool bool_val;
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
            ast_func_t *target; /**< nullptr = builtin */
        } func;
    } as;
};

typedef struct ast_block {
    ast_stmt_t *first;
    ast_stmt_t *last;
    size_t count;
} ast_block_t;

struct ast_stmt {
    unsigned line;
    ast_stmt_t *next;
    stmt_kind kind;
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
            ast_block_t then_blk;
            bool has_else;
            ast_block_t else_blk;
        } if_stmt;
        struct {
            ast_expr_t *cond;
            ast_block_t block;
        } while_stmt;
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
    ast_func_t *first;
    ast_func_t *last;
    size_t count;
} ast_prog_t;

/**
 * @brief Vytvoří hlavní strukturu AST program (allokuje paměť)
 *
 * @return AST programu
 */
ast_prog_t *ast_prog_create();

/**
 * @brief Vytvoří strukturu AST funkce (allokuje paměť)
 *
 * @param line Číslo řádku
 *
 * @return AST funkce
 */
ast_func_t *ast_func_create(unsigned line);

/**
 * @brief Vytvoří strukturu AST příkazu (allokuje paměť)
 *
 * @param line Číslo řádku
 * @param type Druh příkazu
 *
 * @return AST příkazu
 */
ast_stmt_t *ast_stmt_create(unsigned line, stmt_kind kind);

/**
 * @brief Vytvoří strukturu AST výrazu (allokuje paměť)
 *
 * @param line Číslo řádku
 * @param type Druh výrazu
 *
 * @return AST výrazu
 */
ast_expr_t *ast_expr_create(unsigned line, expr_kind kind);

/**
 * @brief Vytvoří strukturu AST unárního výrazu (allokuje paměť)
 *
 * Datový typ je brán automaticky z operandu, jinak je TYPE_UNKNOWN
 *
 * @param line Číslo řádku
 * @param type Druh unární operace
 * @param operand Operand výrazu
 *
 * @return AST unárního výrazu
 */
ast_expr_t *ast_expr_unary_create(unsigned line, op_unary opu,
                                  ast_expr_t *operand);

/**
 * @brief Vytvoří strukturu AST unárního výrazu (allokuje paměť)
 *
 * Datový typ je brán automaticky z levé strany, pokud je levá strana nullptr,
 * tak se bere z pravé, jinak je TYPE_UNKNOWN
 *
 * @param line Číslo řádku
 * @param type Druh binární operace
 * @param left Levý operand výrazu
 * @param right Pravý operand výrazu
 *
 * @return AST binárního výrazu
 */
ast_expr_t *ast_expr_binary_create(unsigned line, op_binary opb,
                                   ast_expr_t *left, ast_expr_t *right);

/**
 * @brief Přídát příkaz do blocku
 *
 * @param[in,out] block Block do kterého přidátvat
 * @param[in] stmt Příkaz co přidát
 *
 * @return Stav úspěchu
 */
bool ast_block_append(ast_block_t *block, ast_stmt_t *stmt);

/**
 * @brief Přídát funkci do programu
 *
 * @param[in,out] program Program do kterého přidátvat
 * @param[in] func Funkce co přidát
 *
 * @return Stav úspěchu
 */
bool ast_prog_append(ast_prog_t *program, ast_func_t *func);

/**
 * @brief Rekurzivně uvolní všechy data součástí AST programu spolu s programem
 *
 * @param program AST programu
 */
void ast_prog_free(ast_prog_t *program);

/**
 * @brief Vytiskne všechny data AST programu
 *
 * @param program AST programu
 * @param out Výstupní `stream`
 */
void ast_prog_dump(const ast_prog_t *program, FILE *out);

#endif
