/**
 * @file common.h
 * @brief Common definitions for C2 server
 */

#ifndef DINOC_COMMON_H
#define DINOC_COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <uuid/uuid.h>

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
    STATUS_ERROR_COMPRESSION = -12,
    STATUS_ERROR_NOT_CONNECTED = -13,
    STATUS_ERROR_SIGNATURE = -14,
    STATUS_ERROR_CRYPTO = -15,
    STATUS_ERROR_FILE_IO = -16,
    STATUS_ERROR_INVALID_FORMAT = -17,
    STATUS_ERROR_ALREADY_EXISTS = -18,
    STATUS_ERROR_GENERIC = -19,
    STATUS_ERROR_SOCKET = -20,
    STATUS_ERROR_BIND = -21,
    STATUS_ERROR_LISTEN = -22,
    STATUS_ERROR_THREAD = -23,
    STATUS_ERROR_SEND = -24
} status_t;

// Forward declarations
typedef struct client client_t;

#endif /* DINOC_COMMON_H */
