/**
 * @file test_protocol_fragmentation.c
 * @brief Test program for protocol fragmentation
 */

#include "../include/protocol.h"
#include "../include/common.h"
#include "../include/client.h"
#include "../protocols/protocol_fragmentation.h"
#include "../common/uuid.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

// Test configuration
#define TEST_MESSAGE "This is a test message that will be fragmented into multiple pieces to test the protocol fragmentation system. It needs to be long enough to be split into multiple fragments."
#define TEST_MAX_FRAGMENT_SIZE 32
#define TEST_TIMEOUT_MS 5000

// Global variables
static bool message_reassembled = false;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// Forward declarations
static void on_message_reassembled(protocol_listener_t* listener, client_t* client, protocol_message_t* message);
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
    fragmentation_shutdown();
}

/**
 * @brief Test fragmentation and reassembly
 */
static void test_fragmentation_reassembly(void) {
    printf("Testing fragmentation and reassembly...\n");
    
    // Initialize fragmentation system
    status_t status = fragmentation_init();
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to initialize fragmentation system: %d\n", status);
        exit(1);
    }
    
    // Create client
    client_t* client = client_create();
    
    if (client == NULL) {
        printf("Failed to create client\n");
        cleanup();
        exit(1);
    }
    
    // Create mock protocol listener
    protocol_listener_t listener;
    memset(&listener, 0, sizeof(listener));
    uuid_generate_compat(&listener.id);
    listener.protocol_type = PROTOCOL_TYPE_TCP;
    
    // Mock send_message function to avoid segmentation fault
    listener.send_message = NULL; // We won't actually call this in the test
    
    // Create message
    uint8_t* data = (uint8_t*)strdup(TEST_MESSAGE);
    size_t data_len = strlen(TEST_MESSAGE);
    
    // Fragment message
    status = fragmentation_send_message(&listener, client, data, data_len, TEST_MAX_FRAGMENT_SIZE);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to fragment message: %d\n", status);
        free(data);
        client_destroy(client);
        cleanup();
        exit(1);
    }
    
    // Simulate receiving fragments
    // In a real scenario, these would be received from the network
    
    // Create fragment header
    fragment_header_t header;
    uint16_t fragment_id = 1234;
    uint8_t total_fragments = (data_len + TEST_MAX_FRAGMENT_SIZE - 1) / TEST_MAX_FRAGMENT_SIZE;
    
    printf("Fragmenting message into %d fragments\n", total_fragments);
    
    // Send fragments in reverse order to test out-of-order reassembly
    for (int i = total_fragments - 1; i >= 0; i--) {
        // Calculate fragment size
        size_t offset = i * TEST_MAX_FRAGMENT_SIZE;
        size_t fragment_size = data_len - offset;
        
        if (fragment_size > TEST_MAX_FRAGMENT_SIZE) {
            fragment_size = TEST_MAX_FRAGMENT_SIZE;
        }
        
        // Create fragment header
        status = fragmentation_create_header(fragment_id, i, total_fragments, 0, &header);
        
        if (status != STATUS_SUCCESS) {
            printf("Failed to create fragment header: %d\n", status);
            free(data);
            client_destroy(client);
            cleanup();
            exit(1);
        }
        
        // Create fragment data
        uint8_t* fragment = (uint8_t*)malloc(sizeof(fragment_header_t) + fragment_size);
        
        if (fragment == NULL) {
            printf("Failed to allocate fragment\n");
            free(data);
            client_destroy(client);
            cleanup();
            exit(1);
        }
        
        // Copy header
        memcpy(fragment, &header, sizeof(fragment_header_t));
        
        // Copy data
        memcpy(fragment + sizeof(fragment_header_t), data + offset, fragment_size);
        
        // Process fragment
        status = fragmentation_process_fragment(&listener, client, fragment, sizeof(fragment_header_t) + fragment_size, on_message_reassembled);
        
        if (status != STATUS_SUCCESS) {
            printf("Failed to process fragment: %d\n", status);
            free(fragment);
            free(data);
            client_destroy(client);
            cleanup();
            exit(1);
        }
        
        free(fragment);
    }
    
    // Wait for message to be reassembled
    pthread_mutex_lock(&mutex);
    
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 5; // 5 second timeout
    
    while (!message_reassembled) {
        int result = pthread_cond_timedwait(&cond, &mutex, &ts);
        
        if (result == ETIMEDOUT) {
            printf("Timeout waiting for message reassembly\n");
            pthread_mutex_unlock(&mutex);
            free(data);
            client_destroy(client);
            cleanup();
            exit(1);
        }
    }
    
    pthread_mutex_unlock(&mutex);
    
    // Clean up
    free(data);
    client_destroy(client);
    
    printf("Fragmentation and reassembly test completed successfully\n");
}

/**
 * @brief Test fragment timeout
 */
static void test_fragment_timeout(void) {
    printf("Testing fragment timeout...\n");
    
    // Initialize fragmentation system
    status_t status = fragmentation_init();
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to initialize fragmentation system: %d\n", status);
        exit(1);
    }
    
    // Create client
    client_t* client = client_create();
    
    if (client == NULL) {
        printf("Failed to create client\n");
        cleanup();
        exit(1);
    }
    
    // Create mock protocol listener
    protocol_listener_t listener;
    memset(&listener, 0, sizeof(listener));
    uuid_generate_compat(&listener.id);
    listener.protocol_type = PROTOCOL_TYPE_TCP;
    
    // Mock send_message function to avoid segmentation fault
    listener.send_message = NULL; // We won't actually call this in the test
    
    // Create message
    uint8_t* data = (uint8_t*)strdup(TEST_MESSAGE);
    size_t data_len = strlen(TEST_MESSAGE);
    
    // Fragment message
    status = fragmentation_send_message(&listener, client, data, data_len, TEST_MAX_FRAGMENT_SIZE);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to fragment message: %d\n", status);
        free(data);
        client_destroy(client);
        cleanup();
        exit(1);
    }
    
    // Simulate receiving only some fragments
    // In a real scenario, these would be received from the network
    
    // Create fragment header
    fragment_header_t header;
    uint16_t fragment_id = 5678;
    uint8_t total_fragments = (data_len + TEST_MAX_FRAGMENT_SIZE - 1) / TEST_MAX_FRAGMENT_SIZE;
    
    printf("Fragmenting message into %d fragments, but only sending half\n", total_fragments);
    
    // Send only half of the fragments
    for (int i = 0; i < total_fragments / 2; i++) {
        // Calculate fragment size
        size_t offset = i * TEST_MAX_FRAGMENT_SIZE;
        size_t fragment_size = data_len - offset;
        
        if (fragment_size > TEST_MAX_FRAGMENT_SIZE) {
            fragment_size = TEST_MAX_FRAGMENT_SIZE;
        }
        
        // Create fragment header
        status = fragmentation_create_header(fragment_id, i, total_fragments, 0, &header);
        
        if (status != STATUS_SUCCESS) {
            printf("Failed to create fragment header: %d\n", status);
            free(data);
            client_destroy(client);
            cleanup();
            exit(1);
        }
        
        // Create fragment data
        uint8_t* fragment = (uint8_t*)malloc(sizeof(fragment_header_t) + fragment_size);
        
        if (fragment == NULL) {
            printf("Failed to allocate fragment\n");
            free(data);
            client_destroy(client);
            cleanup();
            exit(1);
        }
        
        // Copy header
        memcpy(fragment, &header, sizeof(fragment_header_t));
        
        // Copy data
        memcpy(fragment + sizeof(fragment_header_t), data + offset, fragment_size);
        
        // Process fragment
        status = fragmentation_process_fragment(&listener, client, fragment, sizeof(fragment_header_t) + fragment_size, on_message_reassembled);
        
        if (status != STATUS_SUCCESS) {
            printf("Failed to process fragment: %d\n", status);
            free(fragment);
            free(data);
            client_destroy(client);
            cleanup();
            exit(1);
        }
        
        free(fragment);
    }
    
    // Wait for timeout
    printf("Waiting for fragment timeout...\n");
    sleep(5);
    
    // Clean up
    free(data);
    client_destroy(client);
    
    printf("Fragment timeout test completed successfully\n");
}

/**
 * @brief Message reassembled callback
 */
static void on_message_reassembled(protocol_listener_t* listener, client_t* client, protocol_message_t* message) {
    (void)listener; // Unused parameter
    (void)client;   // Unused parameter
    
    printf("Message reassembled: %.*s\n", (int)message->data_len, message->data);
    
    // Check if this is the test message
    if (message->data_len == strlen(TEST_MESSAGE) &&
        memcmp(message->data, TEST_MESSAGE, message->data_len) == 0) {
        pthread_mutex_lock(&mutex);
        message_reassembled = true;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }
}

/**
 * @brief Main function
 */
int main(int argc, char** argv) {
    (void)argc; // Unused parameter
    (void)argv; // Unused parameter
    
    // Set up signal handler
    signal(SIGINT, signal_handler);
    
    // Run tests
    test_fragmentation_reassembly();
    
    // Shutdown fragmentation system before running the next test
    fragmentation_shutdown();
    
    test_fragment_timeout();
    
    // Clean up
    cleanup();
    
    printf("All tests completed successfully\n");
    
    return 0;
}
