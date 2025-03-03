/**
 * @file protocol_header.h
 * @brief Protocol header interface for C2 server
 */

#ifndef DINOC_PROTOCOL_HEADER_H
#define DINOC_PROTOCOL_HEADER_H

#include "common.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Protocol magic number
#define PROTOCOL_MAGIC 0x44494E4F  // "DINO"

// Protocol version
#define PROTOCOL_VERSION 0x01

// Protocol header structure
typedef struct {
    uint32_t magic;       // Magic number
    uint8_t version;      // Protocol version
    uint8_t type;         // Protocol type
    uint16_t flags;       // Protocol flags
    uint32_t timestamp;   // Timestamp
    uint32_t sequence;    // Sequence number
    uint32_t payload_len; // Payload length
} protocol_header_t;

// Protocol packet structure
typedef struct {
    protocol_header_t* header;
    uint8_t* payload;
} protocol_packet_t;

// Protocol header functions
status_t protocol_header_create(protocol_type_t type, uint32_t flags, protocol_header_t** header);
status_t protocol_header_destroy(protocol_header_t* header);

status_t protocol_header_serialize(const protocol_header_t* header, uint8_t* buffer, size_t buffer_len, size_t* bytes_written);
status_t protocol_header_deserialize(const uint8_t* buffer, size_t buffer_len, protocol_header_t** header);

status_t protocol_header_validate(const protocol_header_t* header);

// Protocol packet functions
status_t protocol_packet_create(protocol_type_t type, uint32_t flags, const uint8_t* data, size_t data_len, protocol_packet_t** packet);
status_t protocol_packet_destroy(protocol_packet_t* packet);

status_t protocol_packet_serialize(const protocol_packet_t* packet, uint8_t* buffer, size_t buffer_len, size_t* bytes_written);
status_t protocol_packet_deserialize(const uint8_t* buffer, size_t buffer_len, protocol_packet_t** packet);

// Protocol message functions
status_t protocol_message_create(const uint8_t* data, size_t data_len, protocol_message_t** message);
status_t protocol_message_destroy(protocol_message_t* message);

#endif /* DINOC_PROTOCOL_HEADER_H */
