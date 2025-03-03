/**
 * @file tcp_listener.c
 * @brief Implementation of TCP protocol listener
 */

#include "../include/protocol.h"
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

// Maximum number of pending connections
#define MAX_PENDING_CONNECTIONS 10

// Maximum buffer size for receiving data
#define MAX_BUFFER_SIZE 8192

// TCP listener context
typedef struct {
    int server_socket;              // Server socket
    struct sockaddr_in server_addr; // Server address
    pthread_t listener_thread;      // Listener thread
    bool running;                   // Running flag
    
    // Callbacks
    void (*on_message_received)(protocol_listener_t*, client_t*, protocol_message_t*);
    void (*on_client_connected)(protocol_listener_t*, client_t*);
    void (*on_client_disconnected)(protocol_listener_t*, client_t*);
} tcp_listener_ctx_t;

// Forward declarations
static void* tcp_listener_thread(void* arg);
static status_t tcp_listener_start(protocol_listener_t* listener);
static status_t tcp_listener_stop(protocol_listener_t* listener);
static status_t tcp_listener_destroy(protocol_listener_t* listener);
static status_t tcp_listener_send_message(protocol_listener_t* listener, client_t* client, protocol_message_t* message);

/**
 * @brief Create a TCP protocol listener
 * 
 * @param config Listener configuration
 * @param listener Pointer to store created listener
 * @return status_t Status code
 */
status_t tcp_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener) {
    if (config == NULL || listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Allocate listener
    protocol_listener_t* new_listener = (protocol_listener_t*)malloc(sizeof(protocol_listener_t));
    if (new_listener == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize listener
    memset(new_listener, 0, sizeof(protocol_listener_t));
    uuid_generate(&new_listener->id);
    new_listener->type = PROTOCOL_TCP;
    new_listener->state = LISTENER_STATE_CREATED;
    
    // Copy configuration
    new_listener->config.type = config->type;
    new_listener->config.port = config->port;
    new_listener->config.timeout_ms = config->timeout_ms;
    new_listener->config.auto_start = config->auto_start;
    
    if (config->bind_address != NULL) {
        new_listener->config.bind_address = strdup(config->bind_address);
        if (new_listener->config.bind_address == NULL) {
            free(new_listener);
            return STATUS_ERROR_MEMORY;
        }
    }
    
    // Initialize statistics
    memset(&new_listener->stats, 0, sizeof(protocol_listener_stats_t));
    new_listener->stats.start_time = time(NULL);
    
    // Allocate TCP context
    tcp_listener_ctx_t* tcp_ctx = (tcp_listener_ctx_t*)malloc(sizeof(tcp_listener_ctx_t));
    if (tcp_ctx == NULL) {
        if (new_listener->config.bind_address != NULL) {
            free(new_listener->config.bind_address);
        }
        free(new_listener);
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize TCP context
    memset(tcp_ctx, 0, sizeof(tcp_listener_ctx_t));
    tcp_ctx->server_socket = -1;
    tcp_ctx->running = false;
    
    // Set protocol context
    new_listener->protocol_ctx = tcp_ctx;
    
    // Set function pointers
    new_listener->start = tcp_listener_start;
    new_listener->stop = tcp_listener_stop;
    new_listener->destroy = tcp_listener_destroy;
    new_listener->send_message = tcp_listener_send_message;
    
    // Set listener
    *listener = new_listener;
    
    // Auto-start if configured
    if (config->auto_start) {
        status_t status = tcp_listener_start(new_listener);
        if (status != STATUS_SUCCESS) {
            tcp_listener_destroy(new_listener);
            return status;
        }
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Start TCP listener
 * 
 * @param listener Listener to start
 * @return status_t Status code
 */
static status_t tcp_listener_start(protocol_listener_t* listener) {
    if (listener == NULL || listener->type != PROTOCOL_TCP || listener->protocol_ctx == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    tcp_listener_ctx_t* tcp_ctx = (tcp_listener_ctx_t*)listener->protocol_ctx;
    
    // Check if already running
    if (tcp_ctx->running) {
        return STATUS_SUCCESS;
    }
    
    // Update state
    listener->state = LISTENER_STATE_STARTING;
    
    // Create socket
    tcp_ctx->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_ctx->server_socket < 0) {
        LOG_ERROR("Failed to create TCP socket: %s", strerror(errno));
        listener->state = LISTENER_STATE_ERROR;
        return STATUS_ERROR_NETWORK;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(tcp_ctx->server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        LOG_WARNING("Failed to set TCP socket options: %s", strerror(errno));
    }
    
    // Set non-blocking mode
    int flags = fcntl(tcp_ctx->server_socket, F_GETFL, 0);
    if (flags < 0 || fcntl(tcp_ctx->server_socket, F_SETFL, flags | O_NONBLOCK) < 0) {
        LOG_WARNING("Failed to set TCP socket to non-blocking mode: %s", strerror(errno));
    }
    
    // Prepare server address
    memset(&tcp_ctx->server_addr, 0, sizeof(tcp_ctx->server_addr));
    tcp_ctx->server_addr.sin_family = AF_INET;
    tcp_ctx->server_addr.sin_port = htons(listener->config.port);
    
    // Bind to specific address or all interfaces
    if (listener->config.bind_address != NULL && strcmp(listener->config.bind_address, "0.0.0.0") != 0) {
        if (inet_pton(AF_INET, listener->config.bind_address, &tcp_ctx->server_addr.sin_addr) <= 0) {
            LOG_ERROR("Invalid TCP bind address: %s", listener->config.bind_address);
            close(tcp_ctx->server_socket);
            tcp_ctx->server_socket = -1;
            listener->state = LISTENER_STATE_ERROR;
            return STATUS_ERROR_INVALID_PARAM;
        }
    } else {
        tcp_ctx->server_addr.sin_addr.s_addr = INADDR_ANY;
    }
    
    // Bind socket
    if (bind(tcp_ctx->server_socket, (struct sockaddr*)&tcp_ctx->server_addr, sizeof(tcp_ctx->server_addr)) < 0) {
        LOG_ERROR("Failed to bind TCP socket to %s:%d: %s", 
                 listener->config.bind_address ? listener->config.bind_address : "0.0.0.0",
                 listener->config.port, strerror(errno));
        close(tcp_ctx->server_socket);
        tcp_ctx->server_socket = -1;
        listener->state = LISTENER_STATE_ERROR;
        return STATUS_ERROR_NETWORK;
    }
    
    // Listen for connections
    if (listen(tcp_ctx->server_socket, MAX_PENDING_CONNECTIONS) < 0) {
        LOG_ERROR("Failed to listen on TCP socket: %s", strerror(errno));
        close(tcp_ctx->server_socket);
        tcp_ctx->server_socket = -1;
        listener->state = LISTENER_STATE_ERROR;
        return STATUS_ERROR_NETWORK;
    }
    
    // Set callbacks
    tcp_ctx->on_message_received = listener->on_message_received;
    tcp_ctx->on_client_connected = listener->on_client_connected;
    tcp_ctx->on_client_disconnected = listener->on_client_disconnected;
    
    // Start listener thread
    tcp_ctx->running = true;
    if (pthread_create(&tcp_ctx->listener_thread, NULL, tcp_listener_thread, listener) != 0) {
        LOG_ERROR("Failed to create TCP listener thread: %s", strerror(errno));
        close(tcp_ctx->server_socket);
        tcp_ctx->server_socket = -1;
        tcp_ctx->running = false;
        listener->state = LISTENER_STATE_ERROR;
        return STATUS_ERROR_GENERIC;
    }
    
    // Update state
    listener->state = LISTENER_STATE_RUNNING;
    
    LOG_INFO("TCP listener started on %s:%d", 
             listener->config.bind_address ? listener->config.bind_address : "0.0.0.0",
             listener->config.port);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Stop TCP listener
 * 
 * @param listener Listener to stop
 * @return status_t Status code
 */
static status_t tcp_listener_stop(protocol_listener_t* listener) {
    if (listener == NULL || listener->type != PROTOCOL_TCP || listener->protocol_ctx == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    tcp_listener_ctx_t* tcp_ctx = (tcp_listener_ctx_t*)listener->protocol_ctx;
    
    // Check if already stopped
    if (!tcp_ctx->running) {
        return STATUS_SUCCESS;
    }
    
    // Update state
    listener->state = LISTENER_STATE_STOPPING;
    
    // Stop listener thread
    tcp_ctx->running = false;
    
    // Close server socket to interrupt accept()
    if (tcp_ctx->server_socket >= 0) {
        close(tcp_ctx->server_socket);
        tcp_ctx->server_socket = -1;
    }
    
    // Wait for listener thread to exit
    pthread_join(tcp_ctx->listener_thread, NULL);
    
    // Update state
    listener->state = LISTENER_STATE_STOPPED;
    
    LOG_INFO("TCP listener stopped on port %d", listener->config.port);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Destroy TCP listener
 * 
 * @param listener Listener to destroy
 * @return status_t Status code
 */
static status_t tcp_listener_destroy(protocol_listener_t* listener) {
    if (listener == NULL || listener->type != PROTOCOL_TCP) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Stop listener if running
    if (listener->state == LISTENER_STATE_RUNNING) {
        tcp_listener_stop(listener);
    }
    
    // Free TCP context
    if (listener->protocol_ctx != NULL) {
        tcp_listener_ctx_t* tcp_ctx = (tcp_listener_ctx_t*)listener->protocol_ctx;
        
        // Close server socket if open
        if (tcp_ctx->server_socket >= 0) {
            close(tcp_ctx->server_socket);
        }
        
        free(tcp_ctx);
        listener->protocol_ctx = NULL;
    }
    
    // Free configuration
    if (listener->config.bind_address != NULL) {
        free(listener->config.bind_address);
        listener->config.bind_address = NULL;
    }
    
    // Free listener
    free(listener);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Send message to client
 * 
 * @param listener Listener to use
 * @param client Client to send to
 * @param message Message to send
 * @return status_t Status code
 */
static status_t tcp_listener_send_message(protocol_listener_t* listener, client_t* client, protocol_message_t* message) {
    if (listener == NULL || listener->type != PROTOCOL_TCP || client == NULL || message == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // TODO: Implement message sending
    
    return STATUS_ERROR_NOT_IMPLEMENTED;
}

/**
 * @brief TCP listener thread
 * 
 * @param arg Listener pointer
 * @return void* NULL
 */
static void* tcp_listener_thread(void* arg) {
    protocol_listener_t* listener = (protocol_listener_t*)arg;
    tcp_listener_ctx_t* tcp_ctx = (tcp_listener_ctx_t*)listener->protocol_ctx;
    
    LOG_DEBUG("TCP listener thread started");
    
    while (tcp_ctx->running) {
        // Accept new connections
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        
        int client_socket = accept(tcp_ctx->server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                LOG_ERROR("Failed to accept TCP connection: %s", strerror(errno));
            }
            
            // Sleep to avoid busy waiting
            usleep(100000); // 100ms
            continue;
        }
        
        // TODO: Handle new client connection
        
        // For now, just close the socket
        close(client_socket);
    }
    
    LOG_DEBUG("TCP listener thread exited");
    
    return NULL;
}
