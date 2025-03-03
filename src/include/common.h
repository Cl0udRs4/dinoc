/**
 * @file common.h
 * @brief Common definitions for C2 server
 */

#ifndef DINOC_COMMON_H
#define DINOC_COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Status codes
typedef enum {
    STATUS_SUCCESS = 0,
    STATUS_ERROR = -1,
    STATUS_ERROR_INVALID_PARAM = -2,
    STATUS_ERROR_MEMORY = -3,
    STATUS_ERROR_NOT_RUNNING = -4,
    STATUS_ERROR_ALREADY_RUNNING = -5,
    STATUS_ERROR_NOT_FOUND = -6,
    STATUS_ERROR_TIMEOUT = -7,
    STATUS_ERROR_BUFFER_TOO_SMALL = -8,
    STATUS_ERROR_NOT_INITIALIZED = -9,
    STATUS_ERROR_KEY_EXPIRED = -10,
    STATUS_ERROR_CHECKSUM = -11,
    STATUS_ERROR_COMPRESSION = -12
} status_t;

// Client structure
typedef struct client {
    uint32_t id;
    void* data;
} client_t;

#endif /* DINOC_COMMON_H */
