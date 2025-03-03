/**
 * @file test_udp_listener.c
 * @brief Test program for UDP protocol listener
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Test configuration
#define TEST_BIND_ADDRESS "127.0.0.1"
#define TEST_PORT 8081
#define TEST_MESSAGE "Hello, UDP!"
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
 * @brief Test UDP listener creation
 */
static void test_udp_listener_create(void) {
    printf("Testing UDP listener creation...\n");
    
    // Create listener configuration
    protocol_listener_config_t config;
    memset(&config, 0, sizeof(config));
    
    config.bind_address = TEST_BIND_ADDRESS;
    config.port = TEST_PORT;
    config.timeout_ms = TEST_TIMEOUT_MS;
    
    // Create listener
    status_t status = udp_listener_create(&config, &listener);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to create UDP listener: %d\n", status);
        exit(1);
    }
    
    // Register callbacks
    status = listener->register_callbacks(listener, on_message_received, on_client_connected, on_client_disconnected);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to register callbacks: %d\n", status);
        listener->destroy(listener);
        exit(1);
    }
    
    printf("UDP listener created successfully\n");
}

/**
 * @brief Test UDP listener start and stop
 */
static void test_udp_listener_start_stop(void) {
    printf("Testing UDP listener start and stop...\n");
    
    // Start listener
    status_t status = listener->start(listener);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to start UDP listener: %d\n", status);
        listener->destroy(listener);
        exit(1);
    }
    
    printf("UDP listener started successfully\n");
    
    // Sleep for a bit
    sleep(1);
    
    // Stop listener
    status = listener->stop(listener);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to stop UDP listener: %d\n", status);
        listener->destroy(listener);
        exit(1);
    }
    
    printf("UDP listener stopped successfully\n");
    
    // Start listener again for message test
    status = listener->start(listener);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to restart UDP listener: %d\n", status);
        listener->destroy(listener);
        exit(1);
    }
    
    printf("UDP listener restarted successfully\n");
}

/**
 * @brief Test UDP message sending and receiving
 */
static void test_udp_message_send_receive(void) {
    printf("Testing UDP message sending and receiving...\n");
    
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
    
    printf("UDP message test completed successfully\n");
}

/**
 * @brief Client thread function
 */
static void* client_thread(void* arg) {
    // Sleep for a bit to allow server to start
    sleep(1);
    
    // Create socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (sock < 0) {
        perror("socket");
        return NULL;
    }
    
    // Set receive timeout
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt");
        close(sock);
        return NULL;
    }
    
    // Prepare server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(TEST_BIND_ADDRESS);
    server_addr.sin_port = htons(TEST_PORT);
    
    // Send message
    if (sendto(sock, TEST_MESSAGE, strlen(TEST_MESSAGE), 0, 
              (struct sockaddr*)&server_addr, sizeof(server_addr)) != strlen(TEST_MESSAGE)) {
        perror("sendto");
        close(sock);
        return NULL;
    }
    
    printf("Message sent: %s\n", TEST_MESSAGE);
    
    // Receive response
    char buffer[1024];
    struct sockaddr_in from_addr;
    socklen_t from_addr_len = sizeof(from_addr);
    
    ssize_t recv_len = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, 
                               (struct sockaddr*)&from_addr, &from_addr_len);
    
    if (recv_len < 0) {
        perror("recvfrom");
        close(sock);
        return NULL;
    }
    
    buffer[recv_len] = '\0';
    printf("Response received: %s\n", buffer);
    
    // Close socket
    close(sock);
    
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
    test_udp_listener_create();
    test_udp_listener_start_stop();
    test_udp_message_send_receive();
    
    // Clean up
    cleanup();
    protocol_manager_shutdown();
    
    printf("All tests completed successfully\n");
    
    return 0;
}
