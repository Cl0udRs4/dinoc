/**
 * @file protocol_manager.c
 * @brief Implementation of protocol listener management
 */

#include "../include/protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Forward declarations for protocol-specific listener creation
status_t tcp_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener);
status_t udp_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener);
status_t ws_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener);
status_t icmp_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener);
status_t dns_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener);

// Protocol listener manager
typedef struct {
    protocol_listener_t** listeners;    // Array of listeners
    size_t listener_count;              // Number of listeners
    size_t listener_capacity;           // Capacity of listeners array
    pthread_mutex_t mutex;              // Mutex for thread safety
} protocol_manager_t;

// Global protocol manager
static protocol_manager_t* global_manager = NULL;

/**
 * @brief Initialize protocol manager
 * 
 * @return status_t Status code
 */
status_t protocol_manager_init(void) {
    // Check if already initialized
    if (global_manager != NULL) {
        return STATUS_SUCCESS;
    }
    
    // Allocate manager
    global_manager = (protocol_manager_t*)malloc(sizeof(protocol_manager_t));
    if (global_manager == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize manager
    memset(global_manager, 0, sizeof(protocol_manager_t));
    pthread_mutex_init(&global_manager->mutex, NULL);
    
    // Allocate initial listeners array
    global_manager->listener_capacity = 10;
    global_manager->listeners = (protocol_listener_t**)malloc(
        sizeof(protocol_listener_t*) * global_manager->listener_capacity);
    
    if (global_manager->listeners == NULL) {
        pthread_mutex_destroy(&global_manager->mutex);
        free(global_manager);
        global_manager = NULL;
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize listeners array
    memset(global_manager->listeners, 0, sizeof(protocol_listener_t*) * global_manager->listener_capacity);
    global_manager->listener_count = 0;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Clean up protocol manager
 * 
 * @return status_t Status code
 */
status_t protocol_manager_cleanup(void) {
    if (global_manager == NULL) {
        return STATUS_SUCCESS;
    }
    
    // Lock mutex
    pthread_mutex_lock(&global_manager->mutex);
    
    // Destroy all listeners
    for (size_t i = 0; i < global_manager->listener_count; i++) {
        if (global_manager->listeners[i] != NULL) {
            global_manager->listeners[i]->destroy(global_manager->listeners[i]);
            global_manager->listeners[i] = NULL;
        }
    }
    
    // Free listeners array
    free(global_manager->listeners);
    
    // Unlock and destroy mutex
    pthread_mutex_unlock(&global_manager->mutex);
    pthread_mutex_destroy(&global_manager->mutex);
    
    // Free manager
    free(global_manager);
    global_manager = NULL;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Create a protocol listener
 * 
 * @param type Protocol type
 * @param config Listener configuration
 * @param listener Pointer to store created listener
 * @return status_t Status code
 */
status_t protocol_listener_create(protocol_type_t type, 
                                 const protocol_listener_config_t* config,
                                 protocol_listener_t** listener) {
    if (config == NULL || listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Initialize manager if needed
    if (global_manager == NULL) {
        status_t status = protocol_manager_init();
        if (status != STATUS_SUCCESS) {
            return status;
        }
    }
    
    // Create listener based on protocol type
    status_t status;
    
    switch (type) {
        case PROTOCOL_TCP:
            status = tcp_listener_create(config, listener);
            break;
            
        case PROTOCOL_UDP:
            status = udp_listener_create(config, listener);
            break;
            
        case PROTOCOL_WS:
            status = ws_listener_create(config, listener);
            break;
            
        case PROTOCOL_ICMP:
            status = icmp_listener_create(config, listener);
            break;
            
        case PROTOCOL_DNS:
            status = dns_listener_create(config, listener);
            break;
            
        default:
            LOG_ERROR("Unsupported protocol type: %d", type);
            return STATUS_ERROR_NOT_IMPLEMENTED;
    }
    
    if (status != STATUS_SUCCESS) {
        return status;
    }
    
    // Add listener to manager
    pthread_mutex_lock(&global_manager->mutex);
    
    // Check if we need to resize listeners array
    if (global_manager->listener_count >= global_manager->listener_capacity) {
        // Double capacity
        size_t new_capacity = global_manager->listener_capacity * 2;
        protocol_listener_t** new_listeners = (protocol_listener_t**)realloc(
            global_manager->listeners, sizeof(protocol_listener_t*) * new_capacity);
        
        if (new_listeners == NULL) {
            pthread_mutex_unlock(&global_manager->mutex);
            (*listener)->destroy(*listener);
            *listener = NULL;
            return STATUS_ERROR_MEMORY;
        }
        
        global_manager->listeners = new_listeners;
        global_manager->listener_capacity = new_capacity;
    }
    
    // Add listener to array
    global_manager->listeners[global_manager->listener_count++] = *listener;
    
    pthread_mutex_unlock(&global_manager->mutex);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Find a protocol listener by ID
 * 
 * @param id Listener ID
 * @return protocol_listener_t* Found listener or NULL
 */
protocol_listener_t* protocol_listener_find(const uuid_t* id) {
    if (id == NULL || global_manager == NULL) {
        return NULL;
    }
    
    protocol_listener_t* found = NULL;
    
    pthread_mutex_lock(&global_manager->mutex);
    
    // Search for listener with matching ID
    for (size_t i = 0; i < global_manager->listener_count; i++) {
        if (global_manager->listeners[i] != NULL && 
            uuid_compare(id, &global_manager->listeners[i]->id) == 0) {
            found = global_manager->listeners[i];
            break;
        }
    }
    
    pthread_mutex_unlock(&global_manager->mutex);
    
    return found;
}

/**
 * @brief Start a protocol listener
 * 
 * @param listener Listener to start
 * @return status_t Status code
 */
status_t protocol_listener_start(protocol_listener_t* listener) {
    if (listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Call listener's start function
    return listener->start(listener);
}

/**
 * @brief Stop a protocol listener
 * 
 * @param listener Listener to stop
 * @return status_t Status code
 */
status_t protocol_listener_stop(protocol_listener_t* listener) {
    if (listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Call listener's stop function
    return listener->stop(listener);
}

/**
 * @brief Get listener status
 * 
 * @param listener Listener to query
 * @param state Pointer to store listener state
 * @param stats Pointer to store listener statistics (can be NULL)
 * @return status_t Status code
 */
status_t protocol_listener_get_status(protocol_listener_t* listener, 
                                     listener_state_t* state,
                                     protocol_listener_stats_t* stats) {
    if (listener == NULL || state == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Get state
    *state = listener->state;
    
    // Copy statistics if requested
    if (stats != NULL) {
        memcpy(stats, &listener->stats, sizeof(protocol_listener_stats_t));
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Destroy a protocol listener
 * 
 * @param listener Listener to destroy
 * @return status_t Status code
 */
status_t protocol_listener_destroy(protocol_listener_t* listener) {
    if (listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Remove listener from manager
    if (global_manager != NULL) {
        pthread_mutex_lock(&global_manager->mutex);
        
        // Find listener in array
        for (size_t i = 0; i < global_manager->listener_count; i++) {
            if (global_manager->listeners[i] == listener) {
                // Remove listener from array by shifting remaining elements
                for (size_t j = i; j < global_manager->listener_count - 1; j++) {
                    global_manager->listeners[j] = global_manager->listeners[j + 1];
                }
                
                global_manager->listeners[global_manager->listener_count - 1] = NULL;
                global_manager->listener_count--;
                break;
            }
        }
        
        pthread_mutex_unlock(&global_manager->mutex);
    }
    
    // Call listener's destroy function
    return listener->destroy(listener);
}

/**
 * @brief Send a message to a client
 * 
 * @param listener Listener to use
 * @param client Client to send to
 * @param message Message to send
 * @return status_t Status code
 */
status_t protocol_send_message(protocol_listener_t* listener, 
                              client_t* client, 
                              protocol_message_t* message) {
    if (listener == NULL || client == NULL || message == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Call listener's send_message function
    return listener->send_message(listener, client, message);
}
