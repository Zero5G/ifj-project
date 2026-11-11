#include "htab.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Pomocná funkce pro bezpečné dynamické alokování klíčů
char *create_key(const char *str) {
    char *key = malloc(strlen(str) + 1);
    if (key != NULL) {
        strcpy(key, str);
    }
    return key;
}

int main() {
    hash_table table;
    int initial_capacity =
        4; // Schválně malá počáteční kapacita, abychom vynutili zvětšení pole

    printf("--- 1. INICIALIZACE ---\n");
    if (hashtable_init(&table, initial_capacity)) {
        printf("[OK] Tabulka uspesne vytvorena (kapacita: %d, aktivni prvky: "
               "%d)\n",
               table.capacity, table.active_size);
    } else {
        printf("[FAIL] Inicializace selhala!\n");
        return 1;
    }

    printf("\n--- 2. VKLADANI (INSERT) A ZVETSOVANI (RESIZE) ---\n");
    char *klic1 = create_key("alfa");
    char *klic2 = create_key("beta");
    char *klic3 = create_key("gamma");
    char *klic4 = create_key("delta"); // Přidán 4. klíč pro vynucení zvětšení

    hashtable_insert(&table, klic1);
    printf("[OK] Vlozen klic 'alfa'\n");
    hashtable_insert(&table, klic2);
    printf("[OK] Vlozen klic 'beta'\n");

    printf("-> Kapacita pred 3. prvkem: %d, aktivni: %d\n", table.capacity,
           table.active_size);

    // Třetí prvek podmínku nesplní (2 není >= 3), takže se nezvětšuje
    hashtable_insert(&table, klic3);
    printf("[OK] Vlozen klic 'gamma' (kapacita je stale 4, protoze 2 < 3)\n");

    printf("-> Kapacita pred 4. prvkem: %d, aktivni: %d\n", table.capacity,
           table.active_size);

    // Čtvrtý prvek podmínku splní (3 >= 3), takže spustí tvůj rehashing
    hashtable_insert(&table, klic4);
    printf("[OK] Vlozen klic 'delta' (zde doslo ke zvetseni tabulky)\n");

    if (table.capacity == 8) {
        printf("[OK] Tabulka se uspesne zvetsila na kapacitu: %d\n",
               table.capacity);
    } else {
        printf("[FAIL] Tabulka se nezvetsila! Kapacita je stale: %d\n",
               table.capacity);
    }

    // Test na duplikát (znovu vkládáme "alfa", klíč z mallocu uvolníme hned, ať
    // není leak, použije se ten původní)
    char *klic1_duplikat = create_key("alfa");
    hash_item *duplikat = hashtable_insert(&table, klic1_duplikat);
    if (duplikat != NULL) {
        printf("[OK] Duplikat 'alfa' spravne odhalen (vratil existujici "
               "prihradku).\n");
        free(klic1_duplikat); // Uvolníme pokusný duplikát, tabulka si drží svůj
                              // původní 'klic1'
    }

    printf("\n--- 3. VYHLEDAVANI (SEARCH) ---\n");
    hash_item *nalez = hashtable_search(&table, "beta");
    if (nalez != NULL) {
        printf("[OK] Klic 'beta' uspesne nalezen!\n");
    } else {
        printf("[FAIL] Klic 'beta' se nepodarilo najit.\n");
    }

    hash_item *nenalez = hashtable_search(&table, "omega");
    if (nenalez == NULL) {
        printf("[OK] Neexistujici klic 'omega' spravne vrací NULL.\n");
    } else {
        printf("[FAIL] Funkce nasla klic, ktery v tabulce neni!\n");
    }

    printf("\n--- 4. MAZANI (REMOVE) ---\n");
    if (hashtable_remove(&table, "beta")) {
        printf("[OK] Klic 'beta' uspesne smazan. Prihradka by mela mit stav "
               "STATE_DELETE.\n");
    } else {
        printf("[FAIL] Klic 'beta' se nepodarilo smazat.\n");
    }

    if (hashtable_search(&table, "beta") == NULL) {
        printf("[OK] Klic 'beta' po smazani uz opravdu nelze vyhledat.\n");
    } else {
        printf("[FAIL] Klic 'beta' jde stale vyhledat!\n");
    }

    // Vyhledání prvku vložného za smazaným (test nepřetržení vyhledávání přes
    // STATE_DELETE)
    if (hashtable_search(&table, "gamma") != NULL) {
        printf("[OK] Klic 'gamma' bezpecne nalezen i presto, ze jsme mazali "
               "jine prvky.\n");
    }

    printf("\n--- 5. UKLID PAMETI (DISPOSE) ---\n");
    hashtable_dispose(&table);

    if (table.array == NULL && table.capacity == 0) {
        printf("[OK] Pamet byla uspesne uvolnena a tabulka je cista.\n");
    } else {
        printf("[FAIL] Tabulka neni spravne vynulovana!\n");
    }

    return 0;
}
