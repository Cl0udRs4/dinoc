/**
 * @file protocol_header.c
 * @brief Implementation of protocol header functions for encryption detection
 */

#include "../include/protocol.h"
#include "../include/encryption.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Protocol header structure
typedef struct {
    uint8_t magic;          // Magic byte for encryption type
    uint8_t version;        // Protocol version
    uint16_t flags;         // Protocol flags
    uint32_t payload_len;   // Payload length
} __attribute__((packed)) protocol_header_t;

// Magic bytes for encryption type
#define AES_MAGIC_BYTE      0xA3
#define CHACHA20_MAGIC_BYTE 0xC2
#define PROTOCOL_VERSION    0x01

/**
 * @brief Create a protocol header
 * 
 * @param type Encryption type
 * @param version Protocol version
 * @param flags Protocol flags
 * @param payload_len Payload length
 * @param header Output buffer for header (must be at least sizeof(protocol_header_t))
 * @return status_t Status code
 */
status_t protocol_header_create(encryption_type_t type, uint8_t version, uint16_t flags,
                               uint32_t payload_len, uint8_t* header) {
    if (header == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Create header
    protocol_header_t* hdr = (protocol_header_t*)header;
    
    // Set magic byte based on encryption type
    switch (type) {
        case ENCRYPTION_AES:
            hdr->magic = AES_MAGIC_BYTE;
            break;
            
        case ENCRYPTION_CHACHA20:
            hdr->magic = CHACHA20_MAGIC_BYTE;
            break;
            
        default:
            LOG_ERROR("Unsupported encryption type for protocol header: %d", type);
            return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Set other fields
    hdr->version = version;
    hdr->flags = flags;
    hdr->payload_len = payload_len;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Parse a protocol header
 * 
 * @param header Header data
 * @param type Pointer to store encryption type
 * @param version Pointer to store protocol version
 * @param flags Pointer to store protocol flags
 * @param payload_len Pointer to store payload length
 * @return status_t Status code
 */
status_t protocol_header_parse(const uint8_t* header, encryption_type_t* type,
                              uint8_t* version, uint16_t* flags, uint32_t* payload_len) {
    if (header == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Parse header
    const protocol_header_t* hdr = (const protocol_header_t*)header;
    
    // Determine encryption type
    switch (hdr->magic) {
        case AES_MAGIC_BYTE:
            if (type != NULL) {
                *type = ENCRYPTION_AES;
            }
            break;
            
        case CHACHA20_MAGIC_BYTE:
            if (type != NULL) {
                *type = ENCRYPTION_CHACHA20;
            }
            break;
            
        default:
            LOG_ERROR("Unknown encryption type in protocol header: 0x%02x", hdr->magic);
            if (type != NULL) {
                *type = ENCRYPTION_UNKNOWN;
            }
            return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Set other fields
    if (version != NULL) {
        *version = hdr->version;
    }
    
    if (flags != NULL) {
        *flags = hdr->flags;
    }
    
    if (payload_len != NULL) {
        *payload_len = hdr->payload_len;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Detect encryption type from protocol header
 * 
 * @param data Protocol header data
 * @param data_len Data length in bytes
 * @return encryption_type_t Detected encryption type
 */
static encryption_type_t protocol_detect_encryption(const uint8_t* data, size_t data_len) {
    if (data == NULL || data_len < sizeof(protocol_header_t)) {
        return ENCRYPTION_UNKNOWN;
    }
    
    // Parse header
    encryption_type_t type;
    if (protocol_header_parse(data, &type, NULL, NULL, NULL) != STATUS_SUCCESS) {
        return ENCRYPTION_UNKNOWN;
    }
    
    return type;
}

/**
 * @brief Create a message with protocol header
 * 
 * @param type Encryption type
 * @param message_type Message type
 * @param payload Payload data
 * @param payload_len Payload length
 * @param message Output buffer for message
 * @param message_len Pointer to store message length
 * @return status_t Status code
 */
status_t protocol_create_message(encryption_type_t enc_type, uint16_t message_type,
                                const uint8_t* payload, size_t payload_len,
                                uint8_t* message, size_t* message_len) {
    if (payload == NULL || message == NULL || message_len == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check message buffer size
    if (*message_len < sizeof(protocol_header_t) + payload_len) {
        LOG_ERROR("Message buffer too small: %zu < %zu", *message_len, sizeof(protocol_header_t) + payload_len);
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Create header
    status_t status = protocol_header_create(enc_type, PROTOCOL_VERSION, message_type, payload_len, message);
    if (status != STATUS_SUCCESS) {
        return status;
    }
    
    // Copy payload
    memcpy(message + sizeof(protocol_header_t), payload, payload_len);
    
    // Set message length
    *message_len = sizeof(protocol_header_t) + payload_len;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Parse a message with protocol header
 * 
 * @param message Message data
 * @param message_len Message length
 * @param enc_type Pointer to store encryption type
 * @param message_type Pointer to store message type
 * @param payload Pointer to store payload pointer
 * @param payload_len Pointer to store payload length
 * @return status_t Status code
 */
status_t protocol_parse_message(const uint8_t* message, size_t message_len,
                               encryption_type_t* enc_type, uint16_t* message_type,
                               const uint8_t** payload, size_t* payload_len) {
    if (message == NULL || message_len < sizeof(protocol_header_t)) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Parse header
    uint8_t version;
    uint16_t flags;
    uint32_t header_payload_len;
    
    status_t status = protocol_header_parse(message, enc_type, &version, &flags, &header_payload_len);
    if (status != STATUS_SUCCESS) {
        return status;
    }
    
    // Check message length
    if (message_len < sizeof(protocol_header_t) + header_payload_len) {
        LOG_ERROR("Message too short: %zu < %zu", message_len, sizeof(protocol_header_t) + header_payload_len);
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Set message type
    if (message_type != NULL) {
        *message_type = flags;
    }
    
    // Set payload
    if (payload != NULL) {
        *payload = message + sizeof(protocol_header_t);
    }
    
    if (payload_len != NULL) {
        *payload_len = header_payload_len;
    }
    
    return STATUS_SUCCESS;
}
