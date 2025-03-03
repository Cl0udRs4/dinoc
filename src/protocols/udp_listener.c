/**
 * @file udp_listener.c
 * @brief UDP protocol listener implementation
 */

#include "../include/protocol.h"
#include "../include/common.h"
#include "../include/client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

// UDP listener context
typedef struct {
    int server_socket;
    bool running;
    pthread_t receive_thread;
    pthread_mutex_t clients_mutex;
    client_t** clients;
    size_t client_count;
    size_t client_capacity;
    
    // Configuration
    char* bind_address;
    uint16_t port;
    uint32_t timeout_ms;
    
    // Callbacks
    void (*on_message_received)(protocol_listener_t*, client_t*, protocol_message_t*);
    void (*on_client_connected)(protocol_listener_t*, client_t*);
    void (*on_client_disconnected)(protocol_listener_t*, client_t*);
} udp_listener_context_t;

// Forward declarations
static status_t udp_listener_start(protocol_listener_t* listener);
static status_t udp_listener_stop(protocol_listener_t* listener);
static status_t udp_listener_destroy(protocol_listener_t* listener);
static status_t udp_listener_send_message(protocol_listener_t* listener, client_t* client, protocol_message_t* message);
static status_t udp_listener_register_callbacks(protocol_listener_t* listener,
                                             void (*on_message_received)(protocol_listener_t*, client_t*, protocol_message_t*),
                                             void (*on_client_connected)(protocol_listener_t*, client_t*),
                                             void (*on_client_disconnected)(protocol_listener_t*, client_t*));
static void* udp_receive_thread(void* arg);
static client_t* udp_find_or_create_client(udp_listener_context_t* context, struct sockaddr_in* addr, socklen_t addr_len);
static void udp_remove_client(udp_listener_context_t* context, client_t* client);

/**
 * @brief Create a UDP listener
 */
status_t udp_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener) {
    if (config == NULL || listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Create listener
    protocol_listener_t* new_listener = (protocol_listener_t*)malloc(sizeof(protocol_listener_t));
    if (new_listener == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Create context
    udp_listener_context_t* context = (udp_listener_context_t*)malloc(sizeof(udp_listener_context_t));
    if (context == NULL) {
        free(new_listener);
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize context
    memset(context, 0, sizeof(udp_listener_context_t));
    context->server_socket = -1;
    context->running = false;
    pthread_mutex_init(&context->clients_mutex, NULL);
    
    context->client_capacity = 16;
    context->client_count = 0;
    context->clients = (client_t**)malloc(context->client_capacity * sizeof(client_t*));
    
    if (context->clients == NULL) {
        pthread_mutex_destroy(&context->clients_mutex);
        free(context);
        free(new_listener);
        return STATUS_ERROR_MEMORY;
    }
    
    // Copy configuration
    if (config->bind_address != NULL) {
        context->bind_address = strdup(config->bind_address);
        if (context->bind_address == NULL) {
            free(context->clients);
            pthread_mutex_destroy(&context->clients_mutex);
            free(context);
            free(new_listener);
            return STATUS_ERROR_MEMORY;
        }
    } else {
        context->bind_address = strdup("0.0.0.0");
        if (context->bind_address == NULL) {
            free(context->clients);
            pthread_mutex_destroy(&context->clients_mutex);
            free(context);
            free(new_listener);
            return STATUS_ERROR_MEMORY;
        }
    }
    
    context->port = config->port > 0 ? config->port : 8080;
    context->timeout_ms = config->timeout_ms > 0 ? config->timeout_ms : 30000;
    
    // Initialize listener
    uuid_generate(new_listener->id);
    new_listener->protocol_type = PROTOCOL_TYPE_UDP;
    new_listener->protocol_context = context;
    
    // Set function pointers
    new_listener->start = udp_listener_start;
    new_listener->stop = udp_listener_stop;
    new_listener->destroy = udp_listener_destroy;
    new_listener->send_message = udp_listener_send_message;
    new_listener->register_callbacks = udp_listener_register_callbacks;
    
    *listener = new_listener;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Start UDP listener
 */
static status_t udp_listener_start(protocol_listener_t* listener) {
    if (listener == NULL || listener->protocol_context == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    udp_listener_context_t* context = (udp_listener_context_t*)listener->protocol_context;
    
    // Check if already running
    if (context->running) {
        return STATUS_ERROR_ALREADY_RUNNING;
    }
    
    // Create server socket
    context->server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (context->server_socket < 0) {
        return STATUS_ERROR_SOCKET;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(context->server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(context->server_socket);
        context->server_socket = -1;
        return STATUS_ERROR_SOCKET;
    }
    
    // Bind socket
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(context->bind_address);
    server_addr.sin_port = htons(context->port);
    
    if (bind(context->server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(context->server_socket);
        context->server_socket = -1;
        return STATUS_ERROR_BIND;
    }
    
    // Set running flag
    context->running = true;
    
    // Create receive thread
    if (pthread_create(&context->receive_thread, NULL, udp_receive_thread, listener) != 0) {
        close(context->server_socket);
        context->server_socket = -1;
        context->running = false;
        return STATUS_ERROR_THREAD;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Stop UDP listener
 */
static status_t udp_listener_stop(protocol_listener_t* listener) {
    if (listener == NULL || listener->protocol_context == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    udp_listener_context_t* context = (udp_listener_context_t*)listener->protocol_context;
    
    // Check if running
    if (!context->running) {
        return STATUS_SUCCESS;
    }
    
    // Set running flag
    context->running = false;
    
    // Close server socket
    if (context->server_socket >= 0) {
        close(context->server_socket);
        context->server_socket = -1;
    }
    
    // Wait for receive thread to exit
    pthread_join(context->receive_thread, NULL);
    
    // Clean up clients
    pthread_mutex_lock(&context->clients_mutex);
    
    for (size_t i = 0; i < context->client_count; i++) {
        client_t* client = context->clients[i];
        
        // Call client disconnected callback
        if (context->on_client_disconnected != NULL) {
            context->on_client_disconnected(listener, client);
        }
        
        // Destroy client
        client_destroy(client);
    }
    
    context->client_count = 0;
    pthread_mutex_unlock(&context->clients_mutex);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Destroy UDP listener
 */
static status_t udp_listener_destroy(protocol_listener_t* listener) {
    if (listener == NULL || listener->protocol_context == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    udp_listener_context_t* context = (udp_listener_context_t*)listener->protocol_context;
    
    // Stop listener if running
    if (context->running) {
        udp_listener_stop(listener);
    }
    
    // Free bind address
    if (context->bind_address != NULL) {
        free(context->bind_address);
    }
    
    // Free clients array
    free(context->clients);
    
    // Destroy mutex
    pthread_mutex_destroy(&context->clients_mutex);
    
    // Free context
    free(context);
    
    // Free listener
    free(listener);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Send message to client
 */
static status_t udp_listener_send_message(protocol_listener_t* listener, client_t* client, protocol_message_t* message) {
    if (listener == NULL || listener->protocol_context == NULL || client == NULL || message == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    udp_listener_context_t* context = (udp_listener_context_t*)listener->protocol_context;
    
    // Check if running
    if (!context->running) {
        return STATUS_ERROR_NOT_RUNNING;
    }
    
    // Send message
    ssize_t sent = sendto(context->server_socket, message->data, message->data_len, 0,
                         (struct sockaddr*)&client->addr, client->addr_len);
    
    if (sent != (ssize_t)message->data_len) {
        return STATUS_ERROR_SEND;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Register callbacks
 */
static status_t udp_listener_register_callbacks(protocol_listener_t* listener,
                                             void (*on_message_received)(protocol_listener_t*, client_t*, protocol_message_t*),
                                             void (*on_client_connected)(protocol_listener_t*, client_t*),
                                             void (*on_client_disconnected)(protocol_listener_t*, client_t*)) {
    if (listener == NULL || listener->protocol_context == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    udp_listener_context_t* context = (udp_listener_context_t*)listener->protocol_context;
    
    context->on_message_received = on_message_received;
    context->on_client_connected = on_client_connected;
    context->on_client_disconnected = on_client_disconnected;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Receive thread function
 */
static void* udp_receive_thread(void* arg) {
    protocol_listener_t* listener = (protocol_listener_t*)arg;
    udp_listener_context_t* context = (udp_listener_context_t*)listener->protocol_context;
    
    // Set socket timeout
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    
    if (setsockopt(context->server_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt");
    }
    
    // Buffer for receiving data
    uint8_t buffer[65536];
    
    while (context->running) {
        // Receive data
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        
        ssize_t recv_len = recvfrom(context->server_socket, buffer, sizeof(buffer), 0,
                                   (struct sockaddr*)&client_addr, &client_addr_len);
        
        if (recv_len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Timeout, continue
                continue;
            }
            
            perror("recvfrom");
            break;
        }
        
        // Find or create client
        client_t* client = udp_find_or_create_client(context, &client_addr, client_addr_len);
        
        if (client == NULL) {
            continue;
        }
        
        // Create message
        protocol_message_t message;
        message.data = (uint8_t*)malloc(recv_len);
        
        if (message.data == NULL) {
            continue;
        }
        
        memcpy(message.data, buffer, recv_len);
        message.data_len = recv_len;
        
        // Call message received callback
        if (context->on_message_received != NULL) {
            context->on_message_received(listener, client, &message);
        }
        
        // Free message data
        free(message.data);
    }
    
    return NULL;
}

/**
 * @brief Find or create client
 */
static client_t* udp_find_or_create_client(udp_listener_context_t* context, struct sockaddr_in* addr, socklen_t addr_len) {
    pthread_mutex_lock(&context->clients_mutex);
    
    // Find existing client
    for (size_t i = 0; i < context->client_count; i++) {
        client_t* client = context->clients[i];
        
        if (client->addr_len == addr_len &&
            memcmp(&client->addr, addr, addr_len) == 0) {
            pthread_mutex_unlock(&context->clients_mutex);
            return client;
        }
    }
    
    // Create new client
    client_t* client = client_create();
    
    if (client == NULL) {
        pthread_mutex_unlock(&context->clients_mutex);
        return NULL;
    }
    
    // Set client address
    memcpy(&client->addr, addr, addr_len);
    client->addr_len = addr_len;
    
    // Add client to array
    if (context->client_count >= context->client_capacity) {
        size_t new_capacity = context->client_capacity * 2;
        client_t** new_clients = (client_t**)realloc(context->clients, new_capacity * sizeof(client_t*));
        
        if (new_clients == NULL) {
            pthread_mutex_unlock(&context->clients_mutex);
            client_destroy(client);
            return NULL;
        }
        
        context->clients = new_clients;
        context->client_capacity = new_capacity;
    }
    
    context->clients[context->client_count++] = client;
    pthread_mutex_unlock(&context->clients_mutex);
    
    // Call client connected callback
    if (context->on_client_connected != NULL) {
        context->on_client_connected((protocol_listener_t*)context, client);
    }
    
    return client;
}

/**
 * @brief Remove client from array
 */
static void udp_remove_client(udp_listener_context_t* context, client_t* client) {
    pthread_mutex_lock(&context->clients_mutex);
    
    for (size_t i = 0; i < context->client_count; i++) {
        if (context->clients[i] == client) {
            context->clients[i] = context->clients[--context->client_count];
            break;
        }
    }
    
    pthread_mutex_unlock(&context->clients_mutex);
}
