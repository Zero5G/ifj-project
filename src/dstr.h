/**
 * @file dstr.h
 *
 * @author Jakub Manoušek
 * @author Jakub Machač
 * @brief Modul pro dynamické řetězce
 *
 * @details
 */

#ifndef DSTR_H
#define DSTR_H

struct dstr_t {
    char *raw;
    unsigned len;
    unsigned cap;
};

typedef struct dstr_t *dstr;

/**
 * @param[in] str 
 * @return
 */
dstr dstr_create(char *str);

/**
 * @param str_a
 * @param [in] str_b 
 * @return
 */
dstr dstr_concat_raw(dstr str_a, char *str_b);

/**
 * @param str_a
 * @param str_b 
 * @return
 */
dstr dstr_concat(dstr str_a, dstr str_b);

/**
 * @param str_a
 * @param str_b 
 * @return
 */
dstr dstr_cmp(dstr str_a, dstr str_b);

/**
 * @param str_a
 * @param [in] str_b 
 * @return
 */
dstr dstr_cmp_raw(dstr str_a, char *str_b);

dstr dstr_push_char();

dstr dstr_delete_char();

dstr dstr_delete();

/**
 * 
 * @param str 
 * @return délku stringu
 */
unsigned dstr_len(dstr str);

#endif
