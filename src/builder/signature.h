/**
 * @file signature.h
 * @brief Signature verification for builder-generated clients
 */

#ifndef DINOC_SIGNATURE_H
#define DINOC_SIGNATURE_H

#include "../include/common.h"
#include <stdint.h>
#include <stddef.h>

// Signature structure
typedef struct {
    uint8_t signature[64];     // Signature data (512 bits)
    uint64_t timestamp;        // Signature creation timestamp
    uint16_t version_major;    // Client version major
    uint16_t version_minor;    // Client version minor
    uint16_t version_patch;    // Client version patch
} client_signature_t;

/**
 * @brief Initialize signature module
 * 
 * @return status_t Status code
 */
status_t signature_init(void);

/**
 * @brief Shutdown signature module
 * 
 * @return status_t Status code
 */
status_t signature_shutdown(void);

/**
 * @brief Sign client binary
 * 
 * @param client_data Client binary data
 * @param client_len Client binary length
 * @param version_major Client version major
 * @param version_minor Client version minor
 * @param version_patch Client version patch
 * @param signature Pointer to store signature
 * @return status_t Status code
 */
status_t signature_sign_client(const uint8_t* client_data, size_t client_len,
                             uint16_t version_major, uint16_t version_minor, uint16_t version_patch,
                             client_signature_t* signature);

/**
 * @brief Verify client signature
 * 
 * @param client_data Client binary data
 * @param client_len Client binary length
 * @param signature Client signature
 * @return status_t Status code
 */
status_t signature_verify_client(const uint8_t* client_data, size_t client_len,
                               const client_signature_t* signature);

/**
 * @brief Append signature to client binary
 * 
 * @param client_data Client binary data
 * @param client_len Client binary length
 * @param signature Client signature
 * @param output Output buffer for signed client
 * @param output_len Pointer to store output length
 * @param max_output_len Maximum output buffer size
 * @return status_t Status code
 */
status_t signature_append_to_client(const uint8_t* client_data, size_t client_len,
                                  const client_signature_t* signature,
                                  uint8_t* output, size_t* output_len,
                                  size_t max_output_len);

/**
 * @brief Extract signature from client binary
 * 
 * @param client_data Client binary data with signature
 * @param client_len Client binary length
 * @param signature Pointer to store extracted signature
 * @param original_data Output buffer for original client data
 * @param original_len Pointer to store original data length
 * @param max_original_len Maximum original data buffer size
 * @return status_t Status code
 */
status_t signature_extract_from_client(const uint8_t* client_data, size_t client_len,
                                     client_signature_t* signature,
                                     uint8_t* original_data, size_t* original_len,
                                     size_t max_original_len);

#endif /* DINOC_SIGNATURE_H */
