/**
 * @file utils.h
 * @brief Utility functions for C2 server
 */

#ifndef DINOC_UTILS_H
#define DINOC_UTILS_H

#include "../include/common.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Calculate CRC32 checksum
 * 
 * @param data Data buffer
 * @param len Data length
 * @return uint32_t CRC32 checksum
 */
uint32_t utils_crc32(const uint8_t* data, size_t len);

/**
 * @brief Calculate Fletcher-16 checksum
 * 
 * @param data Data buffer
 * @param len Data length
 * @return uint16_t Fletcher-16 checksum
 */
uint16_t utils_fletcher16(const uint8_t* data, size_t len);

/**
 * @brief Calculate entropy of data
 * 
 * @param data Data buffer
 * @param len Data length
 * @return double Entropy value (0.0 - 8.0)
 */
double utils_entropy(const uint8_t* data, size_t len);

/**
 * @brief Compress data using zlib
 * 
 * @param data Data buffer
 * @param data_len Data length
 * @param compressed_data Compressed data buffer (allocated by function)
 * @param compressed_len Compressed data length
 * @return status_t Status code
 */
status_t utils_compress(const uint8_t* data, size_t data_len, uint8_t** compressed_data, size_t* compressed_len);

/**
 * @brief Decompress data using zlib
 * 
 * @param compressed_data Compressed data buffer
 * @param compressed_len Compressed data length
 * @param data Data buffer (allocated by function)
 * @param data_len Data length
 * @return status_t Status code
 */
status_t utils_decompress(const uint8_t* compressed_data, size_t compressed_len, uint8_t** data, size_t* data_len);

/**
 * @brief Generate random bytes
 * 
 * @param buffer Buffer to store random bytes
 * @param len Buffer length
 * @return status_t Status code
 */
status_t utils_random_bytes(uint8_t* buffer, size_t len);

/**
 * @brief Get current timestamp in milliseconds
 * 
 * @return uint64_t Current timestamp
 */
uint64_t utils_get_timestamp(void);

/**
 * @brief Get hostname
 * 
 * @param hostname Buffer to store hostname
 * @param size Buffer size
 * @return status_t Status code
 */
status_t utils_get_hostname(char* hostname, size_t size);

/**
 * @brief Get IP address
 * 
 * @param ip_address Buffer to store IP address
 * @param size Buffer size
 * @return status_t Status code
 */
status_t utils_get_ip_address(char* ip_address, size_t size);

/**
 * @brief Get OS information
 * 
 * @param os_info Buffer to store OS information
 * @param size Buffer size
 * @return status_t Status code
 */
status_t utils_get_os_info(char* os_info, size_t size);

/**
 * @brief Base64 encode
 * 
 * @param data Data buffer
 * @param data_len Data length
 * @param encoded Encoded data buffer (allocated by function)
 * @param encoded_len Encoded data length
 * @return status_t Status code
 */
status_t utils_base64_encode(const uint8_t* data, size_t data_len, char** encoded, size_t* encoded_len);

/**
 * @brief Base64 decode
 * 
 * @param encoded Encoded data buffer
 * @param encoded_len Encoded data length
 * @param data Data buffer (allocated by function)
 * @param data_len Data length
 * @return status_t Status code
 */
status_t utils_base64_decode(const char* encoded, size_t encoded_len, uint8_t** data, size_t* data_len);

/**
 * @brief Hex encode
 * 
 * @param data Data buffer
 * @param data_len Data length
 * @param encoded Encoded data buffer (allocated by function)
 * @param encoded_len Encoded data length
 * @return status_t Status code
 */
status_t utils_hex_encode(const uint8_t* data, size_t data_len, char** encoded, size_t* encoded_len);

/**
 * @brief Hex decode
 * 
 * @param encoded Encoded data buffer
 * @param encoded_len Encoded data length
 * @param data Data buffer (allocated by function)
 * @param data_len Data length
 * @return status_t Status code
 */
status_t utils_hex_decode(const char* encoded, size_t encoded_len, uint8_t** data, size_t* data_len);

#endif /* DINOC_UTILS_H */
