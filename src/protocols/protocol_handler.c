/**
 * @file protocol_handler.c
 * @brief Protocol handler implementation
 */

#include "../include/protocol.h"
#include "../include/protocol_header.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * @brief Create protocol message
 */
status_t protocol_message_create(const uint8_t* data, size_t data_len, protocol_message_t** message) {
    if (data == NULL || message == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Allocate message
    protocol_message_t* new_message = (protocol_message_t*)malloc(sizeof(protocol_message_t));
    if (new_message == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Allocate data buffer
    new_message->data = (uint8_t*)malloc(data_len);
    if (new_message->data == NULL) {
        free(new_message);
        return STATUS_ERROR_MEMORY;
    }
    
    // Copy data
    memcpy(new_message->data, data, data_len);
    new_message->data_len = data_len;
    
    *message = new_message;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Destroy protocol message
 */
status_t protocol_message_destroy(protocol_message_t* message) {
    if (message == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Free data buffer
    if (message->data != NULL) {
        free(message->data);
    }
    
    // Free message
    free(message);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Create protocol packet
 */
status_t protocol_packet_create(protocol_type_t type, uint32_t flags, const uint8_t* data, size_t data_len, protocol_packet_t** packet) {
    if (packet == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Allocate packet
    protocol_packet_t* new_packet = (protocol_packet_t*)malloc(sizeof(protocol_packet_t));
    if (new_packet == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Create header
    status_t status = protocol_header_create(type, flags, &new_packet->header);
    if (status != STATUS_SUCCESS) {
        free(new_packet);
        return status;
    }
    
    // Set payload length
    new_packet->header->payload_len = data_len;
    
    // Allocate payload
    new_packet->payload = NULL;
    
    if (data_len > 0) {
        new_packet->payload = (uint8_t*)malloc(data_len);
        if (new_packet->payload == NULL) {
            protocol_header_destroy(new_packet->header);
            free(new_packet);
            return STATUS_ERROR_MEMORY;
        }
        
        // Copy payload
        memcpy(new_packet->payload, data, data_len);
    }
    
    *packet = new_packet;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Destroy protocol packet
 */
status_t protocol_packet_destroy(protocol_packet_t* packet) {
    if (packet == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Destroy header
    if (packet->header != NULL) {
        protocol_header_destroy(packet->header);
    }
    
    // Free payload
    if (packet->payload != NULL) {
        free(packet->payload);
    }
    
    // Free packet
    free(packet);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Serialize protocol packet
 */
status_t protocol_packet_serialize(const protocol_packet_t* packet, uint8_t* buffer, size_t buffer_len, size_t* bytes_written) {
    if (packet == NULL || buffer == NULL || bytes_written == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Calculate required buffer size
    size_t required_size = sizeof(protocol_header_t) + packet->header->payload_len;
    
    if (buffer_len < required_size) {
        return STATUS_ERROR_BUFFER_TOO_SMALL;
    }
    
    // Serialize header
    size_t header_size = 0;
    status_t status = protocol_header_serialize(packet->header, buffer, buffer_len, &header_size);
    if (status != STATUS_SUCCESS) {
        return status;
    }
    
    // Copy payload
    if (packet->header->payload_len > 0 && packet->payload != NULL) {
        memcpy(buffer + header_size, packet->payload, packet->header->payload_len);
    }
    
    *bytes_written = header_size + packet->header->payload_len;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Deserialize protocol packet
 */
status_t protocol_packet_deserialize(const uint8_t* buffer, size_t buffer_len, protocol_packet_t** packet) {
    if (buffer == NULL || packet == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    if (buffer_len < sizeof(protocol_header_t)) {
        return STATUS_ERROR_BUFFER_TOO_SMALL;
    }
    
    // Allocate packet
    protocol_packet_t* new_packet = (protocol_packet_t*)malloc(sizeof(protocol_packet_t));
    if (new_packet == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Deserialize header
    status_t status = protocol_header_deserialize(buffer, buffer_len, &new_packet->header);
    if (status != STATUS_SUCCESS) {
        free(new_packet);
        return status;
    }
    
    // Validate header
    status = protocol_header_validate(new_packet->header);
    if (status != STATUS_SUCCESS) {
        protocol_header_destroy(new_packet->header);
        free(new_packet);
        return status;
    }
    
    // Check payload length
    if (new_packet->header->payload_len > buffer_len - sizeof(protocol_header_t)) {
        protocol_header_destroy(new_packet->header);
        free(new_packet);
        return STATUS_ERROR_BUFFER_TOO_SMALL;
    }
    
    // Allocate payload
    new_packet->payload = NULL;
    
    if (new_packet->header->payload_len > 0) {
        new_packet->payload = (uint8_t*)malloc(new_packet->header->payload_len);
        if (new_packet->payload == NULL) {
            protocol_header_destroy(new_packet->header);
            free(new_packet);
            return STATUS_ERROR_MEMORY;
        }
        
        // Copy payload
        memcpy(new_packet->payload, buffer + sizeof(protocol_header_t), new_packet->header->payload_len);
    }
    
    *packet = new_packet;
    
    return STATUS_SUCCESS;
}
