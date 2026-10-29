#include "htab.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool hashtable_init(hash_table *table, int initial_capacity) {
    table->capacity = initial_capacity;
    table->active_size = 0;

    table->array = malloc(sizeof(hash_item) * initial_capacity);
    if (table->array == nullptr) {
        return false;
    }
    for (int i = 0; i < table->capacity; i++) {
        table->array[i].state = STATE_EMPTY;
        table->array[i].key = nullptr;
    }
    return true;
}
// funkce projde vstupni retezec znak po znaku a vypocita znej hash
// vraci index v tabulce
int hash_function(int capacity, char *key) {
    unsigned long magic = HASH_MAGIC_NUMBER;
    for (int i = 0; key[i] != '\0'; i++) {
        magic = magic * 33 + key[i];
    }
    return magic % capacity;
}

hash_item *hashtable_search(hash_table *table, char *key) {
    int index = hash_function(table->capacity, key);
    while (table->array[index].state != STATE_EMPTY) {
        if (table->array[index].state == STATE_DELETE) {
            index = (index + 1) % table->capacity;
        } else if (strcmp(table->array[index].key, key) == 0) {
            return &table->array[index];
        } else {
            index = (index + 1) % table->capacity;
        }
    }
    return nullptr;
}
hash_item *hashtable_insert(hash_table *table, char *key) {
    int index = hash_function(table->capacity, key);
    while (table->array[index].state == STATE_FULL) {
        if (strcmp(table->array[index].key, key) == 0) {
            return &table->array[index];
        } else {
            index = (index + 1) % table->capacity;
        }
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
    for (int i = 0; i < table->capacity; i++) {
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
