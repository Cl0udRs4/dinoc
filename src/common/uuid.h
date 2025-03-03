/**
 * @file uuid.h
 * @brief UUID generation for C2 server
 */

#ifndef DINOC_UUID_H
#define DINOC_UUID_H

#include "../include/common.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief UUID structure (RFC 4122)
 */
typedef struct {
    uint32_t time_low;
    uint16_t time_mid;
    uint16_t time_hi_and_version;
    uint16_t clock_seq;
    uint64_t node;
} uuid_t;

/**
 * @brief Initialize UUID generator
 * 
 * @return status_t Status code
 */
status_t uuid_init(void);

/**
 * @brief Shutdown UUID generator
 * 
 * @return status_t Status code
 */
status_t uuid_shutdown(void);

/**
 * @brief Generate a new UUID
 * 
 * @param uuid Pointer to store generated UUID
 * @return status_t Status code
 */
status_t uuid_generate(uuid_t* uuid);

/**
 * @brief Convert UUID to string
 * 
 * @param uuid UUID to convert
 * @param str Buffer to store string representation
 * @param size Buffer size
 * @return status_t Status code
 */
status_t uuid_to_string(const uuid_t* uuid, char* str, size_t size);

/**
 * @brief Parse UUID from string
 * 
 * @param str String representation of UUID
 * @param uuid Pointer to store parsed UUID
 * @return status_t Status code
 */
status_t uuid_from_string(const char* str, uuid_t* uuid);

/**
 * @brief Compare two UUIDs
 * 
 * @param uuid1 First UUID
 * @param uuid2 Second UUID
 * @return int 0 if equal, negative if uuid1 < uuid2, positive if uuid1 > uuid2
 */
int uuid_compare(const uuid_t* uuid1, const uuid_t* uuid2);

/**
 * @brief Check if UUID is null
 * 
 * @param uuid UUID to check
 * @return bool True if UUID is null
 */
bool uuid_is_null(const uuid_t* uuid);

/**
 * @brief Set UUID to null
 * 
 * @param uuid UUID to set
 * @return status_t Status code
 */
status_t uuid_clear(uuid_t* uuid);

#endif /* DINOC_UUID_H */
