/**
 * @file icmp_listener.c
 * @brief Implementation of ICMP protocol listener with fragmentation support
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <pcap.h>
#include <errno.h>
#include <time.h>

// ICMP protocol constants
#define ICMP_ECHO_REQUEST 8
#define ICMP_ECHO_REPLY 0
#define ICMP_HEADER_SIZE 8
#define IP_HEADER_SIZE 20

// Maximum ICMP data size (to avoid fragmentation at IP level)
#define MAX_ICMP_DATA_SIZE 1400

// ICMP listener context
typedef struct {
    int raw_socket;                 // Raw socket for sending ICMP
    pcap_t* pcap_handle;            // PCAP handle for packet capture
    pthread_t listener_thread;      // Listener thread
    bool running;                   // Running flag
    char* bind_address;             // Bind address
    char* pcap_device;              // PCAP device name
    uint32_t timeout_ms;            // Timeout in milliseconds
    
    // Callbacks
    void (*on_message_received)(protocol_listener_t*, client_t*, protocol_message_t*);
    void (*on_client_connected)(protocol_listener_t*, client_t*);
    void (*on_client_disconnected)(protocol_listener_t*, client_t*);
    
    // Client tracking
    client_t** clients;              // Array of clients
    size_t client_count;             // Number of clients
    size_t client_capacity;          // Capacity of clients array
    pthread_mutex_t clients_mutex;   // Mutex for clients array
} icmp_listener_ctx_t;

// Forward declarations
static void* icmp_listener_thread(void* arg);
static status_t icmp_listener_start(protocol_listener_t* listener);
static status_t icmp_listener_stop(protocol_listener_t* listener);
static status_t icmp_listener_destroy(protocol_listener_t* listener);
static status_t icmp_listener_send_message(protocol_listener_t* listener, client_t* client, protocol_message_t* message);
static status_t icmp_listener_register_callbacks(protocol_listener_t* listener,
                                               void (*on_message_received)(protocol_listener_t*, client_t*, protocol_message_t*),
                                               void (*on_client_connected)(protocol_listener_t*, client_t*),
                                               void (*on_client_disconnected)(protocol_listener_t*, client_t*));
static void icmp_packet_handler(u_char* user, const struct pcap_pkthdr* pkthdr, const u_char* packet);
static client_t* icmp_find_or_create_client(icmp_listener_ctx_t* ctx, const char* ip_address);
static uint16_t icmp_checksum(uint16_t* addr, int len);

/**
 * @brief ICMP packet handler
 */
static void icmp_packet_handler(u_char* user, const struct pcap_pkthdr* pkthdr, const u_char* packet) {
    protocol_listener_t* listener = (protocol_listener_t*)user;
    icmp_listener_ctx_t* ctx = (icmp_listener_ctx_t*)listener;
    
    // Check if packet is large enough to contain IP and ICMP headers
    if (pkthdr->len < IP_HEADER_SIZE + ICMP_HEADER_SIZE) {
        return;
    }
    
    // Get IP header
    const struct ip* ip_header = (struct ip*)(packet);
    int ip_header_len = ip_header->ip_hl * 4;
    
    // Get ICMP header
    const struct icmp* icmp_header = (struct icmp*)(packet + ip_header_len);
    
    // Check if it's an ICMP echo request
    if (icmp_header->icmp_type != ICMP_ECHO_REQUEST) {
        return;
    }
    
    // Get source IP address
    char src_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(ip_header->ip_src), src_ip, INET_ADDRSTRLEN);
    
    // Find or create client
    client_t* client = icmp_find_or_create_client(ctx, src_ip);
    if (client == NULL) {
        return;
    }
    
    // Update client last seen time
    client->last_seen_time = time(NULL);
    
    // Get ICMP data
    const uint8_t* icmp_data = (uint8_t*)(packet + ip_header_len + ICMP_HEADER_SIZE);
    size_t icmp_data_len = pkthdr->len - ip_header_len - ICMP_HEADER_SIZE;
    
    // Create message
    protocol_message_t message;
    message.data = (uint8_t*)icmp_data;
    message.data_len = icmp_data_len;
    
    // Call message received callback
    if (ctx->on_message_received != NULL) {
        ctx->on_message_received(listener, client, &message);
    }
}

/**
 * @brief Find or create client by IP address
 */
static client_t* icmp_find_or_create_client(icmp_listener_ctx_t* ctx, const char* ip_address) {
    if (ctx == NULL || ip_address == NULL) {
        return NULL;
    }
    
    client_t* client = NULL;
    
    pthread_mutex_lock(&ctx->clients_mutex);
    
    // Find client by IP address
    for (size_t i = 0; i < ctx->client_count; i++) {
        if (strcmp(ctx->clients[i]->address, ip_address) == 0) {
            client = ctx->clients[i];
            break;
        }
    }
    
    // Create new client if not found
    if (client == NULL) {
        client = (client_t*)malloc(sizeof(client_t));
        if (client == NULL) {
            pthread_mutex_unlock(&ctx->clients_mutex);
            return NULL;
        }
        
        // Initialize client
        memset(client, 0, sizeof(client_t));
        uuid_generate(&client->id);
        client->connection_time = time(NULL);
        client->last_seen_time = client->connection_time;
        strncpy(client->address, ip_address, sizeof(client->address) - 1);
        
        // Resize clients array if needed
        if (ctx->client_count >= ctx->client_capacity) {
            size_t new_capacity = ctx->client_capacity * 2;
            client_t** new_clients = (client_t**)realloc(ctx->clients, new_capacity * sizeof(client_t*));
            
            if (new_clients == NULL) {
                pthread_mutex_unlock(&ctx->clients_mutex);
                free(client);
                return NULL;
            }
            
            ctx->clients = new_clients;
            ctx->client_capacity = new_capacity;
        }
        
        ctx->clients[ctx->client_count++] = client;
        
        // Call client connected callback
        if (ctx->on_client_connected != NULL) {
            ctx->on_client_connected((protocol_listener_t*)ctx, client);
        }
    }
    
    pthread_mutex_unlock(&ctx->clients_mutex);
    
    return client;
}

/**
 * @brief Calculate ICMP checksum
 */
static uint16_t icmp_checksum(uint16_t* addr, int len) {
    int nleft = len;
    int sum = 0;
    uint16_t* w = addr;
    uint16_t answer = 0;
    
    // Adding 16-bit words
    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }
    
    // Add left-over byte, if any
    if (nleft == 1) {
        *(uint8_t*)(&answer) = *(uint8_t*)w;
        sum += answer;
    }
    
    // Fold 32-bit sum to 16 bits
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    answer = ~sum;
    
    return answer;
}

/**
 * @brief ICMP listener thread
 */
static void* icmp_listener_thread(void* arg) {
    protocol_listener_t* listener = (protocol_listener_t*)arg;
    icmp_listener_ctx_t* ctx = (icmp_listener_ctx_t*)listener;
    
    // Run packet capture loop
    while (ctx->running) {
        pcap_dispatch(ctx->pcap_handle, -1, icmp_packet_handler, (u_char*)listener);
    }
    
    return NULL;
}

/**
 * @brief Create an ICMP protocol listener
 */
status_t icmp_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener) {
    if (config == NULL || listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Create listener context
    icmp_listener_ctx_t* ctx = (icmp_listener_ctx_t*)malloc(sizeof(icmp_listener_ctx_t));
    if (ctx == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize context
    memset(ctx, 0, sizeof(icmp_listener_ctx_t));
    
    // Copy config
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
    base->start = icmp_listener_start;
    base->stop = icmp_listener_stop;
    base->destroy = icmp_listener_destroy;
    base->send_message = icmp_listener_send_message;
    base->register_callbacks = icmp_listener_register_callbacks;
    
    // Set protocol type
    base->protocol_type = PROTOCOL_TYPE_ICMP;
    
    // Generate ID
    uuid_generate(&base->id);
    
    *listener = base;
    return STATUS_SUCCESS;
}

/**
 * @brief Start ICMP listener
 */
static status_t icmp_listener_start(protocol_listener_t* listener) {
    if (listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    icmp_listener_ctx_t* ctx = (icmp_listener_ctx_t*)listener;
    
    // Check if already running
    if (ctx->running) {
        return STATUS_ERROR_ALREADY_RUNNING;
    }
    
    // Create raw socket for sending ICMP
    ctx->raw_socket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (ctx->raw_socket < 0) {
        return STATUS_ERROR_GENERIC;
    }
    
    // Set socket options
    int on = 1;
    if (setsockopt(ctx->raw_socket, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
        close(ctx->raw_socket);
        return STATUS_ERROR_GENERIC;
    }
    
    // Find PCAP device if not specified
    if (ctx->pcap_device == NULL) {
        char errbuf[PCAP_ERRBUF_SIZE];
        pcap_if_t* alldevs;
        
        if (pcap_findalldevs(&alldevs, errbuf) == -1) {
            close(ctx->raw_socket);
            return STATUS_ERROR_GENERIC;
        }
        
        if (alldevs == NULL) {
            close(ctx->raw_socket);
            return STATUS_ERROR_GENERIC;
        }
        
        ctx->pcap_device = strdup(alldevs->name);
        pcap_freealldevs(alldevs);
        
        if (ctx->pcap_device == NULL) {
            close(ctx->raw_socket);
            return STATUS_ERROR_MEMORY;
        }
    }
    
    // Open PCAP device
    char errbuf[PCAP_ERRBUF_SIZE];
    ctx->pcap_handle = pcap_open_live(ctx->pcap_device, 65536, 1, 1000, errbuf);
    if (ctx->pcap_handle == NULL) {
        close(ctx->raw_socket);
        return STATUS_ERROR_GENERIC;
    }
    
    // Set PCAP filter to capture only ICMP echo requests
    struct bpf_program fp;
    char filter_exp[] = "icmp[icmptype] = icmp-echo";
    
    if (pcap_compile(ctx->pcap_handle, &fp, filter_exp, 0, PCAP_NETMASK_UNKNOWN) == -1) {
        pcap_close(ctx->pcap_handle);
        close(ctx->raw_socket);
        return STATUS_ERROR_GENERIC;
    }
    
    if (pcap_setfilter(ctx->pcap_handle, &fp) == -1) {
        pcap_freecode(&fp);
        pcap_close(ctx->pcap_handle);
        close(ctx->raw_socket);
        return STATUS_ERROR_GENERIC;
    }
    
    pcap_freecode(&fp);
    
    // Set running flag
    ctx->running = true;
    
    // Create listener thread
    if (pthread_create(&ctx->listener_thread, NULL, icmp_listener_thread, listener) != 0) {
        pcap_close(ctx->pcap_handle);
        close(ctx->raw_socket);
        ctx->running = false;
        return STATUS_ERROR_GENERIC;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Stop ICMP listener
 */
static status_t icmp_listener_stop(protocol_listener_t* listener) {
    if (listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    icmp_listener_ctx_t* ctx = (icmp_listener_ctx_t*)listener;
    
    // Check if running
    if (!ctx->running) {
        return STATUS_ERROR_NOT_RUNNING;
    }
    
    // Set running flag
    ctx->running = false;
    
    // Wait for listener thread to exit
    pthread_join(ctx->listener_thread, NULL);
    
    // Close PCAP handle
    if (ctx->pcap_handle != NULL) {
        pcap_close(ctx->pcap_handle);
        ctx->pcap_handle = NULL;
    }
    
    // Close raw socket
    if (ctx->raw_socket >= 0) {
        close(ctx->raw_socket);
        ctx->raw_socket = -1;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Destroy ICMP listener
 */
static status_t icmp_listener_destroy(protocol_listener_t* listener) {
    if (listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    icmp_listener_ctx_t* ctx = (icmp_listener_ctx_t*)listener;
    
    // Stop listener if running
    if (ctx->running) {
        icmp_listener_stop(listener);
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
    
    // Free PCAP device
    if (ctx->pcap_device != NULL) {
        free(ctx->pcap_device);
    }
    
    // Free context
    free(ctx);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Send message to ICMP client with fragmentation support
 */
static status_t icmp_listener_send_message(protocol_listener_t* listener, client_t* client, protocol_message_t* message) {
    if (listener == NULL || client == NULL || message == NULL || message->data == NULL || message->data_len == 0) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    icmp_listener_ctx_t* ctx = (icmp_listener_ctx_t*)listener;
    
    // Check if running
    if (!ctx->running) {
        return STATUS_ERROR_NOT_RUNNING;
    }
    
    // Check if message needs fragmentation
    if (message->data_len > MAX_ICMP_DATA_SIZE) {
        // Use fragmentation system to send large messages
        return fragmentation_send_message(listener, client, message->data, message->data_len, MAX_ICMP_DATA_SIZE);
    }
    
    // Prepare destination address
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    
    if (inet_pton(AF_INET, client->address, &dest_addr.sin_addr) <= 0) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Allocate buffer for IP header + ICMP header + data
    size_t packet_size = IP_HEADER_SIZE + ICMP_HEADER_SIZE + message->data_len;
    uint8_t* packet = (uint8_t*)malloc(packet_size);
    if (packet == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Prepare IP header
    struct ip* ip_header = (struct ip*)packet;
    ip_header->ip_v = 4;
    ip_header->ip_hl = 5;
    ip_header->ip_tos = 0;
    ip_header->ip_len = htons(packet_size);
    ip_header->ip_id = htons(rand() & 0xFFFF);
    ip_header->ip_off = 0;
    ip_header->ip_ttl = 64;
    ip_header->ip_p = IPPROTO_ICMP;
    ip_header->ip_sum = 0;
    
    // Set source and destination addresses
    ip_header->ip_src.s_addr = INADDR_ANY;
    ip_header->ip_dst = dest_addr.sin_addr;
    
    // Prepare ICMP header
    struct icmp* icmp_header = (struct icmp*)(packet + IP_HEADER_SIZE);
    icmp_header->icmp_type = ICMP_ECHO_REPLY;
    icmp_header->icmp_code = 0;
    icmp_header->icmp_cksum = 0;
    icmp_header->icmp_id = htons(rand() & 0xFFFF);
    icmp_header->icmp_seq = htons(rand() & 0xFFFF);
    
    // Copy message data
    memcpy(packet + IP_HEADER_SIZE + ICMP_HEADER_SIZE, message->data, message->data_len);
    
    // Calculate ICMP checksum
    icmp_header->icmp_cksum = icmp_checksum((uint16_t*)icmp_header, ICMP_HEADER_SIZE + message->data_len);
    
    // Send packet
    ssize_t sent = sendto(ctx->raw_socket, packet, packet_size, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    
    // Free packet buffer
    free(packet);
    
    if (sent < 0) {
        return STATUS_ERROR_GENERIC;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Register callbacks for ICMP listener
 */
static status_t icmp_listener_register_callbacks(protocol_listener_t* listener,
                                               void (*on_message_received)(protocol_listener_t*, client_t*, protocol_message_t*),
                                               void (*on_client_connected)(protocol_listener_t*, client_t*),
                                               void (*on_client_disconnected)(protocol_listener_t*, client_t*)) {
    if (listener == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    icmp_listener_ctx_t* ctx = (icmp_listener_ctx_t*)listener;
    
    ctx->on_message_received = on_message_received;
    ctx->on_client_connected = on_client_connected;
    ctx->on_client_disconnected = on_client_disconnected;
    
    return STATUS_SUCCESS;
}
