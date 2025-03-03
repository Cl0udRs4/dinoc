/**
 * @file server.c
 * @brief Main server implementation
 */

#include "include/common.h"
#include "include/protocol.h"
#include "include/encryption.h"
#include "include/client.h"
#include "include/api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

// Global variables
static volatile bool running = true;

/**
 * @brief Signal handler
 */
void signal_handler(int sig) {
    printf("Received signal %d, shutting down...\n", sig);
    running = false;
}

/**
 * @brief Message received callback
 */
void on_message_received(protocol_listener_t* listener, client_t* client, protocol_message_t* message) {
    char client_id[37];
    uuid_to_string(&client->id, client_id);
    
    printf("Received message from client %s: type=%d, data_len=%zu\n", 
           client_id, message->type, message->data_len);
}

/**
 * @brief Client connected callback
 */
void on_client_connected(protocol_listener_t* listener, client_t* client) {
    char client_id[37];
    uuid_to_string(&client->id, client_id);
    
    printf("Client connected: %s\n", client_id);
}

/**
 * @brief Client disconnected callback
 */
void on_client_disconnected(protocol_listener_t* listener, client_t* client) {
    char client_id[37];
    uuid_to_string(&client->id, client_id);
    
    printf("Client disconnected: %s\n", client_id);
}

/**
 * @brief Main function
 */
int main(int argc, char** argv) {
    // Initialize logging
    log_init(NULL, LOG_LEVEL_DEBUG);
    LOG_INFO("DinoC server starting...");
    
    // Set up signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize protocol manager
    status_t status = protocol_manager_init();
    if (status != STATUS_SUCCESS) {
        LOG_CRITICAL("Failed to initialize protocol manager: %d", status);
        return 1;
    }
    
    // Create API server configuration
    api_config_t api_config;
    memset(&api_config, 0, sizeof(api_config));
    
    api_config.port = 8080;
    api_config.timeout_ms = 30000;
    api_config.enable_cors = true;
    
    // Initialize API server
    api_server_t* api_server = NULL;
    status = api_server_init(&api_config, &api_server);
    if (status != STATUS_SUCCESS) {
        LOG_CRITICAL("Failed to initialize API server: %d", status);
        protocol_manager_cleanup();
        return 1;
    }
    
    // Start API server
    status = api_server_start(api_server);
    if (status != STATUS_SUCCESS) {
        LOG_CRITICAL("Failed to start API server: %d", status);
        api_server_cleanup(api_server);
        protocol_manager_cleanup();
        return 1;
    }
    
    LOG_INFO("API server started on port %d", api_config.port);
    
    // Create TCP listener for testing
    protocol_listener_config_t listener_config;
    memset(&listener_config, 0, sizeof(listener_config));
    
    listener_config.type = PROTOCOL_TCP;
    listener_config.port = 9001;
    listener_config.timeout_ms = 30000;
    
    protocol_listener_t* tcp_listener = NULL;
    status = protocol_listener_create(PROTOCOL_TCP, &listener_config, &tcp_listener);
    if (status != STATUS_SUCCESS) {
        LOG_ERROR("Failed to create TCP listener: %d", status);
    } else {
        // Set callbacks
        tcp_listener->on_message_received = on_message_received;
        tcp_listener->on_client_connected = on_client_connected;
        tcp_listener->on_client_disconnected = on_client_disconnected;
        
        // Start listener
        status = protocol_listener_start(tcp_listener);
        if (status != STATUS_SUCCESS) {
            LOG_ERROR("Failed to start TCP listener: %d", status);
        } else {
            LOG_INFO("TCP listener started on port %d", listener_config.port);
        }
    }
    
    // Main loop
    LOG_INFO("Server running, press Ctrl+C to exit");
    
    while (running) {
        // Sleep for a while
        sleep(1);
    }
    
    LOG_INFO("Server shutting down...");
    
    // Stop and destroy TCP listener
    if (tcp_listener != NULL) {
        protocol_listener_stop(tcp_listener);
        protocol_listener_destroy(tcp_listener);
    }
    
    // Stop and clean up API server
    api_server_stop(api_server);
    api_server_cleanup(api_server);
    
    // Clean up protocol manager
    protocol_manager_cleanup();
    
    // Shutdown logging
    log_shutdown();
    
    return 0;
}
