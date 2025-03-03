/**
 * @file test_encryption_detection.c
 * @brief Test program for encryption detection functionality
 */

#include "../encryption/encryption.h"
#include "../include/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Test data
#define TEST_PLAINTEXT "This is a test message for encryption detection. It should be long enough to provide meaningful entropy analysis."
#define TEST_ITERATIONS 3

// Forward declarations
static void test_encryption_init_shutdown(void);
static void test_encryption_key_generation(void);
static void test_aes_encryption_decryption(void);
static void test_chacha20_encryption_decryption(void);
static void test_encryption_detection(void);
static void test_encryption_obfuscation(void);
static void test_key_negotiation(void);

/**
 * @brief Main function
 */
int main(int argc, char** argv) {
    // Seed random number generator
    srand((unsigned int)time(NULL));
    
    // Run tests
    test_encryption_init_shutdown();
    test_encryption_key_generation();
    test_aes_encryption_decryption();
    test_chacha20_encryption_decryption();
    test_encryption_detection();
    test_encryption_obfuscation();
    test_key_negotiation();
    
    printf("All encryption tests completed successfully\n");
    
    return 0;
}

/**
 * @brief Test encryption initialization and shutdown
 */
static void test_encryption_init_shutdown(void) {
    printf("Testing encryption initialization and shutdown...\n");
    
    // Initialize encryption system
    status_t status = encryption_init();
    if (status != STATUS_SUCCESS) {
        printf("Failed to initialize encryption system: %d\n", status);
        exit(1);
    }
    
    printf("Encryption system initialized successfully\n");
    
    // Shutdown encryption system
    status = encryption_shutdown();
    if (status != STATUS_SUCCESS) {
        printf("Failed to shutdown encryption system: %d\n", status);
        exit(1);
    }
    
    printf("Encryption system shutdown successfully\n");
    
    // Re-initialize for subsequent tests
    status = encryption_init();
    if (status != STATUS_SUCCESS) {
        printf("Failed to re-initialize encryption system: %d\n", status);
        exit(1);
    }
}

/**
 * @brief Test encryption key generation
 */
static void test_encryption_key_generation(void) {
    printf("Testing encryption key generation...\n");
    
    // Test key generation for each algorithm
    encryption_algorithm_t algorithms[] = {
        ENCRYPTION_AES_128_GCM,
        ENCRYPTION_AES_256_GCM,
        ENCRYPTION_CHACHA20_POLY1305
    };
    
    for (size_t i = 0; i < sizeof(algorithms) / sizeof(algorithms[0]); i++) {
        encryption_key_t key;
        
        // Generate key
        status_t status = encryption_generate_key(algorithms[i], &key, 3600); // 1 hour expiration
        
        if (status != STATUS_SUCCESS) {
            printf("Failed to generate key for algorithm %d: %d\n", algorithms[i], status);
            exit(1);
        }
        
        // Verify key properties
        if (key.algorithm != algorithms[i]) {
            printf("Key algorithm mismatch: expected %d, got %d\n", algorithms[i], key.algorithm);
            exit(1);
        }
        
        // Verify key size
        size_t expected_key_size = 0;
        switch (algorithms[i]) {
            case ENCRYPTION_AES_128_GCM:
                expected_key_size = 16;
                break;
            case ENCRYPTION_AES_256_GCM:
            case ENCRYPTION_CHACHA20_POLY1305:
                expected_key_size = 32;
                break;
            default:
                printf("Unknown algorithm: %d\n", algorithms[i]);
                exit(1);
        }
        
        if (key.key_size != expected_key_size) {
            printf("Key size mismatch: expected %zu, got %zu\n", expected_key_size, key.key_size);
            exit(1);
        }
        
        // Verify expiration time
        if (key.expire_time <= key.created_time) {
            printf("Invalid expiration time: created=%llu, expires=%llu\n", 
                   (unsigned long long)key.created_time, (unsigned long long)key.expire_time);
            exit(1);
        }
        
        printf("Key generation for algorithm %d successful\n", algorithms[i]);
    }
}

/**
 * @brief Test AES encryption and decryption
 */
static void test_aes_encryption_decryption(void) {
    printf("Testing AES encryption and decryption...\n");
    
    // Test both AES-128-GCM and AES-256-GCM
    encryption_algorithm_t algorithms[] = {
        ENCRYPTION_AES_128_GCM,
        ENCRYPTION_AES_256_GCM
    };
    
    for (size_t i = 0; i < sizeof(algorithms) / sizeof(algorithms[0]); i++) {
        // Create encryption context
        encryption_context_t* context = NULL;
        status_t status = encryption_create_context(algorithms[i], &context);
        
        if (status != STATUS_SUCCESS) {
            printf("Failed to create encryption context for algorithm %d: %d\n", algorithms[i], status);
            exit(1);
        }
        
        // Generate key
        encryption_key_t key;
        status = encryption_generate_key(algorithms[i], &key, 3600); // 1 hour expiration
        
        if (status != STATUS_SUCCESS) {
            printf("Failed to generate key for algorithm %d: %d\n", algorithms[i], status);
            encryption_destroy_context(context);
            exit(1);
        }
        
        // Set key
        status = encryption_set_key(context, &key);
        
        if (status != STATUS_SUCCESS) {
            printf("Failed to set key for algorithm %d: %d\n", algorithms[i], status);
            encryption_destroy_context(context);
            exit(1);
        }
        
        // Encrypt data
        const uint8_t* plaintext = (const uint8_t*)TEST_PLAINTEXT;
        size_t plaintext_len = strlen(TEST_PLAINTEXT);
        
        uint8_t ciphertext[1024];
        size_t ciphertext_len = 0;
        
        status = encryption_encrypt(context, plaintext, plaintext_len, 
                                  ciphertext, &ciphertext_len, sizeof(ciphertext));
        
        if (status != STATUS_SUCCESS) {
            printf("Failed to encrypt data for algorithm %d: %d\n", algorithms[i], status);
            encryption_destroy_context(context);
            exit(1);
        }
        
        printf("Encryption successful: %zu bytes -> %zu bytes\n", plaintext_len, ciphertext_len);
        
        // Decrypt data
        uint8_t decrypted[1024];
        size_t decrypted_len = 0;
        
        status = encryption_decrypt(context, ciphertext, ciphertext_len, 
                                  decrypted, &decrypted_len, sizeof(decrypted));
        
        if (status != STATUS_SUCCESS) {
            printf("Failed to decrypt data for algorithm %d: %d\n", algorithms[i], status);
            encryption_destroy_context(context);
            exit(1);
        }
        
        // Verify decrypted data
        if (decrypted_len != plaintext_len) {
            printf("Decrypted length mismatch: expected %zu, got %zu\n", plaintext_len, decrypted_len);
            encryption_destroy_context(context);
            exit(1);
        }
        
        if (memcmp(decrypted, plaintext, plaintext_len) != 0) {
            printf("Decrypted data does not match original plaintext\n");
            encryption_destroy_context(context);
            exit(1);
        }
        
        printf("Decryption successful: %zu bytes -> %zu bytes\n", ciphertext_len, decrypted_len);
        
        // Clean up
        encryption_destroy_context(context);
        
        printf("AES-%s-GCM encryption/decryption test passed\n", 
               algorithms[i] == ENCRYPTION_AES_128_GCM ? "128" : "256");
    }
}

/**
 * @brief Test ChaCha20 encryption and decryption
 */
static void test_chacha20_encryption_decryption(void) {
    printf("Testing ChaCha20-Poly1305 encryption and decryption...\n");
    
    // Create encryption context
    encryption_context_t* context = NULL;
    status_t status = encryption_create_context(ENCRYPTION_CHACHA20_POLY1305, &context);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to create encryption context for ChaCha20-Poly1305: %d\n", status);
        exit(1);
    }
    
    // Generate key
    encryption_key_t key;
    status = encryption_generate_key(ENCRYPTION_CHACHA20_POLY1305, &key, 3600); // 1 hour expiration
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to generate key for ChaCha20-Poly1305: %d\n", status);
        encryption_destroy_context(context);
        exit(1);
    }
    
    // Set key
    status = encryption_set_key(context, &key);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to set key for ChaCha20-Poly1305: %d\n", status);
        encryption_destroy_context(context);
        exit(1);
    }
    
    // Encrypt data
    const uint8_t* plaintext = (const uint8_t*)TEST_PLAINTEXT;
    size_t plaintext_len = strlen(TEST_PLAINTEXT);
    
    uint8_t ciphertext[1024];
    size_t ciphertext_len = 0;
    
    status = encryption_encrypt(context, plaintext, plaintext_len, 
                              ciphertext, &ciphertext_len, sizeof(ciphertext));
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to encrypt data for ChaCha20-Poly1305: %d\n", status);
        encryption_destroy_context(context);
        exit(1);
    }
    
    printf("Encryption successful: %zu bytes -> %zu bytes\n", plaintext_len, ciphertext_len);
    
    // Decrypt data
    uint8_t decrypted[1024];
    size_t decrypted_len = 0;
    
    status = encryption_decrypt(context, ciphertext, ciphertext_len, 
                              decrypted, &decrypted_len, sizeof(decrypted));
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to decrypt data for ChaCha20-Poly1305: %d\n", status);
        encryption_destroy_context(context);
        exit(1);
    }
    
    // Verify decrypted data
    if (decrypted_len != plaintext_len) {
        printf("Decrypted length mismatch: expected %zu, got %zu\n", plaintext_len, decrypted_len);
        encryption_destroy_context(context);
        exit(1);
    }
    
    if (memcmp(decrypted, plaintext, plaintext_len) != 0) {
        printf("Decrypted data does not match original plaintext\n");
        encryption_destroy_context(context);
        exit(1);
    }
    
    printf("Decryption successful: %zu bytes -> %zu bytes\n", ciphertext_len, decrypted_len);
    
    // Clean up
    encryption_destroy_context(context);
    
    printf("ChaCha20-Poly1305 encryption/decryption test passed\n");
}

/**
 * @brief Test encryption detection
 */
static void test_encryption_detection(void) {
    printf("Testing encryption detection...\n");
    
    // Test detection on plaintext
    encryption_detection_result_t result;
    status_t status = encryption_detect((const uint8_t*)TEST_PLAINTEXT, strlen(TEST_PLAINTEXT), &result);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to detect encryption in plaintext: %d\n", status);
        exit(1);
    }
    
    printf("Plaintext detection: is_encrypted=%d, algorithm=%d, confidence=%.2f\n",
           result.is_encrypted, result.detected_algorithm, result.confidence);
    
    // Test detection on encrypted data
    encryption_algorithm_t algorithms[] = {
        ENCRYPTION_AES_128_GCM,
        ENCRYPTION_AES_256_GCM,
        ENCRYPTION_CHACHA20_POLY1305
    };
    
    for (size_t i = 0; i < sizeof(algorithms) / sizeof(algorithms[0]); i++) {
        // Create encryption context
        encryption_context_t* context = NULL;
        status = encryption_create_context(algorithms[i], &context);
        
        if (status != STATUS_SUCCESS) {
            printf("Failed to create encryption context for algorithm %d: %d\n", algorithms[i], status);
            exit(1);
        }
        
        // Generate key
        encryption_key_t key;
        status = encryption_generate_key(algorithms[i], &key, 3600); // 1 hour expiration
        
        if (status != STATUS_SUCCESS) {
            printf("Failed to generate key for algorithm %d: %d\n", algorithms[i], status);
            encryption_destroy_context(context);
            exit(1);
        }
        
        // Set key
        status = encryption_set_key(context, &key);
        
        if (status != STATUS_SUCCESS) {
            printf("Failed to set key for algorithm %d: %d\n", algorithms[i], status);
            encryption_destroy_context(context);
            exit(1);
        }
        
        // Encrypt data
        const uint8_t* plaintext = (const uint8_t*)TEST_PLAINTEXT;
        size_t plaintext_len = strlen(TEST_PLAINTEXT);
        
        uint8_t ciphertext[1024];
        size_t ciphertext_len = 0;
        
        status = encryption_encrypt(context, plaintext, plaintext_len, 
                                  ciphertext, &ciphertext_len, sizeof(ciphertext));
        
        if (status != STATUS_SUCCESS) {
            printf("Failed to encrypt data for algorithm %d: %d\n", algorithms[i], status);
            encryption_destroy_context(context);
            exit(1);
        }
        
        // Detect encryption
        status = encryption_detect(ciphertext, ciphertext_len, &result);
        
        if (status != STATUS_SUCCESS) {
            printf("Failed to detect encryption in ciphertext for algorithm %d: %d\n", algorithms[i], status);
            encryption_destroy_context(context);
            exit(1);
        }
        
        printf("Ciphertext detection (algorithm %d): is_encrypted=%d, algorithm=%d, confidence=%.2f\n",
               algorithms[i], result.is_encrypted, result.detected_algorithm, result.confidence);
        
        // Clean up
        encryption_destroy_context(context);
    }
    
    printf("Encryption detection test passed\n");
}

/**
 * @brief Test encryption obfuscation
 */
static void test_encryption_obfuscation(void) {
    printf("Testing encryption obfuscation...\n");
    
    // Test data
    const uint8_t* data = (const uint8_t*)TEST_PLAINTEXT;
    size_t data_len = strlen(TEST_PLAINTEXT);
    
    // Obfuscate data
    uint8_t obfuscated[1024];
    size_t obfuscated_len = 0;
    
    status_t status = encryption_obfuscate(data, data_len, obfuscated, &obfuscated_len, sizeof(obfuscated));
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to obfuscate data: %d\n", status);
        exit(1);
    }
    
    printf("Obfuscation successful: %zu bytes -> %zu bytes\n", data_len, obfuscated_len);
    
    // Deobfuscate data
    uint8_t deobfuscated[1024];
    size_t deobfuscated_len = 0;
    
    status = encryption_deobfuscate(obfuscated, obfuscated_len, deobfuscated, &deobfuscated_len, sizeof(deobfuscated));
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to deobfuscate data: %d\n", status);
        exit(1);
    }
    
    // Verify deobfuscated data
    if (deobfuscated_len != data_len) {
        printf("Deobfuscated length mismatch: expected %zu, got %zu\n", data_len, deobfuscated_len);
        exit(1);
    }
    
    if (memcmp(deobfuscated, data, data_len) != 0) {
        printf("Deobfuscated data does not match original data\n");
        exit(1);
    }
    
    printf("Deobfuscation successful: %zu bytes -> %zu bytes\n", obfuscated_len, deobfuscated_len);
    
    // Test encryption detection on obfuscated data
    encryption_detection_result_t result;
    status = encryption_detect(obfuscated, obfuscated_len, &result);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to detect encryption in obfuscated data: %d\n", status);
        exit(1);
    }
    
    printf("Obfuscated data detection: is_encrypted=%d, algorithm=%d, confidence=%.2f\n",
           result.is_encrypted, result.detected_algorithm, result.confidence);
    
    printf("Encryption obfuscation test passed\n");
}

/**
 * @brief Test key negotiation
 */
static void test_key_negotiation(void) {
    printf("Testing key negotiation...\n");
    
    // Test key negotiation for each algorithm
    encryption_algorithm_t algorithms[] = {
        ENCRYPTION_AES_128_GCM,
        ENCRYPTION_AES_256_GCM,
        ENCRYPTION_CHACHA20_POLY1305
    };
    
    for (size_t i = 0; i < sizeof(algorithms) / sizeof(algorithms[0]); i++) {
        // Generate peer data
        uint8_t peer_data[32];
        for (size_t j = 0; j < sizeof(peer_data); j++) {
            peer_data[j] = (uint8_t)rand();
        }
        
        // Negotiate key
        encryption_key_t key;
        uint8_t response_data[32];
        size_t response_len = 0;
        
        status_t status = encryption_negotiate_key(algorithms[i], &key, peer_data, sizeof(peer_data),
                                                 response_data, &response_len, sizeof(response_data));
        
        if (status != STATUS_SUCCESS) {
            printf("Failed to negotiate key for algorithm %d: %d\n", algorithms[i], status);
            exit(1);
        }
        
        // Verify key properties
        if (key.algorithm != algorithms[i]) {
            printf("Key algorithm mismatch: expected %d, got %d\n", algorithms[i], key.algorithm);
            exit(1);
        }
        
        // Verify key size
        size_t expected_key_size = 0;
        switch (algorithms[i]) {
            case ENCRYPTION_AES_128_GCM:
                expected_key_size = 16;
                break;
            case ENCRYPTION_AES_256_GCM:
            case ENCRYPTION_CHACHA20_POLY1305:
                expected_key_size = 32;
                break;
            default:
                printf("Unknown algorithm: %d\n", algorithms[i]);
                exit(1);
        }
        
        if (key.key_size != expected_key_size) {
            printf("Key size mismatch: expected %zu, got %zu\n", expected_key_size, key.key_size);
            exit(1);
        }
        
        // Verify response data
        if (response_len == 0) {
            printf("Empty response data\n");
            exit(1);
        }
        
        printf("Key negotiation for algorithm %d successful\n", algorithms[i]);
    }
    
    printf("Key negotiation test passed\n");
}
