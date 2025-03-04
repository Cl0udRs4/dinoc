/**
 * @file test_builder.c
 * @brief Tests for the builder tool
 */

#define _GNU_SOURCE  /* For strdup */

#include "../../builder/builder.h"
#include "../../builder/template_generator.h"
#include "../../builder/signature.h"
#include "../../include/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Test helper functions
static void test_parse_protocols(void);
static void test_parse_servers(void);
static void test_parse_modules(void);
static void test_parse_encryption(void);
static void test_builder_config(void);
static void test_template_generation(void);
static void test_signature_verification(void);

int main(int argc, char** argv) {
    printf("Running builder tests...\n");
    
    // Initialize builder
    status_t status = builder_init();
    assert(status == STATUS_SUCCESS);
    
    // Initialize template generator
    status = template_generator_init();
    assert(status == STATUS_SUCCESS);
    
    // Initialize signature module
    status = signature_init();
    assert(status == STATUS_SUCCESS);
    
    // Run tests
    test_parse_protocols();
    test_parse_servers();
    test_parse_modules();
    test_parse_encryption();
    test_builder_config();
    test_template_generation();
    test_signature_verification();
    
    // Shutdown modules
    signature_shutdown();
    template_generator_shutdown();
    builder_shutdown();
    
    printf("All builder tests passed!\n");
    
    return 0;
}

/**
 * @brief Test protocol parsing
 */
static void test_parse_protocols(void) {
    printf("Testing protocol parsing...\n");
    
    // Test valid protocols
    const char* valid_protocols[] = {
        "tcp",
        "udp",
        "ws",
        "icmp",
        "dns",
        "tcp,udp",
        "tcp,udp,ws,icmp,dns"
    };
    
    for (size_t i = 0; i < sizeof(valid_protocols) / sizeof(valid_protocols[0]); i++) {
        protocol_type_t* protocols = NULL;
        size_t count = 0;
        
        status_t status = builder_parse_args_test_helper(valid_protocols[i], &protocols, &count);
        assert(status == STATUS_SUCCESS);
        assert(protocols != NULL);
        assert(count > 0);
        
        // Verify protocol types
        const char* protocol_list = valid_protocols[i];
        size_t expected_count = 1;
        
        for (const char* p = protocol_list; *p != '\0'; p++) {
            if (*p == ',') {
                expected_count++;
            }
        }
        
        assert(count == expected_count);
        
        free(protocols);
    }
    
    // Test invalid protocols
    const char* invalid_protocols[] = {
        "",
        "invalid",
        "tcp,invalid",
        "tcp,,udp",
        ",tcp",
        "tcp,"
    };
    
    for (size_t i = 0; i < sizeof(invalid_protocols) / sizeof(invalid_protocols[0]); i++) {
        protocol_type_t* protocols = NULL;
        size_t count = 0;
        
        status_t status = builder_parse_args_test_helper(invalid_protocols[i], &protocols, &count);
        assert(status != STATUS_SUCCESS);
    }
    
    printf("Protocol parsing tests passed!\n");
}

/**
 * @brief Test server parsing
 */
static void test_parse_servers(void) {
    printf("Testing server parsing...\n");
    
    // Test valid servers
    const char* valid_servers[] = {
        "127.0.0.1:8080",
        "example.com:80",
        "127.0.0.1:8080,127.0.0.1:53",
        "example.com:80,test.com:443,127.0.0.1:8080"
    };
    
    for (size_t i = 0; i < sizeof(valid_servers) / sizeof(valid_servers[0]); i++) {
        char** servers = NULL;
        size_t count = 0;
        
        status_t status = builder_parse_servers_test_helper(valid_servers[i], &servers, &count);
        assert(status == STATUS_SUCCESS);
        assert(servers != NULL);
        assert(count > 0);
        
        // Verify server count
        const char* server_list = valid_servers[i];
        size_t expected_count = 1;
        
        for (const char* p = server_list; *p != '\0'; p++) {
            if (*p == ',') {
                expected_count++;
            }
        }
        
        assert(count == expected_count);
        
        // Clean up
        for (size_t j = 0; j < count; j++) {
            free(servers[j]);
        }
        
        free(servers);
    }
    
    // Test invalid servers
    const char* invalid_servers[] = {
        "",
        "127.0.0.1",
        "example.com",
        "127.0.0.1:8080,127.0.0.1",
        "127.0.0.1:8080,,127.0.0.1:53",
        ",127.0.0.1:8080",
        "127.0.0.1:8080,"
    };
    
    for (size_t i = 0; i < sizeof(invalid_servers) / sizeof(invalid_servers[0]); i++) {
        char** servers = NULL;
        size_t count = 0;
        
        status_t status = builder_parse_servers_test_helper(invalid_servers[i], &servers, &count);
        assert(status != STATUS_SUCCESS);
    }
    
    printf("Server parsing tests passed!\n");
}

/**
 * @brief Test module parsing
 */
static void test_parse_modules(void) {
    printf("Testing module parsing...\n");
    
    // Test valid modules
    const char* valid_modules[] = {
        "shell",
        "file",
        "keylogger",
        "screenshot",
        "shell,file",
        "shell,file,keylogger,screenshot"
    };
    
    for (size_t i = 0; i < sizeof(valid_modules) / sizeof(valid_modules[0]); i++) {
        char** modules = NULL;
        size_t count = 0;
        
        status_t status = builder_parse_modules_test_helper(valid_modules[i], &modules, &count);
        assert(status == STATUS_SUCCESS);
        assert(modules != NULL);
        assert(count > 0);
        
        // Verify module count
        const char* module_list = valid_modules[i];
        size_t expected_count = 1;
        
        for (const char* p = module_list; *p != '\0'; p++) {
            if (*p == ',') {
                expected_count++;
            }
        }
        
        assert(count == expected_count);
        
        // Clean up
        for (size_t j = 0; j < count; j++) {
            free(modules[j]);
        }
        
        free(modules);
    }
    
    // Test invalid modules (note: unknown modules are allowed with a warning)
    const char* invalid_modules[] = {
        "",
        "shell,,file",
        ",shell",
        "shell,"
    };
    
    for (size_t i = 0; i < sizeof(invalid_modules) / sizeof(invalid_modules[0]); i++) {
        char** modules = NULL;
        size_t count = 0;
        
        status_t status = builder_parse_modules_test_helper(invalid_modules[i], &modules, &count);
        assert(status != STATUS_SUCCESS);
    }
    
    printf("Module parsing tests passed!\n");
}

/**
 * @brief Test encryption parsing
 */
static void test_parse_encryption(void) {
    printf("Testing encryption parsing...\n");
    
    // Test valid encryption algorithms
    const char* valid_encryption[] = {
        "none",
        "aes128",
        "aes256",
        "chacha20"
    };
    
    encryption_algorithm_t expected_algorithms[] = {
        ENCRYPTION_NONE,
        ENCRYPTION_AES_128_GCM,
        ENCRYPTION_AES_256_GCM,
        ENCRYPTION_CHACHA20_POLY1305
    };
    
    for (size_t i = 0; i < sizeof(valid_encryption) / sizeof(valid_encryption[0]); i++) {
        encryption_algorithm_t algorithm;
        
        status_t status = builder_parse_encryption_test_helper(valid_encryption[i], &algorithm);
        assert(status == STATUS_SUCCESS);
        assert(algorithm == expected_algorithms[i]);
    }
    
    // Test invalid encryption algorithms
    const char* invalid_encryption[] = {
        "",
        "invalid",
        "aes",
        "chacha"
    };
    
    for (size_t i = 0; i < sizeof(invalid_encryption) / sizeof(invalid_encryption[0]); i++) {
        encryption_algorithm_t algorithm;
        
        status_t status = builder_parse_encryption_test_helper(invalid_encryption[i], &algorithm);
        assert(status != STATUS_SUCCESS);
    }
    
    printf("Encryption parsing tests passed!\n");
}

/**
 * @brief Test builder configuration
 */
static void test_builder_config(void) {
    printf("Testing builder configuration...\n");
    
    // Test valid configuration
    const char* argv[] = {
        "builder",
        "-p", "tcp,dns",
        "-s", "127.0.0.1:8080,127.0.0.1:53",
        "-d", "test.com",
        "-m", "shell",
        "-e", "aes256",
        "-o", "test_client",
        "-g",
        "-v", "1.2.3",
        "-n",
        "-y"
    };
    
    int argc = sizeof(argv) / sizeof(argv[0]);
    
    builder_config_t config;
    status_t status = builder_parse_args(argc, (char**)argv, &config);
    
    assert(status == STATUS_SUCCESS);
    assert(config.protocol_count == 2);
    assert(config.server_count == 2);
    assert(strcmp(config.domain, "test.com") == 0);
    assert(config.module_count == 1);
    assert(config.encryption_algorithm == ENCRYPTION_AES_256_GCM);
    assert(strcmp(config.output_file, "test_client") == 0);
    assert(config.debug_mode == true);
    assert(config.version_major == 1);
    assert(config.version_minor == 2);
    assert(config.version_patch == 3);
    assert(config.sign_binary == false);
    assert(config.verify_signature == false);
    
    builder_clean_config(&config);
    
    // Test missing required parameters
    const char* missing_protocol[] = {
        "builder",
        "-s", "127.0.0.1:8080",
        "-d", "test.com",
        "-m", "shell"
    };
    
    argc = sizeof(missing_protocol) / sizeof(missing_protocol[0]);
    status = builder_parse_args(argc, (char**)missing_protocol, &config);
    assert(status != STATUS_SUCCESS);
    
    const char* missing_server[] = {
        "builder",
        "-p", "tcp",
        "-d", "test.com",
        "-m", "shell"
    };
    
    argc = sizeof(missing_server) / sizeof(missing_server[0]);
    status = builder_parse_args(argc, (char**)missing_server, &config);
    assert(status != STATUS_SUCCESS);
    
    const char* missing_domain[] = {
        "builder",
        "-p", "tcp,dns",
        "-s", "127.0.0.1:8080,127.0.0.1:53",
        "-m", "shell"
    };
    
    argc = sizeof(missing_domain) / sizeof(missing_domain[0]);
    status = builder_parse_args(argc, (char**)missing_domain, &config);
    assert(status != STATUS_SUCCESS);
    
    printf("Builder configuration tests passed!\n");
}

/**
 * @brief Test template generation
 */
static void test_template_generation(void) {
    printf("Testing template generation...\n");
    
    // Create a test configuration
    builder_config_t config;
    memset(&config, 0, sizeof(builder_config_t));
    
    // Set up protocols
    config.protocol_count = 2;
    config.protocols = (protocol_type_t*)malloc(config.protocol_count * sizeof(protocol_type_t));
    assert(config.protocols != NULL);
    config.protocols[0] = PROTOCOL_TYPE_TCP;
    config.protocols[1] = PROTOCOL_TYPE_DNS;
    
    // Set up servers
    config.server_count = 2;
    config.servers = (char**)malloc(config.server_count * sizeof(char*));
    assert(config.servers != NULL);
    config.servers[0] = strdup("127.0.0.1:8080");
    config.servers[1] = strdup("127.0.0.1:53");
    assert(config.servers[0] != NULL);
    assert(config.servers[1] != NULL);
    
    // Set up domain
    config.domain = strdup("test.com");
    assert(config.domain != NULL);
    
    // Set up modules
    config.module_count = 1;
    config.modules = (char**)malloc(config.module_count * sizeof(char*));
    assert(config.modules != NULL);
    config.modules[0] = strdup("shell");
    assert(config.modules[0] != NULL);
    
    // Set up encryption
    config.encryption_algorithm = ENCRYPTION_AES_256_GCM;
    
    // Set up output file
    config.output_file = strdup("test_client");
    assert(config.output_file != NULL);
    
    // Set up version
    config.version_major = 1;
    config.version_minor = 2;
    config.version_patch = 3;
    
    // Set up debug mode
    config.debug_mode = true;
    
    // Set up signing
    config.sign_binary = true;
    config.verify_signature = true;
    
    // Generate template
    status_t status = template_generator_generate(&config, "test_client.c");
    assert(status == STATUS_SUCCESS);
    
    // Verify that the file was created
    FILE* file = fopen("test_client.c", "r");
    assert(file != NULL);
    fclose(file);
    
    // Clean up
    builder_clean_config(&config);
    
    // Remove test file
    remove("test_client.c");
    
    printf("Template generation tests passed!\n");
}

/**
 * @brief Test signature verification
 */
static void test_signature_verification(void) {
    printf("Testing signature verification...\n");
    
    // Create test data
    const char* test_data = "This is test data for signature verification";
    size_t test_data_len = strlen(test_data);
    
    // Sign test data
    client_signature_t signature;
    status_t status = signature_sign_client((const uint8_t*)test_data, test_data_len,
                                          1, 2, 3, &signature);
    assert(status == STATUS_SUCCESS);
    
    // Verify signature
    status = signature_verify_client((const uint8_t*)test_data, test_data_len, &signature);
    assert(status == STATUS_SUCCESS);
    
    // Modify test data and verify that signature fails
    char* modified_data = strdup(test_data);
    assert(modified_data != NULL);
    modified_data[0] = 'X';  // Change first character
    
    status = signature_verify_client((const uint8_t*)modified_data, test_data_len, &signature);
    assert(status != STATUS_SUCCESS);
    
    free(modified_data);
    
    // Test append and extract
    uint8_t output[1024];
    size_t output_len = 0;
    
    status = signature_append_to_client((const uint8_t*)test_data, test_data_len,
                                      &signature, output, &output_len, sizeof(output));
    assert(status == STATUS_SUCCESS);
    assert(output_len > test_data_len);
    
    client_signature_t extracted_sig;
    uint8_t original_data[1024];
    size_t original_len = 0;
    
    status = signature_extract_from_client(output, output_len,
                                        &extracted_sig, original_data, &original_len,
                                        sizeof(original_data));
    assert(status == STATUS_SUCCESS);
    assert(original_len == test_data_len);
    assert(memcmp(original_data, test_data, test_data_len) == 0);
    assert(memcmp(&extracted_sig, &signature, sizeof(signature)) == 0);
    
    printf("Signature verification tests passed!\n");
}

// Test helper functions (to be implemented in builder.c)
extern status_t builder_parse_args_test_helper(const char* protocols_str, protocol_type_t** protocols, size_t* count);
extern status_t builder_parse_servers_test_helper(const char* servers_str, char*** servers, size_t* count);
extern status_t builder_parse_modules_test_helper(const char* modules_str, char*** modules, size_t* count);
extern status_t builder_parse_encryption_test_helper(const char* encryption_str, encryption_algorithm_t* algorithm);
