#include "htab.h"
#include <criterion/criterion.h>
#include <stdlib.h>
#include <string.h>

// NOLINTBEGIN(readability-function-cognitive-complexity)

// Schválně malá počáteční kapacita, abychom vynutili zvětšení pole
#define INITIAL_CAPACITY 4

// Pomocná funkce pro bezpečné dynamické alokování klíčů.
// Po úspěšném vložení klíč vlastní tabulka (uvolní ho remove i dispose).
static char *create_key(const char *str) {
    char *key = malloc(strlen(str) + 1);
    cr_assert_not_null(key);
    strcpy(key, str);
    return key;
}

static hash_table table;

static void setup(void) {
    cr_assert(hashtable_init(&table, INITIAL_CAPACITY),
              "Inicializace selhala!");
}

static void teardown(void) { hashtable_dispose(&table); }

// Naplní tabulku čtyřmi klíči (vynutí zvětšení na 8)
static void fill_table(void) {
    cr_assert_not_null(hashtable_insert(&table, create_key("alfa")));
    cr_assert_not_null(hashtable_insert(&table, create_key("beta")));
    cr_assert_not_null(hashtable_insert(&table, create_key("gamma")));
    cr_assert_not_null(hashtable_insert(&table, create_key("delta")));
}

TestSuite(htab_lifecycle);
TestSuite(htab, .init = setup, .fini = teardown);

/* --- 1. INICIALIZACE --- */
Test(htab_lifecycle, init) {
    hash_table htab;
    cr_assert(hashtable_init(&htab, INITIAL_CAPACITY), "Inicializace selhala!");
    cr_assert_eq(htab.capacity, (unsigned)INITIAL_CAPACITY);
    cr_assert_eq(htab.active_size, 0U);
    cr_assert_not_null(htab.array);
    hashtable_dispose(&htab);
}

/* --- 2. VKLADANI A ZVETSOVANI --- */
Test(htab, insert_no_resize_below_threshold) {
    cr_assert_not_null(hashtable_insert(&table, create_key("alfa")));
    cr_assert_not_null(hashtable_insert(&table, create_key("beta")));
    // Třetí prvek podmínku nesplní (2 není >= 3), takže se nezvětšuje
    cr_assert_not_null(hashtable_insert(&table, create_key("gamma")));
    cr_assert_eq(table.capacity, (unsigned)INITIAL_CAPACITY,
                 "Kapacita se zvetsila predcasne: %u", table.capacity);
}

Test(htab, insert_triggers_resize) {
    cr_assert_not_null(hashtable_insert(&table, create_key("alfa")));
    cr_assert_not_null(hashtable_insert(&table, create_key("beta")));
    cr_assert_not_null(hashtable_insert(&table, create_key("gamma")));
    // Čtvrtý prvek podmínku splní (3 >= 3), takže spustí rehashing
    cr_assert_not_null(hashtable_insert(&table, create_key("delta")));
    cr_assert_eq(table.capacity, 8U,
                 "Tabulka se nezvetsila! Kapacita je stale: %u",
                 table.capacity);
    cr_assert_eq(table.active_size, 4U);
}

Test(htab, insert_duplicate_returns_existing) {
    hash_item *first = hashtable_insert(&table, create_key("alfa"));
    cr_assert_not_null(first);

    // Duplikát se do tabulky nedostane, tabulka ho nevlastní -> free zde
    char *dup_key = create_key("alfa");
    hash_item *dup = hashtable_insert(&table, dup_key);
    cr_assert_not_null(dup, "Duplikat nebyl odhalen");
    cr_assert_eq(dup, first, "Duplikat nevratil existujici prihradku");
    cr_assert_eq(table.active_size, 1U);
    free(dup_key);
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
    // remove klíč uvolní sám
    cr_assert(hashtable_remove(&table, "beta"),
              "Klic 'beta' se nepodarilo smazat");
    cr_assert_null(hashtable_search(&table, "beta"),
                   "Klic 'beta' jde po smazani stale vyhledat!");
    cr_assert_eq(table.active_size, 3U);
}

Test(htab, remove_missing_returns_false) {
    fill_table();
    cr_assert_not(hashtable_remove(&table, "omega"));
    cr_assert_eq(table.active_size, 4U);
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
    hash_table htab;
    cr_assert(hashtable_init(&htab, INITIAL_CAPACITY));
    cr_assert_not_null(hashtable_insert(&htab, create_key("alfa")));
    // dispose uvolní i klíče
    hashtable_dispose(&htab);
    cr_assert_null(htab.array, "Tabulka neni spravne vynulovana!");
    cr_assert_eq(htab.capacity, 0U, "Tabulka neni spravne vynulovana!");
    cr_assert_eq(htab.active_size, 0U);
}

// NOLINTEND(readability-function-cognitive-complexity)
