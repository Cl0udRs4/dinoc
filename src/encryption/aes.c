/**
 * @file aes.c
 * @brief AES encryption implementation
 */

#include "encryption.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

// AES block size in bytes
#define AES_BLOCK_SIZE 16

// AES-GCM tag size in bytes
#define AES_GCM_TAG_SIZE 16

// AES context structure
typedef struct {
    uint8_t key[32];        // Key (128 or 256 bits)
    uint8_t iv[16];         // Initialization vector
    size_t key_size;        // Key size in bytes (16 or 32)
    size_t iv_size;         // IV size in bytes
} aes_context_t;

/**
 * @brief Create AES context
 */
static status_t aes_create_context(encryption_algorithm_t algorithm, void** context) {
    if (context == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Validate algorithm
    if (algorithm != ENCRYPTION_AES_128_GCM && algorithm != ENCRYPTION_AES_256_GCM) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Allocate context
    aes_context_t* aes_context = (aes_context_t*)malloc(sizeof(aes_context_t));
    if (aes_context == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize context
    memset(aes_context, 0, sizeof(aes_context_t));
    
    // Set key size based on algorithm
    aes_context->key_size = (algorithm == ENCRYPTION_AES_128_GCM) ? 16 : 32;
    aes_context->iv_size = 12; // GCM typically uses 12-byte IV
    
    *context = aes_context;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Destroy AES context
 */
static status_t aes_destroy_context(void* context) {
    if (context == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Securely clear context memory
    aes_context_t* aes_context = (aes_context_t*)context;
    memset(aes_context, 0, sizeof(aes_context_t));
    
    // Free context
    free(aes_context);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Set AES key
 */
static status_t aes_set_key(void* context, const encryption_key_t* key) {
    if (context == NULL || key == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Validate algorithm
    if (key->algorithm != ENCRYPTION_AES_128_GCM && key->algorithm != ENCRYPTION_AES_256_GCM) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Validate key size
    size_t expected_key_size = (key->algorithm == ENCRYPTION_AES_128_GCM) ? 16 : 32;
    if (key->key_size != expected_key_size) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Set key and IV
    aes_context_t* aes_context = (aes_context_t*)context;
    memcpy(aes_context->key, key->key, key->key_size);
    memcpy(aes_context->iv, key->iv, key->iv_size);
    aes_context->key_size = key->key_size;
    aes_context->iv_size = key->iv_size;
    
    return STATUS_SUCCESS;
}

/**
 * @brief AES-GCM encryption
 * 
 * Note: This is a simplified implementation. In a real-world scenario,
 * you would use a cryptographic library like OpenSSL or mbedTLS.
 */
static status_t aes_encrypt(void* context,
                          const uint8_t* plaintext, size_t plaintext_len,
                          uint8_t* ciphertext, size_t* ciphertext_len,
                          size_t max_ciphertext_len) {
    if (context == NULL || plaintext == NULL || ciphertext == NULL || ciphertext_len == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    aes_context_t* aes_context = (aes_context_t*)context;
    
    // Check if output buffer is large enough
    // AES-GCM output size = plaintext size + tag size
    size_t required_size = plaintext_len + AES_GCM_TAG_SIZE;
    if (max_ciphertext_len < required_size) {
        return STATUS_ERROR_BUFFER_TOO_SMALL;
    }
    
    // In a real implementation, you would:
    // 1. Initialize AES-GCM with key and IV
    // 2. Encrypt plaintext
    // 3. Compute authentication tag
    
    // For this simplified version, we'll just copy the plaintext to ciphertext
    // and append a dummy tag
    memcpy(ciphertext, plaintext, plaintext_len);
    
    // Generate a dummy tag (in real implementation, this would be the GCM authentication tag)
    for (size_t i = 0; i < AES_GCM_TAG_SIZE; i++) {
        ciphertext[plaintext_len + i] = (uint8_t)(i ^ aes_context->key[i % aes_context->key_size]);
    }
    
    *ciphertext_len = plaintext_len + AES_GCM_TAG_SIZE;
    
    return STATUS_SUCCESS;
}

/**
 * @brief AES-GCM decryption
 * 
 * Note: This is a simplified implementation. In a real-world scenario,
 * you would use a cryptographic library like OpenSSL or mbedTLS.
 */
static status_t aes_decrypt(void* context,
                          const uint8_t* ciphertext, size_t ciphertext_len,
                          uint8_t* plaintext, size_t* plaintext_len,
                          size_t max_plaintext_len) {
    if (context == NULL || ciphertext == NULL || plaintext == NULL || plaintext_len == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check if ciphertext is large enough to contain tag
    if (ciphertext_len < AES_GCM_TAG_SIZE) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Calculate plaintext length (ciphertext length - tag size)
    size_t actual_plaintext_len = ciphertext_len - AES_GCM_TAG_SIZE;
    
    // Check if output buffer is large enough
    if (max_plaintext_len < actual_plaintext_len) {
        return STATUS_ERROR_BUFFER_TOO_SMALL;
    }
    
    // In a real implementation, you would:
    // 1. Initialize AES-GCM with key and IV
    // 2. Verify authentication tag
    // 3. Decrypt ciphertext
    
    // For this simplified version, we'll just copy the ciphertext to plaintext
    // (excluding the tag)
    memcpy(plaintext, ciphertext, actual_plaintext_len);
    
    *plaintext_len = actual_plaintext_len;
    
    return STATUS_SUCCESS;
}

// Export AES functions
const struct {
    status_t (*create_context)(encryption_algorithm_t algorithm, void** context);
    status_t (*destroy_context)(void* context);
    status_t (*set_key)(void* context, const encryption_key_t* key);
    status_t (*encrypt)(void* context, const uint8_t* plaintext, size_t plaintext_len,
                      uint8_t* ciphertext, size_t* ciphertext_len, size_t max_ciphertext_len);
    status_t (*decrypt)(void* context, const uint8_t* ciphertext, size_t ciphertext_len,
                      uint8_t* plaintext, size_t* plaintext_len, size_t max_plaintext_len);
} aes_functions = {
    .create_context = aes_create_context,
    .destroy_context = aes_destroy_context,
    .set_key = aes_set_key,
    .encrypt = aes_encrypt,
    .decrypt = aes_decrypt
};
