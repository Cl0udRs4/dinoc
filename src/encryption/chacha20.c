/**
 * @file chacha20.c
 * @brief Implementation of ChaCha20 encryption
 */

#include "../include/encryption.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>

// ChaCha20 algorithm context
typedef struct {
    EVP_CIPHER_CTX* ctx;
    const EVP_CIPHER* cipher;
    bool encrypt_mode;
} chacha20_ctx_t;

/**
 * @brief Initialize ChaCha20 encryption context
 * 
 * @param ctx Encryption context
 * @param key Encryption key (32 bytes)
 * @param key_size Key size in bytes (must be 32)
 * @param iv Initialization vector/nonce (12 bytes)
 * @param iv_size IV size in bytes (must be 12)
 * @return status_t Status code
 */
status_t chacha20_init(encryption_ctx_t* ctx, const uint8_t* key, size_t key_size,
                      const uint8_t* iv, size_t iv_size) {
    if (ctx == NULL || key == NULL || iv == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check key size
    if (key_size != 32) {
        LOG_ERROR("Invalid ChaCha20 key size: %zu (must be 32)", key_size);
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check IV size
    if (iv_size != 12) {
        LOG_ERROR("Invalid ChaCha20 IV size: %zu (must be 12)", iv_size);
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Initialize OpenSSL
    static bool openssl_initialized = false;
    if (!openssl_initialized) {
        OpenSSL_add_all_algorithms();
        openssl_initialized = true;
    }
    
    // Create ChaCha20 context
    chacha20_ctx_t* chacha20_ctx = (chacha20_ctx_t*)malloc(sizeof(chacha20_ctx_t));
    if (chacha20_ctx == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Create cipher context
    chacha20_ctx->ctx = EVP_CIPHER_CTX_new();
    if (chacha20_ctx->ctx == NULL) {
        free(chacha20_ctx);
        return STATUS_ERROR_CRYPTO;
    }
    
    // Set cipher
    chacha20_ctx->cipher = EVP_chacha20();
    
    // Initialize encryption context
    ctx->type = ENCRYPTION_CHACHA20;
    ctx->algorithm_ctx = chacha20_ctx;
    
    // Copy key and IV
    memcpy(ctx->key, key, key_size);
    memcpy(ctx->iv, iv, iv_size);
    ctx->key_size = key_size;
    ctx->iv_size = iv_size;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Encrypt data using ChaCha20
 * 
 * @param ctx Encryption context
 * @param plaintext Input plaintext data
 * @param plaintext_len Plaintext length in bytes
 * @param ciphertext Output buffer for encrypted data
 * @param ciphertext_len Pointer to store ciphertext length
 * @return status_t Status code
 */
status_t chacha20_encrypt(encryption_ctx_t* ctx, const uint8_t* plaintext, size_t plaintext_len,
                         uint8_t* ciphertext, size_t* ciphertext_len) {
    if (ctx == NULL || plaintext == NULL || ciphertext == NULL || ciphertext_len == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    if (ctx->type != ENCRYPTION_CHACHA20 || ctx->algorithm_ctx == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    chacha20_ctx_t* chacha20_ctx = (chacha20_ctx_t*)ctx->algorithm_ctx;
    
    // Initialize encryption operation
    if (EVP_EncryptInit_ex(chacha20_ctx->ctx, chacha20_ctx->cipher, NULL, ctx->key, ctx->iv) != 1) {
        LOG_ERROR("ChaCha20 encryption initialization failed: %s", 
                 ERR_error_string(ERR_get_error(), NULL));
        return STATUS_ERROR_CRYPTO;
    }
    
    // Set encrypt mode
    chacha20_ctx->encrypt_mode = true;
    
    int outlen = 0;
    int tmplen = 0;
    
    // Encrypt data
    if (EVP_EncryptUpdate(chacha20_ctx->ctx, ciphertext, &outlen, plaintext, plaintext_len) != 1) {
        LOG_ERROR("ChaCha20 encryption update failed: %s", 
                 ERR_error_string(ERR_get_error(), NULL));
        return STATUS_ERROR_CRYPTO;
    }
    
    // Finalize encryption
    if (EVP_EncryptFinal_ex(chacha20_ctx->ctx, ciphertext + outlen, &tmplen) != 1) {
        LOG_ERROR("ChaCha20 encryption finalization failed: %s", 
                 ERR_error_string(ERR_get_error(), NULL));
        return STATUS_ERROR_CRYPTO;
    }
    
    // Set ciphertext length
    *ciphertext_len = outlen + tmplen;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Decrypt data using ChaCha20
 * 
 * @param ctx Encryption context
 * @param ciphertext Input encrypted data
 * @param ciphertext_len Ciphertext length in bytes
 * @param plaintext Output buffer for decrypted data
 * @param plaintext_len Pointer to store plaintext length
 * @return status_t Status code
 */
status_t chacha20_decrypt(encryption_ctx_t* ctx, const uint8_t* ciphertext, size_t ciphertext_len,
                         uint8_t* plaintext, size_t* plaintext_len) {
    if (ctx == NULL || ciphertext == NULL || plaintext == NULL || plaintext_len == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    if (ctx->type != ENCRYPTION_CHACHA20 || ctx->algorithm_ctx == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    chacha20_ctx_t* chacha20_ctx = (chacha20_ctx_t*)ctx->algorithm_ctx;
    
    // Initialize decryption operation
    if (EVP_DecryptInit_ex(chacha20_ctx->ctx, chacha20_ctx->cipher, NULL, ctx->key, ctx->iv) != 1) {
        LOG_ERROR("ChaCha20 decryption initialization failed: %s", 
                 ERR_error_string(ERR_get_error(), NULL));
        return STATUS_ERROR_CRYPTO;
    }
    
    // Set decrypt mode
    chacha20_ctx->encrypt_mode = false;
    
    int outlen = 0;
    int tmplen = 0;
    
    // Decrypt data
    if (EVP_DecryptUpdate(chacha20_ctx->ctx, plaintext, &outlen, ciphertext, ciphertext_len) != 1) {
        LOG_ERROR("ChaCha20 decryption update failed: %s", 
                 ERR_error_string(ERR_get_error(), NULL));
        return STATUS_ERROR_CRYPTO;
    }
    
    // Finalize decryption
    if (EVP_DecryptFinal_ex(chacha20_ctx->ctx, plaintext + outlen, &tmplen) != 1) {
        LOG_ERROR("ChaCha20 decryption finalization failed: %s", 
                 ERR_error_string(ERR_get_error(), NULL));
        return STATUS_ERROR_CRYPTO;
    }
    
    // Set plaintext length
    *plaintext_len = outlen + tmplen;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Clean up ChaCha20 encryption context
 * 
 * @param ctx Encryption context
 * @return status_t Status code
 */
status_t chacha20_cleanup(encryption_ctx_t* ctx) {
    if (ctx == NULL || ctx->type != ENCRYPTION_CHACHA20 || ctx->algorithm_ctx == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    chacha20_ctx_t* chacha20_ctx = (chacha20_ctx_t*)ctx->algorithm_ctx;
    
    // Free cipher context
    EVP_CIPHER_CTX_free(chacha20_ctx->ctx);
    
    // Free ChaCha20 context
    free(chacha20_ctx);
    ctx->algorithm_ctx = NULL;
    
    return STATUS_SUCCESS;
}
