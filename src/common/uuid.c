/**
 * @file uuid.c
 * @brief UUID generation implementation for C2 server
 */

#include "uuid.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

// UUID state
static bool uuid_initialized = false;
static pthread_mutex_t uuid_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint16_t clock_seq = 0;
static uint64_t node_id = 0;

/**
 * @brief Initialize UUID generator
 */
status_t uuid_init(void) {
    pthread_mutex_lock(&uuid_mutex);
    
    if (uuid_initialized) {
        pthread_mutex_unlock(&uuid_mutex);
        return STATUS_ERROR_ALREADY_RUNNING;
    }
    
    // Initialize clock sequence with random value
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd == -1) {
        // Fallback to time-based seed if /dev/urandom is not available
        srand(time(NULL));
        clock_seq = (uint16_t)rand();
    } else {
        if (read(fd, &clock_seq, sizeof(clock_seq)) != sizeof(clock_seq)) {
            // Fallback to time-based seed if read fails
            srand(time(NULL));
            clock_seq = (uint16_t)rand();
        }
        close(fd);
    }
    
    // Set node ID (ideally should be MAC address, but we'll use a random value)
    fd = open("/dev/urandom", O_RDONLY);
    if (fd == -1) {
        // Fallback to time-based seed if /dev/urandom is not available
        srand(time(NULL) ^ getpid());
        node_id = ((uint64_t)rand() << 32) | (uint64_t)rand();
    } else {
        if (read(fd, &node_id, sizeof(node_id)) != sizeof(node_id)) {
            // Fallback to time-based seed if read fails
            srand(time(NULL) ^ getpid());
            node_id = ((uint64_t)rand() << 32) | (uint64_t)rand();
        }
        close(fd);
    }
    
    // Set multicast bit (bit 0) to indicate this is not a real MAC address
    node_id |= 0x0000010000000000ULL;
    
    uuid_initialized = true;
    
    LOG_INFO("UUID generator initialized");
    
    pthread_mutex_unlock(&uuid_mutex);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Shutdown UUID generator
 */
status_t uuid_shutdown(void) {
    pthread_mutex_lock(&uuid_mutex);
    
    if (!uuid_initialized) {
        pthread_mutex_unlock(&uuid_mutex);
        return STATUS_ERROR_NOT_RUNNING;
    }
    
    uuid_initialized = false;
    
    LOG_INFO("UUID generator shut down");
    
    pthread_mutex_unlock(&uuid_mutex);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Generate a new UUID
 */
status_t uuid_generate(uuid_t* uuid) {
    if (uuid == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&uuid_mutex);
    
    if (!uuid_initialized) {
        pthread_mutex_unlock(&uuid_mutex);
        return STATUS_ERROR_NOT_RUNNING;
    }
    
    // Get current time
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    
    // Convert to 100-nanosecond intervals since UUID epoch (October 15, 1582)
    // This is a simplified version, in a real implementation you would use a proper time conversion
    uint64_t timestamp = ((uint64_t)ts.tv_sec * 10000000) + ((uint64_t)ts.tv_nsec / 100);
    timestamp += 0x01B21DD213814000ULL; // Offset between Unix epoch and UUID epoch
    
    // Increment clock sequence if timestamp went backwards
    static uint64_t last_timestamp = 0;
    if (timestamp <= last_timestamp) {
        clock_seq = (clock_seq + 1) & 0x3FFF;
    }
    last_timestamp = timestamp;
    
    // Set UUID fields
    uuid->time_low = (uint32_t)(timestamp & 0xFFFFFFFF);
    uuid->time_mid = (uint16_t)((timestamp >> 32) & 0xFFFF);
    uuid->time_hi_and_version = (uint16_t)((timestamp >> 48) & 0x0FFF);
    uuid->time_hi_and_version |= (1 << 12); // Version 1 (time-based)
    
    uuid->clock_seq = clock_seq & 0x3FFF;
    uuid->clock_seq |= 0x8000; // Variant 1 (RFC 4122)
    
    uuid->node = node_id;
    
    pthread_mutex_unlock(&uuid_mutex);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Convert UUID to string
 */
status_t uuid_to_string(const uuid_t* uuid, char* str, size_t size) {
    if (uuid == NULL || str == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    if (size < 37) {
        return STATUS_ERROR_BUFFER_TOO_SMALL;
    }
    
    snprintf(str, size, "%08x-%04x-%04x-%04x-%012llx",
             uuid->time_low,
             uuid->time_mid,
             uuid->time_hi_and_version,
             uuid->clock_seq,
             (unsigned long long)uuid->node);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Parse UUID from string
 */
status_t uuid_from_string(const char* str, uuid_t* uuid) {
    if (str == NULL || uuid == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check string length
    if (strlen(str) != 36) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check for hyphens at the right positions
    if (str[8] != '-' || str[13] != '-' || str[18] != '-' || str[23] != '-') {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Parse UUID fields
    unsigned int time_low;
    unsigned int time_mid;
    unsigned int time_hi_and_version;
    unsigned int clock_seq;
    unsigned long long node;
    
    if (sscanf(str, "%08x-%04x-%04x-%04x-%012llx",
               &time_low,
               &time_mid,
               &time_hi_and_version,
               &clock_seq,
               &node) != 5) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    uuid->time_low = time_low;
    uuid->time_mid = time_mid;
    uuid->time_hi_and_version = time_hi_and_version;
    uuid->clock_seq = clock_seq;
    uuid->node = node;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Compare two UUIDs
 */
int uuid_compare(const uuid_t* uuid1, const uuid_t* uuid2) {
    if (uuid1 == NULL && uuid2 == NULL) {
        return 0;
    }
    
    if (uuid1 == NULL) {
        return -1;
    }
    
    if (uuid2 == NULL) {
        return 1;
    }
    
    // Compare fields in order of significance
    if (uuid1->time_low != uuid2->time_low) {
        return (uuid1->time_low < uuid2->time_low) ? -1 : 1;
    }
    
    if (uuid1->time_mid != uuid2->time_mid) {
        return (uuid1->time_mid < uuid2->time_mid) ? -1 : 1;
    }
    
    if (uuid1->time_hi_and_version != uuid2->time_hi_and_version) {
        return (uuid1->time_hi_and_version < uuid2->time_hi_and_version) ? -1 : 1;
    }
    
    if (uuid1->clock_seq != uuid2->clock_seq) {
        return (uuid1->clock_seq < uuid2->clock_seq) ? -1 : 1;
    }
    
    if (uuid1->node != uuid2->node) {
        return (uuid1->node < uuid2->node) ? -1 : 1;
    }
    
    return 0;
}

/**
 * @brief Check if UUID is null
 */
bool uuid_is_null(const uuid_t* uuid) {
    if (uuid == NULL) {
        return true;
    }
    
    return (uuid->time_low == 0 &&
            uuid->time_mid == 0 &&
            uuid->time_hi_and_version == 0 &&
            uuid->clock_seq == 0 &&
            uuid->node == 0);
}

/**
 * @brief Set UUID to null
 */
status_t uuid_clear(uuid_t* uuid) {
    if (uuid == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    uuid->time_low = 0;
    uuid->time_mid = 0;
    uuid->time_hi_and_version = 0;
    uuid->clock_seq = 0;
    uuid->node = 0;
    
    return STATUS_SUCCESS;
}
