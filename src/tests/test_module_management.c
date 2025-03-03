/**
 * @file test_module_management.c
 * @brief Test program for module management functionality
 */

#include "../include/module.h"
#include "../common/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

// Test configuration
#define TEST_TIMEOUT_MS 5000

// Global variables
static bool test_passed = false;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// Test module data
static const char* test_module_data = 
    "/**\n"
    " * @file test_module.c\n"
    " * @brief Test module for C2 server\n"
    " */\n"
    "\n"
    "#include \"../include/module.h\"\n"
    "#include <stdio.h>\n"
    "#include <stdlib.h>\n"
    "#include <string.h>\n"
    "\n"
    "// Module information\n"
    "#define MODULE_NAME \"test\"\n"
    "#define MODULE_VERSION 1\n"
    "#define MODULE_DESCRIPTION \"Test module\"\n"
    "\n"
    "// Module commands\n"
    "static module_command_t commands[] = {\n"
    "    {\n"
    "        .name = \"test\",\n"
    "        .description = \"Test command\",\n"
    "        .usage = \"test [args]\"\n"
    "    }\n"
    "};\n"
    "\n"
    "/**\n"
    " * @brief Initialize module\n"
    " */\n"
    "status_t module_init(module_t* module) {\n"
    "    if (module == NULL) {\n"
    "        return STATUS_ERROR_INVALID_PARAM;\n"
    "    }\n"
    "    \n"
    "    // Set module information\n"
    "    module->name = strdup(MODULE_NAME);\n"
    "    module->version = MODULE_VERSION;\n"
    "    module->description = strdup(MODULE_DESCRIPTION);\n"
    "    module->state = MODULE_STATE_INITIALIZED;\n"
    "    \n"
    "    return STATUS_SUCCESS;\n"
    "}\n"
    "\n"
    "/**\n"
    " * @brief Shutdown module\n"
    " */\n"
    "status_t module_shutdown(module_t* module) {\n"
    "    if (module == NULL) {\n"
    "        return STATUS_ERROR_INVALID_PARAM;\n"
    "    }\n"
    "    \n"
    "    // Free module information\n"
    "    if (module->name != NULL) {\n"
    "        free(module->name);\n"
    "        module->name = NULL;\n"
    "    }\n"
    "    \n"
    "    if (module->description != NULL) {\n"
    "        free(module->description);\n"
    "        module->description = NULL;\n"
    "    }\n"
    "    \n"
    "    return STATUS_SUCCESS;\n"
    "}\n"
    "\n"
    "/**\n"
    " * @brief Test command\n"
    " */\n"
    "status_t cmd_test(const char* args, uint8_t** result, size_t* result_len) {\n"
    "    if (result == NULL || result_len == NULL) {\n"
    "        return STATUS_ERROR_INVALID_PARAM;\n"
    "    }\n"
    "    \n"
    "    // Create result\n"
    "    const char* output = \"Test command executed successfully\";\n"
    "    size_t output_len = strlen(output);\n"
    "    \n"
    "    uint8_t* output_data = (uint8_t*)malloc(output_len);\n"
    "    if (output_data == NULL) {\n"
    "        return STATUS_ERROR_MEMORY;\n"
    "    }\n"
    "    \n"
    "    memcpy(output_data, output, output_len);\n"
    "    \n"
    "    *result = output_data;\n"
    "    *result_len = output_len;\n"
    "    \n"
    "    return STATUS_SUCCESS;\n"
    "}\n"
    "\n"
    "/**\n"
    " * @brief Get module commands\n"
    " */\n"
    "status_t module_get_commands(module_command_t*** commands_out, size_t* count) {\n"
    "    if (commands_out == NULL || count == NULL) {\n"
    "        return STATUS_ERROR_INVALID_PARAM;\n"
    "    }\n"
    "    \n"
    "    // Set function pointers\n"
    "    commands[0].function = cmd_test;\n"
    "    \n"
    "    // Create copy of commands array\n"
    "    size_t commands_count = sizeof(commands) / sizeof(commands[0]);\n"
    "    module_command_t** commands_copy = (module_command_t**)malloc(commands_count * sizeof(module_command_t*));\n"
    "    if (commands_copy == NULL) {\n"
    "        return STATUS_ERROR_MEMORY;\n"
    "    }\n"
    "    \n"
    "    for (size_t i = 0; i < commands_count; i++) {\n"
    "        commands_copy[i] = &commands[i];\n"
    "    }\n"
    "    \n"
    "    *commands_out = commands_copy;\n"
    "    *count = commands_count;\n"
    "    \n"
    "    return STATUS_SUCCESS;\n"
    "}\n";

// Forward declarations
static void cleanup(void);

/**
 * @brief Signal handler
 */
static void signal_handler(int sig) {
    printf("Signal %d received, cleaning up...\n", sig);
    cleanup();
    exit(1);
}

/**
 * @brief Clean up resources
 */
static void cleanup(void) {
    // Clean up resources
    module_manager_shutdown();
}

/**
 * @brief Test module manager initialization
 */
static void test_module_manager_init(void) {
    printf("Testing module manager initialization...\n");
    
    // Initialize module manager
    status_t status = module_manager_init();
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to initialize module manager: %d\n", status);
        exit(1);
    }
    
    printf("Module manager initialized successfully\n");
}

/**
 * @brief Test module loading
 */
static void test_module_load(void) {
    printf("Testing module loading...\n");
    
    // Load module
    module_t* module = NULL;
    status_t status = module_load("test", (const uint8_t*)test_module_data, strlen(test_module_data), &module);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to load module: %d\n", status);
        exit(1);
    }
    
    if (module == NULL) {
        printf("Module is NULL\n");
        exit(1);
    }
    
    // Check module fields
    if (strcmp(module->name, "test") != 0) {
        printf("Module name is incorrect: %s\n", module->name);
        exit(1);
    }
    
    if (module->data == NULL) {
        printf("Module data is NULL\n");
        exit(1);
    }
    
    if (module->data_len != strlen(test_module_data)) {
        printf("Module data length is incorrect: %zu\n", module->data_len);
        exit(1);
    }
    
    printf("Module loaded successfully\n");
}

/**
 * @brief Test module finding
 */
static void test_module_find(void) {
    printf("Testing module finding...\n");
    
    // Find module by name
    module_t* module = module_find("test");
    
    if (module == NULL) {
        printf("Module not found\n");
        exit(1);
    }
    
    // Check module fields
    if (strcmp(module->name, "test") != 0) {
        printf("Module name is incorrect: %s\n", module->name);
        exit(1);
    }
    
    // Find module by ID
    module_t* module_by_id = module_find_by_id(&module->id);
    
    if (module_by_id == NULL) {
        printf("Module not found by ID\n");
        exit(1);
    }
    
    if (module_by_id != module) {
        printf("Module found by ID is not the same as module found by name\n");
        exit(1);
    }
    
    printf("Module finding test passed\n");
}

/**
 * @brief Test module unloading
 */
static void test_module_unload(void) {
    printf("Testing module unloading...\n");
    
    // Find module
    module_t* module = module_find("test");
    
    if (module == NULL) {
        printf("Module not found\n");
        exit(1);
    }
    
    // Unload module
    status_t status = module_unload(module);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to unload module: %d\n", status);
        exit(1);
    }
    
    // Check if module is unloaded
    module = module_find("test");
    
    if (module != NULL) {
        printf("Module still exists after unloading\n");
        exit(1);
    }
    
    printf("Module unloaded successfully\n");
}

/**
 * @brief Test getting all modules
 */
static void test_module_get_all(void) {
    printf("Testing getting all modules...\n");
    
    // Load module
    module_t* module = NULL;
    status_t status = module_load("test", (const uint8_t*)test_module_data, strlen(test_module_data), &module);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to load module: %d\n", status);
        exit(1);
    }
    
    // Get all modules
    module_t** modules = NULL;
    size_t modules_count = 0;
    
    status = module_get_all(&modules, &modules_count);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to get all modules: %d\n", status);
        exit(1);
    }
    
    if (modules == NULL) {
        printf("Modules array is NULL\n");
        exit(1);
    }
    
    if (modules_count != 1) {
        printf("Modules count is incorrect: %zu\n", modules_count);
        exit(1);
    }
    
    if (modules[0] != module) {
        printf("Module in array is not the same as loaded module\n");
        exit(1);
    }
    
    // Free modules array
    free(modules);
    
    printf("Getting all modules test passed\n");
}

/**
 * @brief Main function
 */
int main(int argc, char** argv) {
    // Set up signal handler
    signal(SIGINT, signal_handler);
    
    // Run tests
    test_module_manager_init();
    test_module_load();
    test_module_find();
    test_module_get_all();
    test_module_unload();
    
    // Clean up
    cleanup();
    
    printf("All tests passed\n");
    
    return 0;
}
