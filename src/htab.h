#ifndef HTAB_H
#define HTAB_H
/**
 * @brief Magicke cislo
 */
#define HASH_MAGIC_NUMBER 5381UL;
/**
 * @enum Symbol_kind
 * @brief Urcuje druh symbolu v tabluce symbolu
 */
typedef enum
{
    SYMBOL_VAR,
    SYMBOL_VAL,
    SYMBOL_DEF,
    
} symbol_kind;
/**
 * @enum Symbo_data_type
 * @brief Urcuje typ symbolu, promenne v tabluce symbolu
 */
typedef enum
{
    DT_INT,
    DT_STRING,
    DT_Double,
    DT_UNIT,
    DT_BOOLEAN,
} symbol_data_type;

/**
 * @enum Symbol_state
 * @brief Urcuje stav konkretniho pole v hash tabulce
 */
typedef enum
{
    STATE_EMPTY,
    STATE_FULL,
    STATE_DELETE,
} symbol_state;

/**
 * @enum Haash_item
 * @brief Struktura zaznamu o symbolu
 */
typedef struct
{
    char *key;
    symbol_kind kind;
    symbol_data_type data_type;
    symbol_state state;
    int param_count;
    symbol_data_type *param_types;
} hash_item;


//Struktura hash tabulky
/**
 * @enum Hash_table
 * @brief Struktura ktera obsahuje info o hash tabulce
 */
typedef struct
{
    hash_item *array;
    int capacity;
    int active_size;
} hash_table;


/**
 * @brief Vyhledava prvek v hash tabulce
 * 
 * @param table ukazatel na hash tabulku
 * @param key klic hledaneho prvku, hledana hodnota
 * @return adresa hledaneho prvku
 */
hash_item* hashtable_search(hash_table *table, char *key);

/**
 * @brief Prevede vstup na hash code
 * 
 * @param capacity kapacita tabulky, velikost
 * @param key klic, znak hodnoty ktera bude prevedena na hash kod
 * @return vraci index v tabulce
 */
int hash_function(int capacity,char *key); //prevedeni znaku na hash


/**
 * @brief Inicializace tabulky
 * 
 * 
 * @param capacity kapacita tabulky, velikost
 * @param capacity kapacita tabulky, velikost
 * @param table ukazatel na hash tabulku
 * @return vraci true, false zdali byla uspesna inicializace
 */
bool hashtable_init(hash_table *table,int initial_capacity);


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
hash_item* hashtable_insert(hash_table *table, char *key); 





#endif