/**
 * @file builder.h
 * @brief Builder tool for generating customized C2 clients
 */

#ifndef DINOC_BUILDER_H
#define DINOC_BUILDER_H

#include "../include/common.h"
#include "../include/protocol.h"
#include "../encryption/encryption.h"
#include <stdbool.h>
#include <string.h>

// Builder configuration
typedef struct {
    // Protocol configuration
    protocol_type_t* protocols;      // Array of protocols
    size_t protocol_count;           // Number of protocols
    
    // Server configuration
    char** servers;                  // Array of server addresses (host:port)
    size_t server_count;             // Number of servers
    
    // Domain configuration
    char* domain;                    // Domain for DNS protocol
    
    // Module configuration
    char** modules;                  // Array of module names
    size_t module_count;             // Number of modules
    
    // Encryption configuration
    encryption_algorithm_t encryption_algorithm;  // Encryption algorithm
    
    // Output configuration
    char* output_file;               // Output file path
    bool debug_mode;                 // Debug mode flag
    
    // Version information
    uint16_t version_major;          // Major version
    uint16_t version_minor;          // Minor version
    uint16_t version_patch;          // Patch version
    
    // Security configuration
    bool sign_binary;                // Sign binary flag
    bool verify_signature;           // Verify signature flag
} builder_config_t;

// Builder functions
status_t builder_init(void);
status_t builder_shutdown(void);
status_t builder_parse_args(int argc, char** argv, builder_config_t* config);
status_t builder_build_client(const builder_config_t* config);
status_t builder_clean_config(builder_config_t* config);

#endif /* DINOC_BUILDER_H */
