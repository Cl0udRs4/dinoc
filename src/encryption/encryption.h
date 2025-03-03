/**
 * @file encryption.h
 * @brief Encryption interface for C2 server
 */

#ifndef DINOC_ENCRYPTION_H
#define DINOC_ENCRYPTION_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../include/common.h"

// Encryption algorithms
typedef enum {
    ENCRYPTION_NONE = 0,
    ENCRYPTION_AES_128_GCM = 1,
    ENCRYPTION_AES_256_GCM = 2,
    ENCRYPTION_CHACHA20_POLY1305 = 3
} encryption_algorithm_t;

// Encryption key structure
typedef struct {
    encryption_algorithm_t algorithm;
    uint8_t key[32];        // Maximum key size (256 bits)
    uint8_t iv[16];         // Maximum IV size
    size_t key_size;        // Actual key size in bytes
    size_t iv_size;         // Actual IV size in bytes
    uint64_t created_time;  // Key creation timestamp
    uint64_t expire_time;   // Key expiration timestamp
} encryption_key_t;

// Encryption context
typedef struct {
    encryption_algorithm_t algorithm;
    encryption_key_t* current_key;
    void* algorithm_context;
} encryption_context_t;

// Encryption detection result
typedef struct {
    bool is_encrypted;
    encryption_algorithm_t detected_algorithm;
    float confidence;       // 0.0 to 1.0
} encryption_detection_result_t;

/**
 * @brief Initialize encryption system
 * 
 * @return status_t Status code
 */
status_t encryption_init(void);

/**
 * @brief Shutdown encryption system
 * 
 * @return status_t Status code
 */
status_t encryption_shutdown(void);

/**
 * @brief Create encryption context
 * 
 * @param algorithm Encryption algorithm
 * @param context Pointer to store created context
 * @return status_t Status code
 */
status_t encryption_create_context(encryption_algorithm_t algorithm, encryption_context_t** context);

/**
 * @brief Destroy encryption context
 * 
 * @param context Encryption context
 * @return status_t Status code
 */
status_t encryption_destroy_context(encryption_context_t* context);

/**
 * @brief Generate encryption key
 * 
 * @param algorithm Encryption algorithm
 * @param key Pointer to store generated key
 * @param expire_seconds Key expiration time in seconds (0 for no expiration)
 * @return status_t Status code
 */
status_t encryption_generate_key(encryption_algorithm_t algorithm, encryption_key_t* key, uint64_t expire_seconds);

/**
 * @brief Set encryption key for context
 * 
 * @param context Encryption context
 * @param key Encryption key
 * @return status_t Status code
 */
status_t encryption_set_key(encryption_context_t* context, const encryption_key_t* key);

/**
 * @brief Encrypt data
 * 
 * @param context Encryption context
 * @param plaintext Plaintext data
 * @param plaintext_len Plaintext length
 * @param ciphertext Output buffer for ciphertext
 * @param ciphertext_len Pointer to store ciphertext length
 * @param max_ciphertext_len Maximum ciphertext buffer size
 * @return status_t Status code
 */
status_t encryption_encrypt(encryption_context_t* context,
                          const uint8_t* plaintext, size_t plaintext_len,
                          uint8_t* ciphertext, size_t* ciphertext_len,
                          size_t max_ciphertext_len);

/**
 * @brief Decrypt data
 * 
 * @param context Encryption context
 * @param ciphertext Ciphertext data
 * @param ciphertext_len Ciphertext length
 * @param plaintext Output buffer for plaintext
 * @param plaintext_len Pointer to store plaintext length
 * @param max_plaintext_len Maximum plaintext buffer size
 * @return status_t Status code
 */
status_t encryption_decrypt(encryption_context_t* context,
                          const uint8_t* ciphertext, size_t ciphertext_len,
                          uint8_t* plaintext, size_t* plaintext_len,
                          size_t max_plaintext_len);

/**
 * @brief Detect encryption in data
 * 
 * @param data Data to analyze
 * @param data_len Data length
 * @param result Pointer to store detection result
 * @return status_t Status code
 */
status_t encryption_detect(const uint8_t* data, size_t data_len,
                         encryption_detection_result_t* result);

/**
 * @brief Obfuscate data to avoid detection
 * 
 * @param data Data to obfuscate
 * @param data_len Data length
 * @param obfuscated_data Output buffer for obfuscated data
 * @param obfuscated_len Pointer to store obfuscated length
 * @param max_obfuscated_len Maximum obfuscated buffer size
 * @return status_t Status code
 */
status_t encryption_obfuscate(const uint8_t* data, size_t data_len,
                            uint8_t* obfuscated_data, size_t* obfuscated_len,
                            size_t max_obfuscated_len);

/**
 * @brief Deobfuscate data
 * 
 * @param obfuscated_data Obfuscated data
 * @param obfuscated_len Obfuscated length
 * @param data Output buffer for deobfuscated data
 * @param data_len Pointer to store deobfuscated length
 * @param max_data_len Maximum deobfuscated buffer size
 * @return status_t Status code
 */
status_t encryption_deobfuscate(const uint8_t* obfuscated_data, size_t obfuscated_len,
                              uint8_t* data, size_t* data_len,
                              size_t max_data_len);

/**
 * @brief Negotiate encryption key with peer
 * 
 * @param algorithm Preferred encryption algorithm
 * @param key Pointer to store negotiated key
 * @param peer_data Peer data for key negotiation
 * @param peer_data_len Peer data length
 * @param response_data Output buffer for response data
 * @param response_len Pointer to store response length
 * @param max_response_len Maximum response buffer size
 * @return status_t Status code
 */
status_t encryption_negotiate_key(encryption_algorithm_t algorithm,
                                encryption_key_t* key,
                                const uint8_t* peer_data, size_t peer_data_len,
                                uint8_t* response_data, size_t* response_len,
                                size_t max_response_len);

#endif /* DINOC_ENCRYPTION_H */
