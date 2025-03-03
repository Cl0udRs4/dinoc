/**
 * @file tcp_listener.c
 * @brief TCP protocol listener implementation
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

// TCP listener context
typedef struct {
    int server_socket;
    bool running;
    pthread_t accept_thread;
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
} tcp_listener_context_t;

// Client thread argument
typedef struct {
    protocol_listener_t* listener;
    client_t* client;
} tcp_client_thread_arg_t;

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
static void tcp_remove_client(tcp_listener_context_t* context, client_t* client);

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
    new_listener->protocol_type = PROTOCOL_TYPE_TCP;
    new_listener->protocol_context = context;
    
    // Set function pointers
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
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    tcp_listener_context_t* context = (tcp_listener_context_t*)listener->protocol_context;
    
    // Check if already running
    if (context->running) {
        return STATUS_ERROR_ALREADY_RUNNING;
    }
    
    // Create server socket
    context->server_socket = socket(AF_INET, SOCK_STREAM, 0);
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
    
    // Listen for connections
    if (listen(context->server_socket, 5) < 0) {
        close(context->server_socket);
        context->server_socket = -1;
        return STATUS_ERROR_LISTEN;
    }
    
    // Set running flag
    context->running = true;
    
    // Create accept thread
    if (pthread_create(&context->accept_thread, NULL, tcp_accept_thread, listener) != 0) {
        close(context->server_socket);
        context->server_socket = -1;
        context->running = false;
        return STATUS_ERROR_THREAD;
    }
    
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
        return STATUS_SUCCESS;
    }
    
    // Set running flag
    context->running = false;
    
    // Close server socket
    if (context->server_socket >= 0) {
        close(context->server_socket);
        context->server_socket = -1;
    }
    
    // Wait for accept thread to exit
    pthread_join(context->accept_thread, NULL);
    
    // Close client connections
    pthread_mutex_lock(&context->clients_mutex);
    
    for (size_t i = 0; i < context->client_count; i++) {
        client_t* client = context->clients[i];
        
        // Call client disconnected callback
        if (context->on_client_disconnected != NULL) {
            context->on_client_disconnected(listener, client);
        }
        
        // Close client socket
        if (client->socket >= 0) {
            close(client->socket);
            client->socket = -1;
        }
        
        // Destroy client
        client_destroy(client);
    }
    
    context->client_count = 0;
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
static status_t tcp_listener_send_message(protocol_listener_t* listener, client_t* client, protocol_message_t* message) {
    if (listener == NULL || listener->protocol_context == NULL || client == NULL || message == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    tcp_listener_context_t* context = (tcp_listener_context_t*)listener->protocol_context;
    
    // Check if running
    if (!context->running) {
        return STATUS_ERROR_NOT_RUNNING;
    }
    
    // Get client socket
    int client_socket = client->socket;
    
    if (client_socket < 0) {
        return STATUS_ERROR_SOCKET;
    }
    
    // Send message length
    uint32_t length = htonl((uint32_t)message->data_len);
    if (send(client_socket, &length, sizeof(length), 0) != sizeof(length)) {
        return STATUS_ERROR_SEND;
    }
    
    // Send message data
    if (send(client_socket, message->data, message->data_len, 0) != (ssize_t)message->data_len) {
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
            if (errno == EINTR) {
                continue;
            }
            
            break;
        }
        
        // Create client
        client_t* client = client_create();
        
        if (client == NULL) {
            close(client_socket);
            continue;
        }
        
        // Set client socket and address
        client->socket = client_socket;
        memcpy(&client->addr, &client_addr, sizeof(client_addr));
        client->addr_len = client_addr_len;
        
        // Add client to array
        pthread_mutex_lock(&context->clients_mutex);
        
        if (context->client_count >= context->client_capacity) {
            size_t new_capacity = context->client_capacity * 2;
            client_t** new_clients = (client_t**)realloc(context->clients, new_capacity * sizeof(client_t*));
            
            if (new_clients == NULL) {
                pthread_mutex_unlock(&context->clients_mutex);
                close(client_socket);
                client_destroy(client);
                continue;
            }
            
            context->clients = new_clients;
            context->client_capacity = new_capacity;
        }
        
        context->clients[context->client_count++] = client;
        pthread_mutex_unlock(&context->clients_mutex);
        
        // Call client connected callback
        if (context->on_client_connected != NULL) {
            context->on_client_connected(listener, client);
        }
        
        // Create client thread argument
        tcp_client_thread_arg_t* thread_arg = (tcp_client_thread_arg_t*)malloc(sizeof(tcp_client_thread_arg_t));
        
        if (thread_arg == NULL) {
            tcp_remove_client(context, client);
            
            // Call client disconnected callback
            if (context->on_client_disconnected != NULL) {
                context->on_client_disconnected(listener, client);
            }
            
            // Close socket and destroy client
            close(client_socket);
            client_destroy(client);
            
            continue;
        }
        
        thread_arg->listener = listener;
        thread_arg->client = client;
        
        // Create client thread
        pthread_t client_thread_id;
        pthread_attr_t attr;
        
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        
        if (pthread_create(&client_thread_id, &attr, tcp_client_thread, thread_arg) != 0) {
            pthread_attr_destroy(&attr);
            free(thread_arg);
            
            tcp_remove_client(context, client);
            
            // Call client disconnected callback
            if (context->on_client_disconnected != NULL) {
                context->on_client_disconnected(listener, client);
            }
            
            // Close socket and destroy client
            close(client_socket);
            client_destroy(client);
            
            continue;
        }
        
        pthread_attr_destroy(&attr);
    }
    
    return NULL;
}

/**
 * @brief Client thread function
 */
static void* tcp_client_thread(void* arg) {
    tcp_client_thread_arg_t* thread_arg = (tcp_client_thread_arg_t*)arg;
    protocol_listener_t* listener = thread_arg->listener;
    client_t* client = thread_arg->client;
    tcp_listener_context_t* context = (tcp_listener_context_t*)listener->protocol_context;
    
    // Free thread argument
    free(thread_arg);
    
    // Set socket timeout
    struct timeval tv;
    tv.tv_sec = context->timeout_ms / 1000;
    tv.tv_usec = (context->timeout_ms % 1000) * 1000;
    
    if (setsockopt(client->socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt");
    }
    
    // Receive messages
    while (context->running) {
        // Receive message length
        uint32_t length_network;
        ssize_t recv_result = recv(client->socket, &length_network, sizeof(length_network), 0);
        
        if (recv_result <= 0) {
            if (recv_result == 0 || errno != EAGAIN) {
                // Connection closed or error
                break;
            }
            
            // Timeout, continue
            continue;
        }
        
        // Convert length to host byte order
        uint32_t length = ntohl(length_network);
        
        // Allocate buffer for message
        uint8_t* buffer = (uint8_t*)malloc(length);
        
        if (buffer == NULL) {
            break;
        }
        
        // Receive message data
        size_t total_received = 0;
        
        while (total_received < length) {
            recv_result = recv(client->socket, buffer + total_received, length - total_received, 0);
            
            if (recv_result <= 0) {
                if (recv_result == 0 || errno != EAGAIN) {
                    // Connection closed or error
                    break;
                }
                
                // Timeout, continue
                continue;
            }
            
            total_received += recv_result;
        }
        
        if (total_received < length) {
            // Failed to receive complete message
            free(buffer);
            break;
        }
        
        // Create message
        protocol_message_t message;
        message.data = buffer;
        message.data_len = length;
        
        // Call message received callback
        if (context->on_message_received != NULL) {
            context->on_message_received(listener, client, &message);
        }
        
        // Free buffer
        free(buffer);
    }
    
    // Remove client from array
    tcp_remove_client(context, client);
    
    // Call client disconnected callback
    if (context->on_client_disconnected != NULL) {
        context->on_client_disconnected(listener, client);
    }
    
    // Close socket and destroy client
    close(client->socket);
    client_destroy(client);
    
    return NULL;
}

/**
 * @brief Remove client from array
 */
static void tcp_remove_client(tcp_listener_context_t* context, client_t* client) {
    pthread_mutex_lock(&context->clients_mutex);
    
    for (size_t i = 0; i < context->client_count; i++) {
        if (context->clients[i] == client) {
            context->clients[i] = context->clients[--context->client_count];
            break;
        }
    }
    
    pthread_mutex_unlock(&context->clients_mutex);
}
