/**
 * @file protocol_fragmentation.c
 * @brief Protocol fragmentation implementation
 */

#include "protocol_fragmentation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

// Fragment tracker manager
typedef struct {
    fragment_tracker_t** trackers;      // Array of fragment trackers
    size_t tracker_count;               // Number of trackers
    size_t tracker_capacity;            // Capacity of trackers array
    pthread_mutex_t mutex;              // Mutex for thread safety
    pthread_t cleanup_thread;           // Thread for cleaning up timed out trackers
    bool running;                       // Running flag
} fragment_manager_t;

// Global fragment manager
static fragment_manager_t* global_manager = NULL;

// Fragment timeout in seconds
#define FRAGMENT_TIMEOUT 60

// Forward declarations
static void* fragmentation_cleanup_thread(void* arg);
static fragment_tracker_t* fragmentation_find_tracker(uint16_t fragment_id, client_t* client);
static fragment_tracker_t* fragmentation_create_tracker(uint16_t fragment_id, uint8_t total_fragments, client_t* client, protocol_listener_t* listener);
static status_t fragmentation_add_fragment(fragment_tracker_t* tracker, uint8_t fragment_index, const uint8_t* data, size_t data_len);
static bool fragmentation_is_complete(fragment_tracker_t* tracker);
static status_t fragmentation_reassemble(fragment_tracker_t* tracker, protocol_message_t* message);
static status_t fragmentation_destroy_tracker(fragment_tracker_t* tracker);

/**
 * @brief Initialize fragmentation system
 */
status_t fragmentation_init(void) {
    if (global_manager != NULL) {
        return STATUS_ERROR_ALREADY_EXISTS;
    }
    
    global_manager = (fragment_manager_t*)malloc(sizeof(fragment_manager_t));
    if (global_manager == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    global_manager->tracker_capacity = 16;
    global_manager->tracker_count = 0;
    global_manager->trackers = (fragment_tracker_t**)malloc(global_manager->tracker_capacity * sizeof(fragment_tracker_t*));
    
    if (global_manager->trackers == NULL) {
        free(global_manager);
        global_manager = NULL;
        return STATUS_ERROR_MEMORY;
    }
    
    pthread_mutex_init(&global_manager->mutex, NULL);
    global_manager->running = true;
    
    // Create cleanup thread
    if (pthread_create(&global_manager->cleanup_thread, NULL, fragmentation_cleanup_thread, NULL) != 0) {
        pthread_mutex_destroy(&global_manager->mutex);
        free(global_manager->trackers);
        free(global_manager);
        global_manager = NULL;
        return STATUS_ERROR_GENERIC;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Shutdown fragmentation system
 */
status_t fragmentation_shutdown(void) {
    if (global_manager == NULL) {
        return STATUS_ERROR_NOT_FOUND;
    }
    
    global_manager->running = false;
    pthread_join(global_manager->cleanup_thread, NULL);
    
    pthread_mutex_lock(&global_manager->mutex);
    
    // Free all trackers
    for (size_t i = 0; i < global_manager->tracker_count; i++) {
        fragmentation_destroy_tracker(global_manager->trackers[i]);
    }
    
    free(global_manager->trackers);
    pthread_mutex_unlock(&global_manager->mutex);
    pthread_mutex_destroy(&global_manager->mutex);
    
    free(global_manager);
    global_manager = NULL;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Send a fragmented message
 */
status_t fragmentation_send_message(protocol_listener_t* listener, client_t* client,
                                  const uint8_t* data, size_t data_len,
                                  size_t max_fragment_size) {
    if (listener == NULL || client == NULL || data == NULL || data_len == 0 || max_fragment_size == 0) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Calculate number of fragments needed
    uint8_t total_fragments = (data_len + max_fragment_size - 1) / max_fragment_size;
    if (total_fragments > 255) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Generate fragment ID
    uint16_t fragment_id = (uint16_t)rand();
    
    // Send each fragment
    for (uint8_t i = 0; i < total_fragments; i++) {
        // Calculate fragment size
        size_t offset = i * max_fragment_size;
        size_t fragment_size = (i == total_fragments - 1) ? (data_len - offset) : max_fragment_size;
        
        // Create fragment header
        fragment_header_t header;
        fragmentation_create_header(fragment_id, i, total_fragments, 0, &header);
        
        // Create fragment message
        size_t message_size = sizeof(fragment_header_t) + fragment_size;
        uint8_t* message = (uint8_t*)malloc(message_size);
        if (message == NULL) {
            return STATUS_ERROR_MEMORY;
        }
        
        // Copy header and data
        memcpy(message, &header, sizeof(fragment_header_t));
        memcpy(message + sizeof(fragment_header_t), data + offset, fragment_size);
        
        // Send fragment
        protocol_message_t fragment_message;
        fragment_message.data = message;
        fragment_message.data_len = message_size;
        
        status_t status = STATUS_SUCCESS;
        
        // Check if send_message function is available
        if (listener->send_message != NULL) {
            status = listener->send_message(listener, client, &fragment_message);
        }
        
        free(message);
        
        if (status != STATUS_SUCCESS) {
            return status;
        }
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Process a received fragment
 */
status_t fragmentation_process_fragment(protocol_listener_t* listener, client_t* client,
                                      const uint8_t* data, size_t data_len,
                                      on_message_reassembled_callback callback) {
    if (listener == NULL || client == NULL || data == NULL || data_len < sizeof(fragment_header_t) || callback == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check if fragmentation system is initialized
    if (global_manager == NULL) {
        return STATUS_ERROR_NOT_RUNNING;
    }
    
    // Parse fragment header
    fragment_header_t header;
    status_t status = fragmentation_parse_header(data, data_len, &header);
    if (status != STATUS_SUCCESS) {
        return status;
    }
    
    // Find or create tracker
    fragment_tracker_t* tracker = NULL;
    
    pthread_mutex_lock(&global_manager->mutex);
    
    tracker = fragmentation_find_tracker(header.fragment_id, client);
    if (tracker == NULL) {
        tracker = fragmentation_create_tracker(header.fragment_id, header.total_fragments, client, listener);
        if (tracker == NULL) {
            pthread_mutex_unlock(&global_manager->mutex);
            return STATUS_ERROR_MEMORY;
        }
        
        // Add to manager
        if (global_manager->tracker_count >= global_manager->tracker_capacity) {
            size_t new_capacity = global_manager->tracker_capacity * 2;
            fragment_tracker_t** new_trackers = (fragment_tracker_t**)realloc(global_manager->trackers, new_capacity * sizeof(fragment_tracker_t*));
            
            if (new_trackers == NULL) {
                fragmentation_destroy_tracker(tracker);
                pthread_mutex_unlock(&global_manager->mutex);
                return STATUS_ERROR_MEMORY;
            }
            
            global_manager->trackers = new_trackers;
            global_manager->tracker_capacity = new_capacity;
        }
        
        global_manager->trackers[global_manager->tracker_count++] = tracker;
    }
    
    // Add fragment to tracker
    status = fragmentation_add_fragment(tracker, header.fragment_index, 
                                      data + sizeof(fragment_header_t), 
                                      data_len - sizeof(fragment_header_t));
    
    // Check if message is complete
    if (status == STATUS_SUCCESS && fragmentation_is_complete(tracker)) {
        // Reassemble message
        protocol_message_t message;
        status = fragmentation_reassemble(tracker, &message);
        
        if (status == STATUS_SUCCESS) {
            // Call callback
            callback(listener, client, &message);
            
            // Free message data
            free(message.data);
        }
        
        // Remove tracker from manager
        for (size_t i = 0; i < global_manager->tracker_count; i++) {
            if (global_manager->trackers[i] == tracker) {
                global_manager->trackers[i] = global_manager->trackers[--global_manager->tracker_count];
                break;
            }
        }
        
        // Destroy tracker
        fragmentation_destroy_tracker(tracker);
    }
    
    pthread_mutex_unlock(&global_manager->mutex);
    
    return status;
}

/**
 * @brief Create a fragment header
 */
status_t fragmentation_create_header(uint16_t fragment_id, uint8_t fragment_index,
                                   uint8_t total_fragments, uint8_t flags,
                                   fragment_header_t* header) {
    if (header == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    header->fragment_id = fragment_id;
    header->fragment_index = fragment_index;
    header->total_fragments = total_fragments;
    header->flags = flags;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Parse a fragment header
 */
status_t fragmentation_parse_header(const uint8_t* data, size_t data_len,
                                  fragment_header_t* header) {
    if (data == NULL || data_len < sizeof(fragment_header_t) || header == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    memcpy(header, data, sizeof(fragment_header_t));
    
    return STATUS_SUCCESS;
}

/**
 * @brief Find a fragment tracker
 */
static fragment_tracker_t* fragmentation_find_tracker(uint16_t fragment_id, client_t* client) {
    if (global_manager == NULL) {
        return NULL;
    }
    
    for (size_t i = 0; i < global_manager->tracker_count; i++) {
        fragment_tracker_t* tracker = global_manager->trackers[i];
        
        if (tracker->fragment_id == fragment_id && tracker->client == client) {
            return tracker;
        }
    }
    
    return NULL;
}

/**
 * @brief Create a fragment tracker
 */
static fragment_tracker_t* fragmentation_create_tracker(uint16_t fragment_id, uint8_t total_fragments, client_t* client, protocol_listener_t* listener) {
    fragment_tracker_t* tracker = (fragment_tracker_t*)malloc(sizeof(fragment_tracker_t));
    if (tracker == NULL) {
        return NULL;
    }
    
    // Initialize tracker
    tracker->fragment_id = fragment_id;
    tracker->total_fragments = total_fragments;
    tracker->fragments_received = 0;
    tracker->first_fragment_time = time(NULL);
    tracker->client = client;
    tracker->listener = listener;
    
    // Allocate arrays
    tracker->fragment_data = (uint8_t**)malloc(total_fragments * sizeof(uint8_t*));
    if (tracker->fragment_data == NULL) {
        free(tracker);
        return NULL;
    }
    
    tracker->fragment_sizes = (size_t*)malloc(total_fragments * sizeof(size_t));
    if (tracker->fragment_sizes == NULL) {
        free(tracker->fragment_data);
        free(tracker);
        return NULL;
    }
    
    tracker->fragment_received = (bool*)malloc(total_fragments * sizeof(bool));
    if (tracker->fragment_received == NULL) {
        free(tracker->fragment_sizes);
        free(tracker->fragment_data);
        free(tracker);
        return NULL;
    }
    
    // Initialize arrays
    memset(tracker->fragment_data, 0, total_fragments * sizeof(uint8_t*));
    memset(tracker->fragment_sizes, 0, total_fragments * sizeof(size_t));
    memset(tracker->fragment_received, 0, total_fragments * sizeof(bool));
    
    return tracker;
}

/**
 * @brief Add a fragment to a tracker
 */
static status_t fragmentation_add_fragment(fragment_tracker_t* tracker, uint8_t fragment_index, const uint8_t* data, size_t data_len) {
    if (tracker == NULL || data == NULL || fragment_index >= tracker->total_fragments) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check if fragment already received
    if (tracker->fragment_received[fragment_index]) {
        return STATUS_SUCCESS;
    }
    
    // Allocate memory for fragment data
    tracker->fragment_data[fragment_index] = (uint8_t*)malloc(data_len);
    if (tracker->fragment_data[fragment_index] == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Copy fragment data
    memcpy(tracker->fragment_data[fragment_index], data, data_len);
    tracker->fragment_sizes[fragment_index] = data_len;
    tracker->fragment_received[fragment_index] = true;
    tracker->fragments_received++;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Check if all fragments have been received
 */
static bool fragmentation_is_complete(fragment_tracker_t* tracker) {
    if (tracker == NULL) {
        return false;
    }
    
    return tracker->fragments_received == tracker->total_fragments;
}

/**
 * @brief Reassemble fragments into a complete message
 */
static status_t fragmentation_reassemble(fragment_tracker_t* tracker, protocol_message_t* message) {
    if (tracker == NULL || message == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Calculate total message size
    size_t total_size = 0;
    for (uint8_t i = 0; i < tracker->total_fragments; i++) {
        total_size += tracker->fragment_sizes[i];
    }
    
    // Allocate memory for message data
    message->data = (uint8_t*)malloc(total_size);
    if (message->data == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Copy fragments into message
    size_t offset = 0;
    for (uint8_t i = 0; i < tracker->total_fragments; i++) {
        memcpy(message->data + offset, tracker->fragment_data[i], tracker->fragment_sizes[i]);
        offset += tracker->fragment_sizes[i];
    }
    
    message->data_len = total_size;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Destroy a fragment tracker
 */
static status_t fragmentation_destroy_tracker(fragment_tracker_t* tracker) {
    if (tracker == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Free fragment data
    for (uint8_t i = 0; i < tracker->total_fragments; i++) {
        if (tracker->fragment_data[i] != NULL) {
            free(tracker->fragment_data[i]);
        }
    }
    
    free(tracker->fragment_data);
    free(tracker->fragment_sizes);
    free(tracker->fragment_received);
    free(tracker);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Cleanup thread for removing timed out trackers
 */
static void* fragmentation_cleanup_thread(void* arg) {
    (void)arg; // Unused parameter
    
    while (global_manager != NULL && global_manager->running) {
        pthread_mutex_lock(&global_manager->mutex);
        
        time_t now = time(NULL);
        
        // Check for timed out trackers
        for (size_t i = 0; i < global_manager->tracker_count; /* no increment */) {
            fragment_tracker_t* tracker = global_manager->trackers[i];
            
            if (now - tracker->first_fragment_time > FRAGMENT_TIMEOUT) {
                // Remove tracker from manager
                global_manager->trackers[i] = global_manager->trackers[--global_manager->tracker_count];
                
                // Destroy tracker
                fragmentation_destroy_tracker(tracker);
            } else {
                i++;
            }
        }
        
        pthread_mutex_unlock(&global_manager->mutex);
        
        // Sleep for 1 second
        sleep(1);
    }
    
    return NULL;
}
