/**
 * @file test_client_manager.c
 * @brief Test program for client management functionality
 */

#include "../include/client.h"
#include "../include/protocol.h"
#include "../include/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

// Test configuration
#define TEST_TIMEOUT_MS 5000

// Global variables




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
    client_manager_shutdown();
}

/**
 * @brief Test client manager initialization
 */
static void test_client_manager_init(void) {
    printf("Testing client manager initialization...\n");
    
    // Initialize client manager
    status_t status = client_manager_init();
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to initialize client manager: %d\n", status);
        exit(1);
    }
    
    printf("Client manager initialized successfully\n");
}

/**
 * @brief Test client registration
 */
static void test_client_registration(void) {
    printf("Testing client registration...\n");
    
    // Create mock protocol listener
    protocol_listener_t* listener = (protocol_listener_t*)malloc(sizeof(protocol_listener_t));
    if (listener == NULL) {
        printf("Failed to allocate memory for mock listener\n");
        exit(1);
    }
    
    // Initialize mock listener
    memset(listener, 0, sizeof(protocol_listener_t));
    listener->protocol_type = PROTOCOL_TYPE_TCP;
    
    // Register client
    client_t* client = NULL;
    status_t status = client_register(listener, NULL, &client);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to register client: %d\n", status);
        free(listener);
        exit(1);
    }
    
    if (client == NULL) {
        printf("Client is NULL\n");
        free(listener);
        exit(1);
    }
    
    printf("Client registered successfully\n");
    
    // Clean up
    free(listener);
}

/**
 * @brief Test client state management
 */
static void test_client_state_management(void) {
    printf("Testing client state management...\n");
    
    // Create mock protocol listener
    protocol_listener_t* listener = (protocol_listener_t*)malloc(sizeof(protocol_listener_t));
    if (listener == NULL) {
        printf("Failed to allocate memory for mock listener\n");
        exit(1);
    }
    
    // Initialize mock listener
    memset(listener, 0, sizeof(protocol_listener_t));
    listener->protocol_type = PROTOCOL_TYPE_TCP;
    
    // Register client
    client_t* client = NULL;
    status_t status = client_register(listener, NULL, &client);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to register client: %d\n", status);
        free(listener);
        exit(1);
    }
    
    // Test state transitions
    status = client_update_state(client, CLIENT_STATE_CONNECTED);
    if (status != STATUS_SUCCESS) {
        printf("Failed to update client state to CONNECTED: %d\n", status);
        free(listener);
        exit(1);
    }
    
    if (client->state != CLIENT_STATE_CONNECTED) {
        printf("Client state is not CONNECTED: %d\n", client->state);
        free(listener);
        exit(1);
    }
    
    status = client_update_state(client, CLIENT_STATE_REGISTERED);
    if (status != STATUS_SUCCESS) {
        printf("Failed to update client state to REGISTERED: %d\n", status);
        free(listener);
        exit(1);
    }
    
    if (client->state != CLIENT_STATE_REGISTERED) {
        printf("Client state is not REGISTERED: %d\n", client->state);
        free(listener);
        exit(1);
    }
    
    status = client_update_state(client, CLIENT_STATE_ACTIVE);
    if (status != STATUS_SUCCESS) {
        printf("Failed to update client state to ACTIVE: %d\n", status);
        free(listener);
        exit(1);
    }
    
    if (client->state != CLIENT_STATE_ACTIVE) {
        printf("Client state is not ACTIVE: %d\n", client->state);
        free(listener);
        exit(1);
    }
    
    status = client_update_state(client, CLIENT_STATE_INACTIVE);
    if (status != STATUS_SUCCESS) {
        printf("Failed to update client state to INACTIVE: %d\n", status);
        free(listener);
        exit(1);
    }
    
    if (client->state != CLIENT_STATE_INACTIVE) {
        printf("Client state is not INACTIVE: %d\n", client->state);
        free(listener);
        exit(1);
    }
    
    status = client_update_state(client, CLIENT_STATE_DISCONNECTED);
    if (status != STATUS_SUCCESS) {
        printf("Failed to update client state to DISCONNECTED: %d\n", status);
        free(listener);
        exit(1);
    }
    
    if (client->state != CLIENT_STATE_DISCONNECTED) {
        printf("Client state is not DISCONNECTED: %d\n", client->state);
        free(listener);
        exit(1);
    }
    
    printf("Client state management test passed\n");
    
    // Clean up
    free(listener);
}

/**
 * @brief Test client heartbeat
 */
static void test_client_heartbeat(void) {
    printf("Testing client heartbeat...\n");
    
    // Create mock protocol listener
    protocol_listener_t* listener = (protocol_listener_t*)malloc(sizeof(protocol_listener_t));
    if (listener == NULL) {
        printf("Failed to allocate memory for mock listener\n");
        exit(1);
    }
    
    // Initialize mock listener
    memset(listener, 0, sizeof(protocol_listener_t));
    listener->protocol_type = PROTOCOL_TYPE_TCP;
    
    // Register client
    client_t* client = NULL;
    status_t status = client_register(listener, NULL, &client);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to register client: %d\n", status);
        free(listener);
        exit(1);
    }
    
    // Set client state to ACTIVE
    status = client_update_state(client, CLIENT_STATE_ACTIVE);
    if (status != STATUS_SUCCESS) {
        printf("Failed to update client state to ACTIVE: %d\n", status);
        free(listener);
        exit(1);
    }
    
    // Set heartbeat interval
    status = client_set_heartbeat(client, 10, 2);
    if (status != STATUS_SUCCESS) {
        printf("Failed to set client heartbeat: %d\n", status);
        free(listener);
        exit(1);
    }
    
    // Process heartbeat
    status = client_heartbeat(client);
    if (status != STATUS_SUCCESS) {
        printf("Failed to process client heartbeat: %d\n", status);
        free(listener);
        exit(1);
    }
    
    // Check if heartbeat timeout
    bool timeout = client_is_heartbeat_timeout(client);
    if (timeout) {
        printf("Client heartbeat timed out unexpectedly\n");
        free(listener);
        exit(1);
    }
    
    // Wait for heartbeat timeout
    sleep(15);
    
    // Check if heartbeat timeout
    timeout = client_is_heartbeat_timeout(client);
    if (!timeout) {
        printf("Client heartbeat did not time out as expected\n");
        free(listener);
        exit(1);
    }
    
    printf("Client heartbeat test passed\n");
    
    // Clean up
    free(listener);
}

/**
 * @brief Test client info management
 */
static void test_client_info_management(void) {
    printf("Testing client info management...\n");
    
    // Create mock protocol listener
    protocol_listener_t* listener = (protocol_listener_t*)malloc(sizeof(protocol_listener_t));
    if (listener == NULL) {
        printf("Failed to allocate memory for mock listener\n");
        exit(1);
    }
    
    // Initialize mock listener
    memset(listener, 0, sizeof(protocol_listener_t));
    listener->protocol_type = PROTOCOL_TYPE_TCP;
    
    // Register client
    client_t* client = NULL;
    status_t status = client_register(listener, NULL, &client);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to register client: %d\n", status);
        free(listener);
        exit(1);
    }
    
    // Update client info
    status = client_update_info(client, "test-hostname", "192.168.1.1", "Test User Agent");
    if (status != STATUS_SUCCESS) {
        printf("Failed to update client info: %d\n", status);
        free(listener);
        exit(1);
    }
    
    // Check client info
    if (strcmp(client->hostname, "test-hostname") != 0) {
        printf("Client hostname is not correct: %s\n", client->hostname);
        free(listener);
        exit(1);
    }
    
    if (strcmp(client->ip_address, "192.168.1.1") != 0) {
        printf("Client address is not correct: %s\n", client->ip_address);
        free(listener);
        exit(1);
    }
    
    if (strcmp(client->os_info, "Test User Agent") != 0) {
        printf("Client OS info is not correct: %s\n", client->os_info);
        free(listener);
        exit(1);
    }
    
    printf("Client info management test passed\n");
    
    // Clean up
    free(listener);
}

/**
 * @brief Main function
 */
int main(void) {
    // Set up signal handler
    signal(SIGINT, signal_handler);
    
    // Run tests
    test_client_manager_init();
    test_client_registration();
    test_client_state_management();
    test_client_heartbeat();
    test_client_info_management();
    
    // Clean up
    cleanup();
    
    printf("All tests passed\n");
    
    return 0;
}
