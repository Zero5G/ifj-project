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