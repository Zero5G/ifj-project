/**
 * @file dstr.c
 *
 * @author Jakub Machač
 * @brief Implementace modulu pro dynamické řetězce
 *
 * @details
 */
#include "dstr.h"
#include <string.h>
#include <stdlib.h>

dstr expand(dstr str, unsigned len_cap) {

}

dstr dstr_create(char *str) {
    dstr dstring = malloc(sizeof(struct dstr_t));
    if (dstring == nullptr) {
        return nullptr;
    }
    int len = strlen(str);
    char *string = malloc((len + 1) * sizeof(char));
    if (string == nullptr) {
        free(dstring);
        return nullptr;
    }
    strcpy(string,str);
    dstring->raw = string;
    dstring->len = len;
    dstring->cap = len + 1;

    return dstring;
}

dstr dstr_concat_raw(dstr str_a, char *str_b) {

}

dstr dstr_concat(dstr str_a, dstr str_b) {

}

dstr dstr_dup(dstr src) {

}

int dstr_cmp(dstr str_a, dstr str_b) {
    return strcmp(str_a, str_b);
}

int dstr_cmp_raw(dstr str_a, char *str_b) {

}

bool dstr_push_char(dstr *str, char chr) {
    char *ptr = realloc((*str)->raw , ((*str)->cap + 1) * sizeof(char));
    if (ptr == nullptr) {
        return false;
    }
    (*str)->raw[(*str)->len] = chr;
    (*str)->len += 1;
    (*str)->cap += 1;
    (*str)->raw[(*str)->len] = '\0';
    return true;
}

void dstr_delete_char(dstr *str, unsigned pos) {
    char dchar = (*str)->raw[pos];
    memmove(dchar,dchar,(*str)->len-1);
    (*str)->len -= 1;
}

void dstr_clear(dstr str) {
    str->raw[0] = '\0';
    str->len = 0;
}

void dstr_delete(dstr str) {
    free(str->raw);
    free(str); 
}

unsigned dstr_len(dstr str) {
   return str->len;
}