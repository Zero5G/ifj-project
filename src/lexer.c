/**
 * @file lexer.c
 *
 * @author Vojta Buchmaier
 * @brief Lexikální analyzér
 *
 * @details
 */

#include "lexer.h"
#include "dstr.h"
#include "error.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>

//definice stavu fsm
typedef enum state
{
    S_START,
    S_SLASH,
    S_COMMENT_L,
    S_C_MLS,
    S_C_MLE,
    S_C_MLE2,
    S_C_ML_SLASH,
    S_NUMBER,
    S_DOUBLE,
    S_INTEGER,
    S_K_I,
    S_STRING_S,
    S_STRING,
    S_STRING_ES,
    S_STRING_E,
    S_S_ML,
    S_S_ML_E,
    S_S_ML_E2,
    S_S_ML_E3,
    S_ASSIGN,
    S_EQUALS,
    S_COMMA,
    S_STAR,
    S_NEQ,
    S_NEQ2,
    S_LPAREN,
    S_RPAREN,
    S_LT,
    S_MINUS,
    S_PLUS,
    S_LTE,
    S_DOT,
    S_GT,
    S_GTE,
    S_COLON,
    S_AND,
    S_OR,
} state;

struct lexer
{
    FILE *in;
    int line;
    int col;
    token peek_arr[LEXER_MAX_PEEK];
    unsigned peek_count;
    int indent_stack[256];
    int indent_top;
    int pending_dedents;
    bool at_line_start;
    int paren_depth;

    int nl_col;
    int pushback[4];
    unsigned pushback_count;
};

//pomocna struktura pro hledani klice
typedef struct
{
    char *word;
    keyword kw;
} keys;

//Vyhledavaci tabulka vsech rezervovanych klicovych slov a vestavenych datovych typu
static const keys KEYWORD_TABLE[] = {
    {"def", KW_DEF},
    {"if", KW_IF},
    {"then", KW_THEN},
    {"else", KW_ELSE},
    {"while", KW_WHILE},
    {"do", KW_DO},
    {"val", KW_VAL},
    {"var", KW_VAR},
    {"return", KW_RETURN},
    {"import", KW_IMPORT},
    // Rozsireni
    {"case", KW_CASE},
    {"class", KW_CLASS},
    {"extension", KW_EXTENSION},
    {"for", KW_FOR},
    {"match", KW_MATCH},
    {"print", KW_PRINT},
    {"type", KW_TYPE},
    {"main", KW_MAIN},
    // Datove typy
    {"Int", KW_INT},
    {"Double", KW_DOUBLE},
    {"String", KW_STRING},
    {"Boolean", KW_BOOLEAN},
    {"Unit", KW_UNIT},
    {"List", KW_LIST},
};

/**
    Overi, zda nactene slovo v pole odpovida
    klicovemu slovu z KEYWORD_TABLE. Pokud ano, ulozi jeho typ do 'out_kw' a vrati true.
*/
static bool check_keyword(dstr pole, keyword *out_kw)
{
    size_t count = sizeof(KEYWORD_TABLE) / sizeof(KEYWORD_TABLE[0]);
    for (size_t i = 0; i < count; i++)
    {
        if (dstr_cmp_raw(pole, KEYWORD_TABLE[i].word) == 0)
        {
            *out_kw = KEYWORD_TABLE[i].kw;
            return true;
        }
    }
    return false;
}

static double prevod_double(dstr pole)
{
    errno = 0;
    double hodnota = strtod(dstr_cstr(pole), NULL);
    if (isinf(hodnota) || (hodnota == 0.0 && errno == ERANGE))
    {
        dstr_delete(pole);
        exit(1);
    }
    return hodnota;
}
static int next_char(lexer *lx)
{
    int c;
    if (lx->pushback_count > 0)
    {
        lx->pushback_count -= 1;
        c = lx->pushback[lx->pushback_count];
    }
    else
    {
        c = fgetc(lx->in);
    }
    if (c == '\n')
    {
        lx->nl_col = lx->col;
        lx->line += 1;
        lx->col = 1;
    }
    else if (c != EOF)
    {
        lx->col += 1;
    }
    return c;
}

static void back_char(lexer *lx, int c)
{
    if (c == EOF)
        return;
    lx->pushback[lx->pushback_count] = c;
    lx->pushback_count += 1;
    if (c == '\n')
    {
        lx->line -= 1;
        lx->col = lx->nl_col;
    }
    else
    {
        lx->col -= 1;
    }
}
// Cte vstupni soubor znak po znaku, hlida odsazeni a vraci vzdy jeden kompletni
// token. Pri lexikalni chybe ukonci program s navratovym kodem 1.

//Cte vstupni soubor znak po znaku, hlida odsazeni a vraci vzdy jeden kompletni token. Pri lexikalni chybe ukonci program s navratovym kodem 1.
static token read_token_fsm(lexer *lx)
{
    token tok = {0};
    dstr str = dstr_create("");
    enum state current_state = S_START;
    bool after_underscore = false;
    int comment_depth = 0;
    while (true)
    {
        if (lx->pending_dedents > 0)
        {
            lx->pending_dedents -= 1;
            tok.type = TT_OUTDENT;
            dstr_delete(str);
            return tok;
        }

        if (lx->at_line_start == true)
        {
            if (lx->paren_depth > 0)
            {
                lx->at_line_start = false;
            }
            else
            {
                int space_count = 0;
                int c;
                bool tab = false;
                bool after_comment = false;

                while (true)
                {
                    c = next_char(lx);
                    if (c == ' ')
                    {
                        if (after_comment == false)
                        {
                            space_count += 1;
                        }
                    }
                    else if (c == '\t')
                    {
                        tab = true; // chyba jen pokud radek neni prazdny
                    }
                    else if (c == '\n')
                    {
                        space_count = 0;
                        tab = false;
                        after_comment = false;
                    }
                    else if (c == '/')
                    {
                        int next_chr = next_char(lx);
                        if (next_chr == '/')
                        {
                            // radkovy komentar na jinak prazdnem radku preskocime az do konce radku
                            int k = next_char(lx);
                            while (k != '\n' && k != EOF)
                            {
                                k = next_char(lx);
                            }
                            if (k == '\n')
                            {
                                space_count = 0;
                                tab = false;
                                after_comment = false;
                            }
                            else
                            {
                                c = EOF;
                                break;
                            }
                        }
                        else if (next_chr == '*')
                        {
                            // blokovy komentar na zacatku radku preskocime az po '*/'
                            int depth = 1; // komentare mohou byt vnorene
                            int k = next_char(lx);
                            while (k != EOF)
                            {
                                if (k == '\n')
                                {
                                    space_count = 0; //novy radek uvnitr komentare
                                    tab = false;
                                }
                                else if (k == '*')
                                {
                                    int after_star = next_char(lx);
                                    if (after_star == '/')
                                    {
                                        depth -= 1;
                                        if (depth == 0)
                                        {
                                            break;
                                        }
                                    }
                                    else
                                    {
                                        back_char(lx, after_star);
                                    }
                                }
                                else if (k == '/')
                                {
                                    int after_slash = next_char(lx);
                                    if (after_slash == '*')
                                    {
                                        depth += 1;
                                    }
                                    else
                                    {
                                        back_char(lx, after_slash);
                                    }
                                }
                                k = next_char(lx);
                            }
                            after_comment = true; // mezery za komentarem se do odsazeni nepocitaji
                            if (k == EOF)
                            {
                                dstr_delete(str);
                                exit(1); // neukonceny komentar
                            }
                        }
                        else
                        {
                            // nebyl to komentar, ale deleni '/'
                            if (next_chr != EOF)
                            {
                                back_char(lx, next_chr);
                            }
                            break;
                        }
                    }
                    else
                    {
                        // prvni platny znak
                        break;
                    }
                }

                lx->at_line_start = false;

                // tabulator na zacatku neprazdneho radku
                if (tab == true && c != EOF)
                {
                    dstr_delete(str);
                    exit(1);
                }

                // Konec souboru musime uzavrit vsechny otevrene urovne odsazeni
                if (c == EOF)
                {
                    if (lx->indent_top > 0)
                    {
                        lx->indent_top -= 1;
                        tok.type = TT_OUTDENT;
                        dstr_delete(str);
                        return tok;
                    }
                    else
                    {
                        tok.type = TT_EOF;
                        dstr_delete(str);
                        return tok;
                    }
                }

                // Vratime prvni znak kodu zpet na vstup, aby si ho precetl stav S_START
                back_char(lx, c);

                int top = lx->indent_stack[lx->indent_top];

                if (space_count > top)
                {
                    // Zvetseni odsazeni -> novy blok (TT_INDENT)
                    lx->indent_top += 1;
                    lx->indent_stack[lx->indent_top] = space_count;
                    tok.line = lx->line;
                    tok.col = lx->col;
                    tok.type = TT_INDENT;
                    dstr_delete(str);
                    return tok;
                }
                else if (space_count < top)
                {
                    // Zmenseni odsazeni -> ukonceni jednoho ci vice bloku (TT_DEDENT)
                    int after_outdent = 0;
                    while (lx->indent_top > 0 && lx->indent_stack[lx->indent_top] > space_count)
                    {
                        lx->indent_top -= 1;
                        after_outdent += 1;
                    }

                    // Odsazeni musi presne odpovidat nektere predchozi urovni na zasobniku
                    if (lx->indent_stack[lx->indent_top] != space_count)
                    {
                        dstr_delete(str);
                        exit(1);
                    }

                    lx->pending_dedents = after_outdent - 1;
                    tok.line = lx->line;
                    tok.col = lx->col;
                    tok.type = TT_OUTDENT;
                    dstr_delete(str);
                    return tok;
                }
            }
        }

        int chr = next_char(lx);
        switch (current_state)
        {
            case S_START:
            {
                tok.line = lx->line;
                tok.col = lx->col - 1;
                if (chr == ' ' || chr == '\t')
                {
                    break;
                }
                else if (chr == '@')
                {
                    tok.type = TT_AT;
                    dstr_delete(str);
                    return tok;
                }
                else if (chr == '&')
                {
                    current_state = S_AND;
                }
                else if (chr == '|')
                {
                    current_state = S_OR;
                }
                else if (chr == '\n')
                {
                    if (lx->paren_depth > 0)
                    {
                        break;
                    }
                    else
                    {
                        lx->at_line_start = true;

                        tok.type = TT_EOL;
                        dstr_delete(str);
                        return tok;
                    }
                }
                else if (chr == EOF)
                {
                    if (lx->indent_top > 0)
                    {
                        lx->indent_top -= 1;
                        tok.type = TT_OUTDENT;
                        dstr_delete(str);
                        return tok;
                    }
                    else
                    {
                        tok.type = TT_EOF;
                        dstr_delete(str);
                        return tok;
                    }
                }
                else if (isalpha(chr) || (chr == '_'))
                {
                    current_state = S_K_I;
                    dstr_push_char(str, chr);
                }
                else if (isdigit(chr))
                {
                    current_state = S_NUMBER;
                    dstr_push_char(str, chr);
                }
                else if (chr == '/')
                {
                    current_state = S_SLASH;
                }
                else if (chr == '"')
                {
                    current_state = S_STRING_S;
                    //dstr_push_char(pole,znak);
                }
                else if (chr == '=')
                {
                    current_state = S_ASSIGN;
                    dstr_push_char(str, chr);
                }
                else if (chr == ',')
                {
                    tok.type = TT_COMMA;
                    dstr_delete(str);
                    return tok;
                }
                else if (chr == '*')
                {
                    tok.type = TT_STAR;
                    dstr_delete(str);
                    return tok;
                }
                else if (chr == '!')
                {
                    current_state = S_NEQ;
                    dstr_push_char(str, chr);
                }
                else if (chr == '(')
                {
                    lx->paren_depth += 1;
                    tok.type = TT_LPAREN;
                    dstr_delete(str);
                    return tok;
                }
                else if (chr == ')')
                {
                    if (lx->paren_depth > 0)
                    {
                        lx->paren_depth -= 1;
                    }
                    tok.type = TT_RPAREN;
                    dstr_delete(str);
                    return tok;
                }
                else if (chr == '-')
                {
                    tok.type = TT_MINUS;
                    dstr_delete(str);
                    return tok;
                }
                else if (chr == '<')
                {
                    current_state = S_LT;
                    dstr_push_char(str, chr);
                }
                else if (chr == '>')
                {
                    current_state = S_GT;
                    dstr_push_char(str, chr);
                }
                else if (chr == '+')
                {
                    tok.type = TT_PLUS;
                    dstr_delete(str);
                    return tok;
                }
                else if (chr == '.')
                {
                    int dalsi = next_char(lx);
                    back_char(lx, dalsi);
                    if (isdigit(dalsi)) // desetinny literal bez cele casti, napr. .5
                    {
                        current_state = S_DOUBLE;
                        dstr_push_char(str, chr);
                    }
                    else
                    {
                        tok.type = TT_DOT;
                        dstr_delete(str);
                        return tok;
                    }
                }
                else if (chr == ';')
                {
                    tok.type = TT_SEMICOLON;
                    dstr_delete(str);
                    return tok;
                }
                else if (chr == ':')
                {
                    tok.type = TT_COLON;
                    dstr_delete(str);
                    return tok;
                }
                else
                {
                    dstr_delete(str);
                    exit(1);
                }
                break;
            }
            case S_K_I:
            {
                if (isalpha(chr) || (chr == '_') || (isdigit(chr)))
                {
                    dstr_push_char(str, chr);
                    break;
                }
                else
                {
                    back_char(lx, chr);
                    //je to bool
                    if (dstr_cmp_raw(str, "true") == 0)
                    {
                        tok.type = TT_BOOLEAN_LITERAL;
                        tok.attr.as_bool = true;
                        dstr_delete(str); //boolen nepotrebuje text
                        return tok;
                    }
                    //je to bool
                    else if (dstr_cmp_raw(str, "false") == 0)
                    {
                        tok.type = TT_BOOLEAN_LITERAL;
                        tok.attr.as_bool = false;
                        dstr_delete(str); //boolen nepotrebuje text
                        return tok;
                    }
                    //porovnani se vsema slovama key jako def,if int ....
                    else if (check_keyword(str, &tok.attr.as_keyword) == true)
                    {
                        tok.type = TT_KEYWORD;
                        dstr_delete(str);
                        return tok;
                    }
                    //lexikalni chyba
                    else if (dstr_cmp_raw(str, "_") == 0)
                    {
                        dstr_delete(str);
                        exit(1);
                    }
                    //je to identifikator promenna ......
                    else
                    {
                        tok.type = TT_IDENTIFIER;
                        tok.attr.as_str = str;
                        return tok;
                    }
                }
            }
            case S_ASSIGN:
            {
                if (chr == '=')
                {
                    dstr_delete(str);
                    tok.type = TT_EQ;
                    return tok;
                }
                else
                {
                    back_char(lx, chr);
                    tok.type = TT_ASSIGN;
                    dstr_delete(str);
                    return tok;
                }
            }
            case S_LT:
            {
                if (chr == '=')
                {
                    dstr_delete(str);
                    tok.type = TT_LTE;
                    return tok;
                }
                else
                {
                    back_char(lx, chr);
                    tok.type = TT_LT;
                    dstr_delete(str);
                    return tok;
                }
            }
            case S_GT:
            {
                if (chr == '=')
                {
                    dstr_delete(str);
                    tok.type = TT_GTE;
                    return tok;
                }
                else
                {
                    back_char(lx, chr);
                    tok.type = TT_GT;
                    dstr_delete(str);
                    return tok;
                }
            }
            case S_NEQ:
            {
                if (chr == '=')
                {
                    dstr_delete(str);
                    tok.type = TT_NEQ;
                    return tok;
                }
                //unarni negace
                else
                {
                    back_char(lx, chr);
                    tok.type = TT_NOT;
                    dstr_delete(str);
                    return tok;
                }
            }
            case S_AND:
            {
                if (chr == '&')
                {
                    tok.type = TT_AND;
                    dstr_delete(str);
                    return tok;
                }
                else
                {
                    dstr_delete(str);
                    exit(1);
                }
            }
            case S_OR:
            {
                if (chr == '|')
                {
                    dstr_delete(str);
                    tok.type = TT_OR;
                    return tok;
                }
                else
                {
                    dstr_delete(str);
                    exit(1);
                }
            }
            case S_SLASH:
            {
                if (chr == '/')
                {
                    current_state = S_COMMENT_L;
                    break;
                }
                if (chr == '*')
                {
                    comment_depth = 1;
                    current_state = S_C_MLS;
                    break;
                }
                else //neni to komentar
                {
                    back_char(lx, chr);
                    tok.type = TT_SLASH;
                    dstr_delete(str);
                    return tok;
                }
            }
            case S_COMMENT_L:
            {
                if ((chr == '\n') || (chr == EOF))
                {
                    back_char(lx, chr);
                    current_state = S_START;
                    break;
                }
                else
                {
                    break;
                }
            }
            case S_C_MLS:
            {
                if (chr == '*')
                {
                    current_state = S_C_MLE;
                    break;
                }
                else if (chr == '/')
                {
                    current_state = S_C_ML_SLASH;
                    break;
                }
                else if (chr == EOF)
                {
                    dstr_delete(str);
                    exit(1);
                }
                else
                {
                    current_state = S_C_MLS;
                    break;
                }
            }
            case S_C_ML_SLASH: // '/' uvnitr komentare, muze zacinat vnoreny komentar
            {
                if (chr == '*')
                {
                    comment_depth += 1;
                    current_state = S_C_MLS;
                    break;
                }
                else if (chr == '/')
                {
                    break;
                }
                else if (chr == EOF)
                {
                    dstr_delete(str);
                    exit(1);
                }
                else
                {
                    current_state = S_C_MLS;
                    break;
                }
            }
            case S_C_MLE:
            {
                if (chr == '/')
                {
                    comment_depth -= 1;
                    current_state = (comment_depth == 0) ? S_C_MLE2 : S_C_MLS;
                    break;
                }
                else if (chr == '*')
                {
                    break;
                }
                else
                {
                    current_state = S_C_MLS;
                    break;
                }
            }
            case S_C_MLE2:
            {
                back_char(lx, chr);
                current_state = S_START;
                break;
            }
            case S_NUMBER:
            {
                if (chr == '.')
                {
                    current_state = S_DOUBLE;
                    dstr_push_char(str, chr);
                    break;
                }
                else if (isdigit(chr))
                {
                    current_state = S_INTEGER;
                    dstr_push_char(str, chr);
                    break;
                }
                else if (chr == '_')
                {
                    after_underscore = true;
                    current_state = S_INTEGER;
                    break;
                }
                else if (chr == 'e' || chr == 'E')
                {
                    current_state = S_DOUBLE;
                    dstr_push_char(str, chr);
                    break;
                }
                else
                {
                    if (isalpha(chr) || chr == '_')
                    {
                        dstr_delete(str);
                        exit(1); //nemuze byt 7adfdsfds
                    }
                    back_char(lx, chr);
                    tok.attr.as_int = strtol(dstr_cstr(str), NULL, 10);
                    tok.type = TT_INT_LITERAL;
                    dstr_delete(str);
                    return tok;
                }
            }
            case S_INTEGER:
            {
                if (isdigit(chr))
                {
                    dstr_push_char(str, chr);
                    after_underscore = false;
                    break;
                }
                else if (chr == '_')
                {
                    if (after_underscore == true)
                    {
                        dstr_delete(str);
                        exit(1);
                    }
                    after_underscore = true;
                    break;
                }
                else if (chr == '.')
                {
                    if (after_underscore == true)
                    {
                        dstr_delete(str);
                        exit(1); //chyba pro 152_.44
                    }
                    else
                    {
                        current_state = S_DOUBLE;
                        dstr_push_char(str, chr);
                        break;
                    }
                }
                else if (chr == 'e' || chr == 'E')
                {
                    if (after_underscore == true)
                    {
                        dstr_delete(str);
                        exit(1); //chyba pro 152_.44
                    }
                    else
                    {
                        current_state = S_DOUBLE;
                        dstr_push_char(str, chr);
                        break;
                    }
                }
                else
                {
                    if (isalpha(chr) || chr == '_')
                    {
                        dstr_delete(str);
                        exit(1); //nemuze byt 7adfdsfds
                    }

                    current_state = S_START;
                    if ((dstr_cstr(str)[0] == '0') || (after_underscore == true))
                    {
                        dstr_delete(str);
                        exit(1);
                    }
                    back_char(lx, chr); //vraceni neznameho znaku
                    tok.type = TT_INT_LITERAL;
                    tok.attr.as_int = (int)strtol(dstr_cstr(str), NULL, 10);
                    dstr_delete(str);
                    return tok;
                }
            }
            case S_DOUBLE:
            {
                if (isdigit(chr))
                {
                    after_underscore = false;
                    dstr_push_char(str, chr);
                    break;
                }
                else if (chr == '_')
                {
                    char posledni = dstr_cstr(str)[dstr_len(str) - 1];
                    if ((after_underscore == true) || !isdigit(posledni))
                    {
                        dstr_delete(str);
                        exit(1);
                    }
                    after_underscore = true;
                    break;
                }
                else if (chr == 'e' || chr == 'E')
                {
                    char last_chr = dstr_cstr(str)[dstr_len(str) - 1];
                    if ((after_underscore == true) || !isdigit(last_chr) ||
                        (strpbrk(dstr_cstr(str), "eE") != NULL))
                    {
                        dstr_delete(str);
                        exit(1); // chyba: 1.e5 nebo 1.5_e2 nebo 1e2e3
                    }
                    dstr_push_char(str, chr);
                    break;
                }
                else if (chr == '+' || chr == '-')
                {
                    char last_chr = dstr_cstr(str)[dstr_len(str) - 1];
                    if (last_chr != 'e' && last_chr != 'E')
                    {
                        // + nebo - neni soucasti exponentu -> cislo konci, vratime znak jako operator + nebo -
                        if ((last_chr == '.') || (last_chr == '+') || (last_chr == '-') || (after_underscore == true))
                        {
                            dstr_delete(str);
                            exit(1);
                        }
                        else
                        {
                            current_state = S_START;
                            back_char(lx, chr);
                            tok.type = TT_DOUBLE_LITERAL;
                            tok.attr.as_double = prevod_double(str);
                            dstr_delete(str);
                            return tok;
                        }
                    }
                    else
                    {
                        dstr_push_char(str, chr);
                        break;
                    }
                }
                else if (chr == 'd' || chr == 'D') // nepovinna pripona
                {
                    char last_chr = dstr_cstr(str)[dstr_len(str) - 1];
                    int after_suffix = next_char(lx);
                    back_char(lx, after_suffix);
                    if (!isdigit(last_chr) || (after_underscore == true) ||
                        isalnum(after_suffix) || after_suffix == '_')
                    {
                        dstr_delete(str);
                        exit(1);
                    }
                    tok.type = TT_DOUBLE_LITERAL;
                    tok.attr.as_double = prevod_double(str);
                    dstr_delete(str);
                    return tok;
                }
                else
                {
                    char posledni = dstr_cstr(str)[dstr_len(str) - 1];
                    if ((posledni == '.') || (posledni == 'e') || (posledni == 'E') ||
                        (posledni == '+') || (posledni == '-') || (after_underscore == true))
                    {
                        dstr_delete(str);
                        exit(1);
                    }
                    current_state = S_START;
                    back_char(lx, chr); //vraceni neznameho znaku
                    tok.type = TT_DOUBLE_LITERAL;
                    tok.attr.as_double = prevod_double(str);
                    dstr_delete(str);
                    return tok;
                }
            }
            case S_STRING_S:
            {
                if (chr == '"')
                {
                    current_state = S_STRING_E;
                    break;
                }
                else if (chr == '\n' || chr == EOF)
                {
                    dstr_delete(str);
                    exit(1);
                }
                else if (chr == '\\')
                {
                    current_state = S_STRING_ES;
                    break;
                }
                else
                {
                    current_state = S_STRING;
                    dstr_push_char(str, chr);
                    break;
                }
            }
            case S_STRING:
            {
                if (chr == '"')
                {
                    current_state = S_STRING_E;
                    break;
                }
                else if (chr == '\\')
                {
                    current_state = S_STRING_ES;
                    break;
                }
                else if (chr == '\n' || chr == EOF)
                {
                    dstr_delete(str);
                    exit(1);
                }
                else
                {
                    dstr_push_char(str, chr);
                    break;
                }
            }
            case S_STRING_ES:
            {
                if (chr == 'n')
                {
                    dstr_push_char(str, '\n');
                    current_state = S_STRING;
                    break;
                }
                else if (chr == 't')
                {
                    dstr_push_char(str, '\t');
                    current_state = S_STRING;
                    break;
                }
                else if (chr == 'r')
                {
                    dstr_push_char(str, '\r');
                    current_state = S_STRING;
                    break;
                }
                else if (chr == '"')
                {
                    dstr_push_char(str, chr);
                    current_state = S_STRING;
                    break;
                }
                else if (chr == '\\')
                {
                    dstr_push_char(str, chr);
                    current_state = S_STRING;
                    break;
                }
                else
                {
                    dstr_delete(str);
                    exit(1);
                }
            }
            case S_STRING_E:
            {
                if ((chr == '"') && (dstr_len(str) == 0))
                {
                    current_state = S_S_ML;
                    break;
                }
                else
                {
                    tok.type = TT_STRING_LITERAL;
                    back_char(lx, chr);
                    tok.attr.as_str = str;
                    return tok;
                }
            }
            case S_S_ML:
            {
                if (chr == '"')
                {
                    current_state = S_S_ML_E;
                    break;
                }
                else if (chr == EOF)
                {
                    dstr_delete(str);
                    exit(1);
                }
                else
                {
                    dstr_push_char(str, chr);
                    current_state = S_S_ML;
                    break;
                }
            }
            case S_S_ML_E:
            {
                if (chr == '"')
                {
                    current_state = S_S_ML_E2;
                    break;
                }
                else if (chr == EOF)
                {
                    dstr_delete(str);
                    exit(1);
                }
                else
                {
                    dstr_push_char(str, '"');
                    dstr_push_char(str, chr);
                    current_state = S_S_ML;
                    break;
                }
            }
            case S_S_ML_E2:
            {
                if (chr == '"')
                {
                    current_state = S_S_ML_E3;
                    break;
                }
                else if (chr == EOF)
                {
                    dstr_delete(str);
                    error_fatal_cleanup(ERR_LEX);
                }
                else
                {
                    dstr_push_char(str, '"');
                    dstr_push_char(str, '"');
                    dstr_push_char(str, chr);
                    current_state = S_S_ML;
                    break;
                }
            }
            case S_S_ML_E3:
            {
                if (chr == '"') // vic uvozovek na konci: posledni tri ukoncuji, zbytek patri do obsahu
                {
                    dstr_push_char(str, '"');
                    break;
                }
                back_char(lx, chr); //vraceni neznameho znaku
                tok.type = TT_STRING_LITERAL;
                tok.attr.as_str = str;
                return tok;
            }
            default:
                break;
        }
    }
}

bool token_is_kw(const token *tok, keyword kw)
{
    if (tok == NULL)
    {
        return false;
    }
    if ((tok->type == TT_KEYWORD) && (tok->attr.as_keyword == kw))
    {
        return true;
    }
    return false;
}

void token_free(token *tok)
{
    if (tok == NULL)
    {
        return;
    }

    if ((tok->type == TT_IDENTIFIER) || (tok->type == TT_STRING_LITERAL))
    {
        if (tok->attr.as_str != NULL)
        {
            dstr_delete(tok->attr.as_str);
            tok->attr.as_str = NULL;
        }
    }
}

/**
    Prevede ciselny typ tokenu
    na citelny textovy retezec pro vypis chyb a logovani.
*/
const char *token_type_name(token_type type)
{
    switch (type)
    {
        case TT_KEYWORD:
            return "TT_KEYWORD";
        case TT_IDENTIFIER:
            return "TT_IDENTIFIER";

        // Literaly
        case TT_INT_LITERAL:
            return "TT_INT_LITERAL";
        case TT_DOUBLE_LITERAL:
            return "TT_DOUBLE_LITERAL";
        case TT_STRING_LITERAL:
            return "TT_STRING_LITERAL";
        case TT_BOOLEAN_LITERAL:
            return "TT_BOOLEAN_LITERAL";
        case TT_UNIT_LITERAL:
            return "TT_UNIT_LITERAL";

        // Operatory
        case TT_PLUS:
            return "TT_PLUS";
        case TT_MINUS:
            return "TT_MINUS";
        case TT_STAR:
            return "TT_STAR";
        case TT_SLASH:
            return "TT_SLASH";
        case TT_EQ:
            return "TT_EQ";
        case TT_NEQ:
            return "TT_NEQ";
        case TT_LT:
            return "TT_LT";
        case TT_LTE:
            return "TT_LTE";
        case TT_GT:
            return "TT_GT";
        case TT_GTE:
            return "TT_GTE";

        case TT_ASSIGN:
            return "TT_ASSIGN";
        case TT_AND:
            return "TT_AND";
        case TT_OR:
            return "TT_OR";
        case TT_NOT:
            return "TT_NOT";
        case TT_AT:
            return "TT_AT";

        // Separatory
        case TT_COLON:
            return "TT_COLON";
        case TT_COMMA:
            return "TT_COMMA";
        case TT_DOT:
            return "TT_DOT";
        case TT_SEMICOLON:
            return "TT_SEMICOLON";
        case TT_LPAREN:
            return "TT_LPAREN";
        case TT_RPAREN:
            return "TT_RPAREN";

        // Rizeni bloku a radku
        case TT_INDENT:
            return "TT_INDENT";
        case TT_OUTDENT:
            return "TT_DEDENT";
        case TT_EOF:
            return "TT_EOF";
        case TT_EOL:
            return "TT_EOL";

        default:
            return "UNKNOWN";
    }
}

lexer *lexer_create(FILE *in)
{
    if (in == NULL)
    {
        return NULL;
    }

    lexer *lx = malloc(sizeof(struct lexer));
    if (lx == NULL)
    {
        return NULL;
    }
    lx->indent_stack[0] = 0;
    lx->indent_top = 0;
    lx->pending_dedents = 0;
    lx->at_line_start = true;
    lx->paren_depth = 0;
    lx->in = in;
    lx->line = 1;
    lx->col = 1;
    lx->peek_count = 0;
    lx->nl_col = 1;
    lx->pushback_count = 0;
    return lx;
}

void lexer_destroy(lexer *lx)
{
    if (lx == NULL)
    {
        return;
    }
    for (unsigned i = 0; i < lx->peek_count; i++)
    {
        token_free(&lx->peek_arr[i]);
    }
    free(lx);
}

//zobrazeni tokenu do predu
const token *lexer_peek(lexer *lx, unsigned n)
{
    if (((lx == NULL) || (n < 1) || (n > LEXER_MAX_PEEK)))
    {
        return NULL;
    }
    while (lx->peek_count < n)
    {
        lx->peek_arr[lx->peek_count] = read_token_fsm(lx);
        lx->peek_count += 1;
    }
    return &lx->peek_arr[n - 1];
}
//hlavni verejna funkce pro parser. Vrati dalsi token na rade a posune cteni dal
token get_next_token(lexer *lx)
{
    if (lx == NULL)
    {
        token err = {.type = TT_EOF};
        return err;
    }
    token prvek_fronty;
    if (lx->peek_count > 0)
    {
        prvek_fronty = lx->peek_arr[0];

        if (lx->peek_count == 2)
        {
            lx->peek_arr[0] = lx->peek_arr[1];
        }
        lx->peek_count -= 1;
        return prvek_fronty;
    }
    //kdyz byla prazdna fronta
    return read_token_fsm(lx);
}
