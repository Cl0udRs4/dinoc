#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/protocol.h"
#include "../include/server.h"
#include "../include/common.h"

// Function to directly test TCP listener creation and starting
int main(int argc, char** argv) {
    printf("Starting direct TCP listener test\n");
    
    // Initialize protocol manager
    status_t status = protocol_manager_init();
    if (status != STATUS_SUCCESS) {
        printf("Failed to initialize protocol manager: %d\n", status);
        return 1;
    }
    
    printf("Protocol manager initialized successfully\n");
    
    // Create TCP listener configuration
    protocol_listener_config_t config;
    memset(&config, 0, sizeof(config));
    config.bind_address = "0.0.0.0";
    config.port = 8085;
    
    // Create TCP listener
    protocol_listener_t* listener = NULL;
    printf("Creating TCP listener on %s:%d\n", config.bind_address, config.port);
    
    status = protocol_manager_create_listener(PROTOCOL_TYPE_TCP, &config, &listener);
    if (status != STATUS_SUCCESS) {
        printf("Failed to create TCP listener: %d\n", status);
        return 1;
    }
    
    printf("TCP listener created successfully\n");
    
    // Start TCP listener
    printf("Starting TCP listener\n");
    
    status = protocol_manager_start_listener(listener);
    if (status != STATUS_SUCCESS) {
        printf("Failed to start TCP listener: %d\n", status);
        return 1;
    }
    
    printf("TCP listener started successfully\n");
    
    // Keep the program running
    printf("Direct TCP listener test running, press Ctrl+C to exit\n");
    
    while (1) {
        sleep(1);
    }
    
    return 0;
}
