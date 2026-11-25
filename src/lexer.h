/**
 * @file lexer.h
 *
 * @author Jakub Manoušek a Vojta Buchmaier
 * @brief Modul pro lexikální analyzér
 *
 * @details
 */
#ifndef LEXER_H
#define LEXER_H

#include "dstr.h"
#include <stdbool.h>
#include <stdio.h>

/**
 * @struct lexer
 * @brief Struktura reprezentujici vnitrni stav lexikalniho analyzatoru.
 */
typedef struct lexer lexer;

/**
 * @enum keyword
 * @brief Vycet vsech klicovych slov a vestavenych datovych typu jazyka IFJ26.
 */
typedef enum keyword {
    KW_DEF,
    KW_IF,
    KW_THEN,
    KW_ELSE,
    KW_WHILE,
    KW_DO,
    KW_VAL,
    KW_VAR,
    KW_RETURN,
    KW_IMPORT,
    // Maybe not needed?
    KW_CASE,
    KW_CLASS,
    KW_EXTENSION,
    KW_FOR,
    KW_MATCH,
    KW_PRINT,
    KW_TYPE,
    KW_MAIN,
    //
    KW_INT,
    KW_DOUBLE,
    KW_STRING,
    KW_BOOLEAN,
    KW_UNIT,
    KW_LIST,
} keyword;

/**
 * @enum token_type
 * @brief Vycet vsech typu lexikalniho tokenu rozpoznavanneho scannerem.
 */
typedef enum token_type {
    TT_KEYWORD,
    TT_IDENTIFIER,

    // Literaly
    TT_INT_LITERAL,
    TT_DOUBLE_LITERAL,
    TT_STRING_LITERAL,
    TT_BOOLEAN_LITERAL,
    TT_UNIT_LITERAL,

    // Operátory
    TT_PLUS,
    TT_MINUS,
    TT_STAR,
    TT_SLASH,
    TT_EQ,
    TT_NEQ,
    TT_LT,
    TT_LTE,
    TT_GT,
    TT_GTE,

    TT_ASSIGN,

    // Separátory
    TT_COLON,
    TT_COMMA,
    TT_DOT, 
    TT_SEMICOLON,
    TT_LPAREN,
    TT_RPAREN,
    TT_AND,
    TT_OR,
    TT_NOT,

    TT_AT,
    //
    TT_INDENT,
    TT_DEDENT,
    TT_EOF,
    TT_EOL,
} token_type;

/**
 * @union token_attr
 * @brief Pametove usporna unie pro ulozeni konkretni hodnoty tokenu.
 */
typedef union {
    bool as_bool;
    dstr as_str;
    keyword as_keyword;
    int as_int;
    double as_double;
} token_attr;

/**
 * @struct token
 * @brief Struktura reprezentujici jednu lexikalni token predavanou parseru.
 */
typedef struct token {
    token_type type;
    token_attr attr;
    int line;
    int col;
} token;

/** Maximalni pocet tokenu, o ktere lze nahlednout dopredu pomoci lexer_peek. */
#define LEXER_MAX_PEEK 2

/**
 * @brief Vytvori a inicializuje novou instanci lexikalniho analyzatoru.
 *
 * @param in Ukazatel na otevreny vstupni soubor se zdrojovym kodem.
 * @return Ukazatel na alokovanou strukturu lexer, nebo NULL pri chybe alokace.
 */
lexer *lexer_create(FILE *in);

/**
 * @brief Uvolni veskerou pamet alokovanou pro lexikalni analyzator.
 *
 * @param lx Ukazatel na instanci lexikalniho analyzatoru.
 */
void lexer_destroy(lexer *lx);

/**
 * @brief Nacte a vrati nasledujici token ze vstupu (posune cteni dopredu).
 *
 * @param lx Ukazatel na instanci lexikalniho analyzatoru.
 * @return Nactena struktura token.
 */
token get_next_token(lexer *lx);

/**
 * @brief Nahledne na n-ty nasledujici token
 *
 * @param lx Ukazatel na instanci lexikalniho analyzatoru.
 * @param n Pocet tokenu dopredu.
 * @return Ukazatel na nahlednuty token ve vnitrni vyrovnavaci pameti lexeru.
 */
const token *lexer_peek(lexer *lx, unsigned n);

/**
 * @brief Uvolni dynamicky alokovanou pamet uvnitr tokenu.
 *
 * @param tok Ukazatel na token, jehoz vnitrni zdroje maji byt uvolneny.
 */
void token_free(token *tok);

/**
 * @brief Prevede typ tokenu na jeho textovou reprezentaci
 * @param type Typ tokenu z vyctu token_type.
 * @return Konstantni textovy retezec s nazvem typu tokenu.
 */
const char *token_type_name(token_type type);

/**
 * @brief Overi, zda je dany token konkretnim klicovym slovem.
 *
 * @param tok Ukazatel na testovany token.
 * @param kw Ocekavane klicove slovo z vyctu keyword.
 * @return true pokud jde o token typu TT_KEYWORD se shodnym klicovym slovem,
 * jinak false.
 */
bool token_is_kw(const token *tok, keyword kw);

#endif
