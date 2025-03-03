/**
 * @file uuid.c
 * @brief UUID generation implementation for C2 server
 * 
 * This is a wrapper around the system's UUID library
 */

#define _GNU_SOURCE /* For strdup */

#include "uuid.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

// UUID state
static bool uuid_initialized = false;
static pthread_mutex_t uuid_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Initialize UUID generator
 */
status_t uuid_init(void) {
    pthread_mutex_lock(&uuid_mutex);
    
    if (uuid_initialized) {
        pthread_mutex_unlock(&uuid_mutex);
        return STATUS_ERROR_ALREADY_RUNNING;
    }
    
    uuid_initialized = true;
    
    LOG_INFO("UUID generator initialized");
    
    pthread_mutex_unlock(&uuid_mutex);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Shutdown UUID generator
 */
status_t uuid_shutdown(void) {
    pthread_mutex_lock(&uuid_mutex);
    
    if (!uuid_initialized) {
        pthread_mutex_unlock(&uuid_mutex);
        return STATUS_ERROR_NOT_RUNNING;
    }
    
    uuid_initialized = false;
    
    LOG_INFO("UUID generator shut down");
    
    pthread_mutex_unlock(&uuid_mutex);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Generate a new UUID (wrapper for system uuid_generate)
 */
status_t uuid_generate_wrapper(uuid_t uuid) {
    if (!uuid_initialized) {
        return STATUS_ERROR_NOT_RUNNING;
    }
    
    uuid_generate(uuid);
    return STATUS_SUCCESS;
}

/**
 * @brief Compatibility function for old code that uses uuid_generate with pointer
 */
status_t uuid_generate_compat(uuid_t* uuid) {
    if (!uuid_initialized || uuid == NULL) {
        return STATUS_ERROR_NOT_RUNNING;
    }
    
    uuid_generate(*uuid);
    return STATUS_SUCCESS;
}

/**
 * @brief Convert UUID to string
 */
status_t uuid_to_string(const uuid_t uuid, char* str, size_t size) {
    if (str == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    if (size < 37) {
        return STATUS_ERROR_BUFFER_TOO_SMALL;
    }
    
    uuid_unparse(uuid, str);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Parse UUID from string
 */
status_t uuid_from_string(const char* str, uuid_t uuid) {
    if (str == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    if (uuid_parse(str, uuid) != 0) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Compare two UUIDs
 * 
 * This is a wrapper around the system's uuid_compare function.
 * We need to be careful to avoid recursion since our function has the same name.
 */
int uuid_compare_wrapper(const uuid_t uuid1, const uuid_t uuid2) {
    // Simple memory comparison as a direct implementation
    // to avoid calling the system's uuid_compare which would cause recursion
    return memcmp(uuid1, uuid2, sizeof(uuid_t));
}
