/**
 * @file test_ws_listener.c
 * @brief Test program for WebSocket protocol listener
 */

#include "../include/protocol.h"
#include "../include/common.h"
#include "../include/client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

// Test configuration
#define TEST_BIND_ADDRESS "127.0.0.1"
#define TEST_PORT 9001
#define TEST_WS_PATH "/ws"
#define TEST_MESSAGE "Hello, WebSocket!"
#define TEST_TIMEOUT_MS 5000

// Global variables
static protocol_listener_t* listener = NULL;
static client_t* test_client = NULL;
static bool message_received = false;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// Forward declarations
static void on_message_received(protocol_listener_t* listener, client_t* client, protocol_message_t* message);
static void on_client_connected(protocol_listener_t* listener, client_t* client);
static void on_client_disconnected(protocol_listener_t* listener, client_t* client);
static void* client_thread(void* arg);
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
    if (listener != NULL) {
        listener->stop(listener);
        listener->destroy(listener);
        listener = NULL;
    }
}

/**
 * @brief Test WebSocket listener creation
 */
static void test_ws_listener_create(void) {
    printf("Testing WebSocket listener creation...\n");
    
    // Create listener configuration
    protocol_listener_config_t config;
    memset(&config, 0, sizeof(config));
    
    config.bind_address = TEST_BIND_ADDRESS;
    config.port = TEST_PORT;
    config.timeout_ms = TEST_TIMEOUT_MS;
    config.ws_path = TEST_WS_PATH;
    
    // Create listener
    status_t status = ws_listener_create(&config, &listener);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to create WebSocket listener: %d\n", status);
        exit(1);
    }
    
    // Register callbacks
    status = listener->register_callbacks(listener, on_message_received, on_client_connected, on_client_disconnected);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to register callbacks: %d\n", status);
        listener->destroy(listener);
        exit(1);
    }
    
    printf("WebSocket listener created successfully\n");
}

/**
 * @brief Test WebSocket listener start and stop
 */
static void test_ws_listener_start_stop(void) {
    printf("Testing WebSocket listener start and stop...\n");
    
    // Start listener
    status_t status = listener->start(listener);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to start WebSocket listener: %d\n", status);
        listener->destroy(listener);
        exit(1);
    }
    
    printf("WebSocket listener started successfully\n");
    
    // Sleep for a bit
    sleep(1);
    
    // Stop listener
    status = listener->stop(listener);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to stop WebSocket listener: %d\n", status);
        listener->destroy(listener);
        exit(1);
    }
    
    printf("WebSocket listener stopped successfully\n");
    
    // Start listener again for message test
    status = listener->start(listener);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to restart WebSocket listener: %d\n", status);
        listener->destroy(listener);
        exit(1);
    }
    
    printf("WebSocket listener restarted successfully\n");
}

/**
 * @brief Test WebSocket message sending and receiving
 */
static void test_ws_message_send_receive(void) {
    printf("Testing WebSocket message sending and receiving...\n");
    
    // Create client thread
    pthread_t thread;
    
    if (pthread_create(&thread, NULL, client_thread, NULL) != 0) {
        printf("Failed to create client thread\n");
        cleanup();
        exit(1);
    }
    
    // Wait for message to be received
    pthread_mutex_lock(&mutex);
    
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 5; // 5 second timeout
    
    while (!message_received) {
        int result = pthread_cond_timedwait(&cond, &mutex, &ts);
        
        if (result == ETIMEDOUT) {
            printf("Timeout waiting for message\n");
            pthread_mutex_unlock(&mutex);
            cleanup();
            exit(1);
        }
    }
    
    pthread_mutex_unlock(&mutex);
    
    // Wait for client thread to exit
    pthread_join(thread, NULL);
    
    printf("WebSocket message test completed successfully\n");
}

/**
 * @brief Client thread function
 */
static void* client_thread(void* arg) {
    // Sleep for a bit to allow server to start
    sleep(1);
    
    // Create a real WebSocket client connection
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = NULL;
    info.gid = -1;
    info.uid = -1;
    
    struct lws_context* context = lws_create_context(&info);
    if (!context) {
        printf("Failed to create LWS context\n");
        return NULL;
    }
    
    struct lws_client_connect_info conn_info;
    memset(&conn_info, 0, sizeof(conn_info));
    
    conn_info.context = context;
    conn_info.address = TEST_BIND_ADDRESS;
    conn_info.port = TEST_PORT;
    conn_info.path = TEST_WS_PATH;
    conn_info.host = conn_info.address;
    conn_info.origin = conn_info.address;
    conn_info.protocol = "dinoc-protocol";
    
    struct lws* wsi = lws_client_connect_via_info(&conn_info);
    if (!wsi) {
        printf("Failed to connect to WebSocket server\n");
        lws_context_destroy(context);
        return NULL;
    }
    
    printf("Connected to WebSocket server\n");
    
    // Send test message
    uint8_t* buffer = (uint8_t*)malloc(LWS_PRE + strlen(TEST_MESSAGE));
    if (!buffer) {
        printf("Failed to allocate message buffer\n");
        lws_context_destroy(context);
        return NULL;
    }
    
    memcpy(buffer + LWS_PRE, TEST_MESSAGE, strlen(TEST_MESSAGE));
    
    // Request write callback
    lws_callback_on_writable(wsi);
    
    // Service the LWS context for a few seconds to allow message exchange
    time_t start_time = time(NULL);
    while (time(NULL) - start_time < 5 && !message_received) {
        lws_service(context, 100);
        
        // Check if we can write
        if (lws_is_writable(wsi)) {
            int result = lws_write(wsi, buffer + LWS_PRE, strlen(TEST_MESSAGE), LWS_WRITE_BINARY);
            if (result < 0) {
                printf("Failed to send message\n");
                break;
            }
            printf("Message sent\n");
        }
    }
    
    // Clean up
    free(buffer);
    lws_context_destroy(context);
    
    return NULL;
}

/**
 * @brief Message received callback
 */
static void on_message_received(protocol_listener_t* listener, client_t* client, protocol_message_t* message) {
    printf("Message received: %.*s\n", (int)message->data_len, message->data);
    
    // Check if this is the test message
    if (message->data_len == strlen(TEST_MESSAGE) &&
        memcmp(message->data, TEST_MESSAGE, message->data_len) == 0) {
        pthread_mutex_lock(&mutex);
        message_received = true;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }
    
    // Echo message back to client
    listener->send_message(listener, client, message);
}

/**
 * @brief Client connected callback
 */
static void on_client_connected(protocol_listener_t* listener, client_t* client) {
    printf("Client connected\n");
    test_client = client;
}

/**
 * @brief Client disconnected callback
 */
static void on_client_disconnected(protocol_listener_t* listener, client_t* client) {
    printf("Client disconnected\n");
    
    if (test_client == client) {
        test_client = NULL;
    }
}

/**
 * @brief Main function
 */
int main(int argc, char** argv) {
    // Set up signal handler
    signal(SIGINT, signal_handler);
    
    // Initialize protocol manager
    status_t status = protocol_manager_init();
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to initialize protocol manager: %d\n", status);
        return 1;
    }
    
    // Run tests
    test_ws_listener_create();
    test_ws_listener_start_stop();
    test_ws_message_send_receive();
    
    // Clean up
    cleanup();
    protocol_manager_shutdown();
    
    printf("All tests completed successfully\n");
    
    return 0;
}
