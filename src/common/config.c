/**
 * @file config.c
 * @brief Configuration management implementation for C2 server
 */

#include "config.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>

// Configuration entry structure
typedef struct config_entry {
    char* key;
    config_value_t value;
    struct config_entry* next;
} config_entry_t;

// Configuration state
static config_entry_t* config_entries = NULL;
static char* config_file_path = NULL;
static pthread_mutex_t config_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool config_initialized = false;

// Forward declarations
static status_t config_parse_file(const char* file_path);
static status_t config_parse_line(char* line, char** key, config_value_t* value);
static status_t config_free_value(config_value_t* value);
static status_t config_copy_value(const config_value_t* src, config_value_t* dst);

/**
 * @brief Initialize configuration manager
 */
status_t config_init(const char* config_file) {
    pthread_mutex_lock(&config_mutex);
    
    if (config_initialized) {
        pthread_mutex_unlock(&config_mutex);
        return STATUS_ERROR_ALREADY_RUNNING;
    }
    
    // Store configuration file path
    if (config_file != NULL) {
        config_file_path = strdup(config_file);
        if (config_file_path == NULL) {
            pthread_mutex_unlock(&config_mutex);
            return STATUS_ERROR_MEMORY;
        }
        
        // Parse configuration file
        status_t status = config_parse_file(config_file_path);
        if (status != STATUS_SUCCESS) {
            free(config_file_path);
            config_file_path = NULL;
            pthread_mutex_unlock(&config_mutex);
            return status;
        }
    }
    
    config_initialized = true;
    
    LOG_INFO("Configuration manager initialized");
    
    pthread_mutex_unlock(&config_mutex);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Shutdown configuration manager
 */
status_t config_shutdown(void) {
    pthread_mutex_lock(&config_mutex);
    
    if (!config_initialized) {
        pthread_mutex_unlock(&config_mutex);
        return STATUS_ERROR_NOT_RUNNING;
    }
    
    // Free configuration entries
    config_entry_t* entry = config_entries;
    while (entry != NULL) {
        config_entry_t* next = entry->next;
        
        free(entry->key);
        config_free_value(&entry->value);
        free(entry);
        
        entry = next;
    }
    
    config_entries = NULL;
    
    // Free configuration file path
    free(config_file_path);
    config_file_path = NULL;
    
    config_initialized = false;
    
    LOG_INFO("Configuration manager shut down");
    
    pthread_mutex_unlock(&config_mutex);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Save configuration to file
 */
status_t config_save(void) {
    pthread_mutex_lock(&config_mutex);
    
    if (!config_initialized) {
        pthread_mutex_unlock(&config_mutex);
        return STATUS_ERROR_NOT_RUNNING;
    }
    
    if (config_file_path == NULL) {
        pthread_mutex_unlock(&config_mutex);
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Open configuration file
    FILE* file = fopen(config_file_path, "w");
    if (file == NULL) {
        pthread_mutex_unlock(&config_mutex);
        return STATUS_ERROR;
    }
    
    // Write configuration entries
    config_entry_t* entry = config_entries;
    while (entry != NULL) {
        fprintf(file, "%s = ", entry->key);
        
        switch (entry->value.type) {
            case CONFIG_TYPE_STRING:
                fprintf(file, "\"%s\"\n", entry->value.string_value);
                break;
            
            case CONFIG_TYPE_INT:
                fprintf(file, "%lld\n", (long long)entry->value.int_value);
                break;
            
            case CONFIG_TYPE_BOOL:
                fprintf(file, "%s\n", entry->value.bool_value ? "true" : "false");
                break;
            
            case CONFIG_TYPE_FLOAT:
                fprintf(file, "%f\n", entry->value.float_value);
                break;
            
            default:
                fprintf(file, "unknown\n");
                break;
        }
        
        entry = entry->next;
    }
    
    fclose(file);
    
    LOG_INFO("Configuration saved to %s", config_file_path);
    
    pthread_mutex_unlock(&config_mutex);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Get configuration value
 */
status_t config_get(const char* key, config_value_t* value) {
    if (key == NULL || value == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&config_mutex);
    
    if (!config_initialized) {
        pthread_mutex_unlock(&config_mutex);
        return STATUS_ERROR_NOT_RUNNING;
    }
    
    // Find configuration entry
    config_entry_t* entry = config_entries;
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            // Copy value
            status_t status = config_copy_value(&entry->value, value);
            
            pthread_mutex_unlock(&config_mutex);
            
            return status;
        }
        
        entry = entry->next;
    }
    
    pthread_mutex_unlock(&config_mutex);
    
    return STATUS_ERROR_NOT_FOUND;
}

/**
 * @brief Set configuration value
 */
status_t config_set(const char* key, const config_value_t* value) {
    if (key == NULL || value == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&config_mutex);
    
    if (!config_initialized) {
        pthread_mutex_unlock(&config_mutex);
        return STATUS_ERROR_NOT_RUNNING;
    }
    
    // Find configuration entry
    config_entry_t* entry = config_entries;
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            // Free old value
            config_free_value(&entry->value);
            
            // Copy new value
            status_t status = config_copy_value(value, &entry->value);
            
            pthread_mutex_unlock(&config_mutex);
            
            return status;
        }
        
        entry = entry->next;
    }
    
    // Create new entry
    config_entry_t* new_entry = malloc(sizeof(config_entry_t));
    if (new_entry == NULL) {
        pthread_mutex_unlock(&config_mutex);
        return STATUS_ERROR_MEMORY;
    }
    
    new_entry->key = strdup(key);
    if (new_entry->key == NULL) {
        free(new_entry);
        pthread_mutex_unlock(&config_mutex);
        return STATUS_ERROR_MEMORY;
    }
    
    // Copy value
    status_t status = config_copy_value(value, &new_entry->value);
    if (status != STATUS_SUCCESS) {
        free(new_entry->key);
        free(new_entry);
        pthread_mutex_unlock(&config_mutex);
        return status;
    }
    
    // Add to list
    new_entry->next = config_entries;
    config_entries = new_entry;
    
    pthread_mutex_unlock(&config_mutex);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Get string configuration value
 */
status_t config_get_string(const char* key, char* value, size_t size) {
    if (key == NULL || value == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    config_value_t config_value;
    status_t status = config_get(key, &config_value);
    
    if (status != STATUS_SUCCESS) {
        return status;
    }
    
    if (config_value.type != CONFIG_TYPE_STRING) {
        config_free_value(&config_value);
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    strncpy(value, config_value.string_value, size);
    value[size - 1] = '\0';
    
    config_free_value(&config_value);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Set string configuration value
 */
status_t config_set_string(const char* key, const char* value) {
    if (key == NULL || value == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    config_value_t config_value;
    config_value.type = CONFIG_TYPE_STRING;
    config_value.string_value = strdup(value);
    
    if (config_value.string_value == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    status_t status = config_set(key, &config_value);
    
    config_free_value(&config_value);
    
    return status;
}

/**
 * @brief Get integer configuration value
 */
status_t config_get_int(const char* key, int64_t* value) {
    if (key == NULL || value == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    config_value_t config_value;
    status_t status = config_get(key, &config_value);
    
    if (status != STATUS_SUCCESS) {
        return status;
    }
    
    if (config_value.type != CONFIG_TYPE_INT) {
        config_free_value(&config_value);
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    *value = config_value.int_value;
    
    config_free_value(&config_value);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Set integer configuration value
 */
status_t config_set_int(const char* key, int64_t value) {
    if (key == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    config_value_t config_value;
    config_value.type = CONFIG_TYPE_INT;
    config_value.int_value = value;
    
    return config_set(key, &config_value);
}

/**
 * @brief Get boolean configuration value
 */
status_t config_get_bool(const char* key, bool* value) {
    if (key == NULL || value == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    config_value_t config_value;
    status_t status = config_get(key, &config_value);
    
    if (status != STATUS_SUCCESS) {
        return status;
    }
    
    if (config_value.type != CONFIG_TYPE_BOOL) {
        config_free_value(&config_value);
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    *value = config_value.bool_value;
    
    config_free_value(&config_value);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Set boolean configuration value
 */
status_t config_set_bool(const char* key, bool value) {
    if (key == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    config_value_t config_value;
    config_value.type = CONFIG_TYPE_BOOL;
    config_value.bool_value = value;
    
    return config_set(key, &config_value);
}

/**
 * @brief Get float configuration value
 */
status_t config_get_float(const char* key, double* value) {
    if (key == NULL || value == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    config_value_t config_value;
    status_t status = config_get(key, &config_value);
    
    if (status != STATUS_SUCCESS) {
        return status;
    }
    
    if (config_value.type != CONFIG_TYPE_FLOAT) {
        config_free_value(&config_value);
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    *value = config_value.float_value;
    
    config_free_value(&config_value);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Set float configuration value
 */
status_t config_set_float(const char* key, double value) {
    if (key == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    config_value_t config_value;
    config_value.type = CONFIG_TYPE_FLOAT;
    config_value.float_value = value;
    
    return config_set(key, &config_value);
}

/**
 * @brief Parse configuration file
 */
static status_t config_parse_file(const char* file_path) {
    if (file_path == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Open configuration file
    FILE* file = fopen(file_path, "r");
    if (file == NULL) {
        LOG_WARN("Configuration file %s not found, using defaults", file_path);
        return STATUS_SUCCESS;
    }
    
    // Read configuration file
    char line[1024];
    int line_number = 0;
    
    while (fgets(line, sizeof(line), file) != NULL) {
        line_number++;
        
        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
            len--;
        }
        
        // Skip empty lines and comments
        if (len == 0 || line[0] == '#') {
            continue;
        }
        
        // Parse line
        char* key = NULL;
        config_value_t value;
        
        status_t status = config_parse_line(line, &key, &value);
        if (status != STATUS_SUCCESS) {
            LOG_WARN("Error parsing configuration file %s, line %d: %s", file_path, line_number, line);
            continue;
        }
        
        // Create new entry
        config_entry_t* new_entry = malloc(sizeof(config_entry_t));
        if (new_entry == NULL) {
            free(key);
            config_free_value(&value);
            fclose(file);
            return STATUS_ERROR_MEMORY;
        }
        
        new_entry->key = key;
        new_entry->value = value;
        
        // Add to list
        new_entry->next = config_entries;
        config_entries = new_entry;
    }
    
    fclose(file);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Parse configuration line
 */
static status_t config_parse_line(char* line, char** key, config_value_t* value) {
    if (line == NULL || key == NULL || value == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Find equals sign
    char* equals = strchr(line, '=');
    if (equals == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Split line at equals sign
    *equals = '\0';
    
    // Trim whitespace from key
    char* key_start = line;
    char* key_end = equals - 1;
    
    while (key_start <= key_end && isspace((unsigned char)*key_start)) {
        key_start++;
    }
    
    while (key_end >= key_start && isspace((unsigned char)*key_end)) {
        key_end--;
    }
    
    if (key_start > key_end) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Copy key
    size_t key_len = key_end - key_start + 1;
    *key = malloc(key_len + 1);
    if (*key == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    memcpy(*key, key_start, key_len);
    (*key)[key_len] = '\0';
    
    // Trim whitespace from value
    char* value_start = equals + 1;
    char* value_end = line + strlen(line) - 1;
    
    while (value_start <= value_end && isspace((unsigned char)*value_start)) {
        value_start++;
    }
    
    while (value_end >= value_start && isspace((unsigned char)*value_end)) {
        value_end--;
    }
    
    if (value_start > value_end) {
        free(*key);
        *key = NULL;
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check if value is quoted
    if (*value_start == '"' && *value_end == '"') {
        // String value
        value_start++;
        value_end--;
        
        if (value_start > value_end) {
            value->type = CONFIG_TYPE_STRING;
            value->string_value = strdup("");
            
            if (value->string_value == NULL) {
                free(*key);
                *key = NULL;
                return STATUS_ERROR_MEMORY;
            }
            
            return STATUS_SUCCESS;
        }
        
        size_t value_len = value_end - value_start + 1;
        value->type = CONFIG_TYPE_STRING;
        value->string_value = malloc(value_len + 1);
        
        if (value->string_value == NULL) {
            free(*key);
            *key = NULL;
            return STATUS_ERROR_MEMORY;
        }
        
        memcpy(value->string_value, value_start, value_len);
        value->string_value[value_len] = '\0';
    } else if (strcmp(value_start, "true") == 0) {
        // Boolean value (true)
        value->type = CONFIG_TYPE_BOOL;
        value->bool_value = true;
    } else if (strcmp(value_start, "false") == 0) {
        // Boolean value (false)
        value->type = CONFIG_TYPE_BOOL;
        value->bool_value = false;
    } else {
        // Try to parse as integer or float
        char* endptr;
        int64_t int_value = strtoll(value_start, &endptr, 0);
        
        if (*endptr == '\0') {
            // Integer value
            value->type = CONFIG_TYPE_INT;
            value->int_value = int_value;
        } else {
            // Try to parse as float
            double float_value = strtod(value_start, &endptr);
            
            if (*endptr == '\0') {
                // Float value
                value->type = CONFIG_TYPE_FLOAT;
                value->float_value = float_value;
            } else {
                // Unquoted string value
                size_t value_len = value_end - value_start + 1;
                value->type = CONFIG_TYPE_STRING;
                value->string_value = malloc(value_len + 1);
                
                if (value->string_value == NULL) {
                    free(*key);
                    *key = NULL;
                    return STATUS_ERROR_MEMORY;
                }
                
                memcpy(value->string_value, value_start, value_len);
                value->string_value[value_len] = '\0';
            }
        }
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Free configuration value
 */
static status_t config_free_value(config_value_t* value) {
    if (value == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    if (value->type == CONFIG_TYPE_STRING && value->string_value != NULL) {
        free(value->string_value);
        value->string_value = NULL;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Copy configuration value
 */
static status_t config_copy_value(const config_value_t* src, config_value_t* dst) {
    if (src == NULL || dst == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    dst->type = src->type;
    
    switch (src->type) {
        case CONFIG_TYPE_STRING:
            if (src->string_value != NULL) {
                dst->string_value = strdup(src->string_value);
                if (dst->string_value == NULL) {
                    return STATUS_ERROR_MEMORY;
                }
            } else {
                dst->string_value = NULL;
            }
            break;
        
        case CONFIG_TYPE_INT:
            dst->int_value = src->int_value;
            break;
        
        case CONFIG_TYPE_BOOL:
            dst->bool_value = src->bool_value;
            break;
        
        case CONFIG_TYPE_FLOAT:
            dst->float_value = src->float_value;
            break;
        
        default:
            return STATUS_ERROR_INVALID_PARAM;
    }
    
    return STATUS_SUCCESS;
}
