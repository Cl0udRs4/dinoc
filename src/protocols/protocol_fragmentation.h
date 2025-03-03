/**
 * @file protocol_fragmentation.h
 * @brief Protocol fragmentation interface
 */

#ifndef DINOC_PROTOCOL_FRAGMENTATION_H
#define DINOC_PROTOCOL_FRAGMENTATION_H

#include "../include/common.h"
#include "../include/protocol.h"
#include "../include/client.h"
#include <time.h>
#include <stdbool.h>

/**
 * @brief Fragment header structure
 */
typedef struct {
    uint16_t fragment_id;       // Unique identifier for the message being fragmented
    uint8_t fragment_index;     // Position of this fragment in the sequence
    uint8_t total_fragments;    // Total number of fragments in the message
    uint8_t flags;              // Control flags for special handling
} __attribute__((packed)) fragment_header_t;

/**
 * @brief Fragment tracker structure
 */
typedef struct {
    uint16_t fragment_id;           // Fragment ID
    uint8_t total_fragments;        // Total number of fragments
    uint8_t fragments_received;     // Number of fragments received
    uint8_t** fragment_data;        // Array of fragment data
    size_t* fragment_sizes;         // Array of fragment sizes
    time_t first_fragment_time;     // Time when first fragment was received
    bool* fragment_received;        // Array of flags indicating which fragments have been received
    client_t* client;               // Client that sent the fragments
    protocol_listener_t* listener;  // Listener that received the fragments
} fragment_tracker_t;

/**
 * @brief Callback for when a complete message has been reassembled
 */
typedef void (*on_message_reassembled_callback)(protocol_listener_t* listener, 
                                              client_t* client, 
                                              protocol_message_t* message);

/**
 * @brief Initialize fragmentation system
 * 
 * @return status_t Status code
 */
status_t fragmentation_init(void);

/**
 * @brief Shutdown fragmentation system
 * 
 * @return status_t Status code
 */
status_t fragmentation_shutdown(void);

/**
 * @brief Send a fragmented message
 * 
 * @param listener Protocol listener
 * @param client Client to send to
 * @param data Data to send
 * @param data_len Data length
 * @param max_fragment_size Maximum fragment size
 * @return status_t Status code
 */
status_t fragmentation_send_message(protocol_listener_t* listener, client_t* client,
                                  const uint8_t* data, size_t data_len,
                                  size_t max_fragment_size);

/**
 * @brief Process a received fragment
 * 
 * @param listener Protocol listener
 * @param client Client that sent the fragment
 * @param data Fragment data
 * @param data_len Fragment data length
 * @param callback Callback for when a complete message has been reassembled
 * @return status_t Status code
 */
status_t fragmentation_process_fragment(protocol_listener_t* listener, client_t* client,
                                      const uint8_t* data, size_t data_len,
                                      on_message_reassembled_callback callback);

/**
 * @brief Create a fragment header
 * 
 * @param fragment_id Fragment ID
 * @param fragment_index Fragment index
 * @param total_fragments Total number of fragments
 * @param flags Flags
 * @param header Header buffer to fill
 * @return status_t Status code
 */
status_t fragmentation_create_header(uint16_t fragment_id, uint8_t fragment_index,
                                   uint8_t total_fragments, uint8_t flags,
                                   fragment_header_t* header);

/**
 * @brief Parse a fragment header
 * 
 * @param data Data containing the header
 * @param data_len Data length
 * @param header Header structure to fill
 * @return status_t Status code
 */
status_t fragmentation_parse_header(const uint8_t* data, size_t data_len,
                                  fragment_header_t* header);

#endif /* DINOC_PROTOCOL_FRAGMENTATION_H */
