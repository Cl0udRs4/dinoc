/**
 * @file uuid.c
 * @brief Implementation of UUID functions
 */

#include "../include/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

// Initialize random seed
static int uuid_initialized = 0;

// Initialize UUID generator
static void uuid_init(void) {
    if (uuid_initialized) {
        return;
    }
    
    // Try to use /dev/urandom for better randomness
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        unsigned int seed;
        if (read(fd, &seed, sizeof(seed)) == sizeof(seed)) {
            srand(seed);
        }
        close(fd);
    } else {
        // Fallback to time-based seed
        srand((unsigned int)time(NULL));
    }
    
    uuid_initialized = 1;
}

// Generate random bytes
static void generate_random_bytes(uint8_t* buffer, size_t size) {
    uuid_init();
    
    for (size_t i = 0; i < size; i++) {
        buffer[i] = (uint8_t)(rand() & 0xFF);
    }
}

status_t uuid_generate(uuid_t* uuid) {
    if (uuid == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Generate random bytes
    generate_random_bytes(uuid->bytes, sizeof(uuid->bytes));
    
    // Set version (4 = random UUID)
    uuid->bytes[6] = (uuid->bytes[6] & 0x0F) | 0x40;
    
    // Set variant (RFC 4122)
    uuid->bytes[8] = (uuid->bytes[8] & 0x3F) | 0x80;
    
    return STATUS_SUCCESS;
}

status_t uuid_to_string(const uuid_t* uuid, char* str) {
    if (uuid == NULL || str == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    sprintf(str, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            uuid->bytes[0], uuid->bytes[1], uuid->bytes[2], uuid->bytes[3],
            uuid->bytes[4], uuid->bytes[5],
            uuid->bytes[6], uuid->bytes[7],
            uuid->bytes[8], uuid->bytes[9],
            uuid->bytes[10], uuid->bytes[11], uuid->bytes[12], uuid->bytes[13], uuid->bytes[14], uuid->bytes[15]);
    
    return STATUS_SUCCESS;
}

// Helper function to convert hex character to value
static int hex_to_byte(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

status_t uuid_from_string(const char* str, uuid_t* uuid) {
    if (str == NULL || uuid == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check string length
    size_t len = strlen(str);
    if (len != 36) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check format (8-4-4-4-12 with hyphens)
    if (str[8] != '-' || str[13] != '-' || str[18] != '-' || str[23] != '-') {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Parse hex digits
    int pos = 0;
    for (int i = 0; i < 36; i++) {
        if (str[i] == '-') continue;
        
        int hi = hex_to_byte(str[i++]);
        int lo = hex_to_byte(str[i]);
        
        if (hi < 0 || lo < 0) {
            return STATUS_ERROR_INVALID_PARAM;
        }
        
        uuid->bytes[pos++] = (uint8_t)((hi << 4) | lo);
    }
    
    return STATUS_SUCCESS;
}

int uuid_compare(const uuid_t* uuid1, const uuid_t* uuid2) {
    if (uuid1 == NULL || uuid2 == NULL) {
        return -1;
    }
    
    return memcmp(uuid1->bytes, uuid2->bytes, sizeof(uuid1->bytes));
}
