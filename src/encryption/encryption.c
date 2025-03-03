/**
 * @file encryption.c
 * @brief Implementation of encryption module interface
 */

#include "../include/encryption.h"

// Forward declarations of algorithm-specific functions
status_t aes_init(encryption_ctx_t* ctx, const uint8_t* key, size_t key_size,
                 const uint8_t* iv, size_t iv_size);
status_t aes_encrypt(encryption_ctx_t* ctx, const uint8_t* plaintext, size_t plaintext_len,
                    uint8_t* ciphertext, size_t* ciphertext_len);
status_t aes_decrypt(encryption_ctx_t* ctx, const uint8_t* ciphertext, size_t ciphertext_len,
                    uint8_t* plaintext, size_t* plaintext_len);
status_t aes_cleanup(encryption_ctx_t* ctx);

status_t chacha20_init(encryption_ctx_t* ctx, const uint8_t* key, size_t key_size,
                      const uint8_t* iv, size_t iv_size);
status_t chacha20_encrypt(encryption_ctx_t* ctx, const uint8_t* plaintext, size_t plaintext_len,
                         uint8_t* ciphertext, size_t* ciphertext_len);
status_t chacha20_decrypt(encryption_ctx_t* ctx, const uint8_t* ciphertext, size_t ciphertext_len,
                         uint8_t* plaintext, size_t* plaintext_len);
status_t chacha20_cleanup(encryption_ctx_t* ctx);

// Magic bytes for encryption type detection
#define AES_MAGIC_BYTE      0xA3
#define CHACHA20_MAGIC_BYTE 0xC2

// Protocol header structure for encryption detection
typedef struct {
    uint8_t magic;          // Magic byte for encryption type
    uint8_t version;        // Protocol version
    uint16_t flags;         // Protocol flags
    uint32_t payload_len;   // Payload length
} __attribute__((packed)) protocol_header_t;

status_t encryption_init(encryption_ctx_t* ctx, encryption_type_t type, 
                         const uint8_t* key, size_t key_size,
                         const uint8_t* iv, size_t iv_size) {
    if (ctx == NULL || key == NULL || iv == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Initialize based on encryption type
    switch (type) {
        case ENCRYPTION_AES:
            return aes_init(ctx, key, key_size, iv, iv_size);
            
        case ENCRYPTION_CHACHA20:
            return chacha20_init(ctx, key, key_size, iv, iv_size);
            
        case ENCRYPTION_NONE:
            // No encryption, just copy key and IV
            ctx->type = ENCRYPTION_NONE;
            ctx->algorithm_ctx = NULL;
            
            // Copy key and IV
            memcpy(ctx->key, key, key_size);
            memcpy(ctx->iv, iv, iv_size);
            ctx->key_size = key_size;
            ctx->iv_size = iv_size;
            
            return STATUS_SUCCESS;
            
        default:
            LOG_ERROR("Unsupported encryption type: %d", type);
            return STATUS_ERROR_NOT_IMPLEMENTED;
    }
}

status_t encryption_encrypt(encryption_ctx_t* ctx, 
                           const uint8_t* plaintext, size_t plaintext_len,
                           uint8_t* ciphertext, size_t* ciphertext_len) {
    if (ctx == NULL || plaintext == NULL || ciphertext == NULL || ciphertext_len == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Encrypt based on encryption type
    switch (ctx->type) {
        case ENCRYPTION_AES:
            return aes_encrypt(ctx, plaintext, plaintext_len, ciphertext, ciphertext_len);
            
        case ENCRYPTION_CHACHA20:
            return chacha20_encrypt(ctx, plaintext, plaintext_len, ciphertext, ciphertext_len);
            
        case ENCRYPTION_NONE:
            // No encryption, just copy plaintext
            if (plaintext_len > *ciphertext_len) {
                LOG_ERROR("Ciphertext buffer too small: %zu < %zu", *ciphertext_len, plaintext_len);
                return STATUS_ERROR_INVALID_PARAM;
            }
            
            memcpy(ciphertext, plaintext, plaintext_len);
            *ciphertext_len = plaintext_len;
            
            return STATUS_SUCCESS;
            
        default:
            LOG_ERROR("Unsupported encryption type: %d", ctx->type);
            return STATUS_ERROR_NOT_IMPLEMENTED;
    }
}

status_t encryption_decrypt(encryption_ctx_t* ctx, 
                           const uint8_t* ciphertext, size_t ciphertext_len,
                           uint8_t* plaintext, size_t* plaintext_len) {
    if (ctx == NULL || ciphertext == NULL || plaintext == NULL || plaintext_len == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Decrypt based on encryption type
    switch (ctx->type) {
        case ENCRYPTION_AES:
            return aes_decrypt(ctx, ciphertext, ciphertext_len, plaintext, plaintext_len);
            
        case ENCRYPTION_CHACHA20:
            return chacha20_decrypt(ctx, ciphertext, ciphertext_len, plaintext, plaintext_len);
            
        case ENCRYPTION_NONE:
            // No encryption, just copy ciphertext
            if (ciphertext_len > *plaintext_len) {
                LOG_ERROR("Plaintext buffer too small: %zu < %zu", *plaintext_len, ciphertext_len);
                return STATUS_ERROR_INVALID_PARAM;
            }
            
            memcpy(plaintext, ciphertext, ciphertext_len);
            *plaintext_len = ciphertext_len;
            
            return STATUS_SUCCESS;
            
        default:
            LOG_ERROR("Unsupported encryption type: %d", ctx->type);
            return STATUS_ERROR_NOT_IMPLEMENTED;
    }
}

status_t encryption_cleanup(encryption_ctx_t* ctx) {
    if (ctx == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Clean up based on encryption type
    switch (ctx->type) {
        case ENCRYPTION_AES:
            return aes_cleanup(ctx);
            
        case ENCRYPTION_CHACHA20:
            return chacha20_cleanup(ctx);
            
        case ENCRYPTION_NONE:
            // No cleanup needed
            return STATUS_SUCCESS;
            
        default:
            LOG_ERROR("Unsupported encryption type: %d", ctx->type);
            return STATUS_ERROR_NOT_IMPLEMENTED;
    }
}

encryption_type_t encryption_detect_type(const uint8_t* data, size_t data_len) {
    if (data == NULL || data_len < sizeof(protocol_header_t)) {
        return ENCRYPTION_UNKNOWN;
    }
    
    // Cast data to protocol header
    const protocol_header_t* header = (const protocol_header_t*)data;
    
    // Check magic byte
    switch (header->magic) {
        case AES_MAGIC_BYTE:
            return ENCRYPTION_AES;
            
        case CHACHA20_MAGIC_BYTE:
            return ENCRYPTION_CHACHA20;
            
        default:
            return ENCRYPTION_UNKNOWN;
    }
}

status_t encryption_generate_key(uint8_t* key, size_t key_size) {
    if (key == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check key size
    if (key_size != 16 && key_size != 24 && key_size != 32) {
        LOG_ERROR("Invalid key size: %zu (must be 16, 24, or 32)", key_size);
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Generate random key
    if (RAND_bytes(key, key_size) != 1) {
        LOG_ERROR("Failed to generate random key: %s", 
                 ERR_error_string(ERR_get_error(), NULL));
        return STATUS_ERROR_CRYPTO;
    }
    
    return STATUS_SUCCESS;
}

status_t encryption_generate_iv(uint8_t* iv, size_t iv_size) {
    if (iv == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check IV size
    if (iv_size != 12 && iv_size != 16) {
        LOG_ERROR("Invalid IV size: %zu (must be 12 or 16)", iv_size);
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Generate random IV
    if (RAND_bytes(iv, iv_size) != 1) {
        LOG_ERROR("Failed to generate random IV: %s", 
                 ERR_error_string(ERR_get_error(), NULL));
        return STATUS_ERROR_CRYPTO;
    }
    
    return STATUS_SUCCESS;
}
