/**
 * @file protocol_switch.h
 * @brief Protocol switching interface for C2 server
 */

#ifndef DINOC_PROTOCOL_SWITCH_H
#define DINOC_PROTOCOL_SWITCH_H

#include "../include/protocol.h"
#include "../include/client.h"
#include <stdint.h>
#include <stdbool.h>

// Protocol switch message structure
typedef struct {
    uint32_t magic;              // Magic number to identify protocol switch message
    protocol_type_t protocol;    // Target protocol to switch to
    uint16_t port;               // Port for the target protocol
    char domain[256];            // Domain for DNS protocol (if applicable)
    uint32_t timeout_ms;         // Connection timeout in milliseconds
    uint8_t flags;               // Additional flags
} __attribute__((packed)) protocol_switch_message_t;

// Protocol switch magic number
#define PROTOCOL_SWITCH_MAGIC 0x50535743  // "PSWC"

// Protocol switch flags
#define PROTOCOL_SWITCH_FLAG_NONE      0x00
#define PROTOCOL_SWITCH_FLAG_IMMEDIATE 0x01  // Switch immediately
#define PROTOCOL_SWITCH_FLAG_FALLBACK  0x02  // Use as fallback protocol
#define PROTOCOL_SWITCH_FLAG_TEMPORARY 0x04  // Temporary switch
#define PROTOCOL_SWITCH_FLAG_FORCED    0x08  // Forced switch (ignore errors)

/**
 * @brief Create a protocol switch message
 * 
 * @param protocol Target protocol to switch to
 * @param port Port for the target protocol
 * @param domain Domain for DNS protocol (if applicable)
 * @param timeout_ms Connection timeout in milliseconds
 * @param flags Additional flags
 * @param message Pointer to store the created message
 * @return status_t Status code
 */
status_t protocol_switch_create_message(protocol_type_t protocol, uint16_t port, const char* domain, 
                                      uint32_t timeout_ms, uint8_t flags, protocol_switch_message_t* message);

/**
 * @brief Send a protocol switch message to a client
 * 
 * @param client Client to send the message to
 * @param message Protocol switch message to send
 * @return status_t Status code
 */
status_t protocol_switch_send_message(client_t* client, const protocol_switch_message_t* message);

/**
 * @brief Process a protocol switch message
 * 
 * @param client Client that received the message
 * @param data Message data
 * @param data_len Message data length
 * @return status_t Status code
 */
status_t protocol_switch_process_message(client_t* client, const uint8_t* data, size_t data_len);

/**
 * @brief Check if a message is a protocol switch message
 * 
 * @param data Message data
 * @param data_len Message data length
 * @return bool True if the message is a protocol switch message
 */
bool protocol_switch_is_message(const uint8_t* data, size_t data_len);

#endif /* DINOC_PROTOCOL_SWITCH_H */
