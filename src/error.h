/**
 * @file error.h
 *
 * @author Jakub Manoušek
 * @brief Modul pro chybová hlašení a ukončení programu
 *
 * @details
 */
#include <stdarg.h>
#include <stdint.h>

/** @brief Error kódy aplikace */
typedef enum error_code : uint8_t {
    ERR_LEX = 1,       /**< Chyba lexikální analýzy */
    ERR_SYNT = 2,      /**< Chyba syntaktické analýzy */
    ERR_SEM_UNDEF = 3, /**< Chyba sematiky: nedefinovaný znak */
    ERR_SEM_REDEF = 4, /**< Chyba sematiky: redefinovaný znak */
    ERR_SEM_BADARGS =
        5, /**< Chyba sematiky: špatné argumenty/nepovolené paramentry */
    ERR_SEM_TYPE = 6,   /**< Chyba sematiky: chyba typu */
    ERR_SEM_ROAE = 7,   /**< Chyba sematiky: zápis do read-only proměnné */
    ERR_SEM_OTHER = 10, /**< Chyba sematiky: jiné */
    ERR_OTHER = 99,     /**< Ostatní: programové chyby, atd. */
} error_code;

#define ERROR_OUT stderr
#define SUM_RUNTIME_ERROR 40

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
void error_print(error_code code);

/**
 * @brief Tisk chybového řetězce vlastním chybovým hlašením ve formátu varargs
 *
 * @param code Chyboví kód
 * @param fmt Formát pro vlastní část chybového hlášení
 * @param args Vlastní parametry pro tisk (typ: va_list)
 */
void error_vprintf(error_code code, const char *fmt, va_list args);

/**
 * @brief Tisk chybového řetězce a vlastním chybovým hlašením
 *
 * @param code Chyboví kód
 * @param fmt Formát pro vlastní část chybového hlášení
 * @param ... Vlastní parametry pro tisk
 */
void error_printf(error_code code, char *fmt, ...);

/**
 * @brief Ukončí program s chybou
 *
 * @param code Chyboví kód
 */
[[noreturn]] void error_fatal(error_code code);

/**
 * @brief Ukončí program s chybou a vlastní informací
 *
 * @param code Chyboví kód
 * @param fmt Formát pro vlastní část chybového hlášení
 * @param ... Vlastní parametry pro tisk
 */
[[noreturn]] void error_fatalf(error_code code, const char *fmt, ...);

void error_fatal_set_cleanup(void (*cleanup_func)(void));

/**
 * @brief Ukončí program s chybou a spustí global_cleanup
 *
 * @param code Chyboví kód
 */
[[noreturn]] void error_fatal_cleanup(error_code code);

/**
 * @brief Ukončí program s chybou, vlastní informací a spustěním global_cleanup
 *
 * @param code Chyboví kód
 * @param fmt Formát pro vlastní část chybového hlášení
 * @param ... Vlastní parametry pro tisk
 */
[[noreturn]] void error_fatalf_cleanup(error_code code, const char *fmt, ...);
