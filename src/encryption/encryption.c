/**
 * @file encryption.c
 * @brief Encryption implementation for C2 server
 */

#include "encryption.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// External functions from algorithm implementations
extern const struct {
    status_t (*create_context)(encryption_algorithm_t algorithm, void** context);
    status_t (*destroy_context)(void* context);
    status_t (*set_key)(void* context, const encryption_key_t* key);
    status_t (*encrypt)(void* context, const uint8_t* plaintext, size_t plaintext_len,
                      uint8_t* ciphertext, size_t* ciphertext_len, size_t max_ciphertext_len);
    status_t (*decrypt)(void* context, const uint8_t* ciphertext, size_t ciphertext_len,
                      uint8_t* plaintext, size_t* plaintext_len, size_t max_plaintext_len);
} aes_functions;

extern const struct {
    status_t (*create_context)(encryption_algorithm_t algorithm, void** context);
    status_t (*destroy_context)(void* context);
    status_t (*set_key)(void* context, const encryption_key_t* key);
    status_t (*encrypt)(void* context, const uint8_t* plaintext, size_t plaintext_len,
                      uint8_t* ciphertext, size_t* ciphertext_len, size_t max_ciphertext_len);
    status_t (*decrypt)(void* context, const uint8_t* ciphertext, size_t ciphertext_len,
                      uint8_t* plaintext, size_t* plaintext_len, size_t max_plaintext_len);
} chacha20_functions;

// Global encryption manager
static struct {
    bool initialized;
} encryption_manager = {
    .initialized = false
};

/**
 * @brief Initialize encryption system
 */
status_t encryption_init(void) {
    if (encryption_manager.initialized) {
        return STATUS_SUCCESS;
    }
    
    // Seed random number generator
    srand((unsigned int)time(NULL));
    
    encryption_manager.initialized = true;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Shutdown encryption system
 */
status_t encryption_shutdown(void) {
    if (!encryption_manager.initialized) {
        return STATUS_SUCCESS;
    }
    
    encryption_manager.initialized = false;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Create encryption context
 */
status_t encryption_create_context(encryption_algorithm_t algorithm, encryption_context_t** context) {
    if (context == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check if encryption system is initialized
    if (!encryption_manager.initialized) {
        return STATUS_ERROR_NOT_RUNNING;
    }
    
    // Allocate context
    encryption_context_t* new_context = (encryption_context_t*)malloc(sizeof(encryption_context_t));
    if (new_context == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize context
    memset(new_context, 0, sizeof(encryption_context_t));
    new_context->algorithm = algorithm;
    
    // Create algorithm-specific context
    status_t status;
    
    switch (algorithm) {
        case ENCRYPTION_AES_128_GCM:
        case ENCRYPTION_AES_256_GCM:
            status = aes_functions.create_context(algorithm, &new_context->algorithm_context);
            break;
            
        case ENCRYPTION_CHACHA20_POLY1305:
            status = chacha20_functions.create_context(algorithm, &new_context->algorithm_context);
            break;
            
        default:
            free(new_context);
            return STATUS_ERROR_INVALID_PARAM;
    }
    
    if (status != STATUS_SUCCESS) {
        free(new_context);
        return status;
    }
    
    *context = new_context;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Destroy encryption context
 */
status_t encryption_destroy_context(encryption_context_t* context) {
    if (context == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Destroy algorithm-specific context
    status_t status;
    
    switch (context->algorithm) {
        case ENCRYPTION_AES_128_GCM:
        case ENCRYPTION_AES_256_GCM:
            status = aes_functions.destroy_context(context->algorithm_context);
            break;
            
        case ENCRYPTION_CHACHA20_POLY1305:
            status = chacha20_functions.destroy_context(context->algorithm_context);
            break;
            
        default:
            return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Free key if allocated
    if (context->current_key != NULL) {
        free(context->current_key);
    }
    
    // Free context
    free(context);
    
    return status;
}

/**
 * @brief Generate encryption key
 */
status_t encryption_generate_key(encryption_algorithm_t algorithm, encryption_key_t* key, uint64_t expire_seconds) {
    if (key == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check if encryption system is initialized
    if (!encryption_manager.initialized) {
        return STATUS_ERROR_NOT_RUNNING;
    }
    
    // Initialize key
    memset(key, 0, sizeof(encryption_key_t));
    key->algorithm = algorithm;
    
    // Set key size based on algorithm
    switch (algorithm) {
        case ENCRYPTION_AES_128_GCM:
            key->key_size = 16; // 128 bits
            key->iv_size = 12;  // 96 bits
            break;
            
        case ENCRYPTION_AES_256_GCM:
            key->key_size = 32; // 256 bits
            key->iv_size = 12;  // 96 bits
            break;
            
        case ENCRYPTION_CHACHA20_POLY1305:
            key->key_size = 32; // 256 bits
            key->iv_size = 12;  // 96 bits
            break;
            
        default:
            return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Generate random key
    for (size_t i = 0; i < key->key_size; i++) {
        key->key[i] = (uint8_t)rand();
    }
    
    // Generate random IV
    for (size_t i = 0; i < key->iv_size; i++) {
        key->iv[i] = (uint8_t)rand();
    }
    
    // Set creation time
    key->created_time = (uint64_t)time(NULL);
    
    // Set expiration time
    if (expire_seconds > 0) {
        key->expire_time = key->created_time + expire_seconds;
    } else {
        key->expire_time = 0; // No expiration
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Set encryption key for context
 */
status_t encryption_set_key(encryption_context_t* context, const encryption_key_t* key) {
    if (context == NULL || key == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check if algorithm matches
    if (context->algorithm != key->algorithm) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Set algorithm-specific key
    status_t status;
    
    switch (context->algorithm) {
        case ENCRYPTION_AES_128_GCM:
        case ENCRYPTION_AES_256_GCM:
            status = aes_functions.set_key(context->algorithm_context, key);
            break;
            
        case ENCRYPTION_CHACHA20_POLY1305:
            status = chacha20_functions.set_key(context->algorithm_context, key);
            break;
            
        default:
            return STATUS_ERROR_INVALID_PARAM;
    }
    
    if (status != STATUS_SUCCESS) {
        return status;
    }
    
    // Store key in context
    if (context->current_key != NULL) {
        free(context->current_key);
    }
    
    context->current_key = (encryption_key_t*)malloc(sizeof(encryption_key_t));
    if (context->current_key == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    memcpy(context->current_key, key, sizeof(encryption_key_t));
    
    return STATUS_SUCCESS;
}

/**
 * @brief Encrypt data
 */
status_t encryption_encrypt(encryption_context_t* context,
                          const uint8_t* plaintext, size_t plaintext_len,
                          uint8_t* ciphertext, size_t* ciphertext_len,
                          size_t max_ciphertext_len) {
    if (context == NULL || plaintext == NULL || ciphertext == NULL || ciphertext_len == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check if key is set
    if (context->current_key == NULL) {
        return STATUS_ERROR_NOT_INITIALIZED;
    }
    
    // Check if key is expired
    if (context->current_key->expire_time > 0 && 
        context->current_key->expire_time < (uint64_t)time(NULL)) {
        return STATUS_ERROR_KEY_EXPIRED;
    }
    
    // Encrypt using algorithm-specific function
    status_t status;
    
    switch (context->algorithm) {
        case ENCRYPTION_AES_128_GCM:
        case ENCRYPTION_AES_256_GCM:
            status = aes_functions.encrypt(context->algorithm_context, plaintext, plaintext_len,
                                         ciphertext, ciphertext_len, max_ciphertext_len);
            break;
            
        case ENCRYPTION_CHACHA20_POLY1305:
            status = chacha20_functions.encrypt(context->algorithm_context, plaintext, plaintext_len,
                                              ciphertext, ciphertext_len, max_ciphertext_len);
            break;
            
        default:
            return STATUS_ERROR_INVALID_PARAM;
    }
    
    return status;
}

/**
 * @brief Decrypt data
 */
status_t encryption_decrypt(encryption_context_t* context,
                          const uint8_t* ciphertext, size_t ciphertext_len,
                          uint8_t* plaintext, size_t* plaintext_len,
                          size_t max_plaintext_len) {
    if (context == NULL || ciphertext == NULL || plaintext == NULL || plaintext_len == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check if key is set
    if (context->current_key == NULL) {
        return STATUS_ERROR_NOT_INITIALIZED;
    }
    
    // Check if key is expired
    if (context->current_key->expire_time > 0 && 
        context->current_key->expire_time < (uint64_t)time(NULL)) {
        return STATUS_ERROR_KEY_EXPIRED;
    }
    
    // Decrypt using algorithm-specific function
    status_t status;
    
    switch (context->algorithm) {
        case ENCRYPTION_AES_128_GCM:
        case ENCRYPTION_AES_256_GCM:
            status = aes_functions.decrypt(context->algorithm_context, ciphertext, ciphertext_len,
                                         plaintext, plaintext_len, max_plaintext_len);
            break;
            
        case ENCRYPTION_CHACHA20_POLY1305:
            status = chacha20_functions.decrypt(context->algorithm_context, ciphertext, ciphertext_len,
                                              plaintext, plaintext_len, max_plaintext_len);
            break;
            
        default:
            return STATUS_ERROR_INVALID_PARAM;
    }
    
    return status;
}

/**
 * @brief Calculate entropy of data
 * 
 * Higher entropy indicates more randomness, which is typical of encrypted data.
 */
static float calculate_entropy(const uint8_t* data, size_t data_len) {
    if (data == NULL || data_len == 0) {
        return 0.0f;
    }
    
    // Count occurrences of each byte value
    size_t counts[256] = {0};
    
    for (size_t i = 0; i < data_len; i++) {
        counts[data[i]]++;
    }
    
    // Calculate entropy
    float entropy = 0.0f;
    
    for (size_t i = 0; i < 256; i++) {
        if (counts[i] > 0) {
            float probability = (float)counts[i] / data_len;
            entropy -= probability * log2f(probability);
        }
    }
    
    return entropy;
}

/**
 * @brief Detect encryption in data
 */
status_t encryption_detect(const uint8_t* data, size_t data_len,
                         encryption_detection_result_t* result) {
    if (data == NULL || data_len == 0 || result == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Initialize result
    memset(result, 0, sizeof(encryption_detection_result_t));
    
    // Calculate entropy
    float entropy = calculate_entropy(data, data_len);
    
    // Entropy threshold for encrypted data (empirical value)
    // Typical entropy values:
    // - Plain text: ~4.5-5.5
    // - Compressed data: ~6.5-7.5
    // - Encrypted data: ~7.8-8.0 (close to maximum of 8.0)
    const float ENTROPY_THRESHOLD = 7.5f;
    
    // Check if data is likely encrypted based on entropy
    if (entropy > ENTROPY_THRESHOLD) {
        result->is_encrypted = true;
        
        // Calculate confidence based on how close entropy is to maximum (8.0)
        result->confidence = (entropy - ENTROPY_THRESHOLD) / (8.0f - ENTROPY_THRESHOLD);
        if (result->confidence > 1.0f) {
            result->confidence = 1.0f;
        }
        
        // Try to detect specific algorithm (simplified)
        // In a real implementation, this would involve more sophisticated analysis
        
        // For now, just use a simple heuristic based on data patterns
        if (data_len >= 16 && data[0] == 0x00 && data[1] == 0x01) {
            result->detected_algorithm = ENCRYPTION_AES_128_GCM;
        } else if (data_len >= 16 && data[0] == 0x00 && data[1] == 0x02) {
            result->detected_algorithm = ENCRYPTION_AES_256_GCM;
        } else if (data_len >= 16 && data[0] == 0x00 && data[1] == 0x03) {
            result->detected_algorithm = ENCRYPTION_CHACHA20_POLY1305;
        } else {
            // Unknown algorithm
            result->detected_algorithm = ENCRYPTION_NONE;
        }
    } else {
        result->is_encrypted = false;
        result->detected_algorithm = ENCRYPTION_NONE;
        result->confidence = 0.0f;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Obfuscate data to avoid detection
 * 
 * This implementation uses a simple XOR-based obfuscation with a rolling key.
 * In a real-world scenario, more sophisticated techniques would be used.
 */
status_t encryption_obfuscate(const uint8_t* data, size_t data_len,
                            uint8_t* obfuscated_data, size_t* obfuscated_len,
                            size_t max_obfuscated_len) {
    if (data == NULL || data_len == 0 || obfuscated_data == NULL || 
        obfuscated_len == NULL || max_obfuscated_len < data_len) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Generate obfuscation key
    uint8_t key[16];
    for (size_t i = 0; i < sizeof(key); i++) {
        key[i] = (uint8_t)rand();
    }
    
    // Copy data to output buffer
    memcpy(obfuscated_data, data, data_len);
    
    // Apply XOR obfuscation with rolling key
    for (size_t i = 0; i < data_len; i++) {
        obfuscated_data[i] ^= key[i % sizeof(key)];
    }
    
    // Prepend key to obfuscated data
    if (max_obfuscated_len < data_len + sizeof(key)) {
        return STATUS_ERROR_BUFFER_TOO_SMALL;
    }
    
    // Shift data to make room for key
    memmove(obfuscated_data + sizeof(key), obfuscated_data, data_len);
    
    // Copy key to beginning of buffer
    memcpy(obfuscated_data, key, sizeof(key));
    
    *obfuscated_len = data_len + sizeof(key);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Deobfuscate data
 */
status_t encryption_deobfuscate(const uint8_t* obfuscated_data, size_t obfuscated_len,
                              uint8_t* data, size_t* data_len,
                              size_t max_data_len) {
    if (obfuscated_data == NULL || obfuscated_len <= 16 || data == NULL || 
        data_len == NULL || max_data_len < obfuscated_len - 16) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Extract key from beginning of obfuscated data
    uint8_t key[16];
    memcpy(key, obfuscated_data, sizeof(key));
    
    // Calculate deobfuscated data length
    size_t actual_data_len = obfuscated_len - sizeof(key);
    
    // Check if output buffer is large enough
    if (max_data_len < actual_data_len) {
        return STATUS_ERROR_BUFFER_TOO_SMALL;
    }
    
    // Copy obfuscated data to output buffer (excluding key)
    memcpy(data, obfuscated_data + sizeof(key), actual_data_len);
    
    // Apply XOR deobfuscation with extracted key
    for (size_t i = 0; i < actual_data_len; i++) {
        data[i] ^= key[i % sizeof(key)];
    }
    
    *data_len = actual_data_len;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Negotiate encryption key with peer
 * 
 * This is a simplified implementation of a Diffie-Hellman-like key exchange.
 * In a real-world scenario, a proper key exchange protocol would be used.
 */
status_t encryption_negotiate_key(encryption_algorithm_t algorithm,
                                encryption_key_t* key,
                                const uint8_t* peer_data, size_t peer_data_len,
                                uint8_t* response_data, size_t* response_len,
                                size_t max_response_len) {
    if (key == NULL || peer_data == NULL || peer_data_len == 0 || 
        response_data == NULL || response_len == NULL || max_response_len == 0) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check if encryption system is initialized
    if (!encryption_manager.initialized) {
        return STATUS_ERROR_NOT_RUNNING;
    }
    
    // In a real implementation, this would involve:
    // 1. Generating a key pair
    // 2. Processing peer's public key
    // 3. Deriving a shared secret
    // 4. Using the shared secret to generate the encryption key
    
    // For this simplified version, we'll just generate a random key
    // and use the peer data as a seed
    
    // Initialize key
    memset(key, 0, sizeof(encryption_key_t));
    key->algorithm = algorithm;
    
    // Set key size based on algorithm
    switch (algorithm) {
        case ENCRYPTION_AES_128_GCM:
            key->key_size = 16; // 128 bits
            key->iv_size = 12;  // 96 bits
            break;
            
        case ENCRYPTION_AES_256_GCM:
            key->key_size = 32; // 256 bits
            key->iv_size = 12;  // 96 bits
            break;
            
        case ENCRYPTION_CHACHA20_POLY1305:
            key->key_size = 32; // 256 bits
            key->iv_size = 12;  // 96 bits
            break;
            
        default:
            return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Use peer data to seed random number generator
    uint32_t seed = 0;
    for (size_t i = 0; i < peer_data_len; i++) {
        seed = seed * 31 + peer_data[i];
    }
    srand(seed);
    
    // Generate key
    for (size_t i = 0; i < key->key_size; i++) {
        key->key[i] = (uint8_t)rand();
    }
    
    // Generate IV
    for (size_t i = 0; i < key->iv_size; i++) {
        key->iv[i] = (uint8_t)rand();
    }
    
    // Set creation time
    key->created_time = (uint64_t)time(NULL);
    
    // Set expiration time (24 hours by default)
    key->expire_time = key->created_time + 24 * 60 * 60;
    
    // Generate response data (in a real implementation, this would be the public key)
    if (max_response_len < 32) {
        return STATUS_ERROR_BUFFER_TOO_SMALL;
    }
    
    for (size_t i = 0; i < 32 && i < max_response_len; i++) {
        response_data[i] = (uint8_t)rand();
    }
    
    *response_len = 32;
    
    return STATUS_SUCCESS;
}
