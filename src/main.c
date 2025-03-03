/**
 * @file main.c
 * @brief Main entry point for C2 server
 */

#include "include/server.h"
#include "common/logger.h"
#include "include/protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

// Signal handler
static void signal_handler(int sig) {
    printf("Signal %d received, shutting down...\n", sig);
    server_stop();
    server_shutdown();
    exit(0);
}

/**
 * @brief Main function
 */
int main(int argc, char** argv) {
    // Parse command-line arguments
    server_config_t config;
    status_t status = server_parse_args(argc, argv, &config);
    
    if (status != STATUS_SUCCESS) {
        return 1;
    }
    
    // Set signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize protocol manager
    status = protocol_manager_init();
    if (status != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to initialize protocol manager: %d\n", status);
        return 1;
    }
    
    // Initialize server
    status = server_init(&config);
    if (status != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to initialize server: %d\n", status);
        return 1;
    }
    
    // Start server
    status = server_start();
    if (status != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to start server: %d\n", status);
        server_shutdown();
        return 1;
    }
    
    // Wait for signal
    printf("Server started, press Ctrl+C to stop\n");
    
    // Main loop
    while (1) {
        sleep(1);
    }
    
    // This point should never be reached
    return 0;
}
