/**
 * @file client_template.c
 * @brief Client template implementation for builder to generate customized clients
 */

#define _GNU_SOURCE  /* For strdup */

#include "client_template.h"
#include "../include/common.h"
#include "../include/protocol.h"
#include "../encryption/encryption.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

// Client configuration
static client_config_t client_config;

// Client state
static bool client_initialized = false;
static bool client_connected = false;
static protocol_type_t current_protocol = PROTOCOL_TYPE_TCP;
static time_t last_heartbeat_time = 0;
static pthread_t heartbeat_thread;
static bool heartbeat_thread_running = false;
static pthread_mutex_t heartbeat_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t heartbeat_cond = PTHREAD_COND_INITIALIZER;

// Forward declarations
static void* client_heartbeat_thread(void* arg);
static status_t client_connect_with_protocol(protocol_type_t protocol_type);

/**
 * @brief Initialize client template
 */
status_t client_template_init(const client_config_t* config) {
    if (config == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check if already initialized
    if (client_initialized) {
        return STATUS_ERROR_ALREADY_RUNNING;
    }
    
    // Copy configuration
    memset(&client_config, 0, sizeof(client_config_t));
    
    // Copy protocols
    if (config->protocol_count > 0) {
        client_config.protocols = (protocol_type_t*)malloc(config->protocol_count * sizeof(protocol_type_t));
        if (client_config.protocols == NULL) {
            return STATUS_ERROR_MEMORY;
        }
        
        memcpy(client_config.protocols, config->protocols, config->protocol_count * sizeof(protocol_type_t));
        client_config.protocol_count = config->protocol_count;
    }
    
    // Copy servers
    if (config->server_count > 0) {
        client_config.servers = (char**)malloc(config->server_count * sizeof(char*));
        if (client_config.servers == NULL) {
            free(client_config.protocols);
            return STATUS_ERROR_MEMORY;
        }
        
        for (size_t i = 0; i < config->server_count; i++) {
            client_config.servers[i] = strdup(config->servers[i]);
            if (client_config.servers[i] == NULL) {
                for (size_t j = 0; j < i; j++) {
                    free(client_config.servers[j]);
                }
                
                free(client_config.servers);
                free(client_config.protocols);
                return STATUS_ERROR_MEMORY;
            }
        }
        
        client_config.server_count = config->server_count;
    }
    
    // Copy domain
    if (config->domain != NULL) {
        client_config.domain = strdup(config->domain);
        if (client_config.domain == NULL) {
            for (size_t i = 0; i < client_config.server_count; i++) {
                free(client_config.servers[i]);
            }
            
            free(client_config.servers);
            free(client_config.protocols);
            return STATUS_ERROR_MEMORY;
        }
    }
    
    // Copy modules
    if (config->module_count > 0) {
        client_config.modules = (char**)malloc(config->module_count * sizeof(char*));
        if (client_config.modules == NULL) {
            free(client_config.domain);
            
            for (size_t i = 0; i < client_config.server_count; i++) {
                free(client_config.servers[i]);
            }
            
            free(client_config.servers);
            free(client_config.protocols);
            return STATUS_ERROR_MEMORY;
        }
        
        for (size_t i = 0; i < config->module_count; i++) {
            client_config.modules[i] = strdup(config->modules[i]);
            if (client_config.modules[i] == NULL) {
                for (size_t j = 0; j < i; j++) {
                    free(client_config.modules[j]);
                }
                
                free(client_config.modules);
                free(client_config.domain);
                
                for (size_t j = 0; j < client_config.server_count; j++) {
                    free(client_config.servers[j]);
                }
                
                free(client_config.servers);
                free(client_config.protocols);
                return STATUS_ERROR_MEMORY;
            }
        }
        
        client_config.module_count = config->module_count;
    }
    
    // Copy other configuration
    client_config.heartbeat_interval = config->heartbeat_interval;
    client_config.heartbeat_jitter = config->heartbeat_jitter;
    client_config.encryption_algorithm = config->encryption_algorithm;
    client_config.version_major = config->version_major;
    client_config.version_minor = config->version_minor;
    client_config.version_patch = config->version_patch;
    client_config.debug_mode = config->debug_mode;
    
    // Set default values if needed
    if (client_config.heartbeat_interval == 0) {
        client_config.heartbeat_interval = 60;  // Default: 1 minute
    }
    
    if (client_config.heartbeat_jitter == 0) {
        client_config.heartbeat_jitter = 10;  // Default: 10 seconds
    }
    
    // Set initial protocol
    if (client_config.protocol_count > 0) {
        current_protocol = client_config.protocols[0];
    }
    
    // Set initialized flag
    client_initialized = true;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Shutdown client template
 */
status_t client_template_shutdown(void) {
    // Check if initialized
    if (!client_initialized) {
        return STATUS_ERROR_NOT_INITIALIZED;
    }
    
    // Disconnect if connected
    if (client_connected) {
        client_template_disconnect();
    }
    
    // Free protocols
    if (client_config.protocols != NULL) {
        free(client_config.protocols);
        client_config.protocols = NULL;
    }
    
    // Free servers
    if (client_config.servers != NULL) {
        for (size_t i = 0; i < client_config.server_count; i++) {
            free(client_config.servers[i]);
        }
        
        free(client_config.servers);
        client_config.servers = NULL;
    }
    
    // Free domain
    if (client_config.domain != NULL) {
        free(client_config.domain);
        client_config.domain = NULL;
    }
    
    // Free modules
    if (client_config.modules != NULL) {
        for (size_t i = 0; i < client_config.module_count; i++) {
            free(client_config.modules[i]);
        }
        
        free(client_config.modules);
        client_config.modules = NULL;
    }
    
    // Reset state
    client_initialized = false;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Connect to server
 */
status_t client_template_connect(void) {
    // Check if initialized
    if (!client_initialized) {
        return STATUS_ERROR_NOT_INITIALIZED;
    }
    
    // Check if already connected
    if (client_connected) {
        return STATUS_ERROR_ALREADY_RUNNING;
    }
    
    // Connect with current protocol
    status_t status = client_connect_with_protocol(current_protocol);
    
    if (status != STATUS_SUCCESS) {
        // Try other protocols
        for (size_t i = 0; i < client_config.protocol_count; i++) {
            if (client_config.protocols[i] != current_protocol) {
                status = client_connect_with_protocol(client_config.protocols[i]);
                
                if (status == STATUS_SUCCESS) {
                    current_protocol = client_config.protocols[i];
                    break;
                }
            }
        }
    }
    
    if (status != STATUS_SUCCESS) {
        return status;
    }
    
    // Start heartbeat thread
    heartbeat_thread_running = true;
    if (pthread_create(&heartbeat_thread, NULL, &client_heartbeat_thread, NULL) != 0) {
        client_connected = false;
        return STATUS_ERROR;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Disconnect from server
 */
status_t client_template_disconnect(void) {
    // Check if initialized
    if (!client_initialized) {
        return STATUS_ERROR_NOT_INITIALIZED;
    }
    
    // Check if connected
    if (!client_connected) {
        return STATUS_ERROR_NOT_CONNECTED;
    }
    
    // Stop heartbeat thread
    pthread_mutex_lock(&heartbeat_mutex);
    heartbeat_thread_running = false;
    pthread_cond_signal(&heartbeat_cond);
    pthread_mutex_unlock(&heartbeat_mutex);
    
    pthread_join(heartbeat_thread, NULL);
    
    // Reset state
    client_connected = false;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Send heartbeat to server
 */
status_t client_template_send_heartbeat(void) {
    // Check if initialized
    if (!client_initialized) {
        return STATUS_ERROR_NOT_INITIALIZED;
    }
    
    // Check if connected
    if (!client_connected) {
        return STATUS_ERROR_NOT_CONNECTED;
    }
    
    // Update last heartbeat time
    time(&last_heartbeat_time);
    
    // TODO: Implement actual heartbeat sending
    // This is a placeholder for the actual implementation
    
    return STATUS_SUCCESS;
}

/**
 * @brief Switch to a different protocol
 */
status_t client_template_switch_protocol(protocol_type_t protocol_type) {
    // Check if initialized
    if (!client_initialized) {
        return STATUS_ERROR_NOT_INITIALIZED;
    }
    
    // Check if protocol is supported
    bool protocol_supported = false;
    for (size_t i = 0; i < client_config.protocol_count; i++) {
        if (client_config.protocols[i] == protocol_type) {
            protocol_supported = true;
            break;
        }
    }
    
    if (!protocol_supported) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // If already using this protocol, do nothing
    if (current_protocol == protocol_type) {
        return STATUS_SUCCESS;
    }
    
    // If connected, disconnect first
    bool was_connected = client_connected;
    if (was_connected) {
        client_template_disconnect();
    }
    
    // Switch protocol
    current_protocol = protocol_type;
    
    // Reconnect if needed
    if (was_connected) {
        return client_template_connect();
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Load a module
 */
status_t client_template_load_module(const char* module_name) {
    // Check if initialized
    if (!client_initialized) {
        return STATUS_ERROR_NOT_INITIALIZED;
    }
    
    // Suppress unused parameter warning
    (void)module_name;
    
    // TODO: Implement module loading
    // This is a placeholder for the actual implementation
    
    return STATUS_SUCCESS;
}

/**
 * @brief Unload a module
 */
status_t client_template_unload_module(const char* module_name) {
    // Check if initialized
    if (!client_initialized) {
        return STATUS_ERROR_NOT_INITIALIZED;
    }
    
    // Suppress unused parameter warning
    (void)module_name;
    
    // TODO: Implement module unloading
    // This is a placeholder for the actual implementation
    
    return STATUS_SUCCESS;
}

/**
 * @brief Execute a module
 */
status_t client_template_execute_module(const char* module_name, const uint8_t* data, size_t data_len) {
    // Check if initialized
    if (!client_initialized) {
        return STATUS_ERROR_NOT_INITIALIZED;
    }
    
    // Check if connected
    if (!client_connected) {
        return STATUS_ERROR_NOT_CONNECTED;
    }
    
    // Suppress unused parameter warnings
    (void)module_name;
    (void)data;
    (void)data_len;
    
    // TODO: Implement module execution
    // This is a placeholder for the actual implementation
    
    return STATUS_SUCCESS;
}

/**
 * @brief Connect with a specific protocol
 */
static status_t client_connect_with_protocol(protocol_type_t protocol_type) {
    // Suppress unused parameter warning
    (void)protocol_type;
    
    // TODO: Implement protocol-specific connection
    // This is a placeholder for the actual implementation
    
    // Simulate successful connection
    client_connected = true;
    time(&last_heartbeat_time);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Heartbeat thread function
 */
static void* client_heartbeat_thread(void* arg) {
    // Suppress unused parameter warning
    (void)arg;
    
    struct timespec ts;
    
    while (true) {
        // Wait for condition or timeout
        pthread_mutex_lock(&heartbeat_mutex);
        
        if (!heartbeat_thread_running) {
            pthread_mutex_unlock(&heartbeat_mutex);
            break;
        }
        
        // Calculate next heartbeat time with jitter
        time_t now;
        time(&now);
        
        // Add random jitter
        int jitter = 0;
        if (client_config.heartbeat_jitter > 0) {
            jitter = rand() % (2 * client_config.heartbeat_jitter) - client_config.heartbeat_jitter;
        }
        
        // Calculate next heartbeat time
        time_t next_heartbeat = last_heartbeat_time + client_config.heartbeat_interval + jitter;
        
        // If next heartbeat is in the past, send heartbeat immediately
        if (next_heartbeat <= now) {
            pthread_mutex_unlock(&heartbeat_mutex);
            client_template_send_heartbeat();
            continue;
        }
        
        // Wait until next heartbeat
        ts.tv_sec = next_heartbeat;
        ts.tv_nsec = 0;
        
        pthread_cond_timedwait(&heartbeat_cond, &heartbeat_mutex, &ts);
        
        if (!heartbeat_thread_running) {
            pthread_mutex_unlock(&heartbeat_mutex);
            break;
        }
        
        pthread_mutex_unlock(&heartbeat_mutex);
        
        // Send heartbeat
        client_template_send_heartbeat();
    }
    
    return NULL;
}
