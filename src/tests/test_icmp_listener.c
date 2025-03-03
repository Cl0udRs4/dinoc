/**
 * @file test_icmp_listener.c
 * @brief Test program for ICMP protocol listener
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
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <sys/socket.h>

// Test configuration
#define TEST_PCAP_DEVICE "any"
#define TEST_MESSAGE "Hello, ICMP!"
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
static uint16_t calculate_checksum(uint16_t* addr, int len);

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
 * @brief Test ICMP listener creation
 */
static void test_icmp_listener_create(void) {
    printf("Testing ICMP listener creation...\n");
    
    // Create listener configuration
    protocol_listener_config_t config;
    memset(&config, 0, sizeof(config));
    
    config.pcap_device = TEST_PCAP_DEVICE;
    config.timeout_ms = TEST_TIMEOUT_MS;
    
    // Create listener
    status_t status = icmp_listener_create(&config, &listener);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to create ICMP listener: %d\n", status);
        exit(1);
    }
    
    // Register callbacks
    status = listener->register_callbacks(listener, on_message_received, on_client_connected, on_client_disconnected);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to register callbacks: %d\n", status);
        listener->destroy(listener);
        exit(1);
    }
    
    printf("ICMP listener created successfully\n");
}

/**
 * @brief Test ICMP listener start and stop
 */
static void test_icmp_listener_start_stop(void) {
    printf("Testing ICMP listener start and stop...\n");
    
    // Start listener
    status_t status = listener->start(listener);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to start ICMP listener: %d\n", status);
        listener->destroy(listener);
        exit(1);
    }
    
    printf("ICMP listener started successfully\n");
    
    // Sleep for a bit
    sleep(1);
    
    // Stop listener
    status = listener->stop(listener);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to stop ICMP listener: %d\n", status);
        listener->destroy(listener);
        exit(1);
    }
    
    printf("ICMP listener stopped successfully\n");
    
    // Start listener again for message test
    status = listener->start(listener);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to restart ICMP listener: %d\n", status);
        listener->destroy(listener);
        exit(1);
    }
    
    printf("ICMP listener restarted successfully\n");
}

/**
 * @brief Test ICMP message sending and receiving
 */
static void test_icmp_message_send_receive(void) {
    printf("Testing ICMP message sending and receiving...\n");
    
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
    
    printf("ICMP message test completed successfully\n");
}

/**
 * @brief Client thread function
 */
static void* client_thread(void* arg) {
    // Sleep for a bit to allow server to start
    sleep(1);
    
    // Create raw socket
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    
    if (sock < 0) {
        perror("socket");
        return NULL;
    }
    
    // Set socket options
    int on = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
        perror("setsockopt");
        close(sock);
        return NULL;
    }
    
    // Prepare destination address
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // Prepare ICMP packet
    char packet[sizeof(struct iphdr) + sizeof(struct icmphdr) + strlen(TEST_MESSAGE)];
    memset(packet, 0, sizeof(packet));
    
    // IP header
    struct iphdr* ip = (struct iphdr*)packet;
    ip->version = 4;
    ip->ihl = 5;
    ip->tos = 0;
    ip->tot_len = htons(sizeof(packet));
    ip->id = htons(getpid());
    ip->frag_off = 0;
    ip->ttl = 64;
    ip->protocol = IPPROTO_ICMP;
    ip->check = 0;
    ip->saddr = inet_addr("127.0.0.1");
    ip->daddr = dest_addr.sin_addr.s_addr;
    
    // ICMP header
    struct icmphdr* icmp = (struct icmphdr*)(packet + sizeof(struct iphdr));
    icmp->type = ICMP_ECHO;
    icmp->code = 0;
    icmp->un.echo.id = htons(getpid());
    icmp->un.echo.sequence = htons(1);
    icmp->checksum = 0;
    
    // ICMP data
    char* data = packet + sizeof(struct iphdr) + sizeof(struct icmphdr);
    strcpy(data, TEST_MESSAGE);
    
    // Calculate ICMP checksum
    icmp->checksum = calculate_checksum((uint16_t*)icmp, sizeof(struct icmphdr) + strlen(TEST_MESSAGE));
    
    // Send packet
    if (sendto(sock, packet, sizeof(packet), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr)) < 0) {
        perror("sendto");
        close(sock);
        return NULL;
    }
    
    printf("ICMP packet sent\n");
    
    // Close socket
    close(sock);
    
    return NULL;
}

/**
 * @brief Calculate IP checksum
 */
static uint16_t calculate_checksum(uint16_t* addr, int len) {
    int nleft = len;
    int sum = 0;
    uint16_t* w = addr;
    uint16_t answer = 0;
    
    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }
    
    if (nleft == 1) {
        *(uint8_t*)(&answer) = *(uint8_t*)w;
        sum += answer;
    }
    
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    answer = ~sum;
    
    return answer;
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
    test_icmp_listener_create();
    test_icmp_listener_start_stop();
    test_icmp_message_send_receive();
    
    // Clean up
    cleanup();
    protocol_manager_shutdown();
    
    printf("All tests completed successfully\n");
    
    return 0;
}
