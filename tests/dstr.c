#include "dstr.h"
#include <criterion/criterion.h>
#include <string.h>

// expand
Test(expand, expand_valid) {
    
}

Test(expand, expand_empty) {

}
// dstr_create
// Obyc text
Test(dstr_create, create_valid) {
    char *str = "Hello";
    unsigned len = 5;

    dstr dstring = dstr_create(str);

    // Kontrola alokace
    cr_assert_not_null(dstring, "Chyba: dstr_create vrátil NULL!");

    // Kontrola ukazatele na retezec
    cr_assert_not_null(dstring->raw, "Chyba: Vnitřní ukazatel raw je NULL!");

    // Kontrola delky
    cr_assert_eq(dstring->len, len,
                 "Chyba: Délka (len) neodpovídá textu (očekáváno %u, bylo %u)",
                 len, dstring->len);

    // Kontrola jestli je cap mensi jak len
    cr_assert_geq(dstring->cap, dstring->len,
                  "Chyba: Kapacita (cap) je menší než délka (len)!");

    // Kontrola obsahu retezce
    cr_assert_str_eq(dstring->raw, str,
                     "Chyba: Obsah v raw se neshoduje se vstupním textem!");
}

// Prazdny retezec
Test(dstr_create, create_empty) {
    char *str = "";
    dstr dstring = dstr_create(str);

    cr_assert_not_null(dstring,
                       "Chyba: dstr_create vrátil NULL pro prázdný řetězec!");
    cr_assert_eq(dstring->len, 0,
                 "Chyba: Pro prázdný řetězec musí být len rovno 0!");
    cr_assert_str_eq(dstring->raw, "",
                     "Chyba: Vnitřní řetězec raw by měl být prázdný!");
}
