/**
 * @file aes.c
 * @brief Implementation of AES encryption
 */

#include "../include/encryption.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>

// AES algorithm context
typedef struct {
    EVP_CIPHER_CTX* ctx;
    const EVP_CIPHER* cipher;
    bool encrypt_mode;
} aes_ctx_t;

/**
 * @brief Initialize AES encryption context
 * 
 * @param ctx Encryption context
 * @param key Encryption key
 * @param key_size Key size in bytes (16, 24, or 32 for AES-128, AES-192, or AES-256)
 * @param iv Initialization vector (16 bytes)
 * @param iv_size IV size in bytes (must be 16)
 * @return status_t Status code
 */
status_t aes_init(encryption_ctx_t* ctx, const uint8_t* key, size_t key_size,
                 const uint8_t* iv, size_t iv_size) {
    if (ctx == NULL || key == NULL || iv == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check key size
    if (key_size != 16 && key_size != 24 && key_size != 32) {
        LOG_ERROR("Invalid AES key size: %zu (must be 16, 24, or 32)", key_size);
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check IV size
    if (iv_size != 16) {
        LOG_ERROR("Invalid AES IV size: %zu (must be 16)", iv_size);
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Initialize OpenSSL
    static bool openssl_initialized = false;
    if (!openssl_initialized) {
        OpenSSL_add_all_algorithms();
        openssl_initialized = true;
    }
    
    // Create AES context
    aes_ctx_t* aes_ctx = (aes_ctx_t*)malloc(sizeof(aes_ctx_t));
    if (aes_ctx == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Create cipher context
    aes_ctx->ctx = EVP_CIPHER_CTX_new();
    if (aes_ctx->ctx == NULL) {
        free(aes_ctx);
        return STATUS_ERROR_CRYPTO;
    }
    
    // Select cipher based on key size
    switch (key_size) {
        case 16:
            aes_ctx->cipher = EVP_aes_128_cbc();
            break;
        case 24:
            aes_ctx->cipher = EVP_aes_192_cbc();
            break;
        case 32:
            aes_ctx->cipher = EVP_aes_256_cbc();
            break;
        default:
            EVP_CIPHER_CTX_free(aes_ctx->ctx);
            free(aes_ctx);
            return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Initialize encryption context
    ctx->type = ENCRYPTION_AES;
    ctx->algorithm_ctx = aes_ctx;
    
    // Copy key and IV
    memcpy(ctx->key, key, key_size);
    memcpy(ctx->iv, iv, iv_size);
    ctx->key_size = key_size;
    ctx->iv_size = iv_size;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Encrypt data using AES
 * 
 * @param ctx Encryption context
 * @param plaintext Input plaintext data
 * @param plaintext_len Plaintext length in bytes
 * @param ciphertext Output buffer for encrypted data
 * @param ciphertext_len Pointer to store ciphertext length
 * @return status_t Status code
 */
status_t aes_encrypt(encryption_ctx_t* ctx, const uint8_t* plaintext, size_t plaintext_len,
                    uint8_t* ciphertext, size_t* ciphertext_len) {
    if (ctx == NULL || plaintext == NULL || ciphertext == NULL || ciphertext_len == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    if (ctx->type != ENCRYPTION_AES || ctx->algorithm_ctx == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    aes_ctx_t* aes_ctx = (aes_ctx_t*)ctx->algorithm_ctx;
    
    // Initialize encryption operation
    if (EVP_EncryptInit_ex(aes_ctx->ctx, aes_ctx->cipher, NULL, ctx->key, ctx->iv) != 1) {
        LOG_ERROR("AES encryption initialization failed: %s", 
                 ERR_error_string(ERR_get_error(), NULL));
        return STATUS_ERROR_CRYPTO;
    }
    
    // Set encrypt mode
    aes_ctx->encrypt_mode = true;
    
    int outlen = 0;
    int tmplen = 0;
    
    // Encrypt data
    if (EVP_EncryptUpdate(aes_ctx->ctx, ciphertext, &outlen, plaintext, plaintext_len) != 1) {
        LOG_ERROR("AES encryption update failed: %s", 
                 ERR_error_string(ERR_get_error(), NULL));
        return STATUS_ERROR_CRYPTO;
    }
    
    // Finalize encryption
    if (EVP_EncryptFinal_ex(aes_ctx->ctx, ciphertext + outlen, &tmplen) != 1) {
        LOG_ERROR("AES encryption finalization failed: %s", 
                 ERR_error_string(ERR_get_error(), NULL));
        return STATUS_ERROR_CRYPTO;
    }
    
    // Set ciphertext length
    *ciphertext_len = outlen + tmplen;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Decrypt data using AES
 * 
 * @param ctx Encryption context
 * @param ciphertext Input encrypted data
 * @param ciphertext_len Ciphertext length in bytes
 * @param plaintext Output buffer for decrypted data
 * @param plaintext_len Pointer to store plaintext length
 * @return status_t Status code
 */
status_t aes_decrypt(encryption_ctx_t* ctx, const uint8_t* ciphertext, size_t ciphertext_len,
                    uint8_t* plaintext, size_t* plaintext_len) {
    if (ctx == NULL || ciphertext == NULL || plaintext == NULL || plaintext_len == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    if (ctx->type != ENCRYPTION_AES || ctx->algorithm_ctx == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    aes_ctx_t* aes_ctx = (aes_ctx_t*)ctx->algorithm_ctx;
    
    // Initialize decryption operation
    if (EVP_DecryptInit_ex(aes_ctx->ctx, aes_ctx->cipher, NULL, ctx->key, ctx->iv) != 1) {
        LOG_ERROR("AES decryption initialization failed: %s", 
                 ERR_error_string(ERR_get_error(), NULL));
        return STATUS_ERROR_CRYPTO;
    }
    
    // Set decrypt mode
    aes_ctx->encrypt_mode = false;
    
    int outlen = 0;
    int tmplen = 0;
    
    // Decrypt data
    if (EVP_DecryptUpdate(aes_ctx->ctx, plaintext, &outlen, ciphertext, ciphertext_len) != 1) {
        LOG_ERROR("AES decryption update failed: %s", 
                 ERR_error_string(ERR_get_error(), NULL));
        return STATUS_ERROR_CRYPTO;
    }
    
    // Finalize decryption
    if (EVP_DecryptFinal_ex(aes_ctx->ctx, plaintext + outlen, &tmplen) != 1) {
        LOG_ERROR("AES decryption finalization failed: %s", 
                 ERR_error_string(ERR_get_error(), NULL));
        return STATUS_ERROR_CRYPTO;
    }
    
    // Set plaintext length
    *plaintext_len = outlen + tmplen;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Clean up AES encryption context
 * 
 * @param ctx Encryption context
 * @return status_t Status code
 */
status_t aes_cleanup(encryption_ctx_t* ctx) {
    if (ctx == NULL || ctx->type != ENCRYPTION_AES || ctx->algorithm_ctx == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    aes_ctx_t* aes_ctx = (aes_ctx_t*)ctx->algorithm_ctx;
    
    // Free cipher context
    EVP_CIPHER_CTX_free(aes_ctx->ctx);
    
    // Free AES context
    free(aes_ctx);
    ctx->algorithm_ctx = NULL;
    
    return STATUS_SUCCESS;
}
