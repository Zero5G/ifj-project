#include "lexer.h"
#include "dstr.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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


struct lexer 
{
    FILE *in;
    int line;
    int col;
    token pole_peek[LEXER_MAX_PEEK];
    int peek_count;
  
};

//pomocna struktura pro hledani klice
typedef struct {
    const char *word;
    keyword kw;
} keys;


static const keys KEYWORD_TABLE[] = {
    { "def",       KW_DEF },
    { "if",        KW_IF },
    { "then",      KW_THEN },
    { "else",      KW_ELSE },
    { "while",     KW_WHILE },
    { "do",        KW_DO },
    { "val",       KW_VAL },
    { "var",       KW_VAR },
    { "return",    KW_RETURN },
    { "import",    KW_IMPORT },
    // Rozsireni
    { "case",      KW_CASE },
    { "class",     KW_CLASS },
    { "extension", KW_EXTENSION },
    { "for",       KW_FOR },
    { "match",     KW_MATCH },
    { "print",     KW_PRINT },
    { "type",      KW_TYPE },
    { "main",      KW_MAIN },
    // Datove typy
    { "Int",       KW_INT },
    { "Double",    KW_DOUBLE },
    { "String",    KW_STRING },
    { "Boolean",   KW_BOOLEAN },
    { "Unit",      KW_UNIT },
    { "List",      KW_LIST },
};

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




static token read_token_fsm(lexer *lx)
{
    token tok = {0};
    dstr pole = dstr_create("");
    enum state aktualni_stav = S_START;
    dstr_clear(pole);
    while(true)
    {
        
        
        int znak = fgetc(lx->in);
     
        switch(aktualni_stav)
        {
            case S_START:
            {
                if(znak == ' ' || znak == '\t')
                {
                    continue;
                }
                else if(znak == '\n')
                {
                    tok.type = TT_EOL;
                    dstr_delete(pole);
                    return tok;
                }
                else if(znak == EOF)
                {
                    tok.type = TT_EOF;
                    dstr_delete(pole);
                    return tok;
                    
                }
                else if(isalpha(znak)||(znak == '_'))
                {
                    aktualni_stav = S_K_I;
                    dstr_push_char(pole,znak);
                }
                else if(isdigit(znak))
                {
                    aktualni_stav = S_NUMBER;
                    dstr_push_char(pole,znak);
                }
                else if(znak=='/')
                {
                    aktualni_stav = S_SLASH;
                    dstr_push_char(pole,znak);
                }
                else if(znak == '"')
                {
                    aktualni_stav = S_STRING_S;
                    //dstr_push_char(pole,znak);
                }
                else if(znak == '=')
                {
                    aktualni_stav = S_ASSIGN;
                    dstr_push_char(pole,znak);
                }
                else if(znak == ',')
                {
                    tok.type = TT_COMMA;
                    dstr_delete(pole);
                    return tok;
                }
                
                else if(znak == '*')
                {
                    tok.type = TT_STAR;
                    dstr_delete(pole);
                    return tok;
                }
                else if(znak == '!')
                {
                    aktualni_stav = S_NEQ;
                    dstr_push_char(pole,znak);
                }
                else if(znak == '(')
                {
                    tok.type = TT_LPAREN;
                    dstr_delete(pole);
                    return tok;
                }
                else if(znak == ')')
                {
                    tok.type = TT_RPAREN;
                    dstr_delete(pole);
                    return tok;
                }
                else if(znak == '-')
                {
                    tok.type = TT_MINUS;
                    dstr_delete(pole);
                    return tok;
                }
                else if(znak == '<')
                {
                    aktualni_stav = S_LT;
                    dstr_push_char(pole,znak);
                }
                else if(znak == '>')
                {
                    aktualni_stav = S_GT;
                    dstr_push_char(pole,znak);
                }
                else if(znak == '+')
                {
                    tok.type = TT_PLUS;
                    dstr_delete(pole);
                    return tok;
                    
                }
                else if(znak == '.')
                {
                    tok.type = TT_DOT;
                    dstr_delete(pole);
                    return tok;
                }
                else if(znak == ':')
                {
                    tok.type = TT_COLON;
                    dstr_delete(pole);
                    return tok;

                }
              
                else
                {
                    dstr_delete(pole);
                    exit(1);
                }
                break;
            }
            case S_K_I:
            {
                if(isalpha(znak)||(znak == '_')||(isdigit(znak)))
                {
            
                    dstr_push_char(pole,znak);
                }
                else
                {
                    ungetc(znak, lx->in);
                    
                    //je to bool
                    if(dstr_cmp_raw(pole, "true") == 0)
                    {
                        tok.type = TT_BOOLEAN_LITERAL;
                        tok.attr.as_bool = true;
                        dstr_delete(pole); //boolen nepotrebuje text
                        return tok;
                    }
                    //je to bool
                    else if(dstr_cmp_raw(pole, "false") == 0)
                    {
                        tok.type = TT_BOOLEAN_LITERAL;
                        tok.attr.as_bool = false;
                        dstr_delete(pole); //boolen nepotrebuje text
                        return tok;
                    }
                    //porovnani se vsema slovama key jako def,if int ....
                    else if(check_keyword(pole, &tok.attr.as_keyword)==true)
                    {
                        tok.type = TT_KEYWORD;
                        dstr_delete(pole);
                        return tok;
                    }
                    //lexikalni chyba
                    else if(dstr_cmp_raw(pole, "_") == 0)
                    {
                        dstr_delete(pole);
                        exit(1);
                    }
                    //je to identifikator promenna ......
                    else
                    {
                        tok.type = TT_IDENTIFIER;
                        tok.attr.as_str = pole;
                        return tok;
                    }
                }
            }
            case S_ASSIGN: 
            {
                if(znak == '=')
                {
                    dstr_delete(pole);
                    tok.type = TT_EQ;
                    return tok;
    
                }
                else
                {
                    ungetc(znak,lx->in);
                    tok.type = TT_ASSIGN;
                    dstr_delete(pole);
                    return tok;
                }
            }
            case S_LT:
            {
                if(znak == '=')
                {
                    dstr_delete(pole);
                    tok.type = TT_LTE;
                    return tok;
    
                }
                else
                {
                    ungetc(znak,lx->in);
                    tok.type = TT_LT;
                    dstr_delete(pole);
                    return tok;
                }
            }
            case S_GT:
            {
                if(znak == '=')
                {
                    dstr_delete(pole);
                    tok.type = TT_GTE;
                    return tok;
    
                }
                else
                {
                    ungetc(znak,lx->in);
                    tok.type = TT_GT;
                    dstr_delete(pole);
                    return tok;
                }
            }
            case S_NEQ:
            {
                if(znak == '=')
                {
                    dstr_delete(pole);
                    tok.type = TT_NEQ;
                    return tok;
    
                }
                //samotny ! neni povolen
                else
                {
                    dstr_delete(pole);
                    exit(1);
                }
            }
            case S_SLASH:
            {
                if(znak == '/')
                {
                    aktualni_stav = S_COMMENT_L;
                    break;
                }
                if(znak == '*')
                {
                    aktualni_stav = S_C_MLS;
                    break;
                }
                else //neni to komentar
                {
                    ungetc(znak, lx->in);
                    tok.type = TT_SLASH;
                    dstr_delete(pole);
                    return tok;

                }
            }
            case S_COMMENT_L:
            {
                if(znak == '\n')
                {
                    aktualni_stav = S_C_LE;
                    ungetc(znak, lx->in);
                    
                    break;
                }
                else
                {
                    break;
                }
            }
            case S_C_LE:
            {
                ungetc(znak,lx->in);
                aktualni_stav = S_START;
            }
            case S_C_MLS:
            {
                if(znak == '*')
                {
                    aktualni_stav = S_C_MLE;
                    break;
                }
                else
                {
                    break;
                }
            }
            case S_C_MLE:
            {
                if(znak == '/')
                {
                    aktualni_stav = S_C_MLE2;
                    break;
                }
                else 
                {
                    dstr_delete(pole);
                    exit(1);
                }
            }
            case S_C_MLE2:
            {
                ungetc(znak,lx->in);
                aktualni_stav = S_START;
            }
            case S_NUMBER:
            {
                if(znak == '.')
                {
                    aktualni_stav = S_DOUBLE;
                    dstr_push_char(pole,znak);
                    break;
                }
                else if(isdigit(znak))
                {
                    aktualni_stav = S_INTEGER;
                    dstr_push_char(pole,znak);
                    break;
                }
                else
                {
                    ungetc(znak, lx->in);
                    tok.attr.as_int = strtol(pole->raw, NULL, 10);
                    tok.type = TT_INT_LITERAL;
                    dstr_delete(pole);
                    return tok;
                }
            }
            case S_INTEGER:
            {
                if(isdigit(znak))
                {
                    dstr_push_char(pole,znak);
                    break;
                }
                else
                {
                    aktualni_stav = S_START;
                    if (pole->raw[0] == '0')
                    {
                        dstr_delete(pole);
                        exit(1);
                    }
                    ungetc(znak, lx->in); //vraceni neznameho znaku
                    tok.type = TT_INT_LITERAL;
                    tok.attr.as_int = strtol(pole->raw, NULL, 10);
                    dstr_delete(pole);
                    return tok;
                }
            }
            case S_DOUBLE:
            {
                if(isdigit(znak)||(znak = '.')||(znak = 'e')||(znak = 'E'))
                {
                    aktualni_stav = S_DOUBLE;
                    dstr_push_char(pole,znak);
                    break;
                }
                else
                {
                    aktualni_stav = S_START;
                    if (pole->raw[0] == '0')
                    {
                        dstr_delete(pole);
                        exit(1);
                    }
                    ungetc(znak, lx->in); //vraceni neznameho znaku
                    tok.type = TT_DOUBLE_LITERAL;
                    tok.attr.as_double = strtod(pole->raw, NULL, 10);
                    dstr_delete(pole);
                    return tok;
                }
            }
            case S_STRING:
            {

            }
        }
        
    }
}


bool token_is_kw(const token *tok, keyword kw)
{
    if(tok==NULL)
    {
        return false;
    }
    if((tok->type == TT_KEYWORD)&&(tok->attr.as_keyword == kw))
    {
        return  true;
    }
    return false;
    
   
    
}

void token_free(token *tok)
{
    if(tok==NULL)
    {
        return;
    }

    if((tok->type== TT_IDENTIFIER)||(tok->type == TT_STRING_LITERAL))
    {
        if(tok->attr.as_str != NULL)
        {
            dstr_delete(tok->attr.as_str);
            tok->attr.as_str = NULL;

        }
    }

}


const char *token_type_name(token_type type)
{
    switch(type)
    {
        case TT_KEYWORD:         return "TT_KEYWORD";
        case TT_IDENTIFIER:      return "TT_IDENTIFIER";

        // Literaly
        case TT_INT_LITERAL:     return "TT_INT_LITERAL";
        case TT_DOUBLE_LITERAL:  return "TT_DOUBLE_LITERAL";
        case TT_STRING_LITERAL:  return "TT_STRING_LITERAL";
        case TT_BOOLEAN_LITERAL: return "TT_BOOLEAN_LITERAL";
        case TT_UNIT_LITERAL:    return "TT_UNIT_LITERAL";

        // Operatory
        case TT_PLUS:            return "TT_PLUS";
        case TT_MINUS:           return "TT_MINUS";
        case TT_STAR:            return "TT_STAR";
        case TT_SLASH:           return "TT_SLASH";
        case TT_EQ:              return "TT_EQ";
        case TT_NEQ:             return "TT_NEQ";
        case TT_LT:              return "TT_LT";
        case TT_LTE:             return "TT_LTE";
        case TT_GT:              return "TT_GT";
        case TT_GTE:             return "TT_GTE";

        case TT_ASSIGN:          return "TT_ASSIGN";

        // Separatory
        case TT_COLON:           return "TT_COLON";
        case TT_COMMA:           return "TT_COMMA";
        case TT_DOT:             return "TT_DOT";
        case TT_LPAREN:          return "TT_LPAREN";
        case TT_RPAREN:          return "TT_RPAREN";

        // Rizeni bloku a radku
        case TT_INDENT:          return "TT_INDENT";
        case TT_DEDENT:          return "TT_DEDENT";
        case TT_EOF:             return "TT_EOF";
        case TT_EOL:             return "TT_EOL";

        default:                 return "UNKNOWN";
    }
}







lexer *lexer_create(FILE *in)
{
    if(in==NULL)
    {
        return NULL;
    }
    lexer *lx =malloc(sizeof(struct lexer));
    if(lx==NULL)
    {
        return NULL;
    }
    lx->in = in;
    lx->line = 1;
    lx->col = 1;
    lx->peek_count = 0;
    return lx;
    
}

void lexer_destroy(lexer *lx)
{
    if(lx==NULL)
    {
        return;
    }
    for(int i=0;i<lx->peek_count ;i++)
    {
        token_free(&lx->pole_peek[i]);
        
    }
    free(lx);
}



/*
int get_token( token *token)
{
    dstr pole = dstr_create("");
    enum state aktualni_stav = S_START;
    dstr_clear(pole);
    while(true)
    {
        
        
        int znak = getchar();
     
        switch(aktualni_stav)
        {
            case S_START:
            {
                if(znak == ' ' || znak == '\t')
                {
                    continue;
                }
                if(znak == '\n')
                {
                    token->type = TT_EOL;
                    return tok;
                }
                if(znak == EOF)
                {
                    token->type = TT_EOF;
                    return tok;
                    
                }
                if(isalpha(znak))
                {
                    aktualni_stav = S_K_I;
                    dstr_push_char(pole,znak);
                }
                if(isdigit(znak))
                {
                    aktualni_stav = S_NUMBER;
                    dstr_push_char(pole,znak);
                }
                if(znak=='/')
                {
                    aktualni_stav = S_SLASH;
                    dstr_push_char(pole,znak);
                }
                if(znak == '"')
                {
                    aktualni_stav = S_STRING;
                    dstr_push_char(pole,znak);
                }
                if(znak == '=')
                {
                    aktualni_stav = S_ASSIGN;
                    dstr_push_char(pole,znak);
                }
                if(znak == ',')
                {
                    aktualni_stav = S_COMMA;
                    dstr_push_char(pole,znak);
                }
                
                if(znak == '*')
                {
                    aktualni_stav = S_STAR;
                    dstr_push_char(pole,znak);
                }
                if(znak == '!')
                {
                    aktualni_stav = S_NEQ;
                    dstr_push_char(pole,znak);
                }
                if(znak == '(')
                {
                    aktualni_stav = S_LPAREN;
                    dstr_push_char(pole,znak);
                }
                if(znak == ')')
                {
                    aktualni_stav = S_RPAREN;
                    dstr_push_char(pole,znak);
                }
                if(znak == '-')
                {
                    aktualni_stav = S_MINUS;
                    dstr_push_char(pole,znak);
                }
                if(znak == '<')
                {
                    aktualni_stav = S_LT;
                    dstr_push_char(pole,znak);
                }
                if(znak == '>')
                {
                    aktualni_stav = S_GT;
                    dstr_push_char(pole,znak);
                }
                if(znak == '+')
                {
                    aktualni_stav = S_PLUS;
                    dstr_push_char(pole,znak);
                }
                if(znak == '.')
                {
                    aktualni_stav = S_DOT;
                    dstr_push_char(pole,znak);
                }
                if(znak == ':')
                {
                    aktualni_stav = S_COLON;
                    dstr_push_char(pole,znak);
                }
                break;
            }
        }
        
    }
}

*/
