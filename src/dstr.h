/**
 * @file dstr.h
 *
 * @author Jakub Manoušek (xmanouj00)
 * @author Jakub Machač (xmachaj00)
 * @brief Modul pro dynamické řetězce
 *
 * @details
 */

#ifndef DSTR_H
#define DSTR_H

#include <stddef.h>

typedef struct dstr *dstr;

/**
 * @brief vytvoří a alokuje pamět pro strukturu
 * @param str
 * @return ukazatel na vytvořený dynamický řetězec
 */
dstr dstr_create(const char *str);

/**
 * @brief připojí řetezec na konec dynamického řetezce
 * @param str_a cíl
 * @param str_b
 * @return ukazatel na cílový řetězec
 */
bool dstr_concat_raw(dstr str_a, const char *str_b);

/**
 * @brief připojí jednoho dynamického řetezce na konec druhého
 * @param str_a cíl
 * @param str_b
 * @return Vrací false nebo true podle toho jestli se povede
 */
bool dstr_concat(dstr str_a, const struct dstr *str_b);

/**
 * @brief duplikuje řetezec
 * @param str
 * @return ukazatel na kopii
 */
dstr dstr_dup(const struct dstr *str);

/**
 * @brief porovná dva stringy
 * @param str_a
 * @param str_b
 * @return vrací <0,0,>0
 */
int dstr_cmp(const struct dstr *str_a, const struct dstr *str_b);

/**
 * @brief porovná dynamický řetězec s obyčejným char *
 * @param str_a
 * @param str_b
 * @return vrací <0,0,>0
 */
int dstr_cmp_raw(dstr str_a, const char *str_b);

/**
 * @brief vloží znak na konec řetězce
 * @param str
 * @param chr
 * @return True když se povede a False pokuď se nepovede
 */
bool dstr_push_char(dstr str, char chr);

/**
 * @brief odstraní znak na pozici a posune zbytek znaků
 * @param str
 * @param pos
 */
void dstr_delete_char(dstr str, unsigned pos);

/**
 * @brief odstraní string ale zůstane alokovaná pamět
 * @param str
 */
void dstr_clear(dstr str);

/**
 * @brief odstraní string a uvolní pamět
 * @param str
 */
void dstr_delete(dstr str);

/**
 * @brief vypočítá délku řetezce
 * @param str
 * @return délku stringu
 */
size_t dstr_len(const struct dstr *str);

/**
 * @brief Vrátí C řetězec
 * @param str
 * @return řetězec terminovaný nulou
 */
const char *dstr_cstr(const struct dstr *str);

#endif
