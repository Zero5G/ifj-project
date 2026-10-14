#include "dstr.h"

typedef enum keyword {
    KW_DEF,  KW_IF, KW_THEN,
    KW_ELSE, KW_WHILE, KW_DO,
    KW_VAL, KW_VAR, KW_RETURN,
    KW_IMPORT,
    // Maybe not needed?
    KW_CASE, KW_CLASS, KW_EXTENSION,
    KW_FOR, KW_MATCH, KW_PRINT,
    KW_TYPE, KW_MAIN,
    //
    KW_INT, KW_DOUBLE, KW_STRING,
    KW_BOOLEAN, KW_UNIT, KW_LIST,
} keyword;

typedef enum token_type {
    TT_KEYWORD,
    TT_IDENTIFIER,

    //
    TT_INT_LITERAL, TT_DOUBLE_LITERAL,
    TT_STRING_LITERAL, TT_BOOLEAN_LITERAL,
    TT_UNIT_LITERAL,

    // Operátory
    TT_PLUS, TT_MINUS,
    TT_STAR, TT_SLASH,
    TT_EQ, TT_NEQ, TT_LT,
    TT_LTE, TT_GT, TT_GTE,

    TT_ASSIGN,

    // Separátory
    TT_COLON, TT_COMMA,
    TT_DOT, // TT_SEMICOLON, // TT_AT,
    TT_LPAREN, TT_RPAREN,

    // Whitespace znaky
    TT_INDENT, //< Po novém řádku a je vetší než předchozí odsazení
    TT_OUTDENT, //< Po novém řádku a je menší než předchozí odsazení
    TT_EOF, TT_EOL,
} token_type;

struct token_t {
    token_type type;
    union {
        bool as_bool;
        // dstr as_str;
        keyword as_keyword;
        int as_int;
        double as_double;
    } attr;
};

typedef struct token_t token;
