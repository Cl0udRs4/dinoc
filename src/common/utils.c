/**
 * @file utils.c
 * @brief Implementation of common utility functions
 */

#include "../include/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

/**
 * @brief Generate random data
 * 
 * @param buffer Buffer to fill with random data
 * @param size Size of buffer in bytes
 * @return status_t Status code
 */
status_t generate_random_data(uint8_t* buffer, size_t size) {
    if (buffer == NULL || size == 0) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Try to use /dev/urandom for better randomness
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        ssize_t bytes_read = read(fd, buffer, size);
        close(fd);
        
        if (bytes_read == (ssize_t)size) {
            return STATUS_SUCCESS;
        }
    }
    
    // Fallback to rand() if /dev/urandom failed
    static int initialized = 0;
    if (!initialized) {
        srand((unsigned int)time(NULL));
        initialized = 1;
    }
    
    for (size_t i = 0; i < size; i++) {
        buffer[i] = (uint8_t)(rand() & 0xFF);
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Convert protocol type to string
 * 
 * @param type Protocol type
 * @return const char* Protocol name
 */
const char* protocol_type_to_string(protocol_type_t type) {
    switch (type) {
        case PROTOCOL_NONE:
            return "NONE";
        case PROTOCOL_TCP:
            return "TCP";
        case PROTOCOL_UDP:
            return "UDP";
        case PROTOCOL_WS:
            return "WebSocket";
        case PROTOCOL_ICMP:
            return "ICMP";
        case PROTOCOL_DNS:
            return "DNS";
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Convert encryption type to string
 * 
 * @param type Encryption type
 * @return const char* Encryption name
 */
const char* encryption_type_to_string(encryption_type_t type) {
    switch (type) {
        case ENCRYPTION_NONE:
            return "NONE";
        case ENCRYPTION_AES:
            return "AES";
        case ENCRYPTION_CHACHA20:
            return "ChaCha20";
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Convert listener state to string
 * 
 * @param state Listener state
 * @return const char* State name
 */
const char* listener_state_to_string(listener_state_t state) {
    switch (state) {
        case LISTENER_STATE_CREATED:
            return "CREATED";
        case LISTENER_STATE_STARTING:
            return "STARTING";
        case LISTENER_STATE_RUNNING:
            return "RUNNING";
        case LISTENER_STATE_STOPPING:
            return "STOPPING";
        case LISTENER_STATE_STOPPED:
            return "STOPPED";
        case LISTENER_STATE_ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Convert client state to string
 * 
 * @param state Client state
 * @return const char* State name
 */
const char* client_state_to_string(client_state_t state) {
    switch (state) {
        case CLIENT_STATE_UNKNOWN:
            return "UNKNOWN";
        case CLIENT_STATE_REGISTERED:
            return "REGISTERED";
        case CLIENT_STATE_ACTIVE:
            return "ACTIVE";
        case CLIENT_STATE_IDLE:
            return "IDLE";
        case CLIENT_STATE_DISCONNECTED:
            return "DISCONNECTED";
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Convert task state to string
 * 
 * @param state Task state
 * @return const char* State name
 */
const char* task_state_to_string(task_state_t state) {
    switch (state) {
        case TASK_STATE_CREATED:
            return "CREATED";
        case TASK_STATE_PENDING:
            return "PENDING";
        case TASK_STATE_SENT:
            return "SENT";
        case TASK_STATE_RUNNING:
            return "RUNNING";
        case TASK_STATE_COMPLETED:
            return "COMPLETED";
        case TASK_STATE_FAILED:
            return "FAILED";
        case TASK_STATE_TIMEOUT:
            return "TIMEOUT";
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Get current timestamp in seconds
 * 
 * @return uint64_t Current timestamp
 */
uint64_t get_timestamp(void) {
    return (uint64_t)time(NULL);
}

/**
 * @brief Get current timestamp in milliseconds
 * 
 * @return uint64_t Current timestamp
 */
uint64_t get_timestamp_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

/**
 * @brief Generate a random number within a range
 * 
 * @param min Minimum value (inclusive)
 * @param max Maximum value (inclusive)
 * @return int Random number
 */
int random_range(int min, int max) {
    static int initialized = 0;
    if (!initialized) {
        srand((unsigned int)time(NULL));
        initialized = 1;
    }
    
    return min + rand() % (max - min + 1);
}

/**
 * @brief Safe string copy with size checking
 * 
 * @param dest Destination buffer
 * @param src Source string
 * @param size Maximum size of destination buffer
 * @return size_t Number of bytes copied (excluding null terminator)
 */
size_t safe_strcpy(char* dest, const char* src, size_t size) {
    if (dest == NULL || src == NULL || size == 0) {
        return 0;
    }
    
    size_t i;
    for (i = 0; i < size - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    
    dest[i] = '\0';
    return i;
}

/**
 * @brief Hex dump of binary data
 * 
 * @param data Data to dump
 * @param size Data size in bytes
 * @param output Output buffer
 * @param output_size Output buffer size
 * @return size_t Number of bytes written to output
 */
size_t hex_dump(const uint8_t* data, size_t size, char* output, size_t output_size) {
    if (data == NULL || output == NULL || output_size == 0) {
        return 0;
    }
    
    size_t offset = 0;
    size_t remaining = output_size - 1; // Reserve space for null terminator
    
    for (size_t i = 0; i < size && remaining > 2; i++) {
        int written = snprintf(output + offset, remaining, "%02x", data[i]);
        if (written < 0 || (size_t)written >= remaining) {
            break;
        }
        
        offset += written;
        remaining -= written;
    }
    
    output[offset] = '\0';
    return offset;
}
