/**
 * @file ws_listener.c
 * @brief Implementation of WebSocket protocol listener
 */

#include "../include/protocol.h"
#include "../include/common.h"
#include "../include/client.h"
#include "protocol_fragmentation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <libwebsockets.h>

// WebSocket listener context
typedef struct {
    struct lws_context* context;     // libwebsockets context
    struct lws_vhost* vhost;         // libwebsockets vhost
    pthread_t listener_thread;       // Listener thread
    bool running;                    // Running flag
    char* bind_address;              // Bind address
    uint16_t port;                   // Port
    uint32_t timeout_ms;             // Timeout in milliseconds
    
    // Callbacks
    void (*on_message_received)(protocol_listener_t*, client_t*, protocol_message_t*);
    void (*on_client_connected)(protocol_listener_t*, client_t*);
    void (*on_client_disconnected)(protocol_listener_t*, client_t*);
    
    // Client tracking
    client_t** clients;              // Array of clients
    size_t client_count;             // Number of clients
    size_t client_capacity;          // Capacity of clients array
    pthread_mutex_t clients_mutex;   // Mutex for clients array
} ws_listener_ctx_t;

// WebSocket per-session data
typedef struct {
    client_t* client;                // Client
    protocol_listener_t* listener;   // Listener
    bool established;                // Connection established flag
    uint8_t* rx_buffer;              // Receive buffer
    size_t rx_buffer_len;            // Receive buffer length
    size_t rx_buffer_capacity;       // Receive buffer capacity
} ws_session_data_t;

// Forward declarations
static void* ws_listener_thread(void* arg);
static status_t ws_listener_start(protocol_listener_t* listener);
static status_t ws_listener_stop(protocol_listener_t* listener);
static status_t ws_listener_destroy(protocol_listener_t* listener);
static status_t ws_listener_send_message(protocol_listener_t* listener, client_t* client, protocol_message_t* message);
static status_t ws_listener_register_callbacks(protocol_listener_t* listener,
                                             void (*on_message_received)(protocol_listener_t*, client_t*, protocol_message_t*),
                                             void (*on_client_connected)(protocol_listener_t*, client_t*),
                                             void (*on_client_disconnected)(protocol_listener_t*, client_t*));

// WebSocket protocol callback
static int ws_callback(struct lws* wsi, enum lws_callback_reasons reason,
                     void* user, void* in, size_t len);

// WebSocket protocol definition
static const struct lws_protocols ws_protocols[] = {
    {
        "dinoc-protocol",
        ws_callback,
        sizeof(ws_session_data_t),
        4096, // rx buffer size
        0, NULL, 0
    },
    { NULL, NULL, 0, 0, 0, NULL, 0 } // terminator
};

/**
 * @brief WebSocket callback function
 */
static int ws_callback(struct lws* wsi, enum lws_callback_reasons reason,
                     void* user, void* in, size_t len) {
    ws_session_data_t* session = (ws_session_data_t*)user;
    
    switch (reason) {
        case LWS_CALLBACK_PROTOCOL_INIT:
            // Protocol initialization
            break;
            
        case LWS_CALLBACK_PROTOCOL_DESTROY:
            // Protocol destruction
            break;
            
        case LWS_CALLBACK_ESTABLISHED:
            // Connection established
            {
                // Get listener context
                ws_listener_ctx_t* ctx = (ws_listener_ctx_t*)lws_get_vhost_user(lws_get_vhost(wsi));
                if (ctx == NULL) {
                    return -1;
                }
                
                // Create client
                client_t* client = (client_t*)malloc(sizeof(client_t));
                if (client == NULL) {
                    return -1;
                }
                
                // Initialize client
                memset(client, 0, sizeof(client_t));
                uuid_generate(&client->id);
                client->connection_time = time(NULL);
                client->last_seen_time = client->connection_time;
                client->user_data = wsi;
                
                // Get client IP address
                char client_ip[128];
                lws_get_peer_simple(wsi, client_ip, sizeof(client_ip));
                strncpy(client->address, client_ip, sizeof(client->address) - 1);
                
                // Initialize session data
                session->client = client;
                session->listener = (protocol_listener_t*)ctx;
                session->established = true;
                session->rx_buffer = NULL;
                session->rx_buffer_len = 0;
                session->rx_buffer_capacity = 0;
                
                // Add client to listener
                pthread_mutex_lock(&ctx->clients_mutex);
                
                // Resize clients array if needed
                if (ctx->client_count >= ctx->client_capacity) {
                    size_t new_capacity = ctx->client_capacity * 2;
                    client_t** new_clients = (client_t**)realloc(ctx->clients, new_capacity * sizeof(client_t*));
                    
                    if (new_clients == NULL) {
                        pthread_mutex_unlock(&ctx->clients_mutex);
                        free(client);
                        return -1;
                    }
                    
                    ctx->clients = new_clients;
                    ctx->client_capacity = new_capacity;
                }
                
                ctx->clients[ctx->client_count++] = client;
                pthread_mutex_unlock(&ctx->clients_mutex);
                
                // Call client connected callback
                if (ctx->on_client_connected != NULL) {
                    ctx->on_client_connected((protocol_listener_t*)ctx, client);
                }
            }
            break;
            
        case LWS_CALLBACK_CLOSED:
            // Connection closed
            {
                if (session == NULL || session->client == NULL || !session->established) {
                    break;
                }
                
                // Get listener context
                ws_listener_ctx_t* ctx = (ws_listener_ctx_t*)lws_get_vhost_user(lws_get_vhost(wsi));
                if (ctx == NULL) {
                    break;
                }
                
                // Call client disconnected callback
                if (ctx->on_client_disconnected != NULL) {
                    ctx->on_client_disconnected((protocol_listener_t*)ctx, session->client);
                }
                
                // Remove client from listener
                pthread_mutex_lock(&ctx->clients_mutex);
                
                for (size_t i = 0; i < ctx->client_count; i++) {
                    if (ctx->clients[i] == session->client) {
                        // Replace with last client
                        ctx->clients[i] = ctx->clients[--ctx->client_count];
                        break;
                    }
                }
                
                pthread_mutex_unlock(&ctx->clients_mutex);
                
                // Free client
                free(session->client);
                session->client = NULL;
                session->established = false;
                
                // Free receive buffer
                if (session->rx_buffer != NULL) {
                    free(session->rx_buffer);
                    session->rx_buffer = NULL;
                    session->rx_buffer_len = 0;
                    session->rx_buffer_capacity = 0;
                }
            }
            break;
            
        case LWS_CALLBACK_RECEIVE:
            // Data received
            {
                if (session == NULL || session->client == NULL || !session->established) {
                    break;
                }
                
                // Get listener context
                ws_listener_ctx_t* ctx = (ws_listener_ctx_t*)lws_get_vhost_user(lws_get_vhost(wsi));
                if (ctx == NULL) {
                    break;
                }
                
                // Update client last seen time
                session->client->last_seen_time = time(NULL);
                
                // Check if this is a fragmented message
                const size_t remaining = lws_remaining_packet_payload(wsi);
                const bool is_final_fragment = lws_is_final_fragment(wsi);
                
                // Resize receive buffer if needed
                if (session->rx_buffer_len + len > session->rx_buffer_capacity) {
                    size_t new_capacity = session->rx_buffer_capacity * 2;
                    if (new_capacity == 0) {
                        new_capacity = 4096;
                    }
                    
                    while (new_capacity < session->rx_buffer_len + len) {
                        new_capacity *= 2;
                    }
                    
                    uint8_t* new_buffer = (uint8_t*)realloc(session->rx_buffer, new_capacity);
                    if (new_buffer == NULL) {
                        break;
                    }
                    
                    session->rx_buffer = new_buffer;
                    session->rx_buffer_capacity = new_capacity;
                }
                
                // Append data to receive buffer
                memcpy(session->rx_buffer + session->rx_buffer_len, in, len);
                session->rx_buffer_len += len;
                
                // Process message if this is the final fragment
                if (is_final_fragment && remaining == 0) {
                    // Create message
                    protocol_message_t message;
                    message.data = session->rx_buffer;
                    message.data_len = session->rx_buffer_len;
                    
                    // Call message received callback
                    if (ctx->on_message_received != NULL) {
                        ctx->on_message_received((protocol_listener_t*)ctx, session->client, &message);
                    }
                    
                    // Reset receive buffer
                    session->rx_buffer = NULL;
                    session->rx_buffer_len = 0;
                    session->rx_buffer_capacity = 0;
                }
            }
            break;
            
        case LWS_CALLBACK_SERVER_WRITEABLE:
            // Ready to send data
            break;
            
        default:
            break;
    }
    
    return 0;
}

/**
 * @brief WebSocket listener thread
 */
static void* ws_listener_thread(void* arg) {
    protocol_listener_t* listener = (protocol_listener_t*)arg;
    ws_listener_ctx_t* ctx = (ws_listener_ctx_t*)listener;
    
    // Run event loop
    while (ctx->running) {
        lws_service(ctx->context, 100);
    }
    
    return NULL;
}

/**
 * @brief Create a WebSocket protocol listener
 */
status_t ws_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener) {
    if (config == NULL || listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Validate config
    if (config->port == 0) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Create listener context
    ws_listener_ctx_t* ctx = (ws_listener_ctx_t*)malloc(sizeof(ws_listener_ctx_t));
    if (ctx == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize context
    memset(ctx, 0, sizeof(ws_listener_ctx_t));
    
    // Copy config
    ctx->port = config->port;
    ctx->timeout_ms = config->timeout_ms;
    
    if (config->bind_address != NULL) {
        ctx->bind_address = strdup(config->bind_address);
        if (ctx->bind_address == NULL) {
            free(ctx);
            return STATUS_ERROR_MEMORY;
        }
    }
    
    // Initialize clients array
    ctx->client_capacity = 16;
    ctx->client_count = 0;
    ctx->clients = (client_t**)malloc(ctx->client_capacity * sizeof(client_t*));
    
    if (ctx->clients == NULL) {
        if (ctx->bind_address != NULL) {
            free(ctx->bind_address);
        }
        free(ctx);
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize mutex
    pthread_mutex_init(&ctx->clients_mutex, NULL);
    
    // Set function pointers
    protocol_listener_t* base = (protocol_listener_t*)ctx;
    base->start = ws_listener_start;
    base->stop = ws_listener_stop;
    base->destroy = ws_listener_destroy;
    base->send_message = ws_listener_send_message;
    base->register_callbacks = ws_listener_register_callbacks;
    
    // Set protocol type
    base->protocol_type = PROTOCOL_TYPE_WS;
    
    // Generate ID
    uuid_generate(&base->id);
    
    *listener = base;
    return STATUS_SUCCESS;
}

/**
 * @brief Start WebSocket listener
 */
static status_t ws_listener_start(protocol_listener_t* listener) {
    if (listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    ws_listener_ctx_t* ctx = (ws_listener_ctx_t*)listener;
    
    // Check if already running
    if (ctx->running) {
        return STATUS_ERROR_ALREADY_RUNNING;
    }
    
    // Create libwebsockets context
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    
    info.port = ctx->port;
    info.iface = ctx->bind_address;
    info.protocols = ws_protocols;
    info.vhost_name = "dinoc-server";
    info.options = LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;
    info.user = ctx;
    
    ctx->context = lws_create_context(&info);
    if (ctx->context == NULL) {
        return STATUS_ERROR_GENERIC;
    }
    
    // Get vhost
    ctx->vhost = lws_get_vhost_by_name(ctx->context, "dinoc-server");
    if (ctx->vhost == NULL) {
        lws_context_destroy(ctx->context);
        ctx->context = NULL;
        return STATUS_ERROR_GENERIC;
    }
    
    // Set vhost user pointer
    lws_set_vhost_user(ctx->vhost, ctx);
    
    // Set running flag
    ctx->running = true;
    
    // Create listener thread
    if (pthread_create(&ctx->listener_thread, NULL, ws_listener_thread, listener) != 0) {
        lws_context_destroy(ctx->context);
        ctx->context = NULL;
        ctx->running = false;
        return STATUS_ERROR_GENERIC;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Stop WebSocket listener
 */
static status_t ws_listener_stop(protocol_listener_t* listener) {
    if (listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    ws_listener_ctx_t* ctx = (ws_listener_ctx_t*)listener;
    
    // Check if running
    if (!ctx->running) {
        return STATUS_ERROR_NOT_RUNNING;
    }
    
    // Set running flag
    ctx->running = false;
    
    // Wait for listener thread to exit
    pthread_join(ctx->listener_thread, NULL);
    
    // Destroy libwebsockets context
    if (ctx->context != NULL) {
        lws_context_destroy(ctx->context);
        ctx->context = NULL;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Destroy WebSocket listener
 */
static status_t ws_listener_destroy(protocol_listener_t* listener) {
    if (listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    ws_listener_ctx_t* ctx = (ws_listener_ctx_t*)listener;
    
    // Stop listener if running
    if (ctx->running) {
        ws_listener_stop(listener);
    }
    
    // Free clients
    pthread_mutex_lock(&ctx->clients_mutex);
    
    for (size_t i = 0; i < ctx->client_count; i++) {
        free(ctx->clients[i]);
    }
    
    free(ctx->clients);
    pthread_mutex_unlock(&ctx->clients_mutex);
    pthread_mutex_destroy(&ctx->clients_mutex);
    
    // Free bind address
    if (ctx->bind_address != NULL) {
        free(ctx->bind_address);
    }
    
    // Free context
    free(ctx);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Send message to WebSocket client
 */
static status_t ws_listener_send_message(protocol_listener_t* listener, client_t* client, protocol_message_t* message) {
    if (listener == NULL || client == NULL || message == NULL || message->data == NULL || message->data_len == 0) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    ws_listener_ctx_t* ctx = (ws_listener_ctx_t*)listener;
    
    // Check if running
    if (!ctx->running) {
        return STATUS_ERROR_NOT_RUNNING;
    }
    
    // Get WebSocket connection
    struct lws* wsi = (struct lws*)client->user_data;
    if (wsi == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Allocate buffer with LWS_PRE bytes at the beginning
    uint8_t* buffer = (uint8_t*)malloc(LWS_PRE + message->data_len);
    if (buffer == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Copy message data
    memcpy(buffer + LWS_PRE, message->data, message->data_len);
    
    // Send message
    int result = lws_write(wsi, buffer + LWS_PRE, message->data_len, LWS_WRITE_BINARY);
    
    // Free buffer
    free(buffer);
    
    if (result < 0) {
        return STATUS_ERROR_GENERIC;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Register callbacks for WebSocket listener
 */
static status_t ws_listener_register_callbacks(protocol_listener_t* listener,
                                             void (*on_message_received)(protocol_listener_t*, client_t*, protocol_message_t*),
                                             void (*on_client_connected)(protocol_listener_t*, client_t*),
                                             void (*on_client_disconnected)(protocol_listener_t*, client_t*)) {
    if (listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    ws_listener_ctx_t* ctx = (ws_listener_ctx_t*)listener;
    
    ctx->on_message_received = on_message_received;
    ctx->on_client_connected = on_client_connected;
    ctx->on_client_disconnected = on_client_disconnected;
    
    return STATUS_SUCCESS;
}
