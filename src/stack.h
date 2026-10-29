/**
 * @file stack.h
 *
 * @author Jakub Manoušek
 * @brief Modul pro datovou strukturu ADT TStack určenou pro tokeny
 *
 * @details
 */

#ifndef STACK_H
#define STACK_H

#include "lexer.h"

/** @brief Definice položky zásobníku */
struct stack_item_t {
    token *data;               /**< Data položky (token) */
    struct stack_item_t *next; /**< Další položka v zásobníku */
};

typedef struct stack_item_t *stack_item;

/** @brief Definice zásobníku */
struct stack_t {
    stack_item top; /**< Položka na vrchu zásobníku */
};

/** @brief Alias pro struct stack_t */
typedef struct stack_t stack;

/**
 * @brief Alokuje zásobník
 *
 * @return Vrací ukazatel na vrcholu zásobníku
 */
stack *stack_create();

/**
 * @brief Uvolní alokovaný zásobník
 *
 * Tato funkce uvolní pamět použitou pro zásobník,
 * jen **pokud je zásovník prázdný**
 *
 * @param stk Ukazatel na ukazatel na zásobník
 *
 * @return Vrací true pokuď zásobník byl uvolněn
 */
bool stack_delete(stack **stk);

/**
 * @brief Kontroluje jestli je zásobník prázdný
 *
 * @param stk Ukazatel na zásobník
 *
 * @return Vrací true pokuď je zásobník prázdný
 */
bool stack_is_empty(stack *stk);

/**
 * @brief Inicializuje zásobník
 *
 * @param stk Ukazatel na zásobník
 */
void stack_init(stack *stk);

/**
 * @brief Přídá položku zásobník
 *
 * Funkce alokuje paměť pro položku a kopii dat,
 * data vloží do položky a poĺožku na vrchol zásobníku
 *
 * @param stk Ukazatel na zásobník
 * @param data Data která budou přidána
 *
 * @return Vrací true pokuď se povedlo přidát na vrchol zásobníku
 */
bool stack_push(stack *stk, token data);

/**
 * @brief Vrací hodnotu položky z vrcholu zásobníku
 *
 * Funkce odebere položku ze zásbníku a vrátí ukazatel na její data,
 * uživatel si pak sám paměť použitou pro data musí uvolnit
 *
 * @param stk Ukazatel na zásobník
 *
 * @return Vrací data pokuď existují, jinak nullptr
 */
token *stack_pop(stack *stk);

/**
 * @brief Vrací hodnotu položky z vrcholu zásobníku bez odebrání
 *
 * Funkce vrácí ukazatel na data položky na vrcholu zásobníku
 *
 * @param stk Ukazatel na zásobník
 *
 * @return Vrací data pokuď existují, jinak nullptr
 */
token *stack_top(stack *stk);

#endif
