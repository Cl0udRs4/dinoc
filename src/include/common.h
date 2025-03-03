/**
 * @file common.h
 * @brief Common definitions and utilities for the C2 system
 */

#ifndef DINOC_COMMON_H
#define DINOC_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/**
 * @brief Status codes for operations
 */
typedef enum {
    STATUS_SUCCESS = 0,
    STATUS_ERROR_GENERIC = -1,
    STATUS_ERROR_MEMORY = -2,
    STATUS_ERROR_INVALID_PARAM = -3,
    STATUS_ERROR_NOT_IMPLEMENTED = -4,
    STATUS_ERROR_NOT_FOUND = -5,
    STATUS_ERROR_ALREADY_EXISTS = -6,
    STATUS_ERROR_TIMEOUT = -7,
    STATUS_ERROR_NETWORK = -8,
    STATUS_ERROR_CRYPTO = -9,
    STATUS_ERROR_PERMISSION = -10
} status_t;

/**
 * @brief Encryption algorithms supported by the system
 */
typedef enum {
    ENCRYPTION_NONE = 0,
    ENCRYPTION_AES = 1,
    ENCRYPTION_CHACHA20 = 2,
    ENCRYPTION_UNKNOWN = 255
} encryption_type_t;

/**
 * @brief Protocol types supported by the system
 */
typedef enum {
    PROTOCOL_NONE = 0,
    PROTOCOL_TCP = 1,
    PROTOCOL_UDP = 2,
    PROTOCOL_WS = 3,
    PROTOCOL_ICMP = 4,
    PROTOCOL_DNS = 5,
    PROTOCOL_UNKNOWN = 255
} protocol_type_t;

/**
 * @brief Listener states
 */
typedef enum {
    LISTENER_STATE_CREATED = 0,
    LISTENER_STATE_STARTING = 1,
    LISTENER_STATE_RUNNING = 2,
    LISTENER_STATE_STOPPING = 3,
    LISTENER_STATE_STOPPED = 4,
    LISTENER_STATE_ERROR = 5
} listener_state_t;

/**
 * @brief Client states
 */
typedef enum {
    CLIENT_STATE_UNKNOWN = 0,
    CLIENT_STATE_REGISTERED = 1,
    CLIENT_STATE_ACTIVE = 2,
    CLIENT_STATE_IDLE = 3,
    CLIENT_STATE_DISCONNECTED = 4
} client_state_t;

/**
 * @brief Task states
 */
typedef enum {
    TASK_STATE_CREATED = 0,
    TASK_STATE_PENDING = 1,
    TASK_STATE_SENT = 2,
    TASK_STATE_RUNNING = 3,
    TASK_STATE_COMPLETED = 4,
    TASK_STATE_FAILED = 5,
    TASK_STATE_TIMEOUT = 6
} task_state_t;

/**
 * @brief UUID type for unique identifiers
 */
typedef struct {
    uint8_t bytes[16];
} uuid_t;

/**
 * @brief Generate a new UUID
 * 
 * @param uuid Pointer to UUID structure to fill
 * @return status_t Status code
 */
status_t uuid_generate(uuid_t* uuid);

/**
 * @brief Convert UUID to string
 * 
 * @param uuid UUID to convert
 * @param str Buffer to store string (must be at least 37 bytes)
 * @return status_t Status code
 */
status_t uuid_to_string(const uuid_t* uuid, char* str);

/**
 * @brief Parse UUID from string
 * 
 * @param str String to parse
 * @param uuid UUID structure to fill
 * @return status_t Status code
 */
status_t uuid_from_string(const char* str, uuid_t* uuid);

/**
 * @brief Compare two UUIDs
 * 
 * @param uuid1 First UUID
 * @param uuid2 Second UUID
 * @return int 0 if equal, non-zero otherwise
 */
int uuid_compare(const uuid_t* uuid1, const uuid_t* uuid2);

/**
 * @brief Logger levels
 */
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARNING = 2,
    LOG_LEVEL_ERROR = 3,
    LOG_LEVEL_CRITICAL = 4
} log_level_t;

/**
 * @brief Initialize logging system
 * 
 * @param log_file Path to log file, NULL for stdout
 * @param level Minimum log level to record
 * @return status_t Status code
 */
status_t log_init(const char* log_file, log_level_t level);

/**
 * @brief Log a message
 * 
 * @param level Log level
 * @param file Source file
 * @param line Source line
 * @param fmt Format string
 * @param ... Format arguments
 */
void log_message(log_level_t level, const char* file, int line, const char* fmt, ...);

/**
 * @brief Shutdown logging system
 * 
 * @return status_t Status code
 */
status_t log_shutdown(void);

// Convenience macros for logging
#define LOG_DEBUG(fmt, ...) log_message(LOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) log_message(LOG_LEVEL_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) log_message(LOG_LEVEL_WARNING, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) log_message(LOG_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_CRITICAL(fmt, ...) log_message(LOG_LEVEL_CRITICAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#endif /* DINOC_COMMON_H */
