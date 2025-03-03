/**
 * @file encryption.h
 * @brief Encryption module interface
 */

#ifndef DINOC_ENCRYPTION_H
#define DINOC_ENCRYPTION_H

#include "common.h"

/**
 * @brief Maximum key size in bytes
 */
#define MAX_KEY_SIZE 32

/**
 * @brief Maximum IV/nonce size in bytes
 */
#define MAX_IV_SIZE 16

/**
 * @brief Encryption context structure
 */
typedef struct encryption_ctx {
    encryption_type_t type;
    uint8_t key[MAX_KEY_SIZE];
    uint8_t iv[MAX_IV_SIZE];
    size_t key_size;
    size_t iv_size;
    void* algorithm_ctx; // Algorithm-specific context
} encryption_ctx_t;

/**
 * @brief Initialize encryption context
 * 
 * @param ctx Encryption context to initialize
 * @param type Encryption algorithm type
 * @param key Encryption key
 * @param key_size Key size in bytes
 * @param iv Initialization vector/nonce
 * @param iv_size IV/nonce size in bytes
 * @return status_t Status code
 */
status_t encryption_init(encryption_ctx_t* ctx, encryption_type_t type, 
                         const uint8_t* key, size_t key_size,
                         const uint8_t* iv, size_t iv_size);

/**
 * @brief Encrypt data
 * 
 * @param ctx Encryption context
 * @param plaintext Input plaintext data
 * @param plaintext_len Plaintext length in bytes
 * @param ciphertext Output buffer for encrypted data
 * @param ciphertext_len Pointer to store ciphertext length
 * @return status_t Status code
 */
status_t encryption_encrypt(encryption_ctx_t* ctx, 
                           const uint8_t* plaintext, size_t plaintext_len,
                           uint8_t* ciphertext, size_t* ciphertext_len);

/**
 * @brief Decrypt data
 * 
 * @param ctx Encryption context
 * @param ciphertext Input encrypted data
 * @param ciphertext_len Ciphertext length in bytes
 * @param plaintext Output buffer for decrypted data
 * @param plaintext_len Pointer to store plaintext length
 * @return status_t Status code
 */
status_t encryption_decrypt(encryption_ctx_t* ctx, 
                           const uint8_t* ciphertext, size_t ciphertext_len,
                           uint8_t* plaintext, size_t* plaintext_len);

/**
 * @brief Clean up encryption context
 * 
 * @param ctx Encryption context to clean up
 * @return status_t Status code
 */
status_t encryption_cleanup(encryption_ctx_t* ctx);

/**
 * @brief Detect encryption type from protocol header
 * 
 * @param data Protocol header data
 * @param data_len Data length in bytes
 * @return encryption_type_t Detected encryption type
 */
encryption_type_t encryption_detect_type(const uint8_t* data, size_t data_len);

/**
 * @brief Generate random key
 * 
 * @param key Buffer to store key
 * @param key_size Key size in bytes
 * @return status_t Status code
 */
status_t encryption_generate_key(uint8_t* key, size_t key_size);

/**
 * @brief Generate random IV/nonce
 * 
 * @param iv Buffer to store IV/nonce
 * @param iv_size IV/nonce size in bytes
 * @return status_t Status code
 */
status_t encryption_generate_iv(uint8_t* iv, size_t iv_size);

#endif /* DINOC_ENCRYPTION_H */
