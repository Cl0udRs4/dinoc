/**
 * @file http_server.c
 * @brief Implementation of HTTP API server
 */

#include "../include/api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <microhttpd.h>
#include <jansson.h>

// HTTP server context
typedef struct {
    struct MHD_Daemon* daemon;      // MicroHTTPD daemon
    pthread_mutex_t mutex;          // Mutex for thread safety
    bool running;                   // Running flag
    
    // Authentication
    char* auth_username;            // Authentication username
    char* auth_password;            // Authentication password
    bool auth_required;             // Authentication required flag
} http_server_ctx_t;

// Forward declarations
static int http_request_handler(void* cls, struct MHD_Connection* connection,
                               const char* url, const char* method,
                               const char* version, const char* upload_data,
                               size_t* upload_data_size, void** con_cls);
static int http_authenticate(struct MHD_Connection* connection, const char* username, const char* password);
static int http_send_response(struct MHD_Connection* connection, int status_code, const char* content_type, const char* body);
static int http_send_json_response(struct MHD_Connection* connection, int status_code, json_t* json);
static int http_send_error(struct MHD_Connection* connection, int status_code, const char* error);

/**
 * @brief Initialize API server
 * 
 * @param config Server configuration
 * @param server Pointer to store server instance
 * @return status_t Status code
 */
status_t api_server_init(const api_config_t* config, api_server_t** server) {
    if (config == NULL || server == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Allocate server
    api_server_t* new_server = (api_server_t*)malloc(sizeof(api_server_t));
    if (new_server == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize server
    memset(new_server, 0, sizeof(api_server_t));
    
    // Copy configuration
    new_server->config.port = config->port;
    new_server->config.timeout_ms = config->timeout_ms;
    new_server->config.enable_cors = config->enable_cors;
    
    if (config->bind_address != NULL) {
        new_server->config.bind_address = strdup(config->bind_address);
        if (new_server->config.bind_address == NULL) {
            free(new_server);
            return STATUS_ERROR_MEMORY;
        }
    }
    
    if (config->cert_file != NULL) {
        new_server->config.cert_file = strdup(config->cert_file);
        if (new_server->config.cert_file == NULL) {
            if (new_server->config.bind_address != NULL) {
                free(new_server->config.bind_address);
            }
            free(new_server);
            return STATUS_ERROR_MEMORY;
        }
    }
    
    if (config->key_file != NULL) {
        new_server->config.key_file = strdup(config->key_file);
        if (new_server->config.key_file == NULL) {
            if (new_server->config.cert_file != NULL) {
                free(new_server->config.cert_file);
            }
            if (new_server->config.bind_address != NULL) {
                free(new_server->config.bind_address);
            }
            free(new_server);
            return STATUS_ERROR_MEMORY;
        }
    }
    
    if (config->auth_username != NULL) {
        new_server->config.auth_username = strdup(config->auth_username);
        if (new_server->config.auth_username == NULL) {
            if (new_server->config.key_file != NULL) {
                free(new_server->config.key_file);
            }
            if (new_server->config.cert_file != NULL) {
                free(new_server->config.cert_file);
            }
            if (new_server->config.bind_address != NULL) {
                free(new_server->config.bind_address);
            }
            free(new_server);
            return STATUS_ERROR_MEMORY;
        }
    }
    
    if (config->auth_password != NULL) {
        new_server->config.auth_password = strdup(config->auth_password);
        if (new_server->config.auth_password == NULL) {
            if (new_server->config.auth_username != NULL) {
                free(new_server->config.auth_username);
            }
            if (new_server->config.key_file != NULL) {
                free(new_server->config.key_file);
            }
            if (new_server->config.cert_file != NULL) {
                free(new_server->config.cert_file);
            }
            if (new_server->config.bind_address != NULL) {
                free(new_server->config.bind_address);
            }
            free(new_server);
            return STATUS_ERROR_MEMORY;
        }
    }
    
    // Allocate HTTP server context
    http_server_ctx_t* http_ctx = (http_server_ctx_t*)malloc(sizeof(http_server_ctx_t));
    if (http_ctx == NULL) {
        if (new_server->config.auth_password != NULL) {
            free(new_server->config.auth_password);
        }
        if (new_server->config.auth_username != NULL) {
            free(new_server->config.auth_username);
        }
        if (new_server->config.key_file != NULL) {
            free(new_server->config.key_file);
        }
        if (new_server->config.cert_file != NULL) {
            free(new_server->config.cert_file);
        }
        if (new_server->config.bind_address != NULL) {
            free(new_server->config.bind_address);
        }
        free(new_server);
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize HTTP server context
    memset(http_ctx, 0, sizeof(http_server_ctx_t));
    pthread_mutex_init(&http_ctx->mutex, NULL);
    http_ctx->running = false;
    
    // Set authentication
    if (new_server->config.auth_username != NULL && new_server->config.auth_password != NULL) {
        http_ctx->auth_username = strdup(new_server->config.auth_username);
        http_ctx->auth_password = strdup(new_server->config.auth_password);
        http_ctx->auth_required = true;
    } else {
        http_ctx->auth_required = false;
    }
    
    // Set server context
    new_server->server_ctx = http_ctx;
    new_server->running = false;
    
    // Set server
    *server = new_server;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Start API server
 * 
 * @param server Server to start
 * @return status_t Status code
 */
status_t api_server_start(api_server_t* server) {
    if (server == NULL || server->server_ctx == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    http_server_ctx_t* http_ctx = (http_server_ctx_t*)server->server_ctx;
    
    // Lock mutex
    pthread_mutex_lock(&http_ctx->mutex);
    
    // Check if already running
    if (http_ctx->running) {
        pthread_mutex_unlock(&http_ctx->mutex);
        return STATUS_SUCCESS;
    }
    
    // Determine bind address
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server->config.port);
    
    if (server->config.bind_address != NULL && strcmp(server->config.bind_address, "0.0.0.0") != 0) {
        if (inet_pton(AF_INET, server->config.bind_address, &addr.sin_addr) <= 0) {
            LOG_ERROR("Invalid HTTP bind address: %s", server->config.bind_address);
            pthread_mutex_unlock(&http_ctx->mutex);
            return STATUS_ERROR_INVALID_PARAM;
        }
    } else {
        addr.sin_addr.s_addr = INADDR_ANY;
    }
    
    // Start HTTP server
    http_ctx->daemon = MHD_start_daemon(
        MHD_USE_SELECT_INTERNALLY | MHD_USE_THREAD_PER_CONNECTION,
        server->config.port,
        NULL, NULL,
        &http_request_handler, server,
        MHD_OPTION_SOCK_ADDR, &addr,
        MHD_OPTION_CONNECTION_TIMEOUT, server->config.timeout_ms / 1000,
        MHD_OPTION_END
    );
    
    if (http_ctx->daemon == NULL) {
        LOG_ERROR("Failed to start HTTP server on %s:%d",
                 server->config.bind_address ? server->config.bind_address : "0.0.0.0",
                 server->config.port);
        pthread_mutex_unlock(&http_ctx->mutex);
        return STATUS_ERROR_GENERIC;
    }
    
    // Set running flag
    http_ctx->running = true;
    server->running = true;
    
    pthread_mutex_unlock(&http_ctx->mutex);
    
    LOG_INFO("HTTP API server started on %s:%d",
             server->config.bind_address ? server->config.bind_address : "0.0.0.0",
             server->config.port);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Stop API server
 * 
 * @param server Server to stop
 * @return status_t Status code
 */
status_t api_server_stop(api_server_t* server) {
    if (server == NULL || server->server_ctx == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    http_server_ctx_t* http_ctx = (http_server_ctx_t*)server->server_ctx;
    
    // Lock mutex
    pthread_mutex_lock(&http_ctx->mutex);
    
    // Check if already stopped
    if (!http_ctx->running) {
        pthread_mutex_unlock(&http_ctx->mutex);
        return STATUS_SUCCESS;
    }
    
    // Stop HTTP server
    if (http_ctx->daemon != NULL) {
        MHD_stop_daemon(http_ctx->daemon);
        http_ctx->daemon = NULL;
    }
    
    // Clear running flag
    http_ctx->running = false;
    server->running = false;
    
    pthread_mutex_unlock(&http_ctx->mutex);
    
    LOG_INFO("HTTP API server stopped on port %d", server->config.port);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Clean up API server
 * 
 * @param server Server to clean up
 * @return status_t Status code
 */
status_t api_server_cleanup(api_server_t* server) {
    if (server == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Stop server if running
    if (server->running) {
        api_server_stop(server);
    }
    
    // Free HTTP server context
    if (server->server_ctx != NULL) {
        http_server_ctx_t* http_ctx = (http_server_ctx_t*)server->server_ctx;
        
        // Destroy mutex
        pthread_mutex_destroy(&http_ctx->mutex);
        
        // Free authentication
        if (http_ctx->auth_username != NULL) {
            free(http_ctx->auth_username);
        }
        if (http_ctx->auth_password != NULL) {
            free(http_ctx->auth_password);
        }
        
        free(http_ctx);
        server->server_ctx = NULL;
    }
    
    // Free configuration
    if (server->config.bind_address != NULL) {
        free(server->config.bind_address);
    }
    if (server->config.cert_file != NULL) {
        free(server->config.cert_file);
    }
    if (server->config.key_file != NULL) {
        free(server->config.key_file);
    }
    if (server->config.auth_username != NULL) {
        free(server->config.auth_username);
    }
    if (server->config.auth_password != NULL) {
        free(server->config.auth_password);
    }
    
    // Free server
    free(server);
    
    return STATUS_SUCCESS;
}

/**
 * @brief HTTP request handler
 * 
 * @param cls User data (server instance)
 * @param connection MHD connection
 * @param url URL
 * @param method HTTP method
 * @param version HTTP version
 * @param upload_data Upload data
 * @param upload_data_size Upload data size
 * @param con_cls Connection data
 * @return int MHD status code
 */
static int http_request_handler(void* cls, struct MHD_Connection* connection,
                               const char* url, const char* method,
                               const char* version, const char* upload_data,
                               size_t* upload_data_size, void** con_cls) {
    api_server_t* server = (api_server_t*)cls;
    http_server_ctx_t* http_ctx = (http_server_ctx_t*)server->server_ctx;
    
    // Check authentication
    if (http_ctx->auth_required) {
        int auth_result = http_authenticate(connection, http_ctx->auth_username, http_ctx->auth_password);
        if (auth_result != MHD_YES) {
            return auth_result;
        }
    }
    
    // Handle CORS preflight request
    if (server->config.enable_cors && strcmp(method, "OPTIONS") == 0) {
        struct MHD_Response* response = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
        if (response == NULL) {
            return MHD_NO;
        }
        
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        MHD_add_response_header(response, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        MHD_add_response_header(response, "Access-Control-Allow-Headers", "Content-Type, Authorization");
        MHD_add_response_header(response, "Access-Control-Max-Age", "86400");
        
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }
    
    // TODO: Route request to appropriate handler
    
    // Default: Not found
    return http_send_error(connection, MHD_HTTP_NOT_FOUND, "Not Found");
}

/**
 * @brief Authenticate HTTP request
 * 
 * @param connection MHD connection
 * @param username Expected username
 * @param password Expected password
 * @return int MHD status code
 */
static int http_authenticate(struct MHD_Connection* connection, const char* username, const char* password) {
    char* auth_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
    if (auth_header == NULL) {
        // No authentication provided, request authentication
        struct MHD_Response* response = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
        if (response == NULL) {
            return MHD_NO;
        }
        
        MHD_add_response_header(response, "WWW-Authenticate", "Basic realm=\"DinoC API\"");
        int ret = MHD_queue_response(connection, MHD_HTTP_UNAUTHORIZED, response);
        MHD_destroy_response(response);
        return ret;
    }
    
    // Parse Basic authentication
    if (strncmp(auth_header, "Basic ", 6) != 0) {
        return http_send_error(connection, MHD_HTTP_UNAUTHORIZED, "Invalid authentication method");
    }
    
    char* base64 = auth_header + 6;
    size_t out_len = 0;
    char* decoded = (char*)MHD_base64_decode(base64, strlen(base64), &out_len);
    if (decoded == NULL) {
        return http_send_error(connection, MHD_HTTP_UNAUTHORIZED, "Invalid authentication data");
    }
    
    // Find username:password separator
    char* colon = strchr(decoded, ':');
    if (colon == NULL) {
        free(decoded);
        return http_send_error(connection, MHD_HTTP_UNAUTHORIZED, "Invalid authentication format");
    }
    
    // Split username and password
    *colon = '\0';
    char* auth_username = decoded;
    char* auth_password = colon + 1;
    
    // Check credentials
    if (strcmp(auth_username, username) != 0 || strcmp(auth_password, password) != 0) {
        free(decoded);
        return http_send_error(connection, MHD_HTTP_UNAUTHORIZED, "Invalid credentials");
    }
    
    free(decoded);
    return MHD_YES;
}

/**
 * @brief Send HTTP response
 * 
 * @param connection MHD connection
 * @param status_code HTTP status code
 * @param content_type Content type
 * @param body Response body
 * @return int MHD status code
 */
static int http_send_response(struct MHD_Connection* connection, int status_code, const char* content_type, const char* body) {
    struct MHD_Response* response = MHD_create_response_from_buffer(
        strlen(body), (void*)body, MHD_RESPMEM_MUST_COPY);
    
    if (response == NULL) {
        return MHD_NO;
    }
    
    MHD_add_response_header(response, "Content-Type", content_type);
    int ret = MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);
    
    return ret;
}

/**
 * @brief Send JSON HTTP response
 * 
 * @param connection MHD connection
 * @param status_code HTTP status code
 * @param json JSON data
 * @return int MHD status code
 */
static int http_send_json_response(struct MHD_Connection* connection, int status_code, json_t* json) {
    char* body = json_dumps(json, JSON_COMPACT);
    if (body == NULL) {
        return MHD_NO;
    }
    
    int ret = http_send_response(connection, status_code, "application/json", body);
    free(body);
    
    return ret;
}

/**
 * @brief Send error HTTP response
 * 
 * @param connection MHD connection
 * @param status_code HTTP status code
 * @param error Error message
 * @return int MHD status code
 */
static int http_send_error(struct MHD_Connection* connection, int status_code, const char* error) {
    json_t* json = json_object();
    if (json == NULL) {
        return MHD_NO;
    }
    
    json_object_set_new(json, "error", json_string(error));
    
    int ret = http_send_json_response(connection, status_code, json);
    json_decref(json);
    
    return ret;
}
