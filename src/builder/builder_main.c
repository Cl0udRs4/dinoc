/**
 * @file builder_main.c
 * @brief Main entry point for the builder tool
 */

#include "builder.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    // Initialize builder
    status_t status = builder_init();
    if (status != STATUS_SUCCESS) {
        fprintf(stderr, "Error: Failed to initialize builder\n");
        return EXIT_FAILURE;
    }
    
    // Parse command line arguments
    builder_config_t config;
    status = builder_parse_args(argc, argv, &config);
    
    if (status != STATUS_SUCCESS) {
        builder_shutdown();
        return EXIT_FAILURE;
    }
    
    // Build client
    status = builder_build_client(&config);
    
    if (status != STATUS_SUCCESS) {
        fprintf(stderr, "Error: Failed to build client\n");
        builder_clean_config(&config);
        builder_shutdown();
        return EXIT_FAILURE;
    }
    
    // Clean up
    builder_clean_config(&config);
    builder_shutdown();
    
    return EXIT_SUCCESS;
}
