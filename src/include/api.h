/**
 * @file api.h
 * @brief HTTP API interface
 */

#ifndef DINOC_API_H
#define DINOC_API_H

#include "common.h"
#include "protocol.h"
#include "client.h"
#include "task.h"

/**
 * @brief API server configuration
 */
typedef struct {
    char* bind_address;         // Bind address
    uint16_t port;              // Port number
    char* cert_file;            // TLS certificate file (NULL for HTTP)
    char* key_file;             // TLS key file (NULL for HTTP)
    char* auth_username;        // Authentication username
    char* auth_password;        // Authentication password
    bool enable_cors;           // Enable CORS
    uint32_t timeout_ms;        // Request timeout in milliseconds
} api_config_t;

/**
 * @brief API server structure
 */
typedef struct {
    api_config_t config;        // API configuration
    void* server_ctx;           // Server context
    bool running;               // Running flag
} api_server_t;

/**
 * @brief Initialize API server
 * 
 * @param config Server configuration
 * @param server Pointer to store server instance
 * @return status_t Status code
 */
status_t api_server_init(const api_config_t* config, api_server_t** server);

/**
 * @brief Start API server
 * 
 * @param server Server to start
 * @return status_t Status code
 */
status_t api_server_start(api_server_t* server);

/**
 * @brief Stop API server
 * 
 * @param server Server to stop
 * @return status_t Status code
 */
status_t api_server_stop(api_server_t* server);

/**
 * @brief Clean up API server
 * 
 * @param server Server to clean up
 * @return status_t Status code
 */
status_t api_server_cleanup(api_server_t* server);

/**
 * @brief Register listener management endpoints
 * 
 * @param server API server
 * @return status_t Status code
 */
status_t api_register_listener_endpoints(api_server_t* server);

/**
 * @brief Register client management endpoints
 * 
 * @param server API server
 * @return status_t Status code
 */
status_t api_register_client_endpoints(api_server_t* server);

/**
 * @brief Register task management endpoints
 * 
 * @param server API server
 * @return status_t Status code
 */
status_t api_register_task_endpoints(api_server_t* server);

#endif /* DINOC_API_H */
