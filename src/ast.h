/**
 * @file ast.h
 *
 * @author Jakub Manoušek
 * @brief Modulu pro ast
 *
 * @details
 */
typedef enum expr_type {
    E_UNARY,
    E_BINARY,
    E_FUNC,
    E_UNIT_LIT,
    E_INT_LIT,
    E_DOUBLE_LIT,
    E_STRING_LIT,
    E_BOOLEAN_LIT,
    E_LIST_LIT,
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

typedef struct ast_stmnt {

} ast_stmnt_t;
