/**
 * @file test_protocol_switch_minimal.c
 * @brief Minimal test for protocol switching functionality
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

// Include the minimal protocol switch implementation
#include "protocol_switch_minimal.c"

int main(void) {
    printf("Testing protocol switch message creation...\n");
    
    // Create protocol switch message
    protocol_switch_message_t message;
    status_t status = protocol_switch_create_message(PROTOCOL_TYPE_TCP, 8080, "example.com", 5000, 
                                                  PROTOCOL_SWITCH_FLAG_IMMEDIATE, &message);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to create protocol switch message: %d\n", status);
        return 1;
    }
    
    // Check message fields
    if (message.magic != PROTOCOL_SWITCH_MAGIC) {
        printf("Protocol switch message magic is incorrect: 0x%08x\n", message.magic);
        return 1;
    }
    
    if (message.protocol != PROTOCOL_TYPE_TCP) {
        printf("Protocol switch message protocol is incorrect: %d\n", message.protocol);
        return 1;
    }
    
    if (message.port != 8080) {
        printf("Protocol switch message port is incorrect: %d\n", message.port);
        return 1;
    }
    
    if (message.timeout_ms != 5000) {
        printf("Protocol switch message timeout is incorrect: %d\n", message.timeout_ms);
        return 1;
    }
    
    if (message.flags != PROTOCOL_SWITCH_FLAG_IMMEDIATE) {
        printf("Protocol switch message flags are incorrect: 0x%02x\n", message.flags);
        return 1;
    }
    
    if (strcmp(message.domain, "example.com") != 0) {
        printf("Protocol switch message domain is incorrect: %s\n", message.domain);
        return 1;
    }
    
    printf("Protocol switch message creation test passed\n");
    
    printf("Testing protocol switch message detection...\n");
    
    // Check if message is detected
    bool is_message = protocol_switch_is_message((const uint8_t*)&message, sizeof(message));
    if (!is_message) {
        printf("Protocol switch message not detected\n");
        return 1;
    }
    
    // Modify magic number
    message.magic = 0x12345678;
    
    // Check if message is not detected
    is_message = protocol_switch_is_message((const uint8_t*)&message, sizeof(message));
    if (is_message) {
        printf("Invalid protocol switch message detected\n");
        return 1;
    }
    
    printf("Protocol switch message detection test passed\n");
    printf("All tests passed\n");
    
    return 0;
}
