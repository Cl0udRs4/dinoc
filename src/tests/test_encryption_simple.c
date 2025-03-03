/**
 * @file test_encryption_simple.c
 * @brief Simplified test program for encryption detection functionality
 */

#include "../encryption/encryption.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Test data
#define TEST_PLAINTEXT "This is a test message for encryption detection. It should be long enough to provide meaningful entropy analysis."

/**
 * @brief Main function
 */
int main(int argc, char** argv) {
    printf("Starting simplified encryption detection test...\n");
    
    // Initialize encryption system
    status_t status = encryption_init();
    if (status != STATUS_SUCCESS) {
        printf("Failed to initialize encryption system: %d\n", status);
        return 1;
    }
    
    printf("Encryption system initialized successfully\n");
    
    // Test AES encryption
    encryption_context_t* aes_context = NULL;
    status = encryption_create_context(ENCRYPTION_AES_256_GCM, &aes_context);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to create AES context: %d\n", status);
        encryption_shutdown();
        return 1;
    }
    
    // Generate key
    encryption_key_t key;
    status = encryption_generate_key(ENCRYPTION_AES_256_GCM, &key, 3600);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to generate key: %d\n", status);
        encryption_destroy_context(aes_context);
        encryption_shutdown();
        return 1;
    }
    
    // Set key
    status = encryption_set_key(aes_context, &key);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to set key: %d\n", status);
        encryption_destroy_context(aes_context);
        encryption_shutdown();
        return 1;
    }
    
    // Encrypt data
    const uint8_t* plaintext = (const uint8_t*)TEST_PLAINTEXT;
    size_t plaintext_len = strlen(TEST_PLAINTEXT);
    
    uint8_t ciphertext[1024];
    size_t ciphertext_len = 0;
    
    status = encryption_encrypt(aes_context, plaintext, plaintext_len, 
                              ciphertext, &ciphertext_len, sizeof(ciphertext));
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to encrypt data: %d\n", status);
        encryption_destroy_context(aes_context);
        encryption_shutdown();
        return 1;
    }
    
    printf("Encryption successful: %zu bytes -> %zu bytes\n", plaintext_len, ciphertext_len);
    
    // Decrypt data
    uint8_t decrypted[1024];
    size_t decrypted_len = 0;
    
    status = encryption_decrypt(aes_context, ciphertext, ciphertext_len, 
                              decrypted, &decrypted_len, sizeof(decrypted));
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to decrypt data: %d\n", status);
        encryption_destroy_context(aes_context);
        encryption_shutdown();
        return 1;
    }
    
    // Verify decrypted data
    if (decrypted_len != plaintext_len) {
        printf("Decrypted length mismatch: expected %zu, got %zu\n", plaintext_len, decrypted_len);
        encryption_destroy_context(aes_context);
        encryption_shutdown();
        return 1;
    }
    
    if (memcmp(decrypted, plaintext, plaintext_len) != 0) {
        printf("Decrypted data does not match original plaintext\n");
        encryption_destroy_context(aes_context);
        encryption_shutdown();
        return 1;
    }
    
    printf("Decryption successful: %zu bytes -> %zu bytes\n", ciphertext_len, decrypted_len);
    
    // Test encryption detection
    encryption_detection_result_t result;
    status = encryption_detect(ciphertext, ciphertext_len, &result);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to detect encryption: %d\n", status);
        encryption_destroy_context(aes_context);
        encryption_shutdown();
        return 1;
    }
    
    printf("Encryption detection result: is_encrypted=%d, algorithm=%d, confidence=%.2f\n",
           result.is_encrypted, result.detected_algorithm, result.confidence);
    
    // Clean up
    encryption_destroy_context(aes_context);
    encryption_shutdown();
    
    printf("All encryption tests completed successfully\n");
    
    return 0;
}
