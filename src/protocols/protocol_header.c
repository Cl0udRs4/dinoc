/**
 * @file protocol_header.c
 * @brief Protocol header implementation
 */

#include "../include/protocol.h"
#include "../include/protocol_header.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * @brief Create protocol header
 */
status_t protocol_header_create(protocol_type_t type, uint32_t flags, protocol_header_t** header) {
    if (header == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Allocate header
    protocol_header_t* new_header = (protocol_header_t*)malloc(sizeof(protocol_header_t));
    if (new_header == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize header
    new_header->magic = PROTOCOL_MAGIC;
    new_header->version = PROTOCOL_VERSION;
    new_header->type = type;
    new_header->flags = flags;
    new_header->timestamp = time(NULL);
    new_header->sequence = 0;
    new_header->payload_len = 0;
    
    *header = new_header;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Destroy protocol header
 */
status_t protocol_header_destroy(protocol_header_t* header) {
    if (header == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    free(header);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Serialize protocol header
 */
status_t protocol_header_serialize(const protocol_header_t* header, uint8_t* buffer, size_t buffer_len, size_t* bytes_written) {
    if (header == NULL || buffer == NULL || bytes_written == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    if (buffer_len < sizeof(protocol_header_t)) {
        return STATUS_ERROR_BUFFER_TOO_SMALL;
    }
    
    // Copy header to buffer
    memcpy(buffer, header, sizeof(protocol_header_t));
    
    *bytes_written = sizeof(protocol_header_t);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Deserialize protocol header
 */
status_t protocol_header_deserialize(const uint8_t* buffer, size_t buffer_len, protocol_header_t** header) {
    if (buffer == NULL || header == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    if (buffer_len < sizeof(protocol_header_t)) {
        return STATUS_ERROR_BUFFER_TOO_SMALL;
    }
    
    // Allocate header
    protocol_header_t* new_header = (protocol_header_t*)malloc(sizeof(protocol_header_t));
    if (new_header == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Copy buffer to header
    memcpy(new_header, buffer, sizeof(protocol_header_t));
    
    // Validate header
    if (new_header->magic != PROTOCOL_MAGIC) {
        free(new_header);
        return STATUS_ERROR;
    }
    
    *header = new_header;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Validate protocol header
 */
status_t protocol_header_validate(const protocol_header_t* header) {
    if (header == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check magic
    if (header->magic != PROTOCOL_MAGIC) {
        return STATUS_ERROR;
    }
    
    // Check version
    if (header->version != PROTOCOL_VERSION) {
        return STATUS_ERROR;
    }
    
    return STATUS_SUCCESS;
}
