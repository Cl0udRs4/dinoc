/**
 * @file test_encryption_detection.c
 * @brief Simple test for encryption detection
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Magic bytes for encryption type
#define AES_MAGIC_BYTE      0xA3
#define CHACHA20_MAGIC_BYTE 0xC2

// Encryption types
typedef enum {
    ENCRYPTION_NONE = 0,
    ENCRYPTION_AES = 1,
    ENCRYPTION_CHACHA20 = 2,
    ENCRYPTION_UNKNOWN = 255
} encryption_type_t;

// Protocol header structure
typedef struct {
    uint8_t magic;          // Magic byte for encryption type
    uint8_t version;        // Protocol version
    uint16_t flags;         // Protocol flags
    uint32_t payload_len;   // Payload length
} __attribute__((packed)) protocol_header_t;

/**
 * @brief Create a protocol header
 */
void create_header(encryption_type_t type, uint8_t version, uint16_t flags,
                  uint32_t payload_len, uint8_t* header) {
    protocol_header_t* hdr = (protocol_header_t*)header;
    
    // Set magic byte based on encryption type
    switch (type) {
        case ENCRYPTION_AES:
            hdr->magic = AES_MAGIC_BYTE;
            break;
            
        case ENCRYPTION_CHACHA20:
            hdr->magic = CHACHA20_MAGIC_BYTE;
            break;
            
        default:
            hdr->magic = 0xFF;
            break;
    }
    
    // Set other fields
    hdr->version = version;
    hdr->flags = flags;
    hdr->payload_len = payload_len;
}

/**
 * @brief Detect encryption type from protocol header
 */
encryption_type_t detect_encryption(const uint8_t* data, size_t data_len) {
    if (data == NULL || data_len < sizeof(protocol_header_t)) {
        return ENCRYPTION_UNKNOWN;
    }
    
    // Cast data to protocol header
    const protocol_header_t* header = (const protocol_header_t*)data;
    
    // Check magic byte
    switch (header->magic) {
        case AES_MAGIC_BYTE:
            return ENCRYPTION_AES;
            
        case CHACHA20_MAGIC_BYTE:
            return ENCRYPTION_CHACHA20;
            
        default:
            return ENCRYPTION_UNKNOWN;
    }
}

/**
 * @brief Main function
 */
int main(void) {
    printf("Testing encryption detection...\n");
    
    // Create test headers
    uint8_t aes_header[sizeof(protocol_header_t)];
    uint8_t chacha20_header[sizeof(protocol_header_t)];
    
    // Create AES header
    create_header(ENCRYPTION_AES, 0x01, 0x1234, 0x56789ABC, aes_header);
    
    // Create ChaCha20 header
    create_header(ENCRYPTION_CHACHA20, 0x02, 0x5678, 0xDEF01234, chacha20_header);
    
    // Print headers
    printf("AES header: ");
    for (size_t i = 0; i < sizeof(protocol_header_t); i++) {
        printf("%02x ", aes_header[i]);
    }
    printf("\n");
    
    printf("ChaCha20 header: ");
    for (size_t i = 0; i < sizeof(protocol_header_t); i++) {
        printf("%02x ", chacha20_header[i]);
    }
    printf("\n");
    
    // Test encryption detection
    encryption_type_t detected_type;
    
    detected_type = detect_encryption(aes_header, sizeof(aes_header));
    printf("Detected encryption type for AES header: %d (%s)\n", 
           detected_type, detected_type == ENCRYPTION_AES ? "AES" : "Unknown");
    
    detected_type = detect_encryption(chacha20_header, sizeof(chacha20_header));
    printf("Detected encryption type for ChaCha20 header: %d (%s)\n", 
           detected_type, detected_type == ENCRYPTION_CHACHA20 ? "ChaCha20" : "Unknown");
    
    return 0;
}
