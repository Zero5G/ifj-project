/**
 * @file dstr.c
 *
 * @author Jakub Machač
 * @brief Implementace modulu pro dynamické řetězce
 *
 * @details
 */
#include "dstr.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

bool expand(dstr str, unsigned len_cap) {
    unsigned cap = str->cap * 2;
    if (str->cap + len_cap > cap) {
        cap = str->cap + len_cap;
    }
    char *src = realloc(str->raw, cap * sizeof(char));
    if (src == nullptr) {
        return false;
    }
    str->raw = src;
    str->cap = cap;
    return true;
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
    strcpy(string, str);
    dstring->raw = string;
    dstring->len = len;
    dstring->cap = len + 1;

    return dstring;
}

bool dstr_concat_raw(dstr str_a, char *str_b) {
    int len = strlen(str_b) + str_a->len;
    if (expand(str_a, len) == false) {
        return false;
    }
    str_a->len = len;
    strcat(str_a->raw, str_b);
    return true;
}

bool dstr_concat(dstr str_a, dstr str_b) {
    int len = str_b->len + str_a->len;
    if (expand(str_a, len) == false) {
        return false;
    }
    str_a->len = len;
    strcat(str_a->raw, str_b->raw);
    return true;
}

dstr dstr_dup(dstr str) { return dstr_create(str->raw); }

int dstr_cmp(dstr str_a, dstr str_b) { return strcmp(str_a->raw, str_b->raw); }

int dstr_cmp_raw(dstr str_a, char *str_b) { return strcmp(str_a->raw, str_b); }

bool dstr_push_char(dstr str, char chr) {
    if (expand(str, (str->cap + 1)) == false) {
        return false;
    }
    str->raw[str->len] = chr;
    str->len += 1;
    str->raw[str->len] = '\0';
    return true;
}

void dstr_delete_char(dstr str, unsigned pos) {
    char *dchar = str->raw + pos;
    memmove(dchar, dchar + 1, str->len - pos);
    str->len -= 1;
}

void dstr_clear(dstr str) {
    str->raw[0] = '\0';
    str->len = 0;
}

void dstr_delete(dstr str) {
    free(str->raw);
    free(str);
}

unsigned dstr_len(dstr str) { return str->len; }
