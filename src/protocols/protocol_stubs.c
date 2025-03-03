/**
 * @file protocol_stubs.c
 * @brief Stub implementations for protocol listener creation functions
 */

#define _GNU_SOURCE /* For strdup */

#include "../include/protocol.h"
#include "../include/common.h"
#include "../common/uuid.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Declare weak symbols to avoid multiple definition errors
__attribute__((weak)) status_t tcp_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener);
__attribute__((weak)) status_t udp_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener);
__attribute__((weak)) status_t ws_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener);
__attribute__((weak)) status_t icmp_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener);
__attribute__((weak)) status_t dns_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener);

/**
 * @brief Create TCP listener (stub implementation)
 */
__attribute__((weak)) status_t tcp_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener) {
    if (config == NULL || listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Create listener
    protocol_listener_t* new_listener = (protocol_listener_t*)malloc(sizeof(protocol_listener_t));
    if (new_listener == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize listener
    memset(new_listener, 0, sizeof(protocol_listener_t));
    uuid_generate_wrapper(new_listener->id);
    new_listener->protocol_type = PROTOCOL_TYPE_TCP;
    
    *listener = new_listener;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Create UDP listener (stub implementation)
 */
__attribute__((weak)) status_t udp_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener) {
    if (config == NULL || listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Create listener
    protocol_listener_t* new_listener = (protocol_listener_t*)malloc(sizeof(protocol_listener_t));
    if (new_listener == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize listener
    memset(new_listener, 0, sizeof(protocol_listener_t));
    uuid_generate_wrapper(new_listener->id);
    new_listener->protocol_type = PROTOCOL_TYPE_UDP;
    
    *listener = new_listener;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Create WebSocket listener (stub implementation)
 */
__attribute__((weak)) status_t ws_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener) {
    if (config == NULL || listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Create listener
    protocol_listener_t* new_listener = (protocol_listener_t*)malloc(sizeof(protocol_listener_t));
    if (new_listener == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize listener
    memset(new_listener, 0, sizeof(protocol_listener_t));
    uuid_generate_wrapper(new_listener->id);
    new_listener->protocol_type = PROTOCOL_TYPE_WS;
    
    *listener = new_listener;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Create ICMP listener (stub implementation)
 */
__attribute__((weak)) status_t icmp_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener) {
    if (config == NULL || listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Create listener
    protocol_listener_t* new_listener = (protocol_listener_t*)malloc(sizeof(protocol_listener_t));
    if (new_listener == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize listener
    memset(new_listener, 0, sizeof(protocol_listener_t));
    uuid_generate_wrapper(new_listener->id);
    new_listener->protocol_type = PROTOCOL_TYPE_ICMP;
    
    *listener = new_listener;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Create DNS listener (stub implementation)
 */
__attribute__((weak)) status_t dns_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener) {
    if (config == NULL || listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Create listener
    protocol_listener_t* new_listener = (protocol_listener_t*)malloc(sizeof(protocol_listener_t));
    if (new_listener == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize listener
    memset(new_listener, 0, sizeof(protocol_listener_t));
    uuid_generate_wrapper(new_listener->id);
    new_listener->protocol_type = PROTOCOL_TYPE_DNS;
    
    *listener = new_listener;
    
    return STATUS_SUCCESS;
}
