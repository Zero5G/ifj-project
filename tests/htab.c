#include "htab.h"
#include <criterion/criterion.h>
#include <stdlib.h>
#include <string.h>

// NOLINTBEGIN(readability-function-cognitive-complexity)

// Schválně malá počáteční kapacita, abychom vynutili zvětšení pole
#define INITIAL_CAPACITY 4
#define MAX_KEYS 16

// Tabulka klíče neuvolňuje -> testy si všechny alokované klíče evidují
// a uvolní je samy (až po hashtable_dispose)
static char *allocated_keys[MAX_KEYS];
static int allocated_count;

// Pomocná funkce pro bezpečné dynamické alokování klíčů
static char *create_key(const char *str) {
    cr_assert_lt(allocated_count, MAX_KEYS);
    char *key = malloc(strlen(str) + 1);
    cr_assert_not_null(key);
    strcpy(key, str);
    allocated_keys[allocated_count++] = key;
    return key;
}

static void free_keys(void) {
    for (int i = 0; i < allocated_count; i++) {
        free(allocated_keys[i]);
        allocated_keys[i] = NULL;
    }
    allocated_count = 0;
}

static hash_table table;

static void setup(void) {
    allocated_count = 0;
    cr_assert(hashtable_init(&table, INITIAL_CAPACITY),
              "Inicializace selhala!");
}

static void teardown(void) {
    hashtable_dispose(&table);
    free_keys();
}

static void setup_keys(void) { allocated_count = 0; }

// Naplní tabulku čtyřmi klíči (vynutí zvětšení na 8)
static void fill_table(void) {
    cr_assert_not_null(hashtable_insert(&table, create_key("alfa")));
    cr_assert_not_null(hashtable_insert(&table, create_key("beta")));
    cr_assert_not_null(hashtable_insert(&table, create_key("gamma")));
    cr_assert_not_null(hashtable_insert(&table, create_key("delta")));
}

TestSuite(htab_lifecycle, .init = setup_keys, .fini = free_keys);
TestSuite(htab, .init = setup, .fini = teardown);

/* --- 1. INICIALIZACE --- */
Test(htab_lifecycle, init) {
    hash_table table;
    cr_assert(hashtable_init(&table, INITIAL_CAPACITY),
              "Inicializace selhala!");
    cr_assert_eq(table.capacity, INITIAL_CAPACITY);
    cr_assert_eq(table.active_size, 0);
    cr_assert_not_null(table.array);
    hashtable_dispose(&table);
}

/* --- 2. VKLADANI A ZVETSOVANI --- */
Test(htab, insert_no_resize_below_threshold) {
    cr_assert_not_null(hashtable_insert(&table, create_key("alfa")));
    cr_assert_not_null(hashtable_insert(&table, create_key("beta")));
    // Třetí prvek podmínku nesplní (2 není >= 3), takže se nezvětšuje
    cr_assert_not_null(hashtable_insert(&table, create_key("gamma")));
    cr_assert_eq(table.capacity, INITIAL_CAPACITY,
                 "Kapacita se zvetsila predcasne: %d", table.capacity);
}

Test(htab, insert_triggers_resize) {
    cr_assert_not_null(hashtable_insert(&table, create_key("alfa")));
    cr_assert_not_null(hashtable_insert(&table, create_key("beta")));
    cr_assert_not_null(hashtable_insert(&table, create_key("gamma")));
    // Čtvrtý prvek podmínku splní (3 >= 3), takže spustí rehashing
    cr_assert_not_null(hashtable_insert(&table, create_key("delta")));
    cr_assert_eq(table.capacity, 8,
                 "Tabulka se nezvetsila! Kapacita je stale: %d",
                 table.capacity);
}

Test(htab, insert_duplicate_returns_existing) {
    hash_item *first = hashtable_insert(&table, create_key("alfa"));
    cr_assert_not_null(first);

    // Duplikát se do tabulky nedostane; klíč uvolní evidence v teardownu
    hash_item *dup = hashtable_insert(&table, create_key("alfa"));
    cr_assert_not_null(dup, "Duplikat nebyl odhalen");
    cr_assert_eq(dup, first, "Duplikat nevratil existujici prihradku");
}

/* --- 3. VYHLEDAVANI --- */
Test(htab, search_existing) {
    fill_table();
    cr_assert_not_null(hashtable_search(&table, "beta"),
                       "Klic 'beta' se nepodarilo najit");
}

Test(htab, search_missing_returns_null) {
    fill_table();
    cr_assert_null(hashtable_search(&table, "omega"),
                   "Funkce nasla klic, ktery v tabulce neni!");
}

/* --- 4. MAZANI --- */
Test(htab, remove_existing) {
    fill_table();
    cr_assert(hashtable_remove(&table, "beta"),
              "Klic 'beta' se nepodarilo smazat");
    cr_assert_null(hashtable_search(&table, "beta"),
                   "Klic 'beta' jde po smazani stale vyhledat!");
}

Test(htab, search_continues_past_deleted) {
    fill_table();
    cr_assert(hashtable_remove(&table, "beta"));
    // Test nepřetržení vyhledávání přes STATE_DELETE
    cr_assert_not_null(hashtable_search(&table, "gamma"),
                       "Klic 'gamma' nenalezen po smazani jineho prvku");
    cr_assert_not_null(hashtable_search(&table, "alfa"));
    cr_assert_not_null(hashtable_search(&table, "delta"));
}

/* --- 5. UKLID PAMETI --- */
Test(htab_lifecycle, dispose_clears_table) {
    hash_table table;
    cr_assert(hashtable_init(&table, INITIAL_CAPACITY));
    cr_assert_not_null(hashtable_insert(&table, create_key("alfa")));
    hashtable_dispose(&table);
    cr_assert_null(table.array, "Tabulka neni spravne vynulovana!");
    cr_assert_eq(table.capacity, 0, "Tabulka neni spravne vynulovana!");
}

// NOLINTEND(readability-function-cognitive-complexity)
