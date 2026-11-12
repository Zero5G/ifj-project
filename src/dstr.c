/**
 * @file dstr.c
 *
 * @author Jakub Machač
 * @author Jakub Manoušek
 * @brief Implementace modulu pro dynamické řetězce
 *
 * @details
 */
#include "dstr.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct dstr {
    char *buf;
    size_t len;
    size_t cap;
};

/**
 * @brief Realocuje pamět
 *
 * Expanduje
 *
 * @param str
 * @param needed
 * @return False nebo true podle toho jestli se povedla nebo ne
 */
static bool expand(dstr str, size_t needed) {
    if (str->cap > needed) {
        return true;
    }

    size_t cap = str->cap * 2;
    if (needed > cap) {
        cap = needed;
    }

    char *src = realloc(str->buf, cap * sizeof(char));
    if (src == nullptr) {
        return false;
    }

    str->buf = src;
    str->cap = cap;
    return true;
}

dstr dstr_create(const char *str) {
    dstr dstring = malloc(sizeof(struct dstr));
    if (dstring == nullptr) {
        return nullptr;
    }
    size_t len = strlen(str);
    char *string = malloc((len + 1) * sizeof(char));
    if (string == nullptr) {
        free(dstring);
        return nullptr;
    }
    strcpy(string, str);
    dstring->buf = string;
    dstring->len = len;
    dstring->cap = len + 1;

    return dstring;
}

bool dstr_concat_raw(dstr str_a, const char *str_b) {
    size_t len = strlen(str_b) + dstr_len(str_a);
    if (!expand(str_a, len)) {
        return false;
    }
    str_a->len = len;
    strcat(str_a->buf, str_b);
    return true;
}

bool dstr_concat(dstr str_a, const struct dstr *str_b) {
    size_t len = dstr_len(str_a) + dstr_len(str_b);
    if (!expand(str_a, len)) {
        return false;
    }
    str_a->len = len;
    strcat(str_a->buf, str_b->buf);
    return true;
}

dstr dstr_dup(const struct dstr *str) { return dstr_create(str->buf); }

int dstr_cmp(const struct dstr *str_a, const struct dstr *str_b) {
    return strcmp(str_a->buf, str_b->buf);
}

int dstr_cmp_raw(dstr str_a, const char *str_b) {
    return strcmp(str_a->buf, str_b);
}

bool dstr_push_char(dstr str, char chr) {
    if (!expand(str, (str->len + 2))) {
        return false;
    }
    str->buf[dstr_len(str)] = chr;
    str->len += 1;
    str->buf[dstr_len(str)] = '\0';
    return true;
}

void dstr_delete_char(dstr str, unsigned pos) {
    char *dchar = str->buf + pos;
    memmove(dchar, dchar + 1, dstr_len(str) - pos);
    str->len -= 1;
}

void dstr_clear(dstr str) {
    str->buf[0] = '\0';
    str->len = 0;
}

void dstr_delete(dstr str) {
    if (str == nullptr) {
        return;
    }
    free(str->buf);
    free(str);
}

size_t dstr_len(const struct dstr *str) { return str->len; }

const char *dstr_cstr(const struct dstr *str) { return str->buf; }
