/**
 * @file test_client_registration.c
 * @brief Test program for client registration functionality
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
    protocol_manager_shutdown();
}

/**
 * @brief Test client registration with TCP protocol
 */
static void test_client_registration_tcp(void) {
    printf("Testing client registration with TCP protocol...\n");
    
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
    
    // Create mock protocol context
    void* context = malloc(100);
    if (context == NULL) {
        printf("Failed to allocate memory for mock context\n");
        free(listener);
        client_manager_shutdown();
        protocol_manager_shutdown();
        exit(1);
    }
    
    // Register client
    client_t* client = NULL;
    status = client_register(listener, context, &client);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to register client: %d\n", status);
        free(context);
        free(listener);
        client_manager_shutdown();
        protocol_manager_shutdown();
        exit(1);
    }
    
    if (client == NULL) {
        printf("Client is NULL\n");
        free(context);
        free(listener);
        client_manager_shutdown();
        protocol_manager_shutdown();
        exit(1);
    }
    
    // Check client fields
    if (client->protocol_type != PROTOCOL_TYPE_TCP) {
        printf("Client protocol type is incorrect: %d\n", client->protocol_type);
        free(context);
        free(listener);
        client_manager_shutdown();
        protocol_manager_shutdown();
        exit(1);
    }
    
    if (client->protocol_context != context) {
        printf("Client protocol context is incorrect\n");
        free(context);
        free(listener);
        client_manager_shutdown();
        protocol_manager_shutdown();
        exit(1);
    }
    
    if (client->state != CLIENT_STATE_NEW) {
        printf("Client state is incorrect: %d\n", client->state);
        free(context);
        free(listener);
        client_manager_shutdown();
        protocol_manager_shutdown();
        exit(1);
    }
    
    printf("Client registration with TCP protocol test passed\n");
    
    // Clean up
    free(listener);
}

/**
 * @brief Test client registration with UDP protocol
 */
static void test_client_registration_udp(void) {
    printf("Testing client registration with UDP protocol...\n");
    
    // Create mock protocol listener
    protocol_listener_t* listener = (protocol_listener_t*)malloc(sizeof(protocol_listener_t));
    if (listener == NULL) {
        printf("Failed to allocate memory for mock listener\n");
        exit(1);
    }
    
    // Initialize mock listener
    memset(listener, 0, sizeof(protocol_listener_t));
    listener->protocol_type = PROTOCOL_TYPE_UDP;
    
    // Create mock protocol context
    void* context = malloc(100);
    if (context == NULL) {
        printf("Failed to allocate memory for mock context\n");
        free(listener);
        exit(1);
    }
    
    // Register client
    client_t* client = NULL;
    status_t status = client_register(listener, context, &client);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to register client: %d\n", status);
        free(context);
        free(listener);
        exit(1);
    }
    
    if (client == NULL) {
        printf("Client is NULL\n");
        free(context);
        free(listener);
        exit(1);
    }
    
    // Check client fields
    if (client->protocol_type != PROTOCOL_TYPE_UDP) {
        printf("Client protocol type is incorrect: %d\n", client->protocol_type);
        free(context);
        free(listener);
        exit(1);
    }
    
    if (client->protocol_context != context) {
        printf("Client protocol context is incorrect\n");
        free(context);
        free(listener);
        exit(1);
    }
    
    if (client->state != CLIENT_STATE_NEW) {
        printf("Client state is incorrect: %d\n", client->state);
        free(context);
        free(listener);
        exit(1);
    }
    
    printf("Client registration with UDP protocol test passed\n");
    
    // Clean up
    free(listener);
}

/**
 * @brief Test client registration with WebSocket protocol
 */
static void test_client_registration_ws(void) {
    printf("Testing client registration with WebSocket protocol...\n");
    
    // Create mock protocol listener
    protocol_listener_t* listener = (protocol_listener_t*)malloc(sizeof(protocol_listener_t));
    if (listener == NULL) {
        printf("Failed to allocate memory for mock listener\n");
        exit(1);
    }
    
    // Initialize mock listener
    memset(listener, 0, sizeof(protocol_listener_t));
    listener->protocol_type = PROTOCOL_TYPE_WS;
    
    // Create mock protocol context
    void* context = malloc(100);
    if (context == NULL) {
        printf("Failed to allocate memory for mock context\n");
        free(listener);
        exit(1);
    }
    
    // Register client
    client_t* client = NULL;
    status_t status = client_register(listener, context, &client);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to register client: %d\n", status);
        free(context);
        free(listener);
        exit(1);
    }
    
    if (client == NULL) {
        printf("Client is NULL\n");
        free(context);
        free(listener);
        exit(1);
    }
    
    // Check client fields
    if (client->protocol_type != PROTOCOL_TYPE_WS) {
        printf("Client protocol type is incorrect: %d\n", client->protocol_type);
        free(context);
        free(listener);
        exit(1);
    }
    
    if (client->protocol_context != context) {
        printf("Client protocol context is incorrect\n");
        free(context);
        free(listener);
        exit(1);
    }
    
    if (client->state != CLIENT_STATE_NEW) {
        printf("Client state is incorrect: %d\n", client->state);
        free(context);
        free(listener);
        exit(1);
    }
    
    printf("Client registration with WebSocket protocol test passed\n");
    
    // Clean up
    free(listener);
}

/**
 * @brief Test client registration with ICMP protocol
 */
static void test_client_registration_icmp(void) {
    printf("Testing client registration with ICMP protocol...\n");
    
    // Create mock protocol listener
    protocol_listener_t* listener = (protocol_listener_t*)malloc(sizeof(protocol_listener_t));
    if (listener == NULL) {
        printf("Failed to allocate memory for mock listener\n");
        exit(1);
    }
    
    // Initialize mock listener
    memset(listener, 0, sizeof(protocol_listener_t));
    listener->protocol_type = PROTOCOL_TYPE_ICMP;
    
    // Create mock protocol context
    void* context = malloc(100);
    if (context == NULL) {
        printf("Failed to allocate memory for mock context\n");
        free(listener);
        exit(1);
    }
    
    // Register client
    client_t* client = NULL;
    status_t status = client_register(listener, context, &client);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to register client: %d\n", status);
        free(context);
        free(listener);
        exit(1);
    }
    
    if (client == NULL) {
        printf("Client is NULL\n");
        free(context);
        free(listener);
        exit(1);
    }
    
    // Check client fields
    if (client->protocol_type != PROTOCOL_TYPE_ICMP) {
        printf("Client protocol type is incorrect: %d\n", client->protocol_type);
        free(context);
        free(listener);
        exit(1);
    }
    
    if (client->protocol_context != context) {
        printf("Client protocol context is incorrect\n");
        free(context);
        free(listener);
        exit(1);
    }
    
    if (client->state != CLIENT_STATE_NEW) {
        printf("Client state is incorrect: %d\n", client->state);
        free(context);
        free(listener);
        exit(1);
    }
    
    printf("Client registration with ICMP protocol test passed\n");
    
    // Clean up
    free(listener);
}

/**
 * @brief Test client registration with DNS protocol
 */
static void test_client_registration_dns(void) {
    printf("Testing client registration with DNS protocol...\n");
    
    // Create mock protocol listener
    protocol_listener_t* listener = (protocol_listener_t*)malloc(sizeof(protocol_listener_t));
    if (listener == NULL) {
        printf("Failed to allocate memory for mock listener\n");
        exit(1);
    }
    
    // Initialize mock listener
    memset(listener, 0, sizeof(protocol_listener_t));
    listener->protocol_type = PROTOCOL_TYPE_DNS;
    
    // Create mock protocol context
    void* context = malloc(100);
    if (context == NULL) {
        printf("Failed to allocate memory for mock context\n");
        free(listener);
        exit(1);
    }
    
    // Register client
    client_t* client = NULL;
    status_t status = client_register(listener, context, &client);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to register client: %d\n", status);
        free(context);
        free(listener);
        exit(1);
    }
    
    if (client == NULL) {
        printf("Client is NULL\n");
        free(context);
        free(listener);
        exit(1);
    }
    
    // Check client fields
    if (client->protocol_type != PROTOCOL_TYPE_DNS) {
        printf("Client protocol type is incorrect: %d\n", client->protocol_type);
        free(context);
        free(listener);
        exit(1);
    }
    
    if (client->protocol_context != context) {
        printf("Client protocol context is incorrect\n");
        free(context);
        free(listener);
        exit(1);
    }
    
    if (client->state != CLIENT_STATE_NEW) {
        printf("Client state is incorrect: %d\n", client->state);
        free(context);
        free(listener);
        exit(1);
    }
    
    printf("Client registration with DNS protocol test passed\n");
    
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
    test_client_registration_tcp();
    test_client_registration_udp();
    test_client_registration_ws();
    test_client_registration_icmp();
    test_client_registration_dns();
    
    // Clean up
    cleanup();
    
    printf("All tests passed\n");
    
    return 0;
}
