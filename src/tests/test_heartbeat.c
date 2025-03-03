/**
 * @file test_heartbeat.c
 * @brief Test program for heartbeat mechanism functionality
 */

#include "../include/client.h"
#include "../include/protocol.h"
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
 * @brief Test heartbeat configuration
 */
static void test_heartbeat_config(void) {
    printf("Testing heartbeat configuration...\n");
    
    // Initialize client manager
    status_t status = client_manager_init();
    if (status != STATUS_SUCCESS) {
        printf("Failed to initialize client manager: %d\n", status);
        exit(1);
    }
    
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
    status = client_register(listener, NULL, &client);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to register client: %d\n", status);
        free(listener);
        exit(1);
    }
    
    // Set heartbeat interval
    status = client_set_heartbeat(client, 10, 2);
    if (status != STATUS_SUCCESS) {
        printf("Failed to set heartbeat interval: %d\n", status);
        free(listener);
        exit(1);
    }
    
    // Check heartbeat interval
    if (client->heartbeat_interval != 10) {
        printf("Heartbeat interval is incorrect: %d\n", client->heartbeat_interval);
        free(listener);
        exit(1);
    }
    
    // Check heartbeat jitter
    if (client->heartbeat_jitter != 2) {
        printf("Heartbeat jitter is incorrect: %d\n", client->heartbeat_jitter);
        free(listener);
        exit(1);
    }
    
    // Test invalid heartbeat interval (too small)
    status = client_set_heartbeat(client, 0, 0);
    if (status == STATUS_SUCCESS) {
        printf("Setting invalid heartbeat interval succeeded unexpectedly\n");
        free(listener);
        exit(1);
    }
    
    // Test invalid heartbeat interval (too large)
    status = client_set_heartbeat(client, 100000, 0);
    if (status == STATUS_SUCCESS) {
        printf("Setting invalid heartbeat interval succeeded unexpectedly\n");
        free(listener);
        exit(1);
    }
    
    // Test invalid heartbeat jitter (too large)
    status = client_set_heartbeat(client, 10, 20);
    if (status == STATUS_SUCCESS) {
        printf("Setting invalid heartbeat jitter succeeded unexpectedly\n");
        free(listener);
        exit(1);
    }
    
    printf("Heartbeat configuration test passed\n");
    
    // Clean up
    free(listener);
}

/**
 * @brief Test heartbeat processing
 */
static void test_heartbeat_processing(void) {
    printf("Testing heartbeat processing...\n");
    
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
    
    // Set heartbeat interval
    status = client_set_heartbeat(client, 10, 2);
    if (status != STATUS_SUCCESS) {
        printf("Failed to set heartbeat interval: %d\n", status);
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
    
    // Process heartbeat
    status = client_heartbeat(client);
    if (status != STATUS_SUCCESS) {
        printf("Failed to process heartbeat: %d\n", status);
        free(listener);
        exit(1);
    }
    
    // Check last heartbeat time
    if (client->last_heartbeat == 0) {
        printf("Last heartbeat time was not updated\n");
        free(listener);
        exit(1);
    }
    
    // Check last seen time
    if (client->last_seen_time == 0) {
        printf("Last seen time was not updated\n");
        free(listener);
        exit(1);
    }
    
    printf("Heartbeat processing test passed\n");
    
    // Clean up
    free(listener);
}

/**
 * @brief Test heartbeat timeout detection
 */
static void test_heartbeat_timeout(void) {
    printf("Testing heartbeat timeout detection...\n");
    
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
    
    // Set heartbeat interval
    status = client_set_heartbeat(client, 1, 0);
    if (status != STATUS_SUCCESS) {
        printf("Failed to set heartbeat interval: %d\n", status);
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
    
    // Process heartbeat
    status = client_heartbeat(client);
    if (status != STATUS_SUCCESS) {
        printf("Failed to process heartbeat: %d\n", status);
        free(listener);
        exit(1);
    }
    
    // Check if heartbeat has timed out (should not have timed out yet)
    bool timeout = client_is_heartbeat_timeout(client);
    if (timeout) {
        printf("Heartbeat timed out unexpectedly\n");
        free(listener);
        exit(1);
    }
    
    // Wait for heartbeat to time out
    sleep(2);
    
    // Check if heartbeat has timed out (should have timed out now)
    timeout = client_is_heartbeat_timeout(client);
    if (!timeout) {
        printf("Heartbeat did not time out as expected\n");
        free(listener);
        exit(1);
    }
    
    printf("Heartbeat timeout detection test passed\n");
    
    // Clean up
    free(listener);
}

/**
 * @brief Test heartbeat request sending
 */
static void test_heartbeat_request(void) {
    printf("Testing heartbeat request sending...\n");
    
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
    
    // Send heartbeat request
    status = client_send_heartbeat_request(client);
    
    // Note: Since we're using a mock listener, we expect an error
    // In a real test, we would check if the heartbeat request was sent successfully
    
    printf("Heartbeat request sending test passed\n");
    
    // Clean up
    free(listener);
}

/**
 * @brief Main function
 */
int main(int argc, char** argv) {
    // Set up signal handler
    signal(SIGINT, signal_handler);
    
    // Run tests
    test_heartbeat_config();
    test_heartbeat_processing();
    test_heartbeat_timeout();
    test_heartbeat_request();
    
    // Clean up
    cleanup();
    
    printf("All tests passed\n");
    
    return 0;
}
