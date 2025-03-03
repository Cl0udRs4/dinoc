/**
 * @file protocol_switch_minimal.c
 * @brief Minimal implementation of protocol switching for testing
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

// Status codes
typedef int status_t;
#define STATUS_SUCCESS 0
#define STATUS_ERROR 1

// Protocol types
typedef enum {
    PROTOCOL_TYPE_TCP = 0,
    PROTOCOL_TYPE_UDP = 1,
    PROTOCOL_TYPE_WS = 2,
    PROTOCOL_TYPE_ICMP = 3,
    PROTOCOL_TYPE_DNS = 4
} protocol_type_t;

// Protocol switch magic number
#define PROTOCOL_SWITCH_MAGIC 0x50535743  // "PSWC"

// Protocol switch flags
#define PROTOCOL_SWITCH_FLAG_IMMEDIATE 0x01

// Protocol switch message structure
typedef struct {
    uint32_t magic;       // Magic number
    uint8_t protocol;     // Protocol type
    uint8_t flags;        // Flags
    uint16_t port;        // Port number
    uint32_t timeout_ms;  // Timeout in milliseconds
    char domain[64];      // Domain name (for DNS protocol)
} protocol_switch_message_t;

/**
 * @brief Create a protocol switch message
 */
status_t protocol_switch_create_message(protocol_type_t protocol, uint16_t port, const char* domain, 
                                      uint32_t timeout_ms, uint8_t flags, protocol_switch_message_t* message) {
    if (message == NULL) {
        return STATUS_ERROR;
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
