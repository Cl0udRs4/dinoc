/**
 * @file test_module_manager.c
 * @brief Test module management functionality
 */

#include "../include/module.h"
#include "../common/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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

/**
 * @brief Test module loading
 */
void test_module_load(void) {
    printf("Testing module loading...\n");
    
    // Initialize module manager
    status_t status = module_manager_init();
    assert(status == STATUS_SUCCESS);
    
    // Load module
    module_t* module = NULL;
    status = module_load("test", (const uint8_t*)test_module_data, strlen(test_module_data), &module);
    assert(status == STATUS_SUCCESS);
    assert(module != NULL);
    assert(strcmp(module->name, "test") == 0);
    assert(module->data != NULL);
    assert(module->data_len == strlen(test_module_data));
    
    // Find module by name
    module_t* found_module = module_find("test");
    assert(found_module != NULL);
    assert(found_module == module);
    
    // Find module by ID
    module_t* found_module_by_id = module_find_by_id(&module->id);
    assert(found_module_by_id != NULL);
    assert(found_module_by_id == module);
    
    // Get all modules
    module_t** all_modules = NULL;
    size_t all_modules_count = 0;
    status = module_get_all(&all_modules, &all_modules_count);
    assert(status == STATUS_SUCCESS);
    assert(all_modules != NULL);
    assert(all_modules_count == 1);
    assert(all_modules[0] == module);
    free(all_modules);
    
    // Unload module
    status = module_unload(module);
    assert(status == STATUS_SUCCESS);
    
    // Verify module is unloaded
    found_module = module_find("test");
    assert(found_module == NULL);
    
    // Shutdown module manager
    status = module_manager_shutdown();
    assert(status == STATUS_SUCCESS);
    
    printf("Module loading test passed\n");
}

/**
 * @brief Main function
 */
int main(int argc, char** argv) {
    // Run tests
    test_module_load();
    
    printf("All tests passed\n");
    return 0;
}
