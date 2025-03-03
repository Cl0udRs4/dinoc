/**
 * @file module.h
 * @brief Module management interface for C2 server
 */

#ifndef DINOC_MODULE_H
#define DINOC_MODULE_H

#include "common.h"
#include "client.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Module type enumeration
 */
typedef enum {
    MODULE_TYPE_SHELL = 0,       // Shell command execution
    MODULE_TYPE_FILE = 1,        // File operations
    MODULE_TYPE_PROCESS = 2,     // Process operations
    MODULE_TYPE_NETWORK = 3,     // Network operations
    MODULE_TYPE_SYSTEM = 4,      // System operations
    MODULE_TYPE_CUSTOM = 5       // Custom module type
} module_type_t;

/**
 * @brief Module state enumeration
 */
typedef enum {
    MODULE_STATE_UNLOADED = 0,   // Module is not loaded
    MODULE_STATE_LOADED = 1,     // Module is loaded but not initialized
    MODULE_STATE_INITIALIZED = 2, // Module is initialized
    MODULE_STATE_RUNNING = 3,     // Module is running
    MODULE_STATE_ERROR = 4        // Module is in error state
} module_state_t;

/**
 * @brief Module structure
 */
typedef struct {
    uuid_t id;                    // Module ID
    char* name;                   // Module name
    module_type_t type;           // Module type
    module_state_t state;         // Module state
    uint32_t version;             // Module version
    char* description;            // Module description
    uint8_t* data;                // Module data
    size_t data_len;              // Module data length
    void* handle;                 // Module handle (for dynamic loading)
    void* context;                // Module-specific context
} module_t;

/**
 * @brief Module command structure
 */
typedef struct {
    char* name;                   // Command name
    char* description;            // Command description
    char* usage;                  // Command usage
    void* function;               // Command function
} module_command_t;

/**
 * @brief Initialize module manager
 * 
 * @return status_t Status code
 */
status_t module_manager_init(void);

/**
 * @brief Shutdown module manager
 * 
 * @return status_t Status code
 */
status_t module_manager_shutdown(void);

/**
 * @brief Load module
 * 
 * @param name Module name
 * @param data Module data
 * @param data_len Module data length
 * @param module Pointer to store loaded module
 * @return status_t Status code
 */
status_t module_load(const char* name, const uint8_t* data, size_t data_len, module_t** module);

/**
 * @brief Unload module
 * 
 * @param module Module to unload
 * @return status_t Status code
 */
status_t module_unload(module_t* module);

/**
 * @brief Execute module command
 * 
 * @param module Module to execute command on
 * @param command Command to execute
 * @param args Command arguments
 * @param result Pointer to store command result
 * @param result_len Pointer to store command result length
 * @return status_t Status code
 */
status_t module_execute(module_t* module, const char* command, const char* args, uint8_t** result, size_t* result_len);

/**
 * @brief Get module commands
 * 
 * @param module Module to get commands from
 * @param commands Pointer to store commands array
 * @param count Pointer to store number of commands
 * @return status_t Status code
 */
status_t module_get_commands(module_t* module, module_command_t*** commands, size_t* count);

/**
 * @brief Find module by name
 * 
 * @param name Module name
 * @return module_t* Found module or NULL if not found
 */
module_t* module_find(const char* name);

/**
 * @brief Find module by ID
 * 
 * @param id Module ID
 * @return module_t* Found module or NULL if not found
 */
module_t* module_find_by_id(const uuid_t* id);

/**
 * @brief Get all modules
 * 
 * @param modules Pointer to store modules array
 * @param count Pointer to store number of modules
 * @return status_t Status code
 */
status_t module_get_all(module_t*** modules, size_t* count);

/**
 * @brief Load module on client
 * 
 * @param client Client to load module on
 * @param module Module to load
 * @return status_t Status code
 */
status_t module_load_on_client(client_t* client, module_t* module);

/**
 * @brief Unload module from client
 * 
 * @param client Client to unload module from
 * @param module Module to unload
 * @return status_t Status code
 */
status_t module_unload_from_client(client_t* client, module_t* module);

/**
 * @brief Execute module command on client
 * 
 * @param client Client to execute command on
 * @param module Module to execute command on
 * @param command Command to execute
 * @param args Command arguments
 * @param result Pointer to store command result
 * @param result_len Pointer to store command result length
 * @return status_t Status code
 */
status_t module_execute_on_client(client_t* client, module_t* module, const char* command, const char* args, uint8_t** result, size_t* result_len);

/**
 * @brief Get modules loaded on client
 * 
 * @param client Client to get modules from
 * @param modules Pointer to store modules array
 * @param count Pointer to store number of modules
 * @return status_t Status code
 */
status_t module_get_client_modules(client_t* client, module_t*** modules, size_t* count);

#endif /* DINOC_MODULE_H */
