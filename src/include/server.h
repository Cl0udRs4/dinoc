/**
 * @file server.h
 * @brief Server interface for C2 server
 */

#ifndef DINOC_SERVER_H
#define DINOC_SERVER_H

#include "common.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Server configuration structure
 */
typedef struct {
    char* config_file;            // Configuration file path
    char* bind_address;           // Bind address for listeners
    uint16_t tcp_port;            // TCP port
    uint16_t udp_port;            // UDP port
    uint16_t ws_port;             // WebSocket port
    uint16_t dns_port;            // DNS port
    char* dns_domain;             // DNS domain
    char* pcap_device;            // PCAP device for ICMP
    uint16_t http_api_port;       // HTTP API port
    char* log_file;               // Log file path
    uint8_t log_level;            // Log level
    bool enable_tcp;              // Enable TCP listener
    bool enable_udp;              // Enable UDP listener
    bool enable_ws;               // Enable WebSocket listener
    bool enable_icmp;             // Enable ICMP listener
    bool enable_dns;              // Enable DNS listener
    bool enable_http_api;         // Enable HTTP API
    bool enable_console;          // Enable console interface
} server_config_t;

/**
 * @brief Initialize server
 * 
 * @param config Server configuration
 * @return status_t Status code
 */
status_t server_init(const server_config_t* config);

/**
 * @brief Start server
 * 
 * @return status_t Status code
 */
status_t server_start(void);

/**
 * @brief Stop server
 * 
 * @return status_t Status code
 */
status_t server_stop(void);

/**
 * @brief Shutdown server
 * 
 * @return status_t Status code
 */
status_t server_shutdown(void);

/**
 * @brief Get server configuration
 * 
 * @return server_config_t* Server configuration
 */
const server_config_t* server_get_config(void);

/**
 * @brief Parse command-line arguments
 * 
 * @param argc Argument count
 * @param argv Argument values
 * @param config Server configuration
 * @return status_t Status code
 */
status_t server_parse_args(int argc, char** argv, server_config_t* config);

/**
 * @brief Load configuration from file
 * 
 * @param config_file Configuration file path
 * @param config Server configuration
 * @return status_t Status code
 */
status_t server_load_config(const char* config_file, server_config_t* config);

/**
 * @brief Free server configuration
 * 
 * @param config Server configuration
 * @return status_t Status code
 */
status_t server_free_config(server_config_t* config);

#endif /* DINOC_SERVER_H */
