/**
 * @file types.h
 *
 * @author Jakub Manoušek
 * @brief Modulu pro datové typy
 *
 * @details
 */

#ifndef TYPES_H
#define TYPES_H

typedef enum data_type {
    TYPE_UNKNOWN,
    TYPE_INT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_UNIT,
    TYPE_BOOL,
    TYPE_ANY,
} data_type;

#endif
