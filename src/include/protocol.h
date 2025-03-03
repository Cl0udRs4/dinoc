/**
 * @file protocol.h
 * @brief Protocol module interface
 */

#ifndef DINOC_PROTOCOL_H
#define DINOC_PROTOCOL_H

#include "common.h"
#include "encryption.h"

/**
 * @brief Forward declaration of client structure
 */
typedef struct client client_t;

/**
 * @brief Protocol message structure
 */
typedef struct {
    uuid_t id;                  // Message ID
    uint32_t timestamp;         // Message timestamp
    uint16_t type;              // Message type
    uint16_t flags;             // Message flags
    uint32_t data_len;          // Data length
    uint8_t* data;              // Message data
} protocol_message_t;

/**
 * @brief Protocol listener configuration
 */
typedef struct {
    protocol_type_t type;       // Protocol type
    char* bind_address;         // Bind address
    uint16_t port;              // Port number (for applicable protocols)
    char* domain;               // Domain name (for DNS protocol)
    uint32_t timeout_ms;        // Operation timeout in milliseconds
    bool auto_start;            // Auto-start listener on creation
} protocol_listener_config_t;

/**
 * @brief Protocol listener statistics
 */
typedef struct {
    uint64_t bytes_received;    // Total bytes received
    uint64_t bytes_sent;        // Total bytes sent
    uint64_t messages_received; // Total messages received
    uint64_t messages_sent;     // Total messages sent
    uint64_t errors;            // Total errors
    uint64_t clients_connected; // Current connected clients
    uint64_t clients_total;     // Total clients seen
    time_t start_time;          // Listener start time
    time_t last_message_time;   // Last message time
} protocol_listener_stats_t;

/**
 * @brief Protocol listener structure
 */
typedef struct protocol_listener {
    uuid_t id;                                  // Listener ID
    protocol_type_t type;                       // Protocol type
    listener_state_t state;                     // Listener state
    protocol_listener_config_t config;          // Listener configuration
    protocol_listener_stats_t stats;            // Listener statistics
    void* protocol_ctx;                         // Protocol-specific context
    
    // Function pointers for protocol operations
    status_t (*start)(struct protocol_listener*);
    status_t (*stop)(struct protocol_listener*);
    status_t (*destroy)(struct protocol_listener*);
    status_t (*send_message)(struct protocol_listener*, client_t*, protocol_message_t*);
    
    // Callback for received messages
    void (*on_message_received)(struct protocol_listener*, client_t*, protocol_message_t*);
    
    // Callback for client connection/disconnection
    void (*on_client_connected)(struct protocol_listener*, client_t*);
    void (*on_client_disconnected)(struct protocol_listener*, client_t*);
} protocol_listener_t;

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
                                 protocol_listener_t** listener);

/**
 * @brief Start a protocol listener
 * 
 * @param listener Listener to start
 * @return status_t Status code
 */
status_t protocol_listener_start(protocol_listener_t* listener);

/**
 * @brief Stop a protocol listener
 * 
 * @param listener Listener to stop
 * @return status_t Status code
 */
status_t protocol_listener_stop(protocol_listener_t* listener);

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
                                     protocol_listener_stats_t* stats);

/**
 * @brief Destroy a protocol listener
 * 
 * @param listener Listener to destroy
 * @return status_t Status code
 */
status_t protocol_listener_destroy(protocol_listener_t* listener);

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
                              protocol_message_t* message);

/**
 * @brief Create a protocol message
 * 
 * @param type Message type
 * @param data Message data
 * @param data_len Data length
 * @param message Pointer to store created message
 * @return status_t Status code
 */
status_t protocol_message_create(uint16_t type, 
                                const uint8_t* data, size_t data_len,
                                protocol_message_t** message);

/**
 * @brief Destroy a protocol message
 * 
 * @param message Message to destroy
 * @return status_t Status code
 */
status_t protocol_message_destroy(protocol_message_t* message);

#endif /* DINOC_PROTOCOL_H */
