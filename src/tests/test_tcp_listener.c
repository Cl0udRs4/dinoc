/**
 * @file test_tcp_listener.c
 * @brief Test program for TCP protocol listener
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
#define TEST_PORT 8080
#define TEST_MESSAGE "Hello, TCP!"
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
 * @brief Test TCP listener creation
 */
static void test_tcp_listener_create(void) {
    printf("Testing TCP listener creation...\n");
    
    // Create listener configuration
    protocol_listener_config_t config;
    memset(&config, 0, sizeof(config));
    
    config.bind_address = TEST_BIND_ADDRESS;
    config.port = TEST_PORT;
    config.timeout_ms = TEST_TIMEOUT_MS;
    
    // Create listener
    status_t status = tcp_listener_create(&config, &listener);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to create TCP listener: %d\n", status);
        exit(1);
    }
    
    // Register callbacks
    status = listener->register_callbacks(listener, on_message_received, on_client_connected, on_client_disconnected);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to register callbacks: %d\n", status);
        listener->destroy(listener);
        exit(1);
    }
    
    printf("TCP listener created successfully\n");
}

/**
 * @brief Test TCP listener start and stop
 */
static void test_tcp_listener_start_stop(void) {
    printf("Testing TCP listener start and stop...\n");
    
    // Start listener
    status_t status = listener->start(listener);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to start TCP listener: %d\n", status);
        listener->destroy(listener);
        exit(1);
    }
    
    printf("TCP listener started successfully\n");
    
    // Sleep for a bit
    sleep(1);
    
    // Stop listener
    status = listener->stop(listener);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to stop TCP listener: %d\n", status);
        listener->destroy(listener);
        exit(1);
    }
    
    printf("TCP listener stopped successfully\n");
    
    // Start listener again for message test
    status = listener->start(listener);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to restart TCP listener: %d\n", status);
        listener->destroy(listener);
        exit(1);
    }
    
    printf("TCP listener restarted successfully\n");
}

/**
 * @brief Test TCP message sending and receiving
 */
static void test_tcp_message_send_receive(void) {
    printf("Testing TCP message sending and receiving...\n");
    
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
    
    printf("TCP message test completed successfully\n");
}

/**
 * @brief Client thread function
 */
static void* client_thread(void* arg) {
    // Sleep for a bit to allow server to start
    sleep(1);
    
    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sock < 0) {
        perror("socket");
        return NULL;
    }
    
    // Connect to server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(TEST_BIND_ADDRESS);
    server_addr.sin_port = htons(TEST_PORT);
    
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        return NULL;
    }
    
    printf("Connected to TCP server\n");
    
    // Send message length
    uint32_t length = htonl((uint32_t)strlen(TEST_MESSAGE));
    if (send(sock, &length, sizeof(length), 0) != sizeof(length)) {
        perror("send length");
        close(sock);
        return NULL;
    }
    
    // Send message
    if (send(sock, TEST_MESSAGE, strlen(TEST_MESSAGE), 0) != strlen(TEST_MESSAGE)) {
        perror("send message");
        close(sock);
        return NULL;
    }
    
    printf("Message sent: %s\n", TEST_MESSAGE);
    
    // Receive response length
    uint32_t response_length_network;
    if (recv(sock, &response_length_network, sizeof(response_length_network), 0) != sizeof(response_length_network)) {
        perror("recv length");
        close(sock);
        return NULL;
    }
    
    uint32_t response_length = ntohl(response_length_network);
    
    // Allocate buffer for response
    char* buffer = (char*)malloc(response_length + 1);
    
    if (buffer == NULL) {
        close(sock);
        return NULL;
    }
    
    // Receive response
    size_t total_received = 0;
    
    while (total_received < response_length) {
        ssize_t recv_result = recv(sock, buffer + total_received, response_length - total_received, 0);
        
        if (recv_result <= 0) {
            perror("recv message");
            free(buffer);
            close(sock);
            return NULL;
        }
        
        total_received += recv_result;
    }
    
    buffer[response_length] = '\0';
    printf("Response received: %s\n", buffer);
    
    // Clean up
    free(buffer);
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
    test_tcp_listener_create();
    test_tcp_listener_start_stop();
    test_tcp_message_send_receive();
    
    // Clean up
    cleanup();
    protocol_manager_shutdown();
    
    printf("All tests completed successfully\n");
    
    return 0;
}
