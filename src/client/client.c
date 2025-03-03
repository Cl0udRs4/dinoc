/**
 * @file client.c
 * @brief Client management implementation for C2 server
 */

#include "../include/client.h"
#include "../include/protocol.h"
#include "../protocols/protocol_switch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

// Heartbeat magic number
#define HEARTBEAT_MAGIC 0x48454152  // "HEAR"

// Client list
static client_t** clients = NULL;
static size_t clients_count = 0;
static pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// Forward declarations
static void* client_heartbeat_thread(void* arg);

// Heartbeat thread
static pthread_t heartbeat_thread;
static bool heartbeat_thread_running = false;
static pthread_mutex_t heartbeat_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t heartbeat_cond = PTHREAD_COND_INITIALIZER;

/**
 * @brief Initialize client manager
 */
status_t client_manager_init(void) {
    // Initialize client list
    clients = NULL;
    clients_count = 0;
    
    // Start heartbeat thread
    heartbeat_thread_running = true;
    if (pthread_create(&heartbeat_thread, NULL, &client_heartbeat_thread, NULL) != 0) {
        return STATUS_ERROR;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Shutdown client manager
 */
status_t client_manager_shutdown(void) {
    // Stop heartbeat thread
    pthread_mutex_lock(&heartbeat_mutex);
    heartbeat_thread_running = false;
    pthread_cond_signal(&heartbeat_cond);
    pthread_mutex_unlock(&heartbeat_mutex);
    
    pthread_join(heartbeat_thread, NULL);
    
    // Destroy all clients
    pthread_mutex_lock(&clients_mutex);
    
    for (size_t i = 0; i < clients_count; i++) {
        client_destroy(clients[i]);
    }
    
    free(clients);
    clients = NULL;
    clients_count = 0;
    
    pthread_mutex_unlock(&clients_mutex);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Register a new client
 */
status_t client_register(protocol_listener_t* listener, void* protocol_context, client_t** client) {
    if (listener == NULL || client == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Create client
    client_t* new_client = (client_t*)malloc(sizeof(client_t));
    if (new_client == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize client
    memset(new_client, 0, sizeof(client_t));
    
    // Generate UUID
    uuid_generate(new_client->id);
    
    // Set initial state
    new_client->state = CLIENT_STATE_CONNECTED;
    
    // Set protocol listener
    new_client->listener = listener;
    new_client->protocol_context = protocol_context;
    
    // Set timestamps
    time(&new_client->first_seen_time);
    time(&new_client->last_seen_time);
    time(&new_client->last_heartbeat);
    
    // Set default heartbeat parameters
    new_client->heartbeat_interval = 60;  // 1 minute
    new_client->heartbeat_jitter = 10;    // 10 seconds
    
    // Add client to list
    pthread_mutex_lock(&clients_mutex);
    
    client_t** new_clients = (client_t**)realloc(clients, (clients_count + 1) * sizeof(client_t*));
    if (new_clients == NULL) {
        pthread_mutex_unlock(&clients_mutex);
        free(new_client);
        return STATUS_ERROR_MEMORY;
    }
    
    clients = new_clients;
    clients[clients_count] = new_client;
    clients_count++;
    
    pthread_mutex_unlock(&clients_mutex);
    
    *client = new_client;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Update client state
 */
status_t client_update_state(client_t* client, client_state_t state) {
    if (client == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&clients_mutex);
    client->state = state;
    time(&client->last_seen_time);
    pthread_mutex_unlock(&clients_mutex);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Update client information
 */
status_t client_update_info(client_t* client, const char* hostname, const char* ip_address, const char* os_info) {
    if (client == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&clients_mutex);
    
    // Update hostname
    if (hostname != NULL) {
        if (client->hostname != NULL) {
            free(client->hostname);
        }
        
        client->hostname = strdup(hostname);
        if (client->hostname == NULL) {
            pthread_mutex_unlock(&clients_mutex);
            return STATUS_ERROR_MEMORY;
        }
    }
    
    // Update IP address
    if (ip_address != NULL) {
        if (client->ip_address != NULL) {
            free(client->ip_address);
        }
        
        client->ip_address = strdup(ip_address);
        if (client->ip_address == NULL) {
            pthread_mutex_unlock(&clients_mutex);
            return STATUS_ERROR_MEMORY;
        }
    }
    
    // Update OS information
    if (os_info != NULL) {
        if (client->os_info != NULL) {
            free(client->os_info);
        }
        
        client->os_info = strdup(os_info);
        if (client->os_info == NULL) {
            pthread_mutex_unlock(&clients_mutex);
            return STATUS_ERROR_MEMORY;
        }
    }
    
    time(&client->last_seen_time);
    
    pthread_mutex_unlock(&clients_mutex);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Set client heartbeat parameters
 */
status_t client_set_heartbeat(client_t* client, uint32_t interval, uint32_t jitter) {
    if (client == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Validate parameters
    if (interval < 1 || interval > 86400) {  // 1 second to 24 hours
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    if (jitter > interval / 2) {  // Jitter should not be more than half the interval
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&clients_mutex);
    client->heartbeat_interval = interval;
    client->heartbeat_jitter = jitter;
    pthread_mutex_unlock(&clients_mutex);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Process client heartbeat
 */
status_t client_heartbeat(client_t* client) {
    if (client == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&clients_mutex);
    time(&client->last_heartbeat);
    time(&client->last_seen_time);
    
    // Update state if needed
    if (client->state == CLIENT_STATE_INACTIVE) {
        client->state = CLIENT_STATE_ACTIVE;
    }
    
    pthread_mutex_unlock(&clients_mutex);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Check if client heartbeat has timed out
 */
bool client_is_heartbeat_timeout(const client_t* client) {
    if (client == NULL) {
        return false;
    }
    
    time_t now;
    time(&now);
    
    // Calculate timeout with jitter
    time_t timeout = client->heartbeat_interval + client->heartbeat_jitter;
    
    return (now - client->last_heartbeat) > timeout;
}

/**
 * @brief Switch client protocol
 */
status_t client_switch_protocol(client_t* client, protocol_type_t protocol_type) {
    if (client == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Placeholder for protocol switch message
    // In a real implementation, this would create and send a protocol switch message
    // For now, we'll just simulate success
    
    // Update client protocol type
    client->protocol_type = protocol_type;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Load module on client
 */
status_t client_load_module(client_t* client, const char* module_name, const uint8_t* module_data, size_t module_data_len) {
    if (client == NULL || module_name == NULL || module_data == NULL || module_data_len == 0) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Create module load message
    // Format: module_name_len(4) + module_name + module_data
    size_t module_name_len = strlen(module_name);
    size_t message_data_len = sizeof(uint32_t) + module_name_len + module_data_len;
    uint8_t* message_data = (uint8_t*)malloc(message_data_len);
    if (message_data == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Set module name length
    uint32_t name_len = (uint32_t)module_name_len;
    memcpy(message_data, &name_len, sizeof(uint32_t));
    
    // Set module name
    memcpy(message_data + sizeof(uint32_t), module_name, module_name_len);
    
    // Set module data
    memcpy(message_data + sizeof(uint32_t) + module_name_len, module_data, module_data_len);
    
    // Create protocol message
    protocol_message_t message;
    message.data = message_data;
    message.data_len = message_data_len;
    
    // Placeholder for protocol_manager_send_message
    // In a real implementation, this would send the message to the client
    // For now, we'll just simulate success
    free(message_data);
    return STATUS_SUCCESS;
}

/**
 * @brief Unload module from client
 */
status_t client_unload_module(client_t* client, const char* module_name) {
    if (client == NULL || module_name == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Create module unload message
    // Task data format: module_name
    size_t module_name_len = strlen(module_name);
    uint8_t* message_data = (uint8_t*)malloc(module_name_len);
    if (message_data == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Set module name
    memcpy(message_data, module_name, module_name_len);
    
    // Create protocol message
    protocol_message_t message;
    message.data = message_data;
    message.data_len = module_name_len;
    
    // Placeholder for protocol_manager_send_message
    // In a real implementation, this would send the message to the client
    // For now, we'll just simulate success
    free(message_data);
    return STATUS_SUCCESS;
}

/**
 * @brief Find a client by ID
 */
client_t* client_find(const uuid_t* id) {
    if (id == NULL) {
        return NULL;
    }
    
    pthread_mutex_lock(&clients_mutex);
    
    for (size_t i = 0; i < clients_count; i++) {
        if (uuid_compare(clients[i]->id, *id) == 0) {
            pthread_mutex_unlock(&clients_mutex);
            return clients[i];
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
    
    return NULL;
}

/**
 * @brief Get all clients
 */
status_t client_get_all(client_t*** clients_out, size_t* count) {
    if (clients_out == NULL || count == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&clients_mutex);
    
    // Create copy of clients array
    client_t** clients_copy = NULL;
    
    if (clients_count > 0) {
        clients_copy = (client_t**)malloc(clients_count * sizeof(client_t*));
        if (clients_copy == NULL) {
            pthread_mutex_unlock(&clients_mutex);
            return STATUS_ERROR_MEMORY;
        }
        
        memcpy(clients_copy, clients, clients_count * sizeof(client_t*));
    }
    
    *clients_out = clients_copy;
    *count = clients_count;
    
    pthread_mutex_unlock(&clients_mutex);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Destroy a client
 */
status_t client_destroy(client_t* client) {
    if (client == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Free hostname
    if (client->hostname != NULL) {
        free(client->hostname);
    }
    
    // Free IP address
    if (client->ip_address != NULL) {
        free(client->ip_address);
    }
    
    // Free OS information
    if (client->os_info != NULL) {
        free(client->os_info);
    }
    
    // Free modules
    if (client->modules != NULL) {
        free(client->modules);
    }
    
    // Free client
    free(client);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Send heartbeat request to client
 */
status_t client_send_heartbeat_request(client_t* client) {
    if (client == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check if client is connected
    if (client->state == CLIENT_STATE_DISCONNECTED) {
        return STATUS_ERROR_NOT_CONNECTED;
    }
    
    // Create heartbeat message
    protocol_message_t message;
    uint32_t heartbeat_magic = HEARTBEAT_MAGIC;
    message.data = (uint8_t*)&heartbeat_magic;
    message.data_len = sizeof(heartbeat_magic);
    
    // Placeholder for protocol_manager_send_message
    // In a real implementation, this would send the message to the client
    // For now, we'll just simulate success
    return STATUS_SUCCESS;
}

/**
 * @brief Heartbeat timeout thread function
 */
static void* client_heartbeat_thread(void* arg) {
    struct timespec ts;
    
    while (true) {
        // Wait for condition or timeout
        pthread_mutex_lock(&heartbeat_mutex);
        
        if (!heartbeat_thread_running) {
            pthread_mutex_unlock(&heartbeat_mutex);
            break;
        }
        
        // Wait for 10 seconds
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 10;
        
        pthread_cond_timedwait(&heartbeat_cond, &heartbeat_mutex, &ts);
        
        if (!heartbeat_thread_running) {
            pthread_mutex_unlock(&heartbeat_mutex);
            break;
        }
        
        pthread_mutex_unlock(&heartbeat_mutex);
        
        // Check all clients for heartbeat timeout
        pthread_mutex_lock(&clients_mutex);
        
        for (size_t i = 0; i < clients_count; i++) {
            if (clients[i]->state == CLIENT_STATE_ACTIVE && client_is_heartbeat_timeout(clients[i])) {
                // Update client state
                clients[i]->state = CLIENT_STATE_INACTIVE;
                
                // Log the event
                // We need to implement uuid_to_string in uuid.c
                fprintf(stderr, "Client heartbeat timeout\n");
                
                // Notify protocol listener if available
                // Use protocol_manager_register_callbacks to register callbacks
                // This is a placeholder for now
                
                // Try to send a heartbeat request to the client
                client_send_heartbeat_request(clients[i]);
            }
        }
        
        pthread_mutex_unlock(&clients_mutex);
    }
    
    return NULL;
}
