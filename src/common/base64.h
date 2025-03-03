/**
 * @file base64.h
 * @brief Base64 encoding and decoding functions
 */

#ifndef DINOC_BASE64_H
#define DINOC_BASE64_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Encode binary data to base64 string
 * 
 * @param data Input binary data
 * @param data_len Length of input data
 * @param output Output buffer for base64 string
 * @param output_len Size of output buffer
 * @return size_t Length of encoded string or 0 on error
 */
size_t base64_encode(const uint8_t* data, size_t data_len, char* output, size_t output_len);

/**
 * @brief Decode base64 string to binary data
 * 
 * @param input Input base64 string
 * @param input_len Length of input string
 * @param output Output buffer for binary data
 * @param output_len Size of output buffer
 * @return size_t Length of decoded data or 0 on error
 */
size_t base64_decode(const char* input, size_t input_len, uint8_t* output, size_t output_len);

#endif /* DINOC_BASE64_H */
