/**
 * @file http_server.c
 * @brief HTTP server implementation for C2 API
 */

#define _GNU_SOURCE /* For strdup */

#include "../include/api.h"
#include "../include/common.h"
#include "../include/task.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microhttpd.h>
#include <jansson.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// HTTP server context
typedef struct {
    struct MHD_Daemon* daemon;       // MHD daemon
    pthread_t server_thread;         // Server thread
    bool running;                    // Running flag
    char* bind_address;              // Bind address
    uint16_t port;                   // Port
    
    // API handlers
    api_handler_t* handlers;         // Array of handlers
    size_t handler_count;            // Number of handlers
    size_t handler_capacity;         // Capacity of handlers array
    pthread_mutex_t handlers_mutex;  // Mutex for handlers array
} http_server_ctx_t;

// Global HTTP server context
static http_server_ctx_t* global_server = NULL;

// Forward declarations
static int http_request_handler(void* cls, struct MHD_Connection* connection,
                              const char* url, const char* method,
                              const char* version, const char* upload_data,
                              size_t* upload_data_size, void** con_cls);
static void* http_server_thread(void* arg);

/**
 * @brief Initialize HTTP server
 */
status_t http_server_init(const char* bind_address, uint16_t port) {
    if (global_server != NULL) {
        return STATUS_ERROR_ALREADY_EXISTS;
    }
    
    // Create server context
    global_server = (http_server_ctx_t*)malloc(sizeof(http_server_ctx_t));
    if (global_server == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize context
    memset(global_server, 0, sizeof(http_server_ctx_t));
    
    // Copy bind address
    if (bind_address != NULL) {
        global_server->bind_address = strdup(bind_address);
        if (global_server->bind_address == NULL) {
            free(global_server);
            global_server = NULL;
            return STATUS_ERROR_MEMORY;
        }
    }
    
    // Set port
    global_server->port = port;
    
    // Initialize handlers array
    global_server->handler_capacity = 16;
    global_server->handler_count = 0;
    global_server->handlers = (api_handler_t*)malloc(global_server->handler_capacity * sizeof(api_handler_t));
    
    if (global_server->handlers == NULL) {
        if (global_server->bind_address != NULL) {
            free(global_server->bind_address);
        }
        free(global_server);
        global_server = NULL;
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize mutex
    pthread_mutex_init(&global_server->handlers_mutex, NULL);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Start HTTP server
 */
status_t http_server_start(void) {
    if (global_server == NULL) {
        return STATUS_ERROR_NOT_FOUND;
    }
    
    // Check if already running
    if (global_server->running) {
        return STATUS_ERROR_ALREADY_RUNNING;
    }
    
    // Create MHD daemon
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(global_server->port);
    
    if (global_server->bind_address != NULL) {
        addr.sin_addr.s_addr = inet_addr(global_server->bind_address);
    } else {
        addr.sin_addr.s_addr = INADDR_ANY;
    }
    
    global_server->daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, 0,
                                           NULL, NULL,
                                           &http_request_handler, global_server,
                                           MHD_OPTION_SOCK_ADDR, &addr,
                                           MHD_OPTION_END);
    
    if (global_server->daemon == NULL) {
        return STATUS_ERROR_GENERIC;
    }
    
    // Set running flag
    global_server->running = true;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Stop HTTP server
 */
status_t http_server_stop(void) {
    if (global_server == NULL) {
        return STATUS_ERROR_NOT_FOUND;
    }
    
    // Check if running
    if (!global_server->running) {
        return STATUS_ERROR_NOT_RUNNING;
    }
    
    // Stop MHD daemon
    MHD_stop_daemon(global_server->daemon);
    global_server->daemon = NULL;
    
    // Set running flag
    global_server->running = false;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Shutdown HTTP server
 */
status_t http_server_shutdown(void) {
    if (global_server == NULL) {
        return STATUS_ERROR_NOT_FOUND;
    }
    
    // Stop server if running
    if (global_server->running) {
        http_server_stop();
    }
    
    // Free handlers
    pthread_mutex_lock(&global_server->handlers_mutex);
    
    for (size_t i = 0; i < global_server->handler_count; i++) {
        free(global_server->handlers[i].url);
        free(global_server->handlers[i].method);
    }
    
    free(global_server->handlers);
    pthread_mutex_unlock(&global_server->handlers_mutex);
    pthread_mutex_destroy(&global_server->handlers_mutex);
    
    // Free bind address
    if (global_server->bind_address != NULL) {
        free(global_server->bind_address);
    }
    
    // Free context
    free(global_server);
    global_server = NULL;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Register API handler
 */
status_t http_server_register_handler(const char* url, const char* method, api_handler_func_t handler) {
    if (global_server == NULL) {
        return STATUS_ERROR_NOT_FOUND;
    }
    
    if (url == NULL || method == NULL || handler == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&global_server->handlers_mutex);
    
    // Check if handler already exists
    for (size_t i = 0; i < global_server->handler_count; i++) {
        if (strcmp(global_server->handlers[i].url, url) == 0 &&
            strcmp(global_server->handlers[i].method, method) == 0) {
            pthread_mutex_unlock(&global_server->handlers_mutex);
            return STATUS_ERROR_ALREADY_EXISTS;
        }
    }
    
    // Resize handlers array if needed
    if (global_server->handler_count >= global_server->handler_capacity) {
        size_t new_capacity = global_server->handler_capacity * 2;
        api_handler_t* new_handlers = (api_handler_t*)realloc(global_server->handlers, new_capacity * sizeof(api_handler_t));
        
        if (new_handlers == NULL) {
            pthread_mutex_unlock(&global_server->handlers_mutex);
            return STATUS_ERROR_MEMORY;
        }
        
        global_server->handlers = new_handlers;
        global_server->handler_capacity = new_capacity;
    }
    
    // Add handler
    api_handler_t* handler_entry = &global_server->handlers[global_server->handler_count++];
    
    handler_entry->url = strdup(url);
    if (handler_entry->url == NULL) {
        pthread_mutex_unlock(&global_server->handlers_mutex);
        return STATUS_ERROR_MEMORY;
    }
    
    handler_entry->method = strdup(method);
    if (handler_entry->method == NULL) {
        free(handler_entry->url);
        pthread_mutex_unlock(&global_server->handlers_mutex);
        return STATUS_ERROR_MEMORY;
    }
    
    handler_entry->handler = handler;
    
    pthread_mutex_unlock(&global_server->handlers_mutex);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Find API handler
 */
static api_handler_func_t find_handler(const char* url, const char* method) {
    if (global_server == NULL || url == NULL || method == NULL) {
        return NULL;
    }
    
    api_handler_func_t handler = NULL;
    
    pthread_mutex_lock(&global_server->handlers_mutex);
    
    // Find exact match
    for (size_t i = 0; i < global_server->handler_count; i++) {
        if (strcmp(global_server->handlers[i].url, url) == 0 &&
            strcmp(global_server->handlers[i].method, method) == 0) {
            handler = global_server->handlers[i].handler;
            break;
        }
    }
    
    // If no exact match, try prefix match
    if (handler == NULL) {
        for (size_t i = 0; i < global_server->handler_count; i++) {
            size_t url_len = strlen(global_server->handlers[i].url);
            
            if (url_len > 0 && global_server->handlers[i].url[url_len - 1] == '/' &&
                strncmp(global_server->handlers[i].url, url, url_len) == 0 &&
                strcmp(global_server->handlers[i].method, method) == 0) {
                handler = global_server->handlers[i].handler;
                break;
            }
        }
    }
    
    pthread_mutex_unlock(&global_server->handlers_mutex);
    
    return handler;
}

/**
 * @brief HTTP request handler
 */
static int http_request_handler(void* cls, struct MHD_Connection* connection,
                              const char* url, const char* method,
                              const char* version, const char* upload_data,
                              size_t* upload_data_size, void** con_cls) {
    // Check if this is the first call
    if (*con_cls == NULL) {
        *con_cls = (void*)1;
        return MHD_YES;
    }
    
    // Check if there is upload data
    if (*upload_data_size > 0) {
        *upload_data_size = 0;
        return MHD_YES;
    }
    
    // Find handler
    api_handler_func_t handler = find_handler(url, method);
    
    if (handler == NULL) {
        // No handler found, return 404
        const char* response = "Not Found";
        struct MHD_Response* mhd_response = MHD_create_response_from_buffer(strlen(response),
                                                                         (void*)response,
                                                                         MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, mhd_response);
        MHD_destroy_response(mhd_response);
        return ret;
    }
    
    // Call handler
    status_t status = handler(connection, url, method, upload_data, *upload_data_size);
    
    if (status != STATUS_SUCCESS) {
        // Handler failed, return 500
        const char* response = "Internal Server Error";
        struct MHD_Response* mhd_response = MHD_create_response_from_buffer(strlen(response),
                                                                         (void*)response,
                                                                         MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, mhd_response);
        MHD_destroy_response(mhd_response);
        return ret;
    }
    
    return MHD_YES;
}

/**
 * @brief Send HTTP response
 */
status_t http_server_send_response(struct MHD_Connection* connection, int status_code,
                                 const char* content_type, const char* response) {
    if (connection == NULL || content_type == NULL || response == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    struct MHD_Response* mhd_response = MHD_create_response_from_buffer(strlen(response),
                                                                     (void*)response,
                                                                     MHD_RESPMEM_MUST_COPY);
    
    if (mhd_response == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    MHD_add_response_header(mhd_response, "Content-Type", content_type);
    
    int ret = MHD_queue_response(connection, status_code, mhd_response);
    MHD_destroy_response(mhd_response);
    
    return ret == MHD_YES ? STATUS_SUCCESS : STATUS_ERROR_GENERIC;
}

/**
 * @brief Send JSON response
 */
status_t http_server_send_json_response(struct MHD_Connection* connection, int status_code, json_t* json) {
    if (connection == NULL || json == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Convert JSON to string
    char* json_str = json_dumps(json, JSON_COMPACT);
    if (json_str == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Send response
    status_t status = http_server_send_response(connection, status_code, "application/json", json_str);
    
    // Free JSON string
    free(json_str);
    
    return status;
}

/**
 * @brief Parse JSON request
 */
status_t http_server_parse_json_request(const char* upload_data, size_t upload_data_size, json_t** json) {
    if (upload_data == NULL || json == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Parse JSON
    json_error_t error;
    *json = json_loadb(upload_data, upload_data_size, 0, &error);
    
    if (*json == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Extract UUID from URL
 */
status_t http_server_extract_uuid_from_url(const char* url, const char* prefix, uuid_t uuid) {
    if (url == NULL || prefix == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check if URL starts with prefix
    size_t prefix_len = strlen(prefix);
    if (strncmp(url, prefix, prefix_len) != 0) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Extract UUID string
    const char* uuid_str = url + prefix_len;
    
    // Parse UUID
    return uuid_from_string(uuid_str, uuid);
}
