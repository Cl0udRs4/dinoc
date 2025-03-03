/**
 * @file test_protocol_header.c
 * @brief Test program for protocol header detection
 */

#include "../include/common.h"
#include "../include/encryption.h"
#include "../include/protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations
status_t protocol_header_create(encryption_type_t type, uint8_t version, uint16_t flags,
                               uint32_t payload_len, uint8_t* header);
status_t protocol_header_parse(const uint8_t* header, encryption_type_t* type,
                              uint8_t* version, uint16_t* flags, uint32_t* payload_len);
encryption_type_t protocol_detect_encryption(const uint8_t* data, size_t data_len);

/**
 * @brief Test protocol header creation and parsing
 */
void test_protocol_header(void) {
    printf("Testing protocol header creation and parsing...\n");
    
    // Create test headers
    uint8_t aes_header[8];
    uint8_t chacha20_header[8];
    
    // Create AES header
    status_t status = protocol_header_create(ENCRYPTION_AES, 0x01, 0x1234, 0x56789ABC, aes_header);
    if (status != STATUS_SUCCESS) {
        printf("Failed to create AES header: %d\n", status);
        return;
    }
    
    // Create ChaCha20 header
    status = protocol_header_create(ENCRYPTION_CHACHA20, 0x02, 0x5678, 0xDEF01234, chacha20_header);
    if (status != STATUS_SUCCESS) {
        printf("Failed to create ChaCha20 header: %d\n", status);
        return;
    }
    
    // Print headers
    printf("AES header: ");
    for (int i = 0; i < 8; i++) {
        printf("%02x ", aes_header[i]);
    }
    printf("\n");
    
    printf("ChaCha20 header: ");
    for (int i = 0; i < 8; i++) {
        printf("%02x ", chacha20_header[i]);
    }
    printf("\n");
    
    // Parse AES header
    encryption_type_t type;
    uint8_t version;
    uint16_t flags;
    uint32_t payload_len;
    
    status = protocol_header_parse(aes_header, &type, &version, &flags, &payload_len);
    if (status != STATUS_SUCCESS) {
        printf("Failed to parse AES header: %d\n", status);
        return;
    }
    
    printf("Parsed AES header: type=%d, version=0x%02x, flags=0x%04x, payload_len=0x%08x\n",
           type, version, flags, payload_len);
    
    // Parse ChaCha20 header
    status = protocol_header_parse(chacha20_header, &type, &version, &flags, &payload_len);
    if (status != STATUS_SUCCESS) {
        printf("Failed to parse ChaCha20 header: %d\n", status);
        return;
    }
    
    printf("Parsed ChaCha20 header: type=%d, version=0x%02x, flags=0x%04x, payload_len=0x%08x\n",
           type, version, flags, payload_len);
    
    // Test encryption detection
    encryption_type_t detected_type;
    
    detected_type = protocol_detect_encryption(aes_header, sizeof(aes_header));
    printf("Detected encryption type for AES header: %d\n", detected_type);
    
    detected_type = protocol_detect_encryption(chacha20_header, sizeof(chacha20_header));
    printf("Detected encryption type for ChaCha20 header: %d\n", detected_type);
}

/**
 * @brief Main function
 */
int main(int argc, char** argv) {
    // Initialize logging
    log_init(NULL, LOG_LEVEL_DEBUG);
    
    // Run tests
    test_protocol_header();
    
    // Clean up
    log_shutdown();
    
    return 0;
}
