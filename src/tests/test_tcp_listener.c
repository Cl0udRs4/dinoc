/**
 * @file test_tcp_listener.c
 * @brief Test program for TCP protocol listener
 */

#include "../include/common.h"
#include "../include/protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

// Global variables
static protocol_listener_t* listener = NULL;
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
    printf("Received message from client: type=%d, data_len=%u\n", message->type, message->data_len);
    
    // Print message data as hex
    printf("Data: ");
    for (size_t i = 0; i < message->data_len && i < 16; i++) {
        printf("%02x ", message->data[i]);
    }
    if (message->data_len > 16) {
        printf("...");
    }
    printf("\n");
}

/**
 * @brief Client connected callback
 */
void on_client_connected(protocol_listener_t* listener, client_t* client) {
    char id_str[37];
    uuid_to_string(&client->id, id_str);
    printf("Client connected: %s\n", id_str);
}

/**
 * @brief Client disconnected callback
 */
void on_client_disconnected(protocol_listener_t* listener, client_t* client) {
    char id_str[37];
    uuid_to_string(&client->id, id_str);
    printf("Client disconnected: %s\n", id_str);
}

/**
 * @brief Main function
 */
int main(int argc, char** argv) {
    // Check arguments
    if (argc < 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }
    
    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        printf("Invalid port: %s\n", argv[1]);
        return 1;
    }
    
    // Initialize logging
    log_init(NULL, LOG_LEVEL_DEBUG);
    
    // Set up signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Create listener configuration
    protocol_listener_config_t config;
    memset(&config, 0, sizeof(config));
    
    config.type = PROTOCOL_TCP;
    config.port = port;
    config.timeout_ms = 30000;
    config.auto_start = false;
    
    // Create listener
    status_t status = protocol_listener_create(PROTOCOL_TCP, &config, &listener);
    if (status != STATUS_SUCCESS) {
        printf("Failed to create TCP listener: %d\n", status);
        return 1;
    }
    
    // Set callbacks
    listener->on_message_received = on_message_received;
    listener->on_client_connected = on_client_connected;
    listener->on_client_disconnected = on_client_disconnected;
    
    // Start listener
    status = protocol_listener_start(listener);
    if (status != STATUS_SUCCESS) {
        printf("Failed to start TCP listener: %d\n", status);
        protocol_listener_destroy(listener);
        return 1;
    }
    
    printf("TCP listener started on port %d\n", port);
    
    // Main loop
    while (running) {
        // Get listener status
        listener_state_t state;
        protocol_listener_stats_t stats;
        
        status = protocol_listener_get_status(listener, &state, &stats);
        if (status != STATUS_SUCCESS) {
            printf("Failed to get listener status: %d\n", status);
            break;
        }
        
        printf("Listener state: %d, clients: %lu, messages: %lu\n",
               state, stats.clients_connected, stats.messages_received);
        
        // Sleep for a while
        sleep(5);
    }
    
    // Stop listener
    status = protocol_listener_stop(listener);
    if (status != STATUS_SUCCESS) {
        printf("Failed to stop TCP listener: %d\n", status);
    }
    
    // Destroy listener
    status = protocol_listener_destroy(listener);
    if (status != STATUS_SUCCESS) {
        printf("Failed to destroy TCP listener: %d\n", status);
    }
    
    // Clean up
    log_shutdown();
    
    return 0;
}
