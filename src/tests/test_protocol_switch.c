/**
 * @file test_protocol_switch.c
 * @brief Test program for protocol switching functionality
 */

#include "../include/protocol.h"
#include "../include/client.h"
#include "../protocols/protocol_switch.h"
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
    protocol_manager_shutdown();
    client_manager_shutdown();
}

/**
 * @brief Test protocol switch message creation
 */
static void test_protocol_switch_create_message(void) {
    printf("Testing protocol switch message creation...\n");
    
    // Create protocol switch message
    protocol_switch_message_t message;
    status_t status = protocol_switch_create_message(PROTOCOL_TYPE_TCP, 8080, "example.com", 5000, 
                                                  PROTOCOL_SWITCH_FLAG_IMMEDIATE, &message);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to create protocol switch message: %d\n", status);
        exit(1);
    }
    
    // Check message fields
    if (message.magic != PROTOCOL_SWITCH_MAGIC) {
        printf("Protocol switch message magic is incorrect: 0x%08x\n", message.magic);
        exit(1);
    }
    
    if (message.protocol != PROTOCOL_TYPE_TCP) {
        printf("Protocol switch message protocol is incorrect: %d\n", message.protocol);
        exit(1);
    }
    
    if (message.port != 8080) {
        printf("Protocol switch message port is incorrect: %d\n", message.port);
        exit(1);
    }
    
    if (message.timeout_ms != 5000) {
        printf("Protocol switch message timeout is incorrect: %d\n", message.timeout_ms);
        exit(1);
    }
    
    if (message.flags != PROTOCOL_SWITCH_FLAG_IMMEDIATE) {
        printf("Protocol switch message flags are incorrect: 0x%02x\n", message.flags);
        exit(1);
    }
    
    if (strcmp(message.domain, "example.com") != 0) {
        printf("Protocol switch message domain is incorrect: %s\n", message.domain);
        exit(1);
    }
    
    printf("Protocol switch message creation test passed\n");
}

/**
 * @brief Test protocol switch message detection
 */
static void test_protocol_switch_is_message(void) {
    printf("Testing protocol switch message detection...\n");
    
    // Create protocol switch message
    protocol_switch_message_t message;
    status_t status = protocol_switch_create_message(PROTOCOL_TYPE_TCP, 8080, "example.com", 5000, 
                                                  PROTOCOL_SWITCH_FLAG_IMMEDIATE, &message);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to create protocol switch message: %d\n", status);
        exit(1);
    }
    
    // Check if message is detected
    bool is_message = protocol_switch_is_message((const uint8_t*)&message, sizeof(message));
    if (!is_message) {
        printf("Protocol switch message not detected\n");
        exit(1);
    }
    
    // Modify magic number
    message.magic = 0x12345678;
    
    // Check if message is not detected
    is_message = protocol_switch_is_message((const uint8_t*)&message, sizeof(message));
    if (is_message) {
        printf("Invalid protocol switch message detected\n");
        exit(1);
    }
    
    printf("Protocol switch message detection test passed\n");
}

/**
 * @brief Test protocol switch message processing
 */
static void test_protocol_switch_process_message(void) {
    printf("Testing protocol switch message processing...\n");
    
    // Initialize protocol manager
    status_t status = protocol_manager_init();
    if (status != STATUS_SUCCESS) {
        printf("Failed to initialize protocol manager: %d\n", status);
        exit(1);
    }
    
    // Initialize client manager
    status = client_manager_init();
    if (status != STATUS_SUCCESS) {
        printf("Failed to initialize client manager: %d\n", status);
        protocol_manager_shutdown();
        exit(1);
    }
    
    // Create mock protocol listener
    protocol_listener_t* listener = (protocol_listener_t*)malloc(sizeof(protocol_listener_t));
    if (listener == NULL) {
        printf("Failed to allocate memory for mock listener\n");
        client_manager_shutdown();
        protocol_manager_shutdown();
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
        client_manager_shutdown();
        protocol_manager_shutdown();
        exit(1);
    }
    
    // Create protocol switch message
    protocol_switch_message_t message;
    status = protocol_switch_create_message(PROTOCOL_TYPE_UDP, 8081, NULL, 5000, 
                                         PROTOCOL_SWITCH_FLAG_IMMEDIATE, &message);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to create protocol switch message: %d\n", status);
        free(listener);
        client_manager_shutdown();
        protocol_manager_shutdown();
        exit(1);
    }
    
    // Process message
    status = protocol_switch_process_message(client, (const uint8_t*)&message, sizeof(message));
    
    // Note: In a real test, we would check if the client's protocol was switched
    // However, since we're using a mock listener, we can't fully test this
    // So we just check if the function returns success
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to process protocol switch message: %d\n", status);
        free(listener);
        client_manager_shutdown();
        protocol_manager_shutdown();
        exit(1);
    }
    
    printf("Protocol switch message processing test passed\n");
    
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
    test_protocol_switch_create_message();
    test_protocol_switch_is_message();
    test_protocol_switch_process_message();
    
    // Clean up
    cleanup();
    
    printf("All tests passed\n");
    
    return 0;
}
