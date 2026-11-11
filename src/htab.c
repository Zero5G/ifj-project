/**
 * @file htab.c
 *
 * @author Vojta Buchmaier
 * @author Jakub Manoušek
 * @brief Implementace modulu pro hašovací tabulku
 *
 * @details
 */
#include "htab.h"
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
            table->array[index].key = nullptr;
            table->array[index].state = STATE_DELETE;
            table->active_size--;
            return true;
        }
        index = (index + 1) % table->capacity;
    }
    return false;
}

hash_item *hashtable_search(hash_table *table, char *key) {
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

    hash_item *old_array = table->array;
    table->array = malloc(sizeof(hash_item) * table->capacity * 2);
    if (table->array == nullptr) {
        return false;
    }
    unsigned old_capacity = table->capacity;
    table->capacity = table->capacity * 2;
    // inicializace noveho pole
    for (unsigned i = 0; i < table->capacity; i++) {
        table->array[i].state = STATE_EMPTY;
        table->array[i].key = nullptr;
    }
    // presun starych prvku do noveho pole
    for (unsigned i = 0; i < old_capacity; i++) {
        if (old_array[i].state == STATE_FULL) {
            unsigned new_index = hash_function(table->capacity, old_array[i].key);
            while (table->array[new_index].state == STATE_FULL) {
                new_index = (new_index + 1) % table->capacity;
            }
            table->array[new_index] = old_array[i];
        }
    }
    free(old_array);
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
            table->array[i].state = STATE_EMPTY;
        }
    }
    free(table->array);
    table->array = nullptr;
    table->capacity = 0;
    table->active_size = 0;
}
