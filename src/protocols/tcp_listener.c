/**
 * @file tcp_listener.c
 * @brief TCP protocol listener implementation
 */

#define _GNU_SOURCE /* For strdup */

#include "../include/protocol.h"
#include "../include/client.h"
#include "../common/logger.h"
#include "../common/uuid.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

/**
 * @brief TCP listener context
 */
typedef struct {
    char* bind_address;
    uint16_t port;
    int server_socket;
    bool running;
    pthread_t accept_thread;
    pthread_mutex_t clients_mutex;
    client_t** clients;
    size_t clients_count;
    size_t clients_capacity;
    void (*on_message_received)(protocol_listener_t*, client_t*, protocol_message_t*);
    void (*on_client_connected)(protocol_listener_t*, client_t*);
    void (*on_client_disconnected)(protocol_listener_t*, client_t*);
} tcp_listener_context_t;

/**
 * @brief TCP client context
 */
typedef struct {
    int socket;
    pthread_t thread;
    bool running;
} tcp_client_context_t;

// Forward declarations
static status_t tcp_listener_start(protocol_listener_t* listener);
static status_t tcp_listener_stop(protocol_listener_t* listener);
static status_t tcp_listener_destroy(protocol_listener_t* listener);
static status_t tcp_listener_send_message(protocol_listener_t* listener, client_t* client, protocol_message_t* message);
static status_t tcp_listener_register_callbacks(protocol_listener_t* listener,
                                             void (*on_message_received)(protocol_listener_t*, client_t*, protocol_message_t*),
                                             void (*on_client_connected)(protocol_listener_t*, client_t*),
                                             void (*on_client_disconnected)(protocol_listener_t*, client_t*));
static void* tcp_accept_thread(void* arg);
static void* tcp_client_thread(void* arg);
static void tcp_remove_client(tcp_listener_context_t* context, client_t* client, protocol_listener_t* listener);

/**
 * @brief Create a TCP listener
 */
status_t tcp_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener) {
    if (config == NULL || listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Create listener
    protocol_listener_t* new_listener = (protocol_listener_t*)malloc(sizeof(protocol_listener_t));
    if (new_listener == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Create context
    tcp_listener_context_t* context = (tcp_listener_context_t*)malloc(sizeof(tcp_listener_context_t));
    if (context == NULL) {
        free(new_listener);
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize context
    memset(context, 0, sizeof(tcp_listener_context_t));
    context->bind_address = strdup(config->bind_address);
    context->port = config->port;
    context->server_socket = -1;
    context->running = false;
    
    // Initialize mutex
    if (pthread_mutex_init(&context->clients_mutex, NULL) != 0) {
        free(context->bind_address);
        free(context);
        free(new_listener);
        return STATUS_ERROR_THREAD;
    }
    
    // Initialize clients array
    context->clients_capacity = 10;
    context->clients = (client_t**)malloc(context->clients_capacity * sizeof(client_t*));
    if (context->clients == NULL) {
        pthread_mutex_destroy(&context->clients_mutex);
        free(context->bind_address);
        free(context);
        free(new_listener);
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize listener
    memset(new_listener, 0, sizeof(protocol_listener_t));
    uuid_generate_wrapper(new_listener->id);
    new_listener->protocol_type = PROTOCOL_TYPE_TCP;
    new_listener->protocol_context = context;
    new_listener->start = tcp_listener_start;
    new_listener->stop = tcp_listener_stop;
    new_listener->destroy = tcp_listener_destroy;
    new_listener->send_message = tcp_listener_send_message;
    new_listener->register_callbacks = tcp_listener_register_callbacks;
    
    *listener = new_listener;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Start TCP listener
 */
static status_t tcp_listener_start(protocol_listener_t* listener) {
    if (listener == NULL || listener->protocol_context == NULL) {
        LOG_ERROR("TCP listener start failed: invalid parameter");
        fprintf(stderr, "TCP listener start failed: invalid parameter\n");
        fflush(stderr);
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    tcp_listener_context_t* context = (tcp_listener_context_t*)listener->protocol_context;
    
    // Check if already running
    if (context->running) {
        LOG_ERROR("TCP listener start failed: already running");
        fprintf(stderr, "TCP listener start failed: already running\n");
        fflush(stderr);
        return STATUS_ERROR_ALREADY_RUNNING;
    }
    
    // Create server socket
    context->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    LOG_INFO("TCP listener: Created server socket: %d", context->server_socket);
    fprintf(stderr, "TCP listener: Created server socket: %d\n", context->server_socket);
    fflush(stderr);
    if (context->server_socket < 0) {
        LOG_ERROR("Failed to create server socket: %s", strerror(errno));
        return STATUS_ERROR_SOCKET;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(context->server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        fprintf(stderr, "TCP listener start failed: setsockopt error: %s\n", strerror(errno));
        fflush(stderr);
        LOG_ERROR("Failed to set socket options: %s", strerror(errno));
        close(context->server_socket);
        context->server_socket = -1;
        return STATUS_ERROR_SOCKET;
    }
    
    // Bind socket
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(context->bind_address);
    if (server_addr.sin_addr.s_addr == INADDR_NONE) {
        server_addr.sin_addr.s_addr = INADDR_ANY;
    }
    
    server_addr.sin_port = htons(context->port);
    
    LOG_INFO("TCP listener: Binding to %s:%d", context->bind_address, context->port);
    fprintf(stderr, "TCP listener: Binding to %s:%d\n", context->bind_address, context->port);
    fflush(stderr);
    
    if (bind(context->server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        LOG_ERROR("Failed to bind socket: %s", strerror(errno));
        fprintf(stderr, "TCP listener start failed: bind error for %s:%d: %s\n", 
                context->bind_address, context->port, strerror(errno));
        fflush(stderr);
        close(context->server_socket);
        context->server_socket = -1;
        return STATUS_ERROR_BIND;
    }
    
    LOG_INFO("TCP listener: Bound socket to %s:%d", context->bind_address, context->port);
    fprintf(stderr, "TCP listener: Bound socket to %s:%d\n", context->bind_address, context->port);
    fflush(stderr);
    
    // Listen for connections
    if (listen(context->server_socket, 5) < 0) {
        fprintf(stderr, "TCP listener start failed: listen error: %s\n", strerror(errno));
        fflush(stderr);
        LOG_ERROR("Failed to listen on socket: %s", strerror(errno));
        close(context->server_socket);
        context->server_socket = -1;
        return STATUS_ERROR_LISTEN;
    }
    
    LOG_INFO("TCP listener started on %s:%d", context->bind_address, context->port);
    fprintf(stderr, "TCP listener: Listening on socket\n");
    fflush(stderr);
    
    // Set running flag
    context->running = true;
    
    // Create accept thread
    if (pthread_create(&context->accept_thread, NULL, tcp_accept_thread, listener) != 0) {
        LOG_ERROR("Failed to create accept thread: %s", strerror(errno));
        fprintf(stderr, "TCP listener start failed: thread creation error: %s\n", strerror(errno));
        fflush(stderr);
        close(context->server_socket);
        context->server_socket = -1;
        context->running = false;
        return STATUS_ERROR_THREAD;
    }
    
    fprintf(stderr, "TCP listener: Created accept thread\n");
    fflush(stderr);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Stop TCP listener
 */
static status_t tcp_listener_stop(protocol_listener_t* listener) {
    if (listener == NULL || listener->protocol_context == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    tcp_listener_context_t* context = (tcp_listener_context_t*)listener->protocol_context;
    
    // Check if running
    if (!context->running) {
        return STATUS_ERROR_NOT_RUNNING;
    }
    
    // Set running flag
    context->running = false;
    
    // Close server socket
    if (context->server_socket >= 0) {
        close(context->server_socket);
        context->server_socket = -1;
    }
    
    // Wait for accept thread to finish
    pthread_join(context->accept_thread, NULL);
    
    // Close client connections
    pthread_mutex_lock(&context->clients_mutex);
    
    for (size_t i = 0; i < context->clients_count; i++) {
        client_t* client = context->clients[i];
        tcp_client_context_t* client_context = (tcp_client_context_t*)client->protocol_context;
        
        // Set running flag
        client_context->running = false;
        
        // Close socket
        if (client_context->socket >= 0) {
            close(client_context->socket);
            client_context->socket = -1;
        }
        
        // Wait for thread to finish
        pthread_join(client_context->thread, NULL);
        
        // Free client context
        free(client_context);
        client->protocol_context = NULL;
        
        // Notify client disconnected
        if (context->on_client_disconnected != NULL) {
            context->on_client_disconnected(listener, client);
        }
    }
    
    // Clear clients array
    context->clients_count = 0;
    
    pthread_mutex_unlock(&context->clients_mutex);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Destroy TCP listener
 */
static status_t tcp_listener_destroy(protocol_listener_t* listener) {
    if (listener == NULL || listener->protocol_context == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    tcp_listener_context_t* context = (tcp_listener_context_t*)listener->protocol_context;
    
    // Stop listener if running
    if (context->running) {
        tcp_listener_stop(listener);
    }
    
    // Free clients array
    free(context->clients);
    
    // Destroy mutex
    pthread_mutex_destroy(&context->clients_mutex);
    
    // Free bind address
    free(context->bind_address);
    
    // Free context
    free(context);
    
    // Free listener
    free(listener);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Send message to client
 */
static status_t tcp_listener_send_message(protocol_listener_t* listener, client_t* client, protocol_message_t* message) {
    if (listener == NULL || listener->protocol_context == NULL || client == NULL || client->protocol_context == NULL || message == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    tcp_client_context_t* client_context = (tcp_client_context_t*)client->protocol_context;
    
    // Check if client is running
    if (!client_context->running) {
        return STATUS_ERROR_NOT_RUNNING;
    }
    
    // Send message size
    uint32_t size = message->data_len;
    if (send(client_context->socket, &size, sizeof(size), 0) != sizeof(size)) {
        return STATUS_ERROR_SEND;
    }
    
    // Send message data
    if (send(client_context->socket, message->data, size, 0) != size) {
        return STATUS_ERROR_SEND;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Register callbacks
 */
static status_t tcp_listener_register_callbacks(protocol_listener_t* listener,
                                             void (*on_message_received)(protocol_listener_t*, client_t*, protocol_message_t*),
                                             void (*on_client_connected)(protocol_listener_t*, client_t*),
                                             void (*on_client_disconnected)(protocol_listener_t*, client_t*)) {
    if (listener == NULL || listener->protocol_context == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    tcp_listener_context_t* context = (tcp_listener_context_t*)listener->protocol_context;
    
    context->on_message_received = on_message_received;
    context->on_client_connected = on_client_connected;
    context->on_client_disconnected = on_client_disconnected;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Accept thread function
 */
static void* tcp_accept_thread(void* arg) {
    protocol_listener_t* listener = (protocol_listener_t*)arg;
    tcp_listener_context_t* context = (tcp_listener_context_t*)listener->protocol_context;
    
    while (context->running) {
        // Accept connection
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        
        int client_socket = accept(context->server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            if (context->running) {
                LOG_ERROR("Failed to accept connection: %s", strerror(errno));
            }
            continue;
        }
        
        // Create client
        client_t* client = NULL;
        status_t status = client_register(listener, NULL, &client);
        if (status != STATUS_SUCCESS || client == NULL) {
            LOG_ERROR("Failed to create client");
            close(client_socket);
            continue;
            
        }
        
        // Create client context
        tcp_client_context_t* client_context = (tcp_client_context_t*)malloc(sizeof(tcp_client_context_t));
        if (client_context == NULL) {
            LOG_ERROR("Failed to create client context");
            client_destroy(client);
            close(client_socket);
            continue;
        }
        
        // Initialize client context
        memset(client_context, 0, sizeof(tcp_client_context_t));
        client_context->socket = client_socket;
        client_context->running = true;
        
        // Set client protocol context
        client->protocol_context = client_context;
        
        // Add client to array
        pthread_mutex_lock(&context->clients_mutex);
        
        if (context->clients_count >= context->clients_capacity) {
            size_t new_capacity = context->clients_capacity * 2;
            client_t** new_clients = (client_t**)realloc(context->clients, new_capacity * sizeof(client_t*));
            if (new_clients == NULL) {
                LOG_ERROR("Failed to resize clients array");
                pthread_mutex_unlock(&context->clients_mutex);
                free(client_context);
                client_destroy(client);
                close(client_socket);
                continue;
            }
            
            context->clients = new_clients;
            context->clients_capacity = new_capacity;
        }
        
        context->clients[context->clients_count++] = client;
        
        pthread_mutex_unlock(&context->clients_mutex);
        
        // Create client thread
        if (pthread_create(&client_context->thread, NULL, tcp_client_thread, arg) != 0) {
            LOG_ERROR("Failed to create client thread");
            tcp_remove_client(context, client, listener);
            continue;
        }
        
        // Notify client connected
        if (context->on_client_connected != NULL) {
            context->on_client_connected(listener, client);
        }
    }
    
    return NULL;
}

/**
 * @brief Client thread function
 */
static void* tcp_client_thread(void* arg) {
    protocol_listener_t* listener = (protocol_listener_t*)arg;
    tcp_listener_context_t* context = (tcp_listener_context_t*)listener->protocol_context;
    
    // Find client
    client_t* client = NULL;
    tcp_client_context_t* client_context = NULL;
    
    pthread_mutex_lock(&context->clients_mutex);
    
    for (size_t i = 0; i < context->clients_count; i++) {
        client_t* c = context->clients[i];
        tcp_client_context_t* cc = (tcp_client_context_t*)c->protocol_context;
        
        if (cc->thread == pthread_self()) {
            client = c;
            client_context = cc;
            break;
        }
    }
    
    pthread_mutex_unlock(&context->clients_mutex);
    
    if (client == NULL || client_context == NULL) {
        LOG_ERROR("Failed to find client");
        return NULL;
    }
    
    while (client_context->running) {
        // Receive message size
        uint32_t size = 0;
        ssize_t bytes_received = recv(client_context->socket, &size, sizeof(size), 0);
        
        if (bytes_received <= 0) {
            if (client_context->running) {
                LOG_ERROR("Failed to receive message size: %s", strerror(errno));
            }
            break;
        }
        
        if (bytes_received != sizeof(size)) {
            LOG_ERROR("Invalid message size received");
            break;
        }
        
        // Allocate message data
        uint8_t* data = (uint8_t*)malloc(size);
        if (data == NULL) {
            LOG_ERROR("Failed to allocate message data");
            break;
        }
        
        // Receive message data
        bytes_received = recv(client_context->socket, data, size, 0);
        
        if (bytes_received <= 0) {
            LOG_ERROR("Failed to receive message data: %s", strerror(errno));
            free(data);
            break;
        }
        
        if (bytes_received != size) {
            LOG_ERROR("Invalid message data received");
            free(data);
            break;
        }
        
        // Create message
        protocol_message_t message;
        message.data = data;
        message.data_len = size;
        
        // Notify message received
        if (context->on_message_received != NULL) {
            context->on_message_received(listener, client, &message);
        }
        
        // Free message data
        free(data);
    }
    
    // Remove client
    tcp_remove_client(context, client, listener);
    
    return NULL;
}

/**
 * @brief Remove client from array
 */
static void tcp_remove_client(tcp_listener_context_t* context, client_t* client, protocol_listener_t* listener) {
    pthread_mutex_lock(&context->clients_mutex);
    
    // Find client index
    size_t index = 0;
    bool found = false;
    
    for (size_t i = 0; i < context->clients_count; i++) {
        if (context->clients[i] == client) {
            index = i;
            found = true;
            break;
        }
    }
    
    if (!found) {
        pthread_mutex_unlock(&context->clients_mutex);
        return;
    }
    
    // Get client context
    tcp_client_context_t* client_context = (tcp_client_context_t*)client->protocol_context;
    
    // Set running flag
    client_context->running = false;
    
    // Close socket
    if (client_context->socket >= 0) {
        close(client_context->socket);
        client_context->socket = -1;
    }
    
    // Remove client from array
    for (size_t i = index; i < context->clients_count - 1; i++) {
        context->clients[i] = context->clients[i + 1];
    }
    
    context->clients_count--;
    
    pthread_mutex_unlock(&context->clients_mutex);
    
    // Wait for thread to finish if not current thread
    if (!pthread_equal(client_context->thread, pthread_self())) {
        pthread_join(client_context->thread, NULL);
    }
    
    // Free client context
    free(client_context);
    client->protocol_context = NULL;
    
    // Notify client disconnected
    if (context->on_client_disconnected != NULL) {
        context->on_client_disconnected(listener, client);
    }
    
    // Destroy client
    client_destroy(client);
}
