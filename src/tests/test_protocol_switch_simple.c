/**
 * @file test_protocol_switch_simple.c
 * @brief Simple test for protocol switching functionality
 */

#include "../include/protocol.h"
#include "../include/client.h"
#include "../protocols/protocol_switch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    
    printf("Protocol switch message creation test passed\n");
    
    printf("Testing protocol switch message detection...\n");
    
    // Check if message is detected
    bool is_message = protocol_switch_is_message((const uint8_t*)&message, sizeof(message));
    if (!is_message) {
        printf("Protocol switch message not detected\n");
        return 1;
    }
    
    printf("Protocol switch message detection test passed\n");
    printf("All tests passed\n");
    
    return 0;
}
