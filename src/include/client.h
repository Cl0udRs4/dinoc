/**
 * @file client.h
 * @brief Client management interface
 */

#ifndef DINOC_CLIENT_H
#define DINOC_CLIENT_H

#include "common.h"
#include "protocol.h"
#include "encryption.h"

/**
 * @brief Client information structure
 */
typedef struct {
    char hostname[256];         // Client hostname
    char username[64];          // Client username
    char os_version[256];       // Client OS version
    char ip_address[64];        // Client IP address
    uint16_t port;              // Client port
    uint64_t install_time;      // Client install timestamp
    uint32_t protocol_version;  // Client protocol version
    char* supported_modules;    // Comma-separated list of supported modules
} client_info_t;

/**
 * @brief Client structure
 */
struct client {
    uuid_t id;                  // Client ID
    client_state_t state;       // Client state
    client_info_t info;         // Client information
    protocol_type_t protocol;   // Current protocol
    encryption_type_t encryption; // Current encryption
    encryption_ctx_t enc_ctx;   // Encryption context
    time_t last_heartbeat;      // Last heartbeat time
    uint32_t heartbeat_interval; // Heartbeat interval in seconds
    void* protocol_ctx;         // Protocol-specific context
    void* user_data;            // User data
};

/**
 * @brief Create a new client
 * 
 * @param id Client ID (NULL for auto-generated)
 * @param client Pointer to store created client
 * @return status_t Status code
 */
status_t client_create(const uuid_t* id, client_t** client);

/**
 * @brief Register client information
 * 
 * @param client Client to register
 * @param info Client information
 * @return status_t Status code
 */
status_t client_register(client_t* client, const client_info_t* info);

/**
 * @brief Update client state
 * 
 * @param client Client to update
 * @param state New state
 * @return status_t Status code
 */
status_t client_update_state(client_t* client, client_state_t state);

/**
 * @brief Update client heartbeat
 * 
 * @param client Client to update
 * @return status_t Status code
 */
status_t client_update_heartbeat(client_t* client);

/**
 * @brief Set client heartbeat interval
 * 
 * @param client Client to update
 * @param interval_seconds Heartbeat interval in seconds
 * @return status_t Status code
 */
status_t client_set_heartbeat_interval(client_t* client, uint32_t interval_seconds);

/**
 * @brief Check if client is active (has recent heartbeat)
 * 
 * @param client Client to check
 * @return bool True if active, false otherwise
 */
bool client_is_active(const client_t* client);

/**
 * @brief Set client protocol
 * 
 * @param client Client to update
 * @param protocol New protocol
 * @return status_t Status code
 */
status_t client_set_protocol(client_t* client, protocol_type_t protocol);

/**
 * @brief Set client encryption
 * 
 * @param client Client to update
 * @param encryption New encryption type
 * @param key Encryption key
 * @param key_size Key size in bytes
 * @param iv Initialization vector/nonce
 * @param iv_size IV/nonce size in bytes
 * @return status_t Status code
 */
status_t client_set_encryption(client_t* client, encryption_type_t encryption,
                              const uint8_t* key, size_t key_size,
                              const uint8_t* iv, size_t iv_size);

/**
 * @brief Destroy a client
 * 
 * @param client Client to destroy
 * @return status_t Status code
 */
status_t client_destroy(client_t* client);

#endif /* DINOC_CLIENT_H */
