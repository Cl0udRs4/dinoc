/**
 * @file test_dns_listener.c
 * @brief Test program for DNS protocol listener
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
#include <arpa/inet.h>
#include <netdb.h>
#include <ares.h>

// Test configuration
#define TEST_DOMAIN "test.example.com"
#define TEST_PORT 53
#define TEST_MESSAGE "Hello, DNS!"
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
static void dns_query_callback(void* arg, int status, int timeouts, unsigned char* abuf, int alen);

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
 * @brief Test DNS listener creation
 */
static void test_dns_listener_create(void) {
    printf("Testing DNS listener creation...\n");
    
    // Create listener configuration
    protocol_listener_config_t config;
    memset(&config, 0, sizeof(config));
    
    config.domain = TEST_DOMAIN;
    config.port = TEST_PORT;
    config.timeout_ms = TEST_TIMEOUT_MS;
    
    // Create listener
    status_t status = dns_listener_create(&config, &listener);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to create DNS listener: %d\n", status);
        exit(1);
    }
    
    // Register callbacks
    status = listener->register_callbacks(listener, on_message_received, on_client_connected, on_client_disconnected);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to register callbacks: %d\n", status);
        listener->destroy(listener);
        exit(1);
    }
    
    printf("DNS listener created successfully\n");
}

/**
 * @brief Test DNS listener start and stop
 */
static void test_dns_listener_start_stop(void) {
    printf("Testing DNS listener start and stop...\n");
    
    // Start listener
    status_t status = listener->start(listener);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to start DNS listener: %d\n", status);
        listener->destroy(listener);
        exit(1);
    }
    
    printf("DNS listener started successfully\n");
    
    // Sleep for a bit
    sleep(1);
    
    // Stop listener
    status = listener->stop(listener);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to stop DNS listener: %d\n", status);
        listener->destroy(listener);
        exit(1);
    }
    
    printf("DNS listener stopped successfully\n");
    
    // Start listener again for message test
    status = listener->start(listener);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to restart DNS listener: %d\n", status);
        listener->destroy(listener);
        exit(1);
    }
    
    printf("DNS listener restarted successfully\n");
}

/**
 * @brief Test DNS message sending and receiving
 */
static void test_dns_message_send_receive(void) {
    printf("Testing DNS message sending and receiving...\n");
    
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
    
    printf("DNS message test completed successfully\n");
}

/**
 * @brief Client thread function
 */
static void* client_thread(void* arg) {
    // Sleep for a bit to allow server to start
    sleep(1);
    
    // Initialize ARES
    ares_channel channel;
    int status = ares_library_init(ARES_LIB_INIT_ALL);
    
    if (status != ARES_SUCCESS) {
        printf("Failed to initialize ARES: %s\n", ares_strerror(status));
        return NULL;
    }
    
    struct ares_options options;
    memset(&options, 0, sizeof(options));
    
    options.flags = ARES_FLAG_STAYOPEN;
    options.timeout = 1000;
    options.tries = 3;
    options.servers = NULL;
    options.nservers = 0;
    
    status = ares_init_options(&channel, &options, ARES_OPT_FLAGS | ARES_OPT_TIMEOUT | ARES_OPT_TRIES);
    
    if (status != ARES_SUCCESS) {
        printf("Failed to initialize ARES channel: %s\n", ares_strerror(status));
        ares_library_cleanup();
        return NULL;
    }
    
    // Set DNS server
    struct ares_addr_node server;
    server.next = NULL;
    server.family = AF_INET;
    server.addr.addr4.s_addr = inet_addr("127.0.0.1");
    
    ares_set_servers(channel, &server);
    
    // Perform DNS query
    ares_query(channel, TEST_DOMAIN, ns_c_in, ns_t_txt, dns_query_callback, NULL);
    
    // Process DNS events
    fd_set read_fds, write_fds;
    int nfds = 0;
    
    struct timeval tv, *tvp;
    time_t start_time = time(NULL);
    
    while (time(NULL) - start_time < 5 && !message_received) {
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        
        tvp = ares_timeout(channel, NULL, &tv);
        ares_fds(channel, &read_fds, &write_fds);
        nfds = ares_socket_max(channel, nfds);
        
        if (nfds > 0) {
            select(nfds + 1, &read_fds, &write_fds, NULL, tvp);
            ares_process(channel, &read_fds, &write_fds);
        } else {
            // No file descriptors, sleep a bit
            usleep(100000);
        }
    }
    
    // Clean up
    ares_destroy(channel);
    ares_library_cleanup();
    
    return NULL;
}

/**
 * @brief DNS query callback
 */
static void dns_query_callback(void* arg, int status, int timeouts, unsigned char* abuf, int alen) {
    if (status != ARES_SUCCESS) {
        printf("DNS query failed: %s\n", ares_strerror(status));
        return;
    }
    
    printf("DNS query completed successfully\n");
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
    test_dns_listener_create();
    test_dns_listener_start_stop();
    test_dns_message_send_receive();
    
    // Clean up
    cleanup();
    protocol_manager_shutdown();
    
    printf("All tests completed successfully\n");
    
    return 0;
}
