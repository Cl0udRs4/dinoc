/**
 * @file client_template.h
 * @brief Client template for builder to generate customized clients
 */

#ifndef DINOC_CLIENT_TEMPLATE_H
#define DINOC_CLIENT_TEMPLATE_H

#include "../include/common.h"
#include "../include/protocol.h"
#include "../encryption/encryption.h"
#include <stdint.h>
#include <stdbool.h>

// Client configuration structure
typedef struct {
    // Protocol configuration
    protocol_type_t* protocols;      // Array of protocols
    size_t protocol_count;           // Number of protocols
    
    // Server configuration
    char** servers;                  // Array of server addresses (host:port)
    size_t server_count;             // Number of servers
    
    // Domain configuration
    char* domain;                    // Domain for DNS protocol
    
    // Heartbeat configuration
    uint32_t heartbeat_interval;     // Heartbeat interval in seconds
    uint32_t heartbeat_jitter;       // Heartbeat jitter in seconds
    
    // Encryption configuration
    encryption_algorithm_t encryption_algorithm;  // Encryption algorithm
    
    // Module configuration
    char** modules;                  // Array of module names
    size_t module_count;             // Number of modules
    
    // Version information
    uint16_t version_major;          // Major version
    uint16_t version_minor;          // Minor version
    uint16_t version_patch;          // Patch version
    
    // Debug configuration
    bool debug_mode;                 // Debug mode flag
} client_config_t;

// Client template functions
status_t client_template_init(const client_config_t* config);
status_t client_template_shutdown(void);
status_t client_template_connect(void);
status_t client_template_disconnect(void);
status_t client_template_send_heartbeat(void);
status_t client_template_switch_protocol(protocol_type_t protocol_type);
status_t client_template_load_module(const char* module_name);
status_t client_template_unload_module(const char* module_name);
status_t client_template_execute_module(const char* module_name, const uint8_t* data, size_t data_len);

#endif /* DINOC_CLIENT_TEMPLATE_H */
