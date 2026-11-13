/**
 * @file htab.h
 *
 * @author Vojta Buchmaier
 * @author Jakub Manoušek
 * @brief Modul pro hašovací tabulku
 *
 * @details
 */
#ifndef HTAB_H
#define HTAB_H

#include "symtable.h"

/**
 * @brief Magicke cislo
 */
#define HTAB_MAGIC_NUMBER 5'381UL
#define HTAB_MAGIC_NUMBER_2 33UL
#define HTAB_EXPAND_TRESHHOLD 0.75

/**
 * @enum Symbol_kind
 * @brief Urcuje druh symbolu v tabluce symbolu
 */
typedef enum {
    SYMBOL_VAR,
    SYMBOL_VAL,
    SYMBOL_DEF,

} symbol_kind;
/**
 * @enum Symbo_data_type
 * @brief Urcuje typ symbolu, promenne v tabluce symbolu
 */
typedef enum {
    DT_INT,
    DT_STRING,
    DT_DOUBLE,
    DT_UNIT,
    DT_BOOLEAN,
} symbol_data_type;

/**
 * @enum Symbol_state
 * @brief Urcuje stav konkretniho pole v hash tabulce
 */
typedef enum {
    STATE_EMPTY,
    STATE_FULL,
    STATE_DELETE,
} item_state;

/**
 * @enum Haash_item
 * @brief Struktura zaznamu o symbolu
 */
typedef struct {
    char *key;
    symbol *symbol;
    item_state state;
} hash_item;

// Struktura hash tabulky
/**
 * @enum hash_table
 * @brief Struktura ktera obsahuje info o hash tabulce
 */
typedef struct {
    hash_item *array;
    unsigned capacity;
    unsigned active_size;
} hash_table;

/**
 * @brief Vyhledava prvek v hash tabulce
 *
 * @param table ukazatel na hash tabulku
 * @param key klic hledaneho prvku, hledana hodnota
 * @return adresa hledaneho prvku
 */
hash_item *hashtable_search(const hash_table *table, const char *key);

/**
 * @brief Prevede vstup na hash code
 *
 * @param capacity kapacita tabulky, velikost
 * @param key klic, znak hodnoty ktera bude prevedena na hash kod
 * @return vraci index v tabulce
 */
int hash_function(unsigned capacity,
                  const char *key); // prevedeni znaku na hash

/**
 * @brief Inicializace tabulky
 *
 *
 * @param capacity kapacita tabulky, velikost
 * @param capacity kapacita tabulky, velikost
 * @param table ukazatel na hash tabulku
 * @return vraci true, false zdali byla uspesna inicializace
 */
bool hashtable_init(hash_table *table, int initial_capacity);

/**
 * @brief Smazani tabulky uvolneni pameti
 *

 * @param table ukazatel na hash tabulku
 * @return smaze tabulku a uvolni pamet
 */
void hashtable_dispose(hash_table *table);

/**
 * @brief Vlozi prvek do tabulky
 *
 * @param table ukazatel na hash tabulku
 * @param key klic prvku
 * @return vlozi novy prvek do tabulky
 */
hash_item *hashtable_insert(hash_table *table, char *key);

/**
 * @brief Rozsiri hash pole tabulky
 *
 * @param table ukazatel na hash tabulku
 * @return vrati nove rozsirene pole hash tabulky
 */
bool hashtable_resize(hash_table *table);

/**
 * @brief Smaze prvek z tabulky
 *
 * @param table ukazatel na hash tabulku
 * @param key klic prvku
 * @return smaze prvek z tabulky
 */
bool hashtable_remove(hash_table *table, char *key);
#endif
