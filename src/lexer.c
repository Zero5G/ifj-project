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
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// definice stavu fsm
typedef enum state {
    S_START,
    S_SLASH,
    S_COMMENT_L,
    S_C_MLS,
    S_C_LE,
    S_C_MLE,
    S_C_MLE2,
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

} state;

struct lexer {
    FILE *in;
    int line;
    int col;
    token pole_peek[LEXER_MAX_PEEK];
    unsigned peek_count;
    int indent_stack[256];
    int indent_top;
    int pending_dedents;
    bool at_line_start;
    int paren_depth;
};

// pomocna struktura pro hledani klice
typedef struct {
    char *word;
    keyword kw;
} keys;

// Vyhledavaci tabulka vsech rezervovanych klicovych slov a vestavenych datovych
// typu
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
 * Overi, zda nactene slovo v pole odpovida
 * klicovemu slovu z KEYWORD_TABLE. Pokud ano, ulozi jeho typ do 'out_kw' a
 * vrati true.
 */
static bool check_keyword(dstr pole, keyword *out_kw) {
    size_t count = sizeof(KEYWORD_TABLE) / sizeof(KEYWORD_TABLE[0]);
    for (size_t i = 0; i < count; i++) {
        if (dstr_cmp_raw(pole, KEYWORD_TABLE[i].word) == 0) {
            *out_kw = KEYWORD_TABLE[i].kw;
            return true;
        }
    }
    return false;
}

// Cte vstupni soubor znak po znaku, hlida odsazeni a vraci vzdy jeden kompletni
// token. Pri lexikalni chybe ukonci program s navratovym kodem 1.

static token read_token_fsm(lexer *lx) {
    token tok = {0};
    dstr pole = dstr_create("");
    enum state aktualni_stav = S_START;
    bool po_podtrzitku = false;

    while (true) {
        if (lx->pending_dedents > 0) {
            lx->pending_dedents -= 1;
            tok.type = TT_DEDENT;
            dstr_delete(pole);
            return tok;
        }

        if (lx->at_line_start == true) {
            if (lx->paren_depth > 0) {
                lx->at_line_start = false;
            } else {
                int mezery = 0;
                int c;

                while (true) {
                    c = fgetc(lx->in);
                    if (c == ' ') {
                        mezery += 1;
                    } else if (c == '\t') {
                        dstr_delete(pole);
                        exit(1);
                    } else if (c == '\n') {

                        mezery = 0;
                    } else if (c == '/') {
                        int dalsi_znak = fgetc(lx->in);
                        if (dalsi_znak == '/') {
                            // radkovy komentar na jinak prazdnem radku
                            // preskocime az do konce radku
                            int k = fgetc(lx->in);
                            while (k != '\n' && k != EOF) {
                                k = fgetc(lx->in);
                            }
                            if (k == '\n') {
                                mezery = 0;
                            } else {
                                c = EOF;
                                break;
                            }
                        } else {
                            // nebyl to komentar, ale deleni '/'
                            if (dalsi_znak != EOF) {
                                ungetc(dalsi_znak, lx->in);
                            }
                            break;
                        }
                    } else {
                        // prvni platny znak
                        break;
                    }
                }

                lx->at_line_start = false;

                // Konec souboru musime uzavrit vsechny otevrene urovne odsazeni
                if (c == EOF) {
                    if (lx->indent_top > 0) {
                        lx->indent_top -= 1;
                        tok.type = TT_DEDENT;
                        dstr_delete(pole);
                        return tok;
                    } else {
                        tok.type = TT_EOF;
                        dstr_delete(pole);
                        return tok;
                    }
                }

                // Vratime prvni znak kodu zpet na vstup, aby si ho precetl stav
                // S_START
                ungetc(c, lx->in);

                int vrchol = lx->indent_stack[lx->indent_top];

                if (mezery > vrchol) {
                    // Zvetseni odsazeni -> novy blok (TT_INDENT)
                    lx->indent_top += 1;
                    lx->indent_stack[lx->indent_top] = mezery;
                    tok.type = TT_INDENT;
                    dstr_delete(pole);
                    return tok;
                } else if (mezery < vrchol) {
                    // Zmenseni odsazeni -> ukonceni jednoho ci vice bloku
                    // (TT_DEDENT)
                    int pocet_dedentu = 0;
                    while (lx->indent_top > 0 &&
                           lx->indent_stack[lx->indent_top] > mezery) {
                        lx->indent_top -= 1;
                        pocet_dedentu += 1;
                    }

                    // Odsazeni musi presne odpovidat nektere predchozi urovni
                    // na zasobniku
                    if (lx->indent_stack[lx->indent_top] != mezery) {
                        dstr_delete(pole);
                        exit(1);
                    }

                    lx->pending_dedents = pocet_dedentu - 1;
                    tok.type = TT_DEDENT;
                    dstr_delete(pole);
                    return tok;
                }
            }
        }

        int znak = fgetc(lx->in);

        switch (aktualni_stav) {
        case S_START: {
            if (znak == ' ' || znak == '\t') {
                break;
            } else if (znak == '\n') {
                if (lx->paren_depth > 0) {
                    break;
                } else {
                    lx->at_line_start = true;

                    tok.type = TT_EOL;
                    dstr_delete(pole);
                    return tok;
                }

            } else if (znak == EOF) {
                if (lx->indent_top > 0) {
                    lx->indent_top -= 1;
                    tok.type = TT_DEDENT;
                    dstr_delete(pole);
                    return tok;
                } else {
                    tok.type = TT_EOF;
                    dstr_delete(pole);
                    return tok;
                }

            } else if (isalpha(znak) || (znak == '_')) {
                aktualni_stav = S_K_I;
                dstr_push_char(pole, znak);
            } else if (isdigit(znak)) {
                aktualni_stav = S_NUMBER;
                dstr_push_char(pole, znak);
            } else if (znak == '/') {
                aktualni_stav = S_SLASH;
                dstr_push_char(pole, znak);
            } else if (znak == '"') {
                aktualni_stav = S_STRING_S;
                // dstr_push_char(pole,znak);
            } else if (znak == '=') {
                aktualni_stav = S_ASSIGN;
                dstr_push_char(pole, znak);
            } else if (znak == ',') {
                tok.type = TT_COMMA;
                dstr_delete(pole);
                return tok;
            }

            else if (znak == '*') {
                tok.type = TT_STAR;
                dstr_delete(pole);
                return tok;
            } else if (znak == '!') {
                aktualni_stav = S_NEQ;
                dstr_push_char(pole, znak);
            } else if (znak == '(') {
                lx->paren_depth += 1;
                tok.type = TT_LPAREN;
                dstr_delete(pole);
                return tok;
            } else if (znak == ')') {
                if (lx->paren_depth > 0) {
                    lx->paren_depth -= 1;
                }
                tok.type = TT_RPAREN;
                dstr_delete(pole);
                return tok;
            } else if (znak == '-') {
                tok.type = TT_MINUS;
                dstr_delete(pole);
                return tok;
            } else if (znak == '<') {
                aktualni_stav = S_LT;
                dstr_push_char(pole, znak);
            } else if (znak == '>') {
                aktualni_stav = S_GT;
                dstr_push_char(pole, znak);
            } else if (znak == '+') {
                tok.type = TT_PLUS;
                dstr_delete(pole);
                return tok;

            } else if (znak == '.') {
                tok.type = TT_DOT;
                dstr_delete(pole);
                return tok;
            } else if (znak == ':') {
                tok.type = TT_COLON;
                dstr_delete(pole);
                return tok;

            }

            else {
                dstr_delete(pole);
                exit(1);
            }
            break;
        }
        case S_K_I: {
            if (isalpha(znak) || (znak == '_') || (isdigit(znak))) {

                dstr_push_char(pole, znak);
                break;
            } else {
                ungetc(znak, lx->in);

                // je to bool
                if (dstr_cmp_raw(pole, "true") == 0) {
                    tok.type = TT_BOOLEAN_LITERAL;
                    tok.attr.as_bool = true;
                    dstr_delete(pole); // boolen nepotrebuje text
                    return tok;
                }
                // je to bool
                else if (dstr_cmp_raw(pole, "false") == 0) {
                    tok.type = TT_BOOLEAN_LITERAL;
                    tok.attr.as_bool = false;
                    dstr_delete(pole); // boolen nepotrebuje text
                    return tok;
                }
                // porovnani se vsema slovama key jako def,if int ....
                else if (check_keyword(pole, &tok.attr.as_keyword) == true) {
                    tok.type = TT_KEYWORD;
                    dstr_delete(pole);
                    return tok;
                }
                // lexikalni chyba
                else if (dstr_cmp_raw(pole, "_") == 0) {
                    dstr_delete(pole);
                    exit(1);
                }
                // je to identifikator promenna ......
                else {
                    tok.type = TT_IDENTIFIER;
                    tok.attr.as_str = pole;
                    return tok;
                }
            }
        }
        case S_ASSIGN: {
            if (znak == '=') {
                dstr_delete(pole);
                tok.type = TT_EQ;
                return tok;

            } else {
                ungetc(znak, lx->in);
                tok.type = TT_ASSIGN;
                dstr_delete(pole);
                return tok;
            }
        }
        case S_LT: {
            if (znak == '=') {
                dstr_delete(pole);
                tok.type = TT_LTE;
                return tok;

            } else {
                ungetc(znak, lx->in);
                tok.type = TT_LT;
                dstr_delete(pole);
                return tok;
            }
        }
        case S_GT: {
            if (znak == '=') {
                dstr_delete(pole);
                tok.type = TT_GTE;
                return tok;

            } else {
                ungetc(znak, lx->in);
                tok.type = TT_GT;
                dstr_delete(pole);
                return tok;
            }
        }
        case S_NEQ: {
            if (znak == '=') {
                dstr_delete(pole);
                tok.type = TT_NEQ;
                return tok;

            }
            // samotny ! neni povolen
            else {
                dstr_delete(pole);
                exit(1);
            }
        }
        case S_SLASH: {
            if (znak == '/') {
                aktualni_stav = S_COMMENT_L;
                break;
            }
            if (znak == '*') {
                aktualni_stav = S_C_MLS;
                break;
            } else // neni to komentar
            {
                ungetc(znak, lx->in);
                tok.type = TT_SLASH;
                dstr_delete(pole);
                return tok;
            }
        }
        case S_COMMENT_L: {
            if ((znak == '\n') || (znak == EOF)) {
                aktualni_stav = S_C_LE;

                break;
            } else {
                break;
            }
        }
        case S_C_LE: {
            if (znak != EOF) {
                ungetc(znak, lx->in);
            }
            aktualni_stav = S_START;
            break;
        }
        case S_C_MLS: {
            if (znak == '*') {
                aktualni_stav = S_C_MLE;
                break;
            } else if (znak == EOF) {
                dstr_delete(pole);
                exit(1);
            } else {
                aktualni_stav = S_C_MLS;
                break;
            }
        }
        case S_C_MLE: {
            if (znak == '/') {
                aktualni_stav = S_C_MLE2;
                break;
            } else {
                aktualni_stav = S_C_MLS;
                break;
            }
        }
        case S_C_MLE2: {
            ungetc(znak, lx->in);
            aktualni_stav = S_START;
            break;
        }
        case S_NUMBER: {
            if (znak == '.') {
                aktualni_stav = S_DOUBLE;
                dstr_push_char(pole, znak);
                break;
            } else if (isdigit(znak)) {
                aktualni_stav = S_INTEGER;
                dstr_push_char(pole, znak);
                break;
            } else if (znak == '_') {

                po_podtrzitku = true;
                aktualni_stav = S_INTEGER;
                break;
            } else {
                ungetc(znak, lx->in);
                tok.attr.as_int = strtol(dstr_cstr(pole), nullptr, 10);
                tok.type = TT_INT_LITERAL;
                dstr_delete(pole);
                return tok;
            }
        }
        case S_INTEGER: {
            if (isdigit(znak)) {
                dstr_push_char(pole, znak);
                po_podtrzitku = false;
                break;
            } else if (znak == '_') {

                if (po_podtrzitku == true) {
                    dstr_delete(pole);
                    exit(1);
                }
                po_podtrzitku = true;
                break;
            } else {

                aktualni_stav = S_START;
                if ((dstr_cstr(pole)[0] == '0') || (po_podtrzitku == true)) {
                    dstr_delete(pole);
                    exit(1);
                }
                ungetc(znak, lx->in); // vraceni neznameho znaku
                tok.type = TT_INT_LITERAL;
                tok.attr.as_int = strtol(dstr_cstr(pole), nullptr, 10);
                dstr_delete(pole);
                return tok;
            }
        }
        case S_DOUBLE: {
            if (isdigit(znak)) {
                po_podtrzitku = false;
                dstr_push_char(pole, znak);
                break;
            } else if (znak == '_') {

                char posledni = dstr_cstr(pole)[dstr_len(pole) - 1];
                if ((po_podtrzitku == true) || !isdigit(posledni)) {
                    dstr_delete(pole);
                    exit(1);
                }
                po_podtrzitku = true;
                break;
            } else if (znak == 'e' || znak == 'E') {
                // pred exponentem nesmi byt _
                if (po_podtrzitku == true) {
                    dstr_delete(pole);
                    exit(1);
                }
                dstr_push_char(pole, znak);
                break;
            } else {
                char posledni = dstr_cstr(pole)[dstr_len(pole) - 1];
                if ((posledni == '.') || (posledni == 'e') ||
                    (posledni == 'E') || (posledni == '+') ||
                    (posledni == '-') || (po_podtrzitku == true)) {
                    dstr_delete(pole);
                    exit(1);
                }
                aktualni_stav = S_START;
                ungetc(znak, lx->in); // vraceni neznameho znaku
                tok.type = TT_DOUBLE_LITERAL;
                tok.attr.as_double = strtod(dstr_cstr(pole), nullptr);
                dstr_delete(pole);
                return tok;
            }
        }
        case S_STRING_S: {
            if (znak == '"') {
                aktualni_stav = S_STRING_E;
                break;
            } else if (znak == '\n' || znak == EOF) {
                dstr_delete(pole);
                exit(1);
            } else if (znak == '\\') {
                aktualni_stav = S_STRING_ES;
                break;
            } else {
                aktualni_stav = S_STRING;
                dstr_push_char(pole, znak);
                break;
            }
        }
        case S_STRING: {
            if (znak == '"') {
                aktualni_stav = S_STRING_E;
                break;
            } else if (znak == '\\') {
                aktualni_stav = S_STRING_ES;
                break;
            } else if (znak == '\n' || znak == EOF) {
                dstr_delete(pole);
                exit(1);
            } else {
                dstr_push_char(pole, znak);
                break;
            }
        }
        case S_STRING_ES: {
            if (znak == 'n') {
                dstr_push_char(pole, '\n');
                aktualni_stav = S_STRING;
                break;
            } else if (znak == 't') {
                dstr_push_char(pole, '\t');
                aktualni_stav = S_STRING;
                break;
            } else if (znak == 'r') {
                dstr_push_char(pole, '\r');
                aktualni_stav = S_STRING;
                break;
            } else if (znak == '"') {
                dstr_push_char(pole, znak);
                aktualni_stav = S_STRING;
                break;
            } else if (znak == '\\') {
                dstr_push_char(pole, znak);
                aktualni_stav = S_STRING;
                break;
            } else {
                dstr_delete(pole);
                exit(1);
            }
        }
        case S_STRING_E: {
            if ((znak == '"') && (dstr_len(pole) == 0)) {

                aktualni_stav = S_S_ML;
                break;
            } else {
                tok.type = TT_STRING_LITERAL;
                ungetc(znak, lx->in);
                tok.attr.as_str = pole;
                return tok;
            }
        }
        case S_S_ML: {
            if (znak == '"') {

                aktualni_stav = S_S_ML_E;
                break;
            } else if (znak == EOF) {
                dstr_delete(pole);
                exit(1);
            } else {
                dstr_push_char(pole, znak);
                aktualni_stav = S_S_ML;
                break;
            }
        }
        case S_S_ML_E: {
            if (znak == '"') {

                aktualni_stav = S_S_ML_E2;
                break;
            } else if (znak == EOF) {
                dstr_delete(pole);
                exit(1);
            } else {
                dstr_push_char(pole, '"');
                dstr_push_char(pole, znak);
                aktualni_stav = S_S_ML;
                break;
            }
        }
        case S_S_ML_E2: {
            if (znak == '"') {

                aktualni_stav = S_S_ML_E3;
                break;
            } else if (znak == EOF) {
                dstr_delete(pole);
                exit(1);
            } else {
                dstr_push_char(pole, '"');
                dstr_push_char(pole, '"');
                dstr_push_char(pole, znak);

                aktualni_stav = S_S_ML;
                break;
            }
        }
        case S_S_ML_E3: {
            ungetc(znak, lx->in); // vraceni neznameho znaku
            tok.type = TT_STRING_LITERAL;
            tok.attr.as_str = pole;

            return tok;
        }
        default:
            break;
        }
    }
}

bool token_is_kw(const token *tok, keyword kw) {
    if (tok == nullptr) {
        return false;
    }
    if ((tok->type == TT_KEYWORD) && (tok->attr.as_keyword == kw)) {
        return true;
    }
    return false;
}

void token_free(token *tok) {
    if (tok == nullptr) {
        return;
    }

    if ((tok->type == TT_IDENTIFIER) || (tok->type == TT_STRING_LITERAL)) {
        if (tok->attr.as_str != nullptr) {
            dstr_delete(tok->attr.as_str);
            tok->attr.as_str = nullptr;
        }
    }
}

/**
 * Prevede ciselny typ tokenu
 * na citelny textovy retezec pro vypis chyb a logovani.
 */
const char *token_type_name(token_type type) {
    switch (type) {
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

    // Separatory
    case TT_COLON:
        return "TT_COLON";
    case TT_COMMA:
        return "TT_COMMA";
    case TT_DOT:
        return "TT_DOT";
    case TT_LPAREN:
        return "TT_LPAREN";
    case TT_RPAREN:
        return "TT_RPAREN";

    // Rizeni bloku a radku
    case TT_INDENT:
        return "TT_INDENT";
    case TT_DEDENT:
        return "TT_DEDENT";
    case TT_EOF:
        return "TT_EOF";
    case TT_EOL:
        return "TT_EOL";

    default:
        return "UNKNOWN";
    }
}

lexer *lexer_create(FILE *in) {
    if (in == nullptr) {
        return nullptr;
    }

    lexer *lx = malloc(sizeof(struct lexer));
    if (lx == nullptr) {
        return nullptr;
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
    return lx;
}

void lexer_destroy(lexer *lx) {
    if (lx == nullptr) {
        return;
    }
    for (unsigned i = 0; i < lx->peek_count; i++) {
        token_free(&lx->pole_peek[i]);
    }
    free(lx);
}

// zobrazeni tokenu do predu
const token *lexer_peek(lexer *lx, unsigned n) {
    if (((lx == nullptr) || (n < 1) || (n > LEXER_MAX_PEEK))) {
        return nullptr;
    }
    while (lx->peek_count < n) {

        lx->pole_peek[lx->peek_count] = read_token_fsm(lx);
        lx->peek_count += 1;
    }
    return &lx->pole_peek[n - 1];
}
// hlavni verejna funkce pro parser. Vrati dalsi token na rade a posune cteni
// dal
token get_next_token(lexer *lx) {
    if (lx == nullptr) {
        token err = {.type = TT_EOF};
        return err;
    }
    token prvek_fronty;
    if (lx->peek_count > 0) {

        prvek_fronty = lx->pole_peek[0];

        if (lx->peek_count == 2) {
            lx->pole_peek[0] = lx->pole_peek[1];
        }
        lx->peek_count -= 1;
        return prvek_fronty;
    }
    // kdyz byla prazdna fronta
    return read_token_fsm(lx);
}
