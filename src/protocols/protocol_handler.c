/**
 * @file protocol_handler.c
 * @brief Implementation of protocol message handling
 */

#include "../include/protocol.h"
#include "../include/encryption.h"
#include "../include/client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations
status_t protocol_header_parse(const uint8_t* header, encryption_type_t* type,
                              uint8_t* version, uint16_t* flags, uint32_t* payload_len);
status_t protocol_header_create(encryption_type_t type, uint8_t version, uint16_t flags,
                               uint32_t payload_len, uint8_t* header);
encryption_type_t protocol_detect_encryption(const uint8_t* data, size_t data_len);
status_t protocol_parse_message(const uint8_t* message, size_t message_len,
                               encryption_type_t* enc_type, uint16_t* message_type,
                               const uint8_t** payload, size_t* payload_len);

/**
 * @brief Process incoming protocol message
 * 
 * @param listener Listener that received the message
 * @param client Client that sent the message
 * @param data Raw message data
 * @param data_len Data length
 * @return status_t Status code
 */
status_t protocol_process_message(protocol_listener_t* listener, client_t* client,
                                 const uint8_t* data, size_t data_len) {
    if (listener == NULL || client == NULL || data == NULL || data_len == 0) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Minimum message size is the protocol header
    if (data_len < sizeof(uint8_t) * 8) {
        LOG_ERROR("Message too short: %zu bytes", data_len);
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Detect encryption type from protocol header
    encryption_type_t enc_type = protocol_detect_encryption(data, data_len);
    
    // If client doesn't have encryption set, set it now
    if (client->encryption == ENCRYPTION_NONE || client->encryption == ENCRYPTION_UNKNOWN) {
        LOG_INFO("Setting client encryption to %s based on protocol header",
                enc_type == ENCRYPTION_AES ? "AES" : 
                enc_type == ENCRYPTION_CHACHA20 ? "ChaCha20" : "Unknown");
        
        client->encryption = enc_type;
        
        // TODO: Initialize encryption context
    }
    
    // Parse message
    const uint8_t* payload;
    size_t payload_len;
    uint16_t message_type;
    
    status_t status = protocol_parse_message(data, data_len, NULL, &message_type, &payload, &payload_len);
    if (status != STATUS_SUCCESS) {
        return status;
    }
    
    // Decrypt payload if needed
    uint8_t* decrypted_payload = NULL;
    size_t decrypted_len = 0;
    
    if (enc_type != ENCRYPTION_NONE && enc_type != ENCRYPTION_UNKNOWN) {
        // Allocate buffer for decrypted payload
        decrypted_payload = (uint8_t*)malloc(payload_len);
        if (decrypted_payload == NULL) {
            return STATUS_ERROR_MEMORY;
        }
        
        decrypted_len = payload_len;
        
        // Decrypt payload
        status = encryption_decrypt(&client->enc_ctx, payload, payload_len, decrypted_payload, &decrypted_len);
        if (status != STATUS_SUCCESS) {
            free(decrypted_payload);
            return status;
        }
    } else {
        // No encryption, use payload as-is
        decrypted_payload = (uint8_t*)payload;
        decrypted_len = payload_len;
    }
    
    // Create protocol message
    protocol_message_t* message = NULL;
    status = protocol_message_create(message_type, decrypted_payload, decrypted_len, &message);
    
    // Free decrypted payload if it was allocated
    if (enc_type != ENCRYPTION_NONE && enc_type != ENCRYPTION_UNKNOWN) {
        free(decrypted_payload);
    }
    
    if (status != STATUS_SUCCESS) {
        return status;
    }
    
    // Call message handler
    if (listener->on_message_received != NULL) {
        listener->on_message_received(listener, client, message);
    }
    
    // Destroy message
    protocol_message_destroy(message);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Send protocol message to client
 * 
 * @param listener Listener to use
 * @param client Client to send to
 * @param message Message to send
 * @return status_t Status code
 */
status_t protocol_send_message(protocol_listener_t* listener, client_t* client, protocol_message_t* message) {
    if (listener == NULL || client == NULL || message == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check if client has encryption set
    if (client->encryption == ENCRYPTION_NONE || client->encryption == ENCRYPTION_UNKNOWN) {
        LOG_WARNING("Client encryption not set, using AES as default");
        client->encryption = ENCRYPTION_AES;
        
        // TODO: Initialize encryption context
    }
    
    // Encrypt message data if needed
    uint8_t* encrypted_data = NULL;
    size_t encrypted_len = 0;
    
    if (client->encryption != ENCRYPTION_NONE) {
        // Allocate buffer for encrypted data (with some extra space for padding)
        encrypted_len = message->data_len + 32;
        encrypted_data = (uint8_t*)malloc(encrypted_len);
        if (encrypted_data == NULL) {
            return STATUS_ERROR_MEMORY;
        }
        
        // Encrypt data
        status_t status = encryption_encrypt(&client->enc_ctx, message->data, message->data_len, encrypted_data, &encrypted_len);
        if (status != STATUS_SUCCESS) {
            free(encrypted_data);
            return status;
        }
    } else {
        // No encryption, use data as-is
        encrypted_data = message->data;
        encrypted_len = message->data_len;
    }
    
    // Allocate buffer for full message (header + encrypted data)
    size_t full_message_len = sizeof(uint8_t) * 8 + encrypted_len;
    uint8_t* full_message = (uint8_t*)malloc(full_message_len);
    if (full_message == NULL) {
        if (client->encryption != ENCRYPTION_NONE) {
            free(encrypted_data);
        }
        return STATUS_ERROR_MEMORY;
    }
    
    // Create protocol header
    status_t status = protocol_header_create(client->encryption, 0x01, message->type, encrypted_len, full_message);
    if (status != STATUS_SUCCESS) {
        free(full_message);
        if (client->encryption != ENCRYPTION_NONE) {
            free(encrypted_data);
        }
        return status;
    }
    
    // Copy encrypted data
    memcpy(full_message + sizeof(uint8_t) * 8, encrypted_data, encrypted_len);
    
    // Free encrypted data if it was allocated
    if (client->encryption != ENCRYPTION_NONE) {
        free(encrypted_data);
    }
    
    // Send message using listener's send function
    status = listener->send_message(listener, client, message);
    
    // Free full message
    free(full_message);
    
    return status;
}

/**
 * @brief Create a protocol message
 * 
 * @param type Message type
 * @param data Message data
 * @param data_len Data length
 * @param message Pointer to store created message
 * @return status_t Status code
 */
status_t protocol_message_create(uint16_t type, const uint8_t* data, size_t data_len, protocol_message_t** message) {
    if (message == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Allocate message
    protocol_message_t* new_message = (protocol_message_t*)malloc(sizeof(protocol_message_t));
    if (new_message == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize message
    memset(new_message, 0, sizeof(protocol_message_t));
    uuid_generate(&new_message->id);
    new_message->timestamp = (uint32_t)time(NULL);
    new_message->type = type;
    new_message->flags = 0;
    
    // Copy data if provided
    if (data != NULL && data_len > 0) {
        new_message->data = (uint8_t*)malloc(data_len);
        if (new_message->data == NULL) {
            free(new_message);
            return STATUS_ERROR_MEMORY;
        }
        
        memcpy(new_message->data, data, data_len);
        new_message->data_len = data_len;
    }
    
    // Set message
    *message = new_message;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Destroy a protocol message
 * 
 * @param message Message to destroy
 * @return status_t Status code
 */
status_t protocol_message_destroy(protocol_message_t* message) {
    if (message == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Free data
    if (message->data != NULL) {
        free(message->data);
    }
    
    // Free message
    free(message);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Detect encryption type from protocol header
 * 
 * @param data Protocol header data
 * @param data_len Data length in bytes
 * @return encryption_type_t Detected encryption type
 */
encryption_type_t protocol_detect_encryption(const uint8_t* data, size_t data_len) {
    if (data == NULL || data_len < sizeof(uint8_t) * 8) {
        return ENCRYPTION_UNKNOWN;
    }
    
    // Parse header
    encryption_type_t type;
    if (protocol_header_parse(data, &type, NULL, NULL, NULL) != STATUS_SUCCESS) {
        return ENCRYPTION_UNKNOWN;
    }
    
    return type;
}
