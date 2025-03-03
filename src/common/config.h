/**
 * @file config.h
 * @brief Configuration management for C2 server
 */

#ifndef DINOC_CONFIG_H
#define DINOC_CONFIG_H

#include "../include/common.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Configuration value types
 */
typedef enum {
    CONFIG_TYPE_STRING = 0,
    CONFIG_TYPE_INT = 1,
    CONFIG_TYPE_BOOL = 2,
    CONFIG_TYPE_FLOAT = 3
} config_type_t;

/**
 * @brief Configuration value structure
 */
typedef struct {
    config_type_t type;
    union {
        char* string_value;
        int64_t int_value;
        bool bool_value;
        double float_value;
    };
} config_value_t;

/**
 * @brief Initialize configuration manager
 * 
 * @param config_file Path to configuration file
 * @return status_t Status code
 */
status_t config_init(const char* config_file);

/**
 * @brief Shutdown configuration manager
 * 
 * @return status_t Status code
 */
status_t config_shutdown(void);

/**
 * @brief Save configuration to file
 * 
 * @return status_t Status code
 */
status_t config_save(void);

/**
 * @brief Get configuration value
 * 
 * @param key Configuration key
 * @param value Pointer to store configuration value
 * @return status_t Status code
 */
status_t config_get(const char* key, config_value_t* value);

/**
 * @brief Set configuration value
 * 
 * @param key Configuration key
 * @param value Configuration value
 * @return status_t Status code
 */
status_t config_set(const char* key, const config_value_t* value);

/**
 * @brief Get string configuration value
 * 
 * @param key Configuration key
 * @param value Pointer to store string value
 * @param size Buffer size
 * @return status_t Status code
 */
status_t config_get_string(const char* key, char* value, size_t size);

/**
 * @brief Set string configuration value
 * 
 * @param key Configuration key
 * @param value String value
 * @return status_t Status code
 */
status_t config_set_string(const char* key, const char* value);

/**
 * @brief Get integer configuration value
 * 
 * @param key Configuration key
 * @param value Pointer to store integer value
 * @return status_t Status code
 */
status_t config_get_int(const char* key, int64_t* value);

/**
 * @brief Set integer configuration value
 * 
 * @param key Configuration key
 * @param value Integer value
 * @return status_t Status code
 */
status_t config_set_int(const char* key, int64_t value);

/**
 * @brief Get boolean configuration value
 * 
 * @param key Configuration key
 * @param value Pointer to store boolean value
 * @return status_t Status code
 */
status_t config_get_bool(const char* key, bool* value);

/**
 * @brief Set boolean configuration value
 * 
 * @param key Configuration key
 * @param value Boolean value
 * @return status_t Status code
 */
status_t config_set_bool(const char* key, bool value);

/**
 * @brief Get float configuration value
 * 
 * @param key Configuration key
 * @param value Pointer to store float value
 * @return status_t Status code
 */
status_t config_get_float(const char* key, double* value);

/**
 * @brief Set float configuration value
 * 
 * @param key Configuration key
 * @param value Float value
 * @return status_t Status code
 */
status_t config_set_float(const char* key, double value);

#endif /* DINOC_CONFIG_H */
