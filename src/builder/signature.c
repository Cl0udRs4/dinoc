/**
 * @file signature.c
 * @brief Signature verification implementation for builder-generated clients
 */

#include "signature.h"
#include "../encryption/encryption.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

// Signature magic bytes for identification
#define SIGNATURE_MAGIC "DINOC_SIG"
#define SIGNATURE_MAGIC_LEN 8

// Builder key (in a real implementation, this would be securely stored)
static const uint8_t BUILDER_KEY[32] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
    0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10,
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00
};

/**
 * @brief Initialize signature module
 */
status_t signature_init(void) {
    // Initialize OpenSSL if needed
    return STATUS_SUCCESS;
}

/**
 * @brief Shutdown signature module
 */
status_t signature_shutdown(void) {
    // Clean up OpenSSL if needed
    return STATUS_SUCCESS;
}

/**
 * @brief Sign client binary
 */
status_t signature_sign_client(const uint8_t* client_data, size_t client_len,
                             uint16_t version_major, uint16_t version_minor, uint16_t version_patch,
                             client_signature_t* signature) {
    if (client_data == NULL || client_len == 0 || signature == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Create HMAC context
    HMAC_CTX* ctx = HMAC_CTX_new();
    if (ctx == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize HMAC with builder key
    if (HMAC_Init_ex(ctx, BUILDER_KEY, sizeof(BUILDER_KEY), EVP_sha512(), NULL) != 1) {
        HMAC_CTX_free(ctx);
        return STATUS_ERROR_CRYPTO;
    }
    
    // Update HMAC with client data
    if (HMAC_Update(ctx, client_data, client_len) != 1) {
        HMAC_CTX_free(ctx);
        return STATUS_ERROR_CRYPTO;
    }
    
    // Add version information to HMAC
    uint8_t version_data[6];
    version_data[0] = (version_major >> 8) & 0xFF;
    version_data[1] = version_major & 0xFF;
    version_data[2] = (version_minor >> 8) & 0xFF;
    version_data[3] = version_minor & 0xFF;
    version_data[4] = (version_patch >> 8) & 0xFF;
    version_data[5] = version_patch & 0xFF;
    
    if (HMAC_Update(ctx, version_data, sizeof(version_data)) != 1) {
        HMAC_CTX_free(ctx);
        return STATUS_ERROR_CRYPTO;
    }
    
    // Finalize HMAC
    unsigned int sig_len = 0;
    if (HMAC_Final(ctx, signature->signature, &sig_len) != 1 || sig_len != 64) {
        HMAC_CTX_free(ctx);
        return STATUS_ERROR_CRYPTO;
    }
    
    // Clean up
    HMAC_CTX_free(ctx);
    
    // Set signature metadata
    signature->timestamp = (uint64_t)time(NULL);
    signature->version_major = version_major;
    signature->version_minor = version_minor;
    signature->version_patch = version_patch;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Verify client signature
 */
status_t signature_verify_client(const uint8_t* client_data, size_t client_len,
                               const client_signature_t* signature) {
    if (client_data == NULL || client_len == 0 || signature == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Create a new signature for comparison
    client_signature_t computed_sig;
    status_t status = signature_sign_client(client_data, client_len,
                                          signature->version_major,
                                          signature->version_minor,
                                          signature->version_patch,
                                          &computed_sig);
    
    if (status != STATUS_SUCCESS) {
        return status;
    }
    
    // Compare signatures
    if (memcmp(computed_sig.signature, signature->signature, sizeof(signature->signature)) != 0) {
        return STATUS_ERROR_SIGNATURE;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Append signature to client binary
 */
status_t signature_append_to_client(const uint8_t* client_data, size_t client_len,
                                  const client_signature_t* signature,
                                  uint8_t* output, size_t* output_len,
                                  size_t max_output_len) {
    if (client_data == NULL || client_len == 0 || signature == NULL ||
        output == NULL || output_len == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Calculate required output size
    size_t required_size = client_len + SIGNATURE_MAGIC_LEN + sizeof(client_signature_t);
    
    if (max_output_len < required_size) {
        return STATUS_ERROR_BUFFER_TOO_SMALL;
    }
    
    // Copy client data
    memcpy(output, client_data, client_len);
    
    // Append magic bytes
    memcpy(output + client_len, SIGNATURE_MAGIC, SIGNATURE_MAGIC_LEN);
    
    // Append signature
    memcpy(output + client_len + SIGNATURE_MAGIC_LEN, signature, sizeof(client_signature_t));
    
    // Set output length
    *output_len = required_size;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Extract signature from client binary
 */
status_t signature_extract_from_client(const uint8_t* client_data, size_t client_len,
                                     client_signature_t* signature,
                                     uint8_t* original_data, size_t* original_len,
                                     size_t max_original_len) {
    if (client_data == NULL || client_len == 0 || signature == NULL ||
        original_data == NULL || original_len == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check if client data is large enough to contain a signature
    size_t min_size = SIGNATURE_MAGIC_LEN + sizeof(client_signature_t);
    if (client_len < min_size) {
        return STATUS_ERROR_INVALID_FORMAT;
    }
    
    // Check for signature magic bytes
    const uint8_t* magic_pos = client_data + client_len - min_size;
    if (memcmp(magic_pos, SIGNATURE_MAGIC, SIGNATURE_MAGIC_LEN) != 0) {
        return STATUS_ERROR_INVALID_FORMAT;
    }
    
    // Extract signature
    const uint8_t* sig_pos = magic_pos + SIGNATURE_MAGIC_LEN;
    memcpy(signature, sig_pos, sizeof(client_signature_t));
    
    // Calculate original data size
    size_t orig_size = client_len - min_size;
    
    if (max_original_len < orig_size) {
        return STATUS_ERROR_BUFFER_TOO_SMALL;
    }
    
    // Extract original data
    memcpy(original_data, client_data, orig_size);
    *original_len = orig_size;
    
    return STATUS_SUCCESS;
}
