#include <stdint.h>

/** @brief Error kódy aplikace */
typedef enum error_code: uint8_t {
    ERR_LEX = 1, /**< Chyba lexikální analýzy */
    ERR_SYNT = 2, /**< Chyba syntaktické analýzy */
    ERR_SEM_UNDEF = 3, /**< Chyba sematiky: nedefinovaný znak */
    ERR_SEM_REDEF = 4, /**< Chyba sematiky: redefinovaný znak */
    ERR_SEM_BADARGS = 5, /**< Chyba sematiky: špatné argumenty/nepovolené paramentry */
    ERR_SEM_TYPE = 6, /**< Chyba sematiky: chyba typu */
    ERR_SEM_ROAE = 7, /**< Chyba sematiky: zápis do read-only proměnné */
    ERR_SEM_OTHER = 10, /**< Chyba sematiky: jiné */
    ERR_OTHER = 99, /**< Ostatní: programové chyby, atd. */
} error_code;

/**
 * @brief Chyboví kód -> řetězec
 *
 * @param code Chyboví kód
 *
 * @return Vrací řetězec pro chyboví kód
 */
const char *error_string(error_code code);


/**
 * @brief Tisk chybového řetězce
 *
 * @param code Chyboví kód
 */
void print_error(error_code code);

/**
 * @brief Tisk chybového řetězce a vlastní informací
 *
 * @param code Chyboví kód
 * @param fmt Formát pro vlastní část chybového hlášení
 * @param ... Vlastní parametry pro tisk
 */
void print_error_custom(error_code code, char *fmt, ...);

/**
 * @brief Ukončí program s chybou
 *
 * @param code Chyboví kód
 */
void error_exit(error_code code);

/**
 * @brief Ukončí program s chybou a vlastní informací
 *
 * @param code Chyboví kód
 * @param fmt Formát pro vlastní část chybového hlášení
 * @param ... Vlastní parametry pro tisk
 */
void error_exit_custom(error_code code, char *fmt, ...);
