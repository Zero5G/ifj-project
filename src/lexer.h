#include "dstr.h"

typedef enum token_type {
    DEF,
    DO,
    IF,
    ELSE,
    IMPORT,
    RETURN,
    THEN,
    VAL,
    VAR,
    WHILE,
    CASE,
    CLASS,
    EXTENSION,
    FOR,
    LIST,
    MATCH,
    TYPE,
    //
    INT,
    DOUBLE,
    STRING,
    BOOLEAN,
    UNIT,
    //
    NEW_LINE,
    EOF,
    INDENT,
    //
    PLUS,
    UPLUS,
    MINUS,
    UMINUS,
    STAR,
    SLASH,
    //
    LPAREN,
    RPAREN,
    //
    HASHTAG,
    MAIN,
    //
    LOWER_THAN,
    LOWER_EQ_THAN,
    HIGHER_THAN,
    HIGHER_EQ_THAN,
    EQUALS,
    NOT_EQUALS,
} token_t;

struct token {
    token_t type;
    union {
        bool as_bool;
        dstr as_str;
        int as_int;
        double as_double;
    } value;
};
