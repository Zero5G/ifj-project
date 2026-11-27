/**
 * @file htab.c
 *
 * @author Vojta Buchmaier (xbuchmv00)
 * @author Jakub Manoušek (xmanouj00)
 * @brief Implementace modulu pro hašovací tabulku
 *
 * @details
 */
#include "hashtable.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

bool hashtable_init(hash_table *table, int initial_capacity) {
    table->capacity = initial_capacity;
    table->active_size = 0;

    table->array = malloc(sizeof(hash_item) * initial_capacity);
    if (table->array == nullptr) {
        return false;
    }
    for (unsigned i = 0; i < table->capacity; i++) {
        table->array[i].state = STATE_EMPTY;
        table->array[i].key = nullptr;
        table->array[i].symbol = nullptr;
    }

    return true;
}

// funkce projde vstupni retezec znak po znaku a vypocita znej hash
// vraci index v tabulce
int hash_function(unsigned capacity, const char *key) {
    unsigned long magic = HTAB_MAGIC_NUMBER;
    for (int i = 0; key[i] != '\0'; i++) {
        magic = (magic * HTAB_MAGIC_NUMBER_2) + key[i];
    }
    return (int)(magic % capacity);
}

bool hashtable_remove(hash_table *table, char *key) {
    unsigned index = hash_function(table->capacity, key);
    for (unsigned i = 0; i < table->capacity; i++) {
        if (table->array[index].state == STATE_EMPTY) {
            return false;
        }
        if (table->array[index].state == STATE_FULL &&
            strcmp(table->array[index].key, key) == 0) {
            free(table->array[index].key);
            free(table->array[index].symbol);
            table->array[index].key = nullptr;
            table->array[index].symbol = nullptr;
            table->array[index].state = STATE_DELETE;
            table->active_size--;
            return true;
        }
        index = (index + 1) % table->capacity;
    }
    return false;
}

hash_item *hashtable_search(const hash_table *table, const char *key) {
    unsigned index = hash_function(table->capacity, key);
    for (unsigned i = 0; i < table->capacity; i++) {
        if (table->array[index].state == STATE_EMPTY) {
            return nullptr;
        }
        if (table->array[index].state == STATE_FULL &&
            strcmp(table->array[index].key, key) == 0) {
            return &table->array[index];
        }
        index = (index + 1) % table->capacity;
    }
    return nullptr;
}

// rozsireni pole a presun prvku do noveho pole hash tabulky
bool hashtable_resize(hash_table *table) {
    if (table->active_size < table->capacity * HTAB_EXPAND_TRESHHOLD) {
        return true;
    }

    unsigned old_capacity = table->capacity;
    unsigned new_capacity = old_capacity * 2;

    hash_item *new_array = malloc(sizeof(hash_item) * new_capacity);
    if (new_array == nullptr) {
        return false;
    }

    for (unsigned i = 0; i < new_capacity; i++) {
        new_array[i].state = STATE_EMPTY;
        new_array[i].key = nullptr;
        new_array[i].symbol = nullptr;
    }
    // presun starych prvku do noveho pole
    for (unsigned i = 0; i < old_capacity; i++) {
        if (table->array[i].state == STATE_FULL) {
            unsigned idx = hash_function(new_capacity, table->array[i].key);
            while (new_array[idx].state == STATE_FULL) {
                idx = (idx + 1) % new_capacity;
            }
            new_array[idx] = table->array[i];
        }
    }

    free(table->array);
    table->array = new_array;
    table->capacity = new_capacity;
    return true;
}

hash_item *hashtable_insert(hash_table *table, char *key) {
    // kontrola jestli existuje klíč
    hash_item *found = hashtable_search(table, key);
    if (found != nullptr) {
        return found;
    }

    // kontrola velikosti pole
    if (!hashtable_resize(table)) {
        return nullptr;
    }

    unsigned index = hash_function(table->capacity, key);
    while (table->array[index].state == STATE_FULL) {
        index = (index + 1) % table->capacity;
    }
    table->array[index].key = key;
    table->array[index].state = STATE_FULL;
    table->active_size++;
    return &table->array[index];
}

void hashtable_dispose(hash_table *table) {
    if (table == nullptr || table->array == nullptr) {
        return;
    }
    for (unsigned i = 0; i < table->capacity; i++) {
        if (table->array[i].state == STATE_FULL) {
            free(table->array[i].key);
            free(table->array[i].symbol);
            table->array[i].state = STATE_EMPTY;
        }
    }
    free(table->array);
    table->array = nullptr;
    table->capacity = 0;
    table->active_size = 0;
}
