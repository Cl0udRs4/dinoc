/**
 * @file protocol_manager.c
 * @brief Protocol manager implementation
 */

#include "../include/protocol.h"
#include "../include/common.h"
#include "../include/client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Protocol manager structure
typedef struct {
    protocol_listener_t** listeners;
    size_t listener_count;
    size_t listener_capacity;
    pthread_mutex_t mutex;
} protocol_manager_t;

// Global protocol manager
static protocol_manager_t* global_manager = NULL;

/**
 * @brief Initialize protocol manager
 */
status_t protocol_manager_init(void) {
    if (global_manager != NULL) {
        return STATUS_ERROR_ALREADY_RUNNING;
    }
    
    global_manager = (protocol_manager_t*)malloc(sizeof(protocol_manager_t));
    if (global_manager == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    global_manager->listener_capacity = 16;
    global_manager->listener_count = 0;
    global_manager->listeners = (protocol_listener_t**)malloc(global_manager->listener_capacity * sizeof(protocol_listener_t*));
    
    if (global_manager->listeners == NULL) {
        free(global_manager);
        global_manager = NULL;
        return STATUS_ERROR_MEMORY;
    }
    
    pthread_mutex_init(&global_manager->mutex, NULL);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Shutdown protocol manager
 */
status_t protocol_manager_shutdown(void) {
    if (global_manager == NULL) {
        return STATUS_ERROR_NOT_FOUND;
    }
    
    pthread_mutex_lock(&global_manager->mutex);
    
    // Stop and destroy all listeners
    for (size_t i = 0; i < global_manager->listener_count; i++) {
        protocol_listener_t* listener = global_manager->listeners[i];
        
        if (listener != NULL) {
            listener->stop(listener);
            listener->destroy(listener);
        }
    }
    
    free(global_manager->listeners);
    pthread_mutex_unlock(&global_manager->mutex);
    pthread_mutex_destroy(&global_manager->mutex);
    
    free(global_manager);
    global_manager = NULL;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Create a protocol listener
 */
status_t protocol_manager_create_listener(protocol_type_t type, const protocol_listener_config_t* config, protocol_listener_t** listener) {
    if (global_manager == NULL) {
        return STATUS_ERROR_NOT_FOUND;
    }
    
    if (config == NULL || listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    status_t status;
    
    // Create listener based on protocol type
    switch (type) {
        case PROTOCOL_TYPE_TCP:
            status = tcp_listener_create(config, listener);
            break;
            
        case PROTOCOL_TYPE_UDP:
            status = udp_listener_create(config, listener);
            break;
            
        case PROTOCOL_TYPE_WS:
            status = ws_listener_create(config, listener);
            break;
            
        case PROTOCOL_TYPE_ICMP:
            status = icmp_listener_create(config, listener);
            break;
            
        case PROTOCOL_TYPE_DNS:
            status = dns_listener_create(config, listener);
            break;
            
        default:
            return STATUS_ERROR_INVALID_PARAM;
    }
    
    if (status != STATUS_SUCCESS) {
        return status;
    }
    
    // Add listener to manager
    pthread_mutex_lock(&global_manager->mutex);
    
    // Resize array if needed
    if (global_manager->listener_count >= global_manager->listener_capacity) {
        size_t new_capacity = global_manager->listener_capacity * 2;
        protocol_listener_t** new_listeners = (protocol_listener_t**)realloc(global_manager->listeners, new_capacity * sizeof(protocol_listener_t*));
        
        if (new_listeners == NULL) {
            pthread_mutex_unlock(&global_manager->mutex);
            (*listener)->destroy(*listener);
            *listener = NULL;
            return STATUS_ERROR_MEMORY;
        }
        
        global_manager->listeners = new_listeners;
        global_manager->listener_capacity = new_capacity;
    }
    
    global_manager->listeners[global_manager->listener_count++] = *listener;
    pthread_mutex_unlock(&global_manager->mutex);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Destroy a protocol listener
 */
status_t protocol_manager_destroy_listener(protocol_listener_t* listener) {
    if (global_manager == NULL) {
        return STATUS_ERROR_NOT_FOUND;
    }
    
    if (listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&global_manager->mutex);
    
    // Find listener in array
    size_t index = 0;
    bool found = false;
    
    for (size_t i = 0; i < global_manager->listener_count; i++) {
        if (global_manager->listeners[i] == listener) {
            index = i;
            found = true;
            break;
        }
    }
    
    if (!found) {
        pthread_mutex_unlock(&global_manager->mutex);
        return STATUS_ERROR_NOT_FOUND;
    }
    
    // Stop listener if running
    listener->stop(listener);
    
    // Destroy listener
    status_t status = listener->destroy(listener);
    
    // Remove listener from array
    for (size_t i = index; i < global_manager->listener_count - 1; i++) {
        global_manager->listeners[i] = global_manager->listeners[i + 1];
    }
    
    global_manager->listener_count--;
    pthread_mutex_unlock(&global_manager->mutex);
    
    return status;
}

/**
 * @brief Start a protocol listener
 */
status_t protocol_manager_start_listener(protocol_listener_t* listener) {
    if (global_manager == NULL) {
        return STATUS_ERROR_NOT_FOUND;
    }
    
    if (listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    return listener->start(listener);
}

/**
 * @brief Stop a protocol listener
 */
status_t protocol_manager_stop_listener(protocol_listener_t* listener) {
    if (global_manager == NULL) {
        return STATUS_ERROR_NOT_FOUND;
    }
    
    if (listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    return listener->stop(listener);
}

/**
 * @brief Send a message through a protocol listener
 */
status_t protocol_manager_send_message(protocol_listener_t* listener, client_t* client, protocol_message_t* message) {
    if (global_manager == NULL) {
        return STATUS_ERROR_NOT_FOUND;
    }
    
    if (listener == NULL || client == NULL || message == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    return listener->send_message(listener, client, message);
}

/**
 * @brief Register callbacks for a protocol listener
 */
status_t protocol_manager_register_callbacks(protocol_listener_t* listener,
                                           void (*on_message_received)(protocol_listener_t*, client_t*, protocol_message_t*),
                                           void (*on_client_connected)(protocol_listener_t*, client_t*),
                                           void (*on_client_disconnected)(protocol_listener_t*, client_t*)) {
    if (global_manager == NULL) {
        return STATUS_ERROR_NOT_FOUND;
    }
    
    if (listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    return listener->register_callbacks(listener, on_message_received, on_client_connected, on_client_disconnected);
}

/**
 * @brief Get callbacks from a protocol listener
 */
status_t protocol_manager_get_callbacks(protocol_listener_t* listener,
                                      void (**on_message_received)(protocol_listener_t*, client_t*, protocol_message_t*),
                                      void (**on_client_connected)(protocol_listener_t*, client_t*),
                                      void (**on_client_disconnected)(protocol_listener_t*, client_t*)) {
    if (global_manager == NULL) {
        return STATUS_ERROR_NOT_FOUND;
    }
    
    if (listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // For now, just return NULL for all callbacks
    // In a real implementation, we would need to store the callbacks
    // in the protocol manager or in the listener context
    if (on_message_received != NULL) {
        *on_message_received = NULL;
    }
    
    if (on_client_connected != NULL) {
        *on_client_connected = NULL;
    }
    
    if (on_client_disconnected != NULL) {
        *on_client_disconnected = NULL;
    }
    
    return STATUS_SUCCESS;
}
