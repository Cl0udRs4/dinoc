/**
 * @file protocol.h
 * @brief Protocol interface for C2 server
 */

#ifndef DINOC_PROTOCOL_H
#define DINOC_PROTOCOL_H

#include "common.h"
#include <stdint.h>
#include <stdbool.h>

// Forward declarations
typedef struct protocol_listener protocol_listener_t;
typedef struct client client_t;

// Protocol types
typedef enum {
    PROTOCOL_TYPE_TCP = 0,
    PROTOCOL_TYPE_UDP = 1,
    PROTOCOL_TYPE_WS = 2,
    PROTOCOL_TYPE_ICMP = 3,
    PROTOCOL_TYPE_DNS = 4
} protocol_type_t;

// Protocol message structure
typedef struct {
    uint8_t* data;
    size_t data_len;
} protocol_message_t;

// Protocol listener configuration
typedef struct {
    char* bind_address;
    uint16_t port;
    uint32_t timeout_ms;
    char* domain;         // For DNS protocol
    char* pcap_device;    // For ICMP protocol
    char* ws_path;        // For WebSocket protocol
} protocol_listener_config_t;

// Protocol listener interface
struct protocol_listener {
    uuid_t id;
    protocol_type_t protocol_type;
    void* protocol_context;  // Protocol-specific context
    
    // Function pointers
    status_t (*start)(protocol_listener_t* listener);
    status_t (*stop)(protocol_listener_t* listener);
    status_t (*destroy)(protocol_listener_t* listener);
    status_t (*send_message)(protocol_listener_t* listener, client_t* client, protocol_message_t* message);
    status_t (*register_callbacks)(protocol_listener_t* listener,
                                 void (*on_message_received)(protocol_listener_t*, client_t*, protocol_message_t*),
                                 void (*on_client_connected)(protocol_listener_t*, client_t*),
                                 void (*on_client_disconnected)(protocol_listener_t*, client_t*));
};

// Protocol manager functions
status_t protocol_manager_init(void);
status_t protocol_manager_shutdown(void);

status_t protocol_manager_create_listener(protocol_type_t type, const protocol_listener_config_t* config, protocol_listener_t** listener);
status_t protocol_manager_destroy_listener(protocol_listener_t* listener);

status_t protocol_manager_start_listener(protocol_listener_t* listener);
status_t protocol_manager_stop_listener(protocol_listener_t* listener);

status_t protocol_manager_send_message(protocol_listener_t* listener, client_t* client, protocol_message_t* message);

status_t protocol_manager_register_callbacks(protocol_listener_t* listener,
                                           void (*on_message_received)(protocol_listener_t*, client_t*, protocol_message_t*),
                                           void (*on_client_connected)(protocol_listener_t*, client_t*),
                                           void (*on_client_disconnected)(protocol_listener_t*, client_t*));

// Protocol listener creation functions
status_t tcp_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener);
status_t udp_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener);
status_t ws_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener);
status_t icmp_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener);
status_t dns_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener);

#endif /* DINOC_PROTOCOL_H */
