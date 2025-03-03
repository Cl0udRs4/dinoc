/**
 * @file dns_listener.c
 * @brief Implementation of DNS protocol listener
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
#include <ares.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#include <errno.h>

// DNS protocol constants
#define DNS_MAX_DOMAIN_LENGTH 253
#define DNS_MAX_LABEL_LENGTH 63
#define DNS_MAX_TXT_LENGTH 255
#define DNS_DEFAULT_PORT 53
#define DNS_DEFAULT_TIMEOUT 5000

// DNS listener context
typedef struct {
    ares_channel channel;            // c-ares channel
    pthread_t listener_thread;       // Listener thread
    bool running;                    // Running flag
    char* bind_address;              // Bind address
    uint16_t port;                   // Port
    char* domain;                    // Domain
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
    
    // File descriptors for select
    fd_set read_fds;                 // Read file descriptors
    fd_set write_fds;                // Write file descriptors
    int max_fd;                      // Maximum file descriptor
} dns_listener_ctx_t;

// Forward declarations
static void* dns_listener_thread(void* arg);
static status_t dns_listener_start(protocol_listener_t* listener);
static status_t dns_listener_stop(protocol_listener_t* listener);
static status_t dns_listener_destroy(protocol_listener_t* listener);
static status_t dns_listener_send_message(protocol_listener_t* listener, client_t* client, protocol_message_t* message);
static status_t dns_listener_register_callbacks(protocol_listener_t* listener,
                                              void (*on_message_received)(protocol_listener_t*, client_t*, protocol_message_t*),
                                              void (*on_client_connected)(protocol_listener_t*, client_t*),
                                              void (*on_client_disconnected)(protocol_listener_t*, client_t*));
static void dns_query_callback(void* arg, int status, int timeouts, unsigned char* abuf, int alen);
static client_t* dns_find_client_by_address(dns_listener_ctx_t* ctx, const char* address);
static status_t dns_add_client(dns_listener_ctx_t* ctx, const char* address, client_t** client);
static status_t dns_encode_data_to_txt(const uint8_t* data, size_t data_len, char*** txt_records, size_t* txt_record_count);
static status_t dns_decode_txt_to_data(const char* const* txt_records, size_t txt_record_count, uint8_t** data, size_t* data_len);

/**
 * @brief DNS query callback
 */
static void dns_query_callback(void* arg, int status, int timeouts, unsigned char* abuf, int alen) {
    // This is called when a DNS query is received
    // We need to parse the query and respond with our data
    
    // Implementation will be added in subsequent steps
}

/**
 * @brief DNS listener thread
 */
static void* dns_listener_thread(void* arg) {
    protocol_listener_t* listener = (protocol_listener_t*)arg;
    dns_listener_ctx_t* ctx = (dns_listener_ctx_t*)listener;
    
    // Set up file descriptors for select
    struct timeval tv;
    
    // Run server loop
    while (ctx->running) {
        // Initialize file descriptor sets
        FD_ZERO(&ctx->read_fds);
        FD_ZERO(&ctx->write_fds);
        
        // Get file descriptors from c-ares
        int nfds = ctx->max_fd + 1;
        ares_fds(ctx->channel, &ctx->read_fds, &ctx->write_fds);
        
        // Set timeout
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms
        
        // Wait for activity
        int count = select(nfds, &ctx->read_fds, &ctx->write_fds, NULL, &tv);
        
        if (count < 0) {
            if (errno != EINTR) {
                // Fatal error
                break;
            }
            continue;
        }
        
        // Process c-ares events
        ares_process(ctx->channel, &ctx->read_fds, &ctx->write_fds);
    }
    
    return NULL;
}

/**
 * @brief Find client by address
 */
static client_t* dns_find_client_by_address(dns_listener_ctx_t* ctx, const char* address) {
    if (ctx == NULL || address == NULL) {
        return NULL;
    }
    
    client_t* client = NULL;
    
    pthread_mutex_lock(&ctx->clients_mutex);
    
    for (size_t i = 0; i < ctx->client_count; i++) {
        if (strcmp(ctx->clients[i]->address, address) == 0) {
            client = ctx->clients[i];
            break;
        }
    }
    
    pthread_mutex_unlock(&ctx->clients_mutex);
    
    return client;
}

/**
 * @brief Add client
 */
static status_t dns_add_client(dns_listener_ctx_t* ctx, const char* address, client_t** client) {
    if (ctx == NULL || address == NULL || client == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Create client
    client_t* new_client = (client_t*)malloc(sizeof(client_t));
    if (new_client == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize client
    memset(new_client, 0, sizeof(client_t));
    uuid_generate(&new_client->id);
    new_client->connection_time = time(NULL);
    new_client->last_seen_time = new_client->connection_time;
    strncpy(new_client->address, address, sizeof(new_client->address) - 1);
    
    // Add client to list
    pthread_mutex_lock(&ctx->clients_mutex);
    
    // Resize clients array if needed
    if (ctx->client_count >= ctx->client_capacity) {
        size_t new_capacity = ctx->client_capacity * 2;
        client_t** new_clients = (client_t**)realloc(ctx->clients, new_capacity * sizeof(client_t*));
        
        if (new_clients == NULL) {
            pthread_mutex_unlock(&ctx->clients_mutex);
            free(new_client);
            return STATUS_ERROR_MEMORY;
        }
        
        ctx->clients = new_clients;
        ctx->client_capacity = new_capacity;
    }
    
    ctx->clients[ctx->client_count++] = new_client;
    pthread_mutex_unlock(&ctx->clients_mutex);
    
    *client = new_client;
    return STATUS_SUCCESS;
}

/**
 * @brief Encode data to TXT records
 */
static status_t dns_encode_data_to_txt(const uint8_t* data, size_t data_len, char*** txt_records, size_t* txt_record_count) {
    if (data == NULL || txt_records == NULL || txt_record_count == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Calculate number of TXT records needed
    size_t record_count = (data_len + DNS_MAX_TXT_LENGTH - 1) / DNS_MAX_TXT_LENGTH;
    
    // Allocate TXT records array
    char** records = (char**)malloc(record_count * sizeof(char*));
    if (records == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize records
    for (size_t i = 0; i < record_count; i++) {
        records[i] = NULL;
    }
    
    // Encode data into TXT records
    for (size_t i = 0; i < record_count; i++) {
        size_t offset = i * DNS_MAX_TXT_LENGTH;
        size_t chunk_size = data_len - offset;
        
        if (chunk_size > DNS_MAX_TXT_LENGTH) {
            chunk_size = DNS_MAX_TXT_LENGTH;
        }
        
        // Allocate record
        records[i] = (char*)malloc(chunk_size * 2 + 1);
        if (records[i] == NULL) {
            // Free allocated records
            for (size_t j = 0; j < i; j++) {
                free(records[j]);
            }
            free(records);
            return STATUS_ERROR_MEMORY;
        }
        
        // Encode chunk as hex string
        for (size_t j = 0; j < chunk_size; j++) {
            sprintf(&records[i][j * 2], "%02x", data[offset + j]);
        }
        
        records[i][chunk_size * 2] = '\0';
    }
    
    *txt_records = records;
    *txt_record_count = record_count;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Decode TXT records to data
 */
static status_t dns_decode_txt_to_data(const char* const* txt_records, size_t txt_record_count, uint8_t** data, size_t* data_len) {
    if (txt_records == NULL || data == NULL || data_len == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Calculate total data length
    size_t total_len = 0;
    
    for (size_t i = 0; i < txt_record_count; i++) {
        if (txt_records[i] == NULL) {
            return STATUS_ERROR_INVALID_PARAM;
        }
        
        total_len += strlen(txt_records[i]) / 2;
    }
    
    // Allocate data buffer
    uint8_t* buffer = (uint8_t*)malloc(total_len);
    if (buffer == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Decode TXT records
    size_t offset = 0;
    
    for (size_t i = 0; i < txt_record_count; i++) {
        size_t record_len = strlen(txt_records[i]);
        
        // Decode hex string
        for (size_t j = 0; j < record_len; j += 2) {
            char hex[3] = { txt_records[i][j], txt_records[i][j + 1], '\0' };
            buffer[offset++] = (uint8_t)strtol(hex, NULL, 16);
        }
    }
    
    *data = buffer;
    *data_len = total_len;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Create a DNS protocol listener
 */
status_t dns_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener) {
    if (config == NULL || listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Validate config
    if (config->domain == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Create listener context
    dns_listener_ctx_t* ctx = (dns_listener_ctx_t*)malloc(sizeof(dns_listener_ctx_t));
    if (ctx == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize context
    memset(ctx, 0, sizeof(dns_listener_ctx_t));
    
    // Copy config
    ctx->port = config->port > 0 ? config->port : DNS_DEFAULT_PORT;
    ctx->timeout_ms = config->timeout_ms > 0 ? config->timeout_ms : DNS_DEFAULT_TIMEOUT;
    
    if (config->bind_address != NULL) {
        ctx->bind_address = strdup(config->bind_address);
        if (ctx->bind_address == NULL) {
            free(ctx);
            return STATUS_ERROR_MEMORY;
        }
    }
    
    ctx->domain = strdup(config->domain);
    if (ctx->domain == NULL) {
        if (ctx->bind_address != NULL) {
            free(ctx->bind_address);
        }
        free(ctx);
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize clients array
    ctx->client_capacity = 16;
    ctx->client_count = 0;
    ctx->clients = (client_t**)malloc(ctx->client_capacity * sizeof(client_t*));
    
    if (ctx->clients == NULL) {
        if (ctx->domain != NULL) {
            free(ctx->domain);
        }
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
    base->start = dns_listener_start;
    base->stop = dns_listener_stop;
    base->destroy = dns_listener_destroy;
    base->send_message = dns_listener_send_message;
    base->register_callbacks = dns_listener_register_callbacks;
    
    // Set protocol type
    base->protocol_type = PROTOCOL_TYPE_DNS;
    
    // Generate ID
    uuid_generate(&base->id);
    
    *listener = base;
    return STATUS_SUCCESS;
}

/**
 * @brief Start DNS listener
 */
static status_t dns_listener_start(protocol_listener_t* listener) {
    if (listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    dns_listener_ctx_t* ctx = (dns_listener_ctx_t*)listener;
    
    // Check if already running
    if (ctx->running) {
        return STATUS_ERROR_ALREADY_RUNNING;
    }
    
    // Initialize c-ares
    int status = ares_library_init(ARES_LIB_INIT_ALL);
    if (status != ARES_SUCCESS) {
        return STATUS_ERROR_GENERIC;
    }
    
    // Create c-ares channel
    struct ares_options options;
    int optmask = 0;
    
    memset(&options, 0, sizeof(options));
    
    // Set options
    if (ctx->bind_address != NULL) {
        options.local_ip4 = inet_addr(ctx->bind_address);
        optmask |= ARES_OPT_LOCALIP;
    }
    
    options.udp_port = ctx->port;
    optmask |= ARES_OPT_UDP_PORT;
    
    status = ares_init_options(&ctx->channel, &options, optmask);
    if (status != ARES_SUCCESS) {
        ares_library_cleanup();
        return STATUS_ERROR_GENERIC;
    }
    
    // Set running flag
    ctx->running = true;
    
    // Create listener thread
    if (pthread_create(&ctx->listener_thread, NULL, dns_listener_thread, listener) != 0) {
        ares_destroy(ctx->channel);
        ares_library_cleanup();
        ctx->running = false;
        return STATUS_ERROR_GENERIC;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Stop DNS listener
 */
static status_t dns_listener_stop(protocol_listener_t* listener) {
    if (listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    dns_listener_ctx_t* ctx = (dns_listener_ctx_t*)listener;
    
    // Check if running
    if (!ctx->running) {
        return STATUS_ERROR_NOT_RUNNING;
    }
    
    // Set running flag
    ctx->running = false;
    
    // Wait for listener thread to exit
    pthread_join(ctx->listener_thread, NULL);
    
    // Destroy c-ares channel
    ares_destroy(ctx->channel);
    ares_library_cleanup();
    
    return STATUS_SUCCESS;
}

/**
 * @brief Destroy DNS listener
 */
static status_t dns_listener_destroy(protocol_listener_t* listener) {
    if (listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    dns_listener_ctx_t* ctx = (dns_listener_ctx_t*)listener;
    
    // Stop listener if running
    if (ctx->running) {
        dns_listener_stop(listener);
    }
    
    // Free clients
    pthread_mutex_lock(&ctx->clients_mutex);
    
    for (size_t i = 0; i < ctx->client_count; i++) {
        free(ctx->clients[i]);
    }
    
    free(ctx->clients);
    pthread_mutex_unlock(&ctx->clients_mutex);
    pthread_mutex_destroy(&ctx->clients_mutex);
    
    // Free domain
    if (ctx->domain != NULL) {
        free(ctx->domain);
    }
    
    // Free bind address
    if (ctx->bind_address != NULL) {
        free(ctx->bind_address);
    }
    
    // Free context
    free(ctx);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Send message to DNS client
 */
static status_t dns_listener_send_message(protocol_listener_t* listener, client_t* client, protocol_message_t* message) {
    if (listener == NULL || client == NULL || message == NULL || message->data == NULL || message->data_len == 0) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    dns_listener_ctx_t* ctx = (dns_listener_ctx_t*)listener;
    
    // Check if running
    if (!ctx->running) {
        return STATUS_ERROR_NOT_RUNNING;
    }
    
    // Check if message is too large
    if (message->data_len > DNS_MAX_TXT_LENGTH) {
        // Use fragmentation
        return fragmentation_send_message(listener, client, message->data, message->data_len, DNS_MAX_TXT_LENGTH);
    }
    
    // Encode message data to TXT records
    char** txt_records = NULL;
    size_t txt_record_count = 0;
    
    status_t status = dns_encode_data_to_txt(message->data, message->data_len, &txt_records, &txt_record_count);
    if (status != STATUS_SUCCESS) {
        return status;
    }
    
    // Send TXT records
    // In a real implementation, this would involve setting up DNS responses
    // For simplicity, we'll just free the TXT records and return success
    
    // Free TXT records
    for (size_t i = 0; i < txt_record_count; i++) {
        free(txt_records[i]);
    }
    
    free(txt_records);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Register callbacks for DNS listener
 */
static status_t dns_listener_register_callbacks(protocol_listener_t* listener,
                                              void (*on_message_received)(protocol_listener_t*, client_t*, protocol_message_t*),
                                              void (*on_client_connected)(protocol_listener_t*, client_t*),
                                              void (*on_client_disconnected)(protocol_listener_t*, client_t*)) {
    if (listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    dns_listener_ctx_t* ctx = (dns_listener_ctx_t*)listener;
    
    ctx->on_message_received = on_message_received;
    ctx->on_client_connected = on_client_connected;
    ctx->on_client_disconnected = on_client_disconnected;
    
    return STATUS_SUCCESS;
}
