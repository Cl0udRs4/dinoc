#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/protocol.h"
#include "../include/common.h"

extern status_t protocol_manager_init(void);
extern status_t protocol_manager_create_listener(protocol_type_t type, const protocol_listener_config_t* config, protocol_listener_t** listener);
extern status_t protocol_manager_start_listener(protocol_listener_t* listener);

int main() {
    printf("Testing protocol manager initialization\n");
    
    // Initialize protocol manager
    status_t status = protocol_manager_init();
    printf("Protocol manager initialization status: %d\n", status);
    
    if (status != STATUS_SUCCESS) {
        return 1;
    }
    
    // Create TCP listener configuration
    protocol_listener_config_t config;
    memset(&config, 0, sizeof(config));
    config.bind_address = "0.0.0.0";
    config.port = 8082;
    
    // Create TCP listener
    protocol_listener_t* listener = NULL;
    printf("Creating TCP listener on %s:%d\n", config.bind_address, config.port);
    
    status = protocol_manager_create_listener(PROTOCOL_TYPE_TCP, &config, &listener);
    printf("TCP listener creation status: %d\n", status);
    
    if (status != STATUS_SUCCESS || listener == NULL) {
        return 1;
    }
    
    // Start TCP listener
    printf("Starting TCP listener\n");
    
    status = protocol_manager_start_listener(listener);
    printf("TCP listener start status: %d\n", status);
    
    if (status != STATUS_SUCCESS) {
        return 1;
    }
    
    printf("TCP listener started successfully\n");
    
    // Keep the program running for a short time
    sleep(5);
    
    return 0;
}
