/**
 * @file udp_listener.c
 * @brief Implementation of UDP protocol listener
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

// Maximum buffer size for receiving data
#define MAX_BUFFER_SIZE 8192

// UDP listener context
typedef struct {
    int server_socket;              // Server socket
    struct sockaddr_in server_addr; // Server address
    pthread_t listener_thread;      // Listener thread
    bool running;                   // Running flag
    
    // Callbacks
    void (*on_message_received)(protocol_listener_t*, client_t*, protocol_message_t*);
    void (*on_client_connected)(protocol_listener_t*, client_t*);
    void (*on_client_disconnected)(protocol_listener_t*, client_t*);
} udp_listener_ctx_t;

// Forward declarations
static void* udp_listener_thread(void* arg);
static status_t udp_listener_start(protocol_listener_t* listener);
static status_t udp_listener_stop(protocol_listener_t* listener);
static status_t udp_listener_destroy(protocol_listener_t* listener);
static status_t udp_listener_send_message(protocol_listener_t* listener, client_t* client, protocol_message_t* message);

/**
 * @brief Create a UDP protocol listener
 * 
 * @param config Listener configuration
 * @param listener Pointer to store created listener
 * @return status_t Status code
 */
status_t udp_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener) {
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
    new_listener->type = PROTOCOL_UDP;
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
    
    // Allocate UDP context
    udp_listener_ctx_t* udp_ctx = (udp_listener_ctx_t*)malloc(sizeof(udp_listener_ctx_t));
    if (udp_ctx == NULL) {
        if (new_listener->config.bind_address != NULL) {
            free(new_listener->config.bind_address);
        }
        free(new_listener);
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize UDP context
    memset(udp_ctx, 0, sizeof(udp_listener_ctx_t));
    udp_ctx->server_socket = -1;
    udp_ctx->running = false;
    
    // Set protocol context
    new_listener->protocol_ctx = udp_ctx;
    
    // Set function pointers
    new_listener->start = udp_listener_start;
    new_listener->stop = udp_listener_stop;
    new_listener->destroy = udp_listener_destroy;
    new_listener->send_message = udp_listener_send_message;
    
    // Set listener
    *listener = new_listener;
    
    // Auto-start if configured
    if (config->auto_start) {
        status_t status = udp_listener_start(new_listener);
        if (status != STATUS_SUCCESS) {
            udp_listener_destroy(new_listener);
            return status;
        }
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Start UDP listener
 * 
 * @param listener Listener to start
 * @return status_t Status code
 */
static status_t udp_listener_start(protocol_listener_t* listener) {
    if (listener == NULL || listener->type != PROTOCOL_UDP || listener->protocol_ctx == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    udp_listener_ctx_t* udp_ctx = (udp_listener_ctx_t*)listener->protocol_ctx;
    
    // Check if already running
    if (udp_ctx->running) {
        return STATUS_SUCCESS;
    }
    
    // Update state
    listener->state = LISTENER_STATE_STARTING;
    
    // Create socket
    udp_ctx->server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_ctx->server_socket < 0) {
        LOG_ERROR("Failed to create UDP socket: %s", strerror(errno));
        listener->state = LISTENER_STATE_ERROR;
        return STATUS_ERROR_NETWORK;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(udp_ctx->server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        LOG_WARNING("Failed to set UDP socket options: %s", strerror(errno));
    }
    
    // Set non-blocking mode
    int flags = fcntl(udp_ctx->server_socket, F_GETFL, 0);
    if (flags < 0 || fcntl(udp_ctx->server_socket, F_SETFL, flags | O_NONBLOCK) < 0) {
        LOG_WARNING("Failed to set UDP socket to non-blocking mode: %s", strerror(errno));
    }
    
    // Prepare server address
    memset(&udp_ctx->server_addr, 0, sizeof(udp_ctx->server_addr));
    udp_ctx->server_addr.sin_family = AF_INET;
    udp_ctx->server_addr.sin_port = htons(listener->config.port);
    
    // Bind to specific address or all interfaces
    if (listener->config.bind_address != NULL && strcmp(listener->config.bind_address, "0.0.0.0") != 0) {
        if (inet_pton(AF_INET, listener->config.bind_address, &udp_ctx->server_addr.sin_addr) <= 0) {
            LOG_ERROR("Invalid UDP bind address: %s", listener->config.bind_address);
            close(udp_ctx->server_socket);
            udp_ctx->server_socket = -1;
            listener->state = LISTENER_STATE_ERROR;
            return STATUS_ERROR_INVALID_PARAM;
        }
    } else {
        udp_ctx->server_addr.sin_addr.s_addr = INADDR_ANY;
    }
    
    // Bind socket
    if (bind(udp_ctx->server_socket, (struct sockaddr*)&udp_ctx->server_addr, sizeof(udp_ctx->server_addr)) < 0) {
        LOG_ERROR("Failed to bind UDP socket to %s:%d: %s", 
                 listener->config.bind_address ? listener->config.bind_address : "0.0.0.0",
                 listener->config.port, strerror(errno));
        close(udp_ctx->server_socket);
        udp_ctx->server_socket = -1;
        listener->state = LISTENER_STATE_ERROR;
        return STATUS_ERROR_NETWORK;
    }
    
    // Set callbacks
    udp_ctx->on_message_received = listener->on_message_received;
    udp_ctx->on_client_connected = listener->on_client_connected;
    udp_ctx->on_client_disconnected = listener->on_client_disconnected;
    
    // Start listener thread
    udp_ctx->running = true;
    if (pthread_create(&udp_ctx->listener_thread, NULL, udp_listener_thread, listener) != 0) {
        LOG_ERROR("Failed to create UDP listener thread: %s", strerror(errno));
        close(udp_ctx->server_socket);
        udp_ctx->server_socket = -1;
        udp_ctx->running = false;
        listener->state = LISTENER_STATE_ERROR;
        return STATUS_ERROR_GENERIC;
    }
    
    // Update state
    listener->state = LISTENER_STATE_RUNNING;
    
    LOG_INFO("UDP listener started on %s:%d", 
             listener->config.bind_address ? listener->config.bind_address : "0.0.0.0",
             listener->config.port);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Stop UDP listener
 * 
 * @param listener Listener to stop
 * @return status_t Status code
 */
static status_t udp_listener_stop(protocol_listener_t* listener) {
    if (listener == NULL || listener->type != PROTOCOL_UDP || listener->protocol_ctx == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    udp_listener_ctx_t* udp_ctx = (udp_listener_ctx_t*)listener->protocol_ctx;
    
    // Check if already stopped
    if (!udp_ctx->running) {
        return STATUS_SUCCESS;
    }
    
    // Update state
    listener->state = LISTENER_STATE_STOPPING;
    
    // Stop listener thread
    udp_ctx->running = false;
    
    // Close server socket to interrupt recvfrom()
    if (udp_ctx->server_socket >= 0) {
        close(udp_ctx->server_socket);
        udp_ctx->server_socket = -1;
    }
    
    // Wait for listener thread to exit
    pthread_join(udp_ctx->listener_thread, NULL);
    
    // Update state
    listener->state = LISTENER_STATE_STOPPED;
    
    LOG_INFO("UDP listener stopped on port %d", listener->config.port);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Destroy UDP listener
 * 
 * @param listener Listener to destroy
 * @return status_t Status code
 */
static status_t udp_listener_destroy(protocol_listener_t* listener) {
    if (listener == NULL || listener->type != PROTOCOL_UDP) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Stop listener if running
    if (listener->state == LISTENER_STATE_RUNNING) {
        udp_listener_stop(listener);
    }
    
    // Free UDP context
    if (listener->protocol_ctx != NULL) {
        udp_listener_ctx_t* udp_ctx = (udp_listener_ctx_t*)listener->protocol_ctx;
        
        // Close server socket if open
        if (udp_ctx->server_socket >= 0) {
            close(udp_ctx->server_socket);
        }
        
        free(udp_ctx);
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
static status_t udp_listener_send_message(protocol_listener_t* listener, client_t* client, protocol_message_t* message) {
    if (listener == NULL || listener->type != PROTOCOL_UDP || client == NULL || message == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // TODO: Implement message sending
    
    return STATUS_ERROR_NOT_IMPLEMENTED;
}

/**
 * @brief UDP listener thread
 * 
 * @param arg Listener pointer
 * @return void* NULL
 */
static void* udp_listener_thread(void* arg) {
    protocol_listener_t* listener = (protocol_listener_t*)arg;
    udp_listener_ctx_t* udp_ctx = (udp_listener_ctx_t*)listener->protocol_ctx;
    
    LOG_DEBUG("UDP listener thread started");
    
    // Buffer for receiving data
    uint8_t buffer[MAX_BUFFER_SIZE];
    
    while (udp_ctx->running) {
        // Receive data
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        
        ssize_t bytes_received = recvfrom(udp_ctx->server_socket, buffer, sizeof(buffer), 0,
                                         (struct sockaddr*)&client_addr, &client_addr_len);
        
        if (bytes_received < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                LOG_ERROR("Failed to receive UDP data: %s", strerror(errno));
            }
            
            // Sleep to avoid busy waiting
            usleep(100000); // 100ms
            continue;
        }
        
        // TODO: Process received data
        
        // Update statistics
        listener->stats.bytes_received += bytes_received;
        listener->stats.messages_received++;
        listener->stats.last_message_time = time(NULL);
    }
    
    LOG_DEBUG("UDP listener thread exited");
    
    return NULL;
}
