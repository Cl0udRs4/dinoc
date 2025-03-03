#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "src/include/protocol.h"
#include "src/include/common.h"
#include "src/common/logger.h"

int main(int argc, char** argv) {
    // Initialize logger
    logger_init(0, "/tmp/test-protocol-manager.log");
    
    printf("Initializing protocol manager\n");
    
    // Initialize protocol manager
    status_t status = protocol_manager_init();
    if (status != STATUS_SUCCESS) {
        printf("Failed to initialize protocol manager: %d\n", status);
        return 1;
    }
    
    printf("Protocol manager initialized successfully\n");
    
    // Create TCP listener
    protocol_listener_t* tcp_listener = NULL;
    protocol_listener_config_t tcp_config;
    memset(&tcp_config, 0, sizeof(tcp_config));
    tcp_config.bind_address = "0.0.0.0";
    tcp_config.port = 8080;
    
    printf("Creating TCP listener on %s:%d\n", tcp_config.bind_address, tcp_config.port);
    
    status = protocol_manager_create_listener(PROTOCOL_TYPE_TCP, &tcp_config, &tcp_listener);
    if (status != STATUS_SUCCESS) {
        printf("Failed to create TCP listener: %d\n", status);
        protocol_manager_shutdown();
        return 1;
    }
    
    printf("TCP listener created successfully\n");
    
    // Start TCP listener
    printf("Starting TCP listener\n");
    
    status = protocol_manager_start_listener(tcp_listener);
    if (status != STATUS_SUCCESS) {
        printf("Failed to start TCP listener: %d\n", status);
        protocol_manager_shutdown();
        return 1;
    }
    
    printf("TCP listener started successfully\n");
    
    // Wait for user input
    printf("Press Enter to exit...\n");
    getchar();
    
    // Shutdown protocol manager
    printf("Shutting down protocol manager\n");
    
    status = protocol_manager_shutdown();
    if (status != STATUS_SUCCESS) {
        printf("Failed to shutdown protocol manager: %d\n", status);
        return 1;
    }
    
    printf("Protocol manager shutdown successfully\n");
    
    return 0;
}
