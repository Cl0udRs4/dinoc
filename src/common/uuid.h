/**
 * @file uuid.h
 * @brief UUID generation for C2 server
 */

#ifndef DINOC_UUID_H
#define DINOC_UUID_H

#include "../include/common.h"
#include <stdint.h>
#include <stdbool.h>
#include <uuid/uuid.h>

/**
 * @brief UUID wrapper for system uuid
 */

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
 * @brief Generate a new UUID (wrapper for system uuid_generate)
 * 
 * @param uuid UUID to generate
 * @return status_t Status code
 */
status_t uuid_generate_wrapper(uuid_t uuid);

/**
 * @brief Compatibility function for old code that uses uuid_generate with pointer
 * 
 * @param uuid Pointer to UUID to generate
 * @return status_t Status code
 */
status_t uuid_generate_compat(uuid_t* uuid);

/**
 * @brief Convert UUID to string
 * 
 * @param uuid UUID to convert
 * @param str Buffer to store string representation
 * @param size Buffer size
 * @return status_t Status code
 */
status_t uuid_to_string(const uuid_t uuid, char* str, size_t size);

/**
 * @brief Parse UUID from string
 * 
 * @param str String representation of UUID
 * @param uuid Pointer to store parsed UUID
 * @return status_t Status code
 */
status_t uuid_from_string(const char* str, uuid_t uuid);

/**
 * @brief Compare two UUIDs (wrapper for system uuid_compare)
 * 
 * @param uuid1 First UUID
 * @param uuid2 Second UUID
 * @return int 0 if equal, negative if uuid1 < uuid2, positive if uuid1 > uuid2
 */
int uuid_compare_wrapper(const uuid_t uuid1, const uuid_t uuid2);

#endif /* DINOC_UUID_H */
