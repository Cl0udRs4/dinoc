/**
 * @file config.c
 * @brief Implementation of configuration management
 */

#include "../include/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Maximum line length in config file
#define MAX_LINE_LENGTH 1024

// Maximum key length
#define MAX_KEY_LENGTH 64

// Maximum value length
#define MAX_VALUE_LENGTH 256

/**
 * @brief Configuration entry structure
 */
typedef struct config_entry {
    char key[MAX_KEY_LENGTH];
    char value[MAX_VALUE_LENGTH];
    struct config_entry* next;
} config_entry_t;

/**
 * @brief Configuration structure
 */
typedef struct {
    config_entry_t* entries;
    char* filename;
} config_t;

// Global configuration
static config_t* global_config = NULL;

/**
 * @brief Trim whitespace from string
 * 
 * @param str String to trim
 * @return char* Trimmed string
 */
static char* trim(char* str) {
    if (str == NULL) {
        return NULL;
    }
    
    // Trim leading whitespace
    while (isspace((unsigned char)*str)) {
        str++;
    }
    
    if (*str == '\0') {
        return str;
    }
    
    // Trim trailing whitespace
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
    
    return str;
}

/**
 * @brief Initialize configuration
 * 
 * @param filename Configuration file path
 * @return status_t Status code
 */
status_t config_init(const char* filename) {
    // Clean up existing configuration
    if (global_config != NULL) {
        config_cleanup();
    }
    
    // Allocate configuration
    global_config = (config_t*)malloc(sizeof(config_t));
    if (global_config == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    global_config->entries = NULL;
    
    // Copy filename
    if (filename != NULL) {
        global_config->filename = strdup(filename);
        if (global_config->filename == NULL) {
            free(global_config);
            global_config = NULL;
            return STATUS_ERROR_MEMORY;
        }
        
        // Load configuration from file
        return config_load(filename);
    } else {
        global_config->filename = NULL;
        return STATUS_SUCCESS;
    }
}

/**
 * @brief Load configuration from file
 * 
 * @param filename Configuration file path
 * @return status_t Status code
 */
status_t config_load(const char* filename) {
    if (global_config == NULL) {
        return STATUS_ERROR_GENERIC;
    }
    
    if (filename == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Open configuration file
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        LOG_WARNING("Failed to open configuration file: %s", filename);
        return STATUS_ERROR_GENERIC;
    }
    
    // Read configuration file
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file) != NULL) {
        // Skip comments and empty lines
        char* trimmed = trim(line);
        if (trimmed[0] == '#' || trimmed[0] == '\0') {
            continue;
        }
        
        // Find key-value separator
        char* separator = strchr(trimmed, '=');
        if (separator == NULL) {
            continue;
        }
        
        // Split key and value
        *separator = '\0';
        char* key = trim(trimmed);
        char* value = trim(separator + 1);
        
        // Set configuration value
        config_set(key, value);
    }
    
    fclose(file);
    return STATUS_SUCCESS;
}

/**
 * @brief Save configuration to file
 * 
 * @param filename Configuration file path (NULL for default)
 * @return status_t Status code
 */
status_t config_save(const char* filename) {
    if (global_config == NULL) {
        return STATUS_ERROR_GENERIC;
    }
    
    // Use default filename if not specified
    if (filename == NULL) {
        filename = global_config->filename;
    }
    
    if (filename == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Open configuration file
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        LOG_ERROR("Failed to open configuration file for writing: %s", filename);
        return STATUS_ERROR_GENERIC;
    }
    
    // Write header
    fprintf(file, "# DinoC Configuration\n");
    fprintf(file, "# Generated on %s\n\n", __DATE__);
    
    // Write configuration entries
    config_entry_t* entry = global_config->entries;
    while (entry != NULL) {
        fprintf(file, "%s = %s\n", entry->key, entry->value);
        entry = entry->next;
    }
    
    fclose(file);
    return STATUS_SUCCESS;
}

/**
 * @brief Get configuration value
 * 
 * @param key Configuration key
 * @param default_value Default value if key not found
 * @return const char* Configuration value
 */
const char* config_get(const char* key, const char* default_value) {
    if (global_config == NULL || key == NULL) {
        return default_value;
    }
    
    // Find configuration entry
    config_entry_t* entry = global_config->entries;
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }
    
    return default_value;
}

/**
 * @brief Get integer configuration value
 * 
 * @param key Configuration key
 * @param default_value Default value if key not found
 * @return int Configuration value
 */
int config_get_int(const char* key, int default_value) {
    const char* value = config_get(key, NULL);
    if (value == NULL) {
        return default_value;
    }
    
    return atoi(value);
}

/**
 * @brief Get boolean configuration value
 * 
 * @param key Configuration key
 * @param default_value Default value if key not found
 * @return bool Configuration value
 */
bool config_get_bool(const char* key, bool default_value) {
    const char* value = config_get(key, NULL);
    if (value == NULL) {
        return default_value;
    }
    
    if (strcasecmp(value, "true") == 0 || 
        strcasecmp(value, "yes") == 0 || 
        strcasecmp(value, "1") == 0) {
        return true;
    }
    
    if (strcasecmp(value, "false") == 0 || 
        strcasecmp(value, "no") == 0 || 
        strcasecmp(value, "0") == 0) {
        return false;
    }
    
    return default_value;
}

/**
 * @brief Set configuration value
 * 
 * @param key Configuration key
 * @param value Configuration value
 * @return status_t Status code
 */
status_t config_set(const char* key, const char* value) {
    if (global_config == NULL || key == NULL || value == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check key length
    if (strlen(key) >= MAX_KEY_LENGTH) {
        LOG_ERROR("Configuration key too long: %s", key);
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check value length
    if (strlen(value) >= MAX_VALUE_LENGTH) {
        LOG_ERROR("Configuration value too long for key: %s", key);
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Find existing entry
    config_entry_t* entry = global_config->entries;
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            // Update existing entry
            strcpy(entry->value, value);
            return STATUS_SUCCESS;
        }
        entry = entry->next;
    }
    
    // Create new entry
    entry = (config_entry_t*)malloc(sizeof(config_entry_t));
    if (entry == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    strcpy(entry->key, key);
    strcpy(entry->value, value);
    
    // Add to linked list
    entry->next = global_config->entries;
    global_config->entries = entry;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Set integer configuration value
 * 
 * @param key Configuration key
 * @param value Configuration value
 * @return status_t Status code
 */
status_t config_set_int(const char* key, int value) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d", value);
    return config_set(key, buffer);
}

/**
 * @brief Set boolean configuration value
 * 
 * @param key Configuration key
 * @param value Configuration value
 * @return status_t Status code
 */
status_t config_set_bool(const char* key, bool value) {
    return config_set(key, value ? "true" : "false");
}

/**
 * @brief Clean up configuration
 * 
 * @return status_t Status code
 */
status_t config_cleanup(void) {
    if (global_config == NULL) {
        return STATUS_SUCCESS;
    }
    
    // Free configuration entries
    config_entry_t* entry = global_config->entries;
    while (entry != NULL) {
        config_entry_t* next = entry->next;
        free(entry);
        entry = next;
    }
    
    // Free filename
    if (global_config->filename != NULL) {
        free(global_config->filename);
    }
    
    // Free configuration
    free(global_config);
    global_config = NULL;
    
    return STATUS_SUCCESS;
}
