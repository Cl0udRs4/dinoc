/**
 * @file base64.c
 * @brief Base64 encoding and decoding implementation
 */

#include "base64.h"
#include <string.h>

// Base64 encoding table
static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Base64 decoding table
static const int base64_decode_table[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

/**
 * @brief Encode binary data to base64 string
 */
size_t base64_encode(const uint8_t* data, size_t data_len, char* output, size_t output_len) {
    if (data == NULL || output == NULL || data_len == 0) {
        return 0;
    }
    
    // Calculate required output size
    size_t required_len = ((data_len + 2) / 3) * 4;
    
    // Check if output buffer is large enough
    if (output_len < required_len + 1) {  // +1 for null terminator
        return 0;
    }
    
    size_t i, j;
    uint32_t n;
    
    // Process input data in 3-byte chunks
    for (i = 0, j = 0; i < data_len; i += 3) {
        // Combine 3 bytes into a 24-bit number
        n = data[i] << 16;
        
        if (i + 1 < data_len) {
            n += data[i + 1] << 8;
        }
        
        if (i + 2 < data_len) {
            n += data[i + 2];
        }
        
        // Extract 4 6-bit values and convert to base64 characters
        output[j++] = base64_table[(n >> 18) & 0x3F];
        output[j++] = base64_table[(n >> 12) & 0x3F];
        
        if (i + 1 < data_len) {
            output[j++] = base64_table[(n >> 6) & 0x3F];
        } else {
            output[j++] = '=';  // Padding
        }
        
        if (i + 2 < data_len) {
            output[j++] = base64_table[n & 0x3F];
        } else {
            output[j++] = '=';  // Padding
        }
    }
    
    // Add null terminator
    output[j] = '\0';
    
    return j;
}

/**
 * @brief Decode base64 string to binary data
 */
size_t base64_decode(const char* input, size_t input_len, uint8_t* output, size_t output_len) {
    if (input == NULL || output == NULL || input_len == 0) {
        return 0;
    }
    
    // Skip trailing padding characters
    while (input_len > 0 && input[input_len - 1] == '=') {
        input_len--;
    }
    
    // Calculate maximum output size
    size_t max_output_len = (input_len * 3) / 4;
    
    // Check if output buffer is large enough
    if (output_len < max_output_len) {
        return 0;
    }
    
    size_t i, j;
    uint32_t n;
    int padding = 0;
    
    // Process input data in 4-character chunks
    for (i = 0, j = 0; i < input_len; i += 4) {
        // Combine 4 6-bit values into a 24-bit number
        n = 0;
        
        // Process up to 4 characters
        for (int k = 0; k < 4 && i + k < input_len; k++) {
            int val = base64_decode_table[(unsigned char)input[i + k]];
            
            if (val == -1) {
                // Invalid character
                return 0;
            }
            
            n = (n << 6) | val;
            
            if (i + k + 1 >= input_len) {
                padding++;
            }
        }
        
        // Extract 3 bytes from the 24-bit number
        if (j < output_len) {
            output[j++] = (n >> 16) & 0xFF;
        }
        
        if (padding <= 2 && j < output_len) {
            output[j++] = (n >> 8) & 0xFF;
        }
        
        if (padding <= 1 && j < output_len) {
            output[j++] = n & 0xFF;
        }
    }
    
    return j;
}
