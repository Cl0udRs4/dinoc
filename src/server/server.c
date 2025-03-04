/**
 * @file server.c
 * @brief Server implementation for C2 server
 */

#define _GNU_SOURCE /* For strdup */

#include "../include/server.h"
#include "../include/protocol.h"
#include "../include/client.h"
#include "../include/task.h"
#include "../include/module.h"
#include "../include/console.h"
#include "../common/logger.h"
#include "../common/config.h"
#include "../common/uuid.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <pthread.h>

// Server state
static bool server_running = false;
static pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;
static server_config_t server_config;

// Protocol listeners
static protocol_listener_t* tcp_listener = NULL;
static protocol_listener_t* udp_listener = NULL;
static protocol_listener_t* ws_listener = NULL;
static protocol_listener_t* icmp_listener = NULL;
static protocol_listener_t* dns_listener = NULL;

// Forward declarations
static void on_message_received(protocol_listener_t* listener, client_t* client, protocol_message_t* message);
static void on_client_connected(protocol_listener_t* listener, client_t* client);
static void on_client_disconnected(protocol_listener_t* listener, client_t* client);

/**
 * @brief Initialize server
 */
status_t server_init(const server_config_t* config) {
    if (config == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Copy configuration
    memcpy(&server_config, config, sizeof(server_config_t));
    
    // Initialize components
    status_t status;
    
    status = logger_init(server_config.log_file, server_config.log_level);
    if (status != STATUS_SUCCESS) return status;
    
    status = protocol_manager_init();
    if (status != STATUS_SUCCESS) {
        logger_shutdown();
        return status;
    }
    
    status = client_manager_init();
    if (status != STATUS_SUCCESS) {
        protocol_manager_shutdown();
        logger_shutdown();
        return status;
    }
    
    status = task_manager_init();
    if (status != STATUS_SUCCESS) {
        client_manager_shutdown();
        protocol_manager_shutdown();
        logger_shutdown();
        return status;
    }
    
    status = module_manager_init();
    if (status != STATUS_SUCCESS) {
        task_manager_shutdown();
        client_manager_shutdown();
        protocol_manager_shutdown();
        logger_shutdown();
        return status;
    }
    
    status = console_init();
    if (status != STATUS_SUCCESS) {
        module_manager_shutdown();
        task_manager_shutdown();
        client_manager_shutdown();
        protocol_manager_shutdown();
        logger_shutdown();
        return status;
    }
    
    LOG_INFO("Server initialized");
    return STATUS_SUCCESS;
}

/**
 * @brief Start server
 */
status_t server_start(void) {
    pthread_mutex_lock(&server_mutex);
    
    if (server_running) {
        pthread_mutex_unlock(&server_mutex);
        return STATUS_ERROR_ALREADY_RUNNING;
    }
    
    LOG_INFO("Starting server");
    fprintf(stderr, "Starting server\n");
    
    // Protocol manager is already initialized in server_init()
    status_t status = STATUS_SUCCESS;
    
    LOG_INFO("Server starting with protocol manager");
    fprintf(stderr, "Server starting with protocol manager\n");
    
    // Create protocol listeners
    protocol_listener_config_t config;
    protocol_listener_t* listener;
    
    // TCP listener
    if (server_config.enable_tcp) {
        memset(&config, 0, sizeof(config));
        config.bind_address = server_config.bind_address;
        config.port = server_config.tcp_port;
        
        LOG_INFO("Creating TCP listener on %s:%d", config.bind_address, config.port);
        fprintf(stderr, "Creating TCP listener on %s:%d\n", config.bind_address, config.port);
        
        status = protocol_manager_create_listener(PROTOCOL_TYPE_TCP, &config, &tcp_listener);
        if (status != STATUS_SUCCESS) {
            return status;
        }
        
        LOG_INFO("TCP listener created successfully");
        fprintf(stderr, "TCP listener created successfully\n");
        
        // Start listener
        status = protocol_manager_start_listener(tcp_listener);
        if (status != STATUS_SUCCESS) {
            return status;
        }
        
        LOG_INFO("TCP listener started successfully");
        fprintf(stderr, "TCP listener started successfully\n");
    }
    
    // UDP listener
    if (server_config.enable_udp) {
        memset(&config, 0, sizeof(config));
        config.bind_address = server_config.bind_address;
        config.port = server_config.udp_port;
        
        LOG_INFO("Creating UDP listener on %s:%d", config.bind_address, config.port);
        fprintf(stderr, "Creating UDP listener on %s:%d\n", config.bind_address, config.port);
        
        status = protocol_manager_create_listener(PROTOCOL_TYPE_UDP, &config, &udp_listener);
        if (status != STATUS_SUCCESS) {
            return status;
        }
        
        LOG_INFO("UDP listener created successfully");
        fprintf(stderr, "UDP listener created successfully\n");
        
        // Start listener
        status = protocol_manager_start_listener(udp_listener);
        if (status != STATUS_SUCCESS) {
            return status;
        }
        
        LOG_INFO("UDP listener started successfully");
        fprintf(stderr, "UDP listener started successfully\n");
    }
    
    // WebSocket listener
    if (server_config.enable_ws) {
        memset(&config, 0, sizeof(config));
        config.bind_address = server_config.bind_address;
        config.port = server_config.ws_port;
        config.ws_path = "/";
        
        LOG_INFO("Creating WebSocket listener on %s:%d", config.bind_address, config.port);
        fprintf(stderr, "Creating WebSocket listener on %s:%d\n", config.bind_address, config.port);
        
        status = protocol_manager_create_listener(PROTOCOL_TYPE_WS, &config, &ws_listener);
        if (status != STATUS_SUCCESS) {
            return status;
        }
        
        LOG_INFO("WebSocket listener created successfully");
        fprintf(stderr, "WebSocket listener created successfully\n");
        
        // Start listener
        status = protocol_manager_start_listener(ws_listener);
        if (status != STATUS_SUCCESS) {
            return status;
        }
        
        LOG_INFO("WebSocket listener started successfully");
        fprintf(stderr, "WebSocket listener started successfully\n");
    }
    
    // ICMP listener
    if (server_config.enable_icmp) {
        memset(&config, 0, sizeof(config));
        config.pcap_device = server_config.pcap_device;
        
        LOG_INFO("Creating ICMP listener on device %s", config.pcap_device);
        fprintf(stderr, "Creating ICMP listener on device %s\n", config.pcap_device);
        
        status = protocol_manager_create_listener(PROTOCOL_TYPE_ICMP, &config, &icmp_listener);
        if (status != STATUS_SUCCESS) {
            return status;
        }
        
        LOG_INFO("ICMP listener created successfully");
        fprintf(stderr, "ICMP listener created successfully\n");
        
        // Start listener
        status = protocol_manager_start_listener(icmp_listener);
        if (status != STATUS_SUCCESS) {
            return status;
        }
        
        LOG_INFO("ICMP listener started successfully");
        fprintf(stderr, "ICMP listener started successfully\n");
    }
    
    // DNS listener
    if (server_config.enable_dns) {
        memset(&config, 0, sizeof(config));
        config.bind_address = server_config.bind_address;
        config.port = server_config.dns_port;
        config.domain = server_config.dns_domain;
        
        LOG_INFO("Creating DNS listener on %s:%d", config.bind_address, config.port);
        fprintf(stderr, "Creating DNS listener on %s:%d\n", config.bind_address, config.port);
        
        status = protocol_manager_create_listener(PROTOCOL_TYPE_DNS, &config, &dns_listener);
        if (status != STATUS_SUCCESS) {
            return status;
        }
        
        LOG_INFO("DNS listener created successfully");
        fprintf(stderr, "DNS listener created successfully\n");
        
        // Start listener
        status = protocol_manager_start_listener(dns_listener);
        if (status != STATUS_SUCCESS) {
            return status;
        }
        
        LOG_INFO("DNS listener started successfully");
        fprintf(stderr, "DNS listener started successfully\n");
    }
    
    // Start HTTP API server
    if (server_config.enable_http_api) {
        status = http_server_start(server_config.bind_address, server_config.http_api_port);
        if (status != STATUS_SUCCESS) {
            LOG_ERROR("Failed to start HTTP API server");
            fprintf(stderr, "Failed to start HTTP API server\n");
            return status;
        }
        
        LOG_INFO("HTTP API server started successfully");
    }
    
    // Start console
    if (server_config.enable_console) {
        status = console_init();
        if (status != STATUS_SUCCESS) {
            LOG_ERROR("Failed to initialize console");
            fprintf(stderr, "Failed to initialize console\n");
            return status;
        }
        
        status = console_start();
        if (status != STATUS_SUCCESS) {
            LOG_ERROR("Failed to start console");
            fprintf(stderr, "Failed to start console\n");
            return status;
        }
    }
    
    LOG_INFO("Server started successfully");
    fprintf(stderr, "Server started successfully\n");
    
    server_running = true;
    pthread_mutex_unlock(&server_mutex);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Stop server
 */
status_t server_stop(void) {
    pthread_mutex_lock(&server_mutex);
    
    if (!server_running) {
        pthread_mutex_unlock(&server_mutex);
        return STATUS_SUCCESS;
    }
    
    // Stop console if enabled
    if (server_config.enable_console) {
        console_stop();
        LOG_INFO("Console interface stopped");
    }
    
    // Stop HTTP API if enabled
    if (server_config.enable_http_api) {
        // TODO: Implement HTTP API
        LOG_INFO("HTTP API not implemented yet");
    }
    
    // Stop protocol listeners
    if (tcp_listener != NULL) {
        protocol_manager_stop_listener(tcp_listener);
        protocol_manager_destroy_listener(tcp_listener);
        tcp_listener = NULL;
        LOG_INFO("TCP listener stopped");
    }
    
    if (udp_listener != NULL) {
        protocol_manager_stop_listener(udp_listener);
        protocol_manager_destroy_listener(udp_listener);
        udp_listener = NULL;
        LOG_INFO("UDP listener stopped");
    }
    
    if (ws_listener != NULL) {
        protocol_manager_stop_listener(ws_listener);
        protocol_manager_destroy_listener(ws_listener);
        ws_listener = NULL;
        LOG_INFO("WebSocket listener stopped");
    }
    
    if (icmp_listener != NULL) {
        protocol_manager_stop_listener(icmp_listener);
        protocol_manager_destroy_listener(icmp_listener);
        icmp_listener = NULL;
        LOG_INFO("ICMP listener stopped");
    }
    
    if (dns_listener != NULL) {
        protocol_manager_stop_listener(dns_listener);
        protocol_manager_destroy_listener(dns_listener);
        dns_listener = NULL;
        LOG_INFO("DNS listener stopped");
    }
    
    server_running = false;
    pthread_mutex_unlock(&server_mutex);
    
    LOG_INFO("Server stopped");
    return STATUS_SUCCESS;
}

/**
 * @brief Shutdown server
 */
status_t server_shutdown(void) {
    // Stop server if running
    if (server_running) {
        server_stop();
    }
    
    // Shutdown components
    console_shutdown();
    module_manager_shutdown();
    task_manager_shutdown();
    client_manager_shutdown();
    protocol_manager_shutdown();
    logger_shutdown();
    
    // Free configuration
    server_free_config(&server_config);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Get server configuration
 */
const server_config_t* server_get_config(void) {
    return &server_config;
}

/**
 * @brief Parse command-line arguments
 */
status_t server_parse_args(int argc, char** argv, server_config_t* config) {
    if (config == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Initialize configuration with defaults
    memset(config, 0, sizeof(server_config_t));
    config->tcp_port = 8080;
    config->udp_port = 8081;
    config->ws_port = 8082;
    config->dns_port = 53;
    config->http_api_port = 8083;
    config->log_level = LOG_LEVEL_INFO;
    config->enable_tcp = true;
    config->enable_udp = true;
    config->enable_ws = true;
    config->enable_icmp = true;
    config->enable_dns = true;
    config->enable_http_api = true;
    config->enable_console = true;
    
    // Define options
    static struct option long_options[] = {
        {"config", required_argument, 0, 'c'},
        {"bind", required_argument, 0, 'b'},
        {"tcp-port", required_argument, 0, 't'},
        {"udp-port", required_argument, 0, 'u'},
        {"ws-port", required_argument, 0, 'w'},
        {"dns-port", required_argument, 0, 'd'},
        {"dns-domain", required_argument, 0, 'D'},
        {"pcap-device", required_argument, 0, 'p'},
        {"http-port", required_argument, 0, 'h'},
        {"log-file", required_argument, 0, 'l'},
        {"log-level", required_argument, 0, 'L'},
        {"disable-tcp", no_argument, 0, 1},
        {"disable-udp", no_argument, 0, 2},
        {"disable-ws", no_argument, 0, 3},
        {"disable-icmp", no_argument, 0, 4},
        {"disable-dns", no_argument, 0, 5},
        {"disable-http-api", no_argument, 0, 6},
        {"disable-console", no_argument, 0, 7},
        {"help", no_argument, 0, '?'},
        {0, 0, 0, 0}
    };
    
    // Parse options
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "c:b:t:u:w:d:D:p:h:l:L:?", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'c':
                config->config_file = strdup(optarg);
                break;
                
            case 'b':
                config->bind_address = strdup(optarg);
                break;
                
            case 't':
                config->tcp_port = (uint16_t)atoi(optarg);
                break;
                
            case 'u':
                config->udp_port = (uint16_t)atoi(optarg);
                break;
                
            case 'w':
                config->ws_port = (uint16_t)atoi(optarg);
                break;
                
            case 'd':
                config->dns_port = (uint16_t)atoi(optarg);
                break;
                
            case 'D':
                config->dns_domain = strdup(optarg);
                break;
                
            case 'p':
                config->pcap_device = strdup(optarg);
                break;
                
            case 'h':
                config->http_api_port = (uint16_t)atoi(optarg);
                break;
                
            case 'l':
                config->log_file = strdup(optarg);
                break;
                
            case 'L':
                config->log_level = (uint8_t)atoi(optarg);
                break;
                
            case 1:
                config->enable_tcp = false;
                break;
                
            case 2:
                config->enable_udp = false;
                break;
                
            case 3:
                config->enable_ws = false;
                break;
                
            case 4:
                config->enable_icmp = false;
                break;
                
            case 5:
                config->enable_dns = false;
                break;
                
            case 6:
                config->enable_http_api = false;
                break;
                
            case 7:
                config->enable_console = false;
                break;
                
            case '?':
                printf("Usage: %s [options]\n", argv[0]);
                printf("Options:\n");
                printf("  -c, --config FILE       Configuration file path\n");
                printf("  -b, --bind ADDRESS      Bind address for listeners\n");
                printf("  -t, --tcp-port PORT     TCP port (default: 8080)\n");
                printf("  -u, --udp-port PORT     UDP port (default: 8081)\n");
                printf("  -w, --ws-port PORT      WebSocket port (default: 8082)\n");
                printf("  -d, --dns-port PORT     DNS port (default: 53)\n");
                printf("  -D, --dns-domain DOMAIN DNS domain\n");
                printf("  -p, --pcap-device DEV   PCAP device for ICMP (default: any)\n");
                printf("  -h, --http-port PORT    HTTP API port (default: 8083)\n");
                printf("  -l, --log-file FILE     Log file path\n");
                printf("  -L, --log-level LEVEL   Log level (0-5, default: 3)\n");
                printf("      --disable-tcp       Disable TCP listener\n");
                printf("      --disable-udp       Disable UDP listener\n");
                printf("      --disable-ws        Disable WebSocket listener\n");
                printf("      --disable-icmp      Disable ICMP listener\n");
                printf("      --disable-dns       Disable DNS listener\n");
                printf("      --disable-http-api  Disable HTTP API\n");
                printf("      --disable-console   Disable console interface\n");
                printf("  -?, --help              Show this help message\n");
                return STATUS_ERROR_INVALID_PARAM;
                
            default:
                return STATUS_ERROR_INVALID_PARAM;
        }
    }
    
    // Load configuration file if specified
    if (config->config_file != NULL) {
        status_t status = server_load_config(config->config_file, config);
        if (status != STATUS_SUCCESS) {
            return status;
        }
    }
    
    // Set default values for unspecified options
    if (config->pcap_device == NULL) {
        config->pcap_device = strdup("any");
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Load configuration from file
 */
status_t server_load_config(const char* config_file, server_config_t* config) {
    if (config_file == NULL || config == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Load configuration from file
    status_t status = config_init(config_file);
    if (status != STATUS_SUCCESS) {
        return status;
    }
    
    // Get configuration values
    char bind_address[256] = {0};
    status = config_get_string("bind_address", bind_address, sizeof(bind_address));
    if (status == STATUS_SUCCESS && bind_address[0] != '\0') {
        if (config->bind_address != NULL) {
            free(config->bind_address);
        }
        config->bind_address = strdup(bind_address);
    }
    
    int64_t tcp_port = 0;
    status = config_get_int("tcp_port", &tcp_port);
    if (status == STATUS_SUCCESS && tcp_port > 0) {
        config->tcp_port = (uint16_t)tcp_port;
    }
    
    int64_t udp_port = 0;
    status = config_get_int("udp_port", &udp_port);
    if (status == STATUS_SUCCESS && udp_port > 0) {
        config->udp_port = (uint16_t)udp_port;
    }
    
    int64_t ws_port = 0;
    status = config_get_int("ws_port", &ws_port);
    if (status == STATUS_SUCCESS && ws_port > 0) {
        config->ws_port = (uint16_t)ws_port;
    }
    
    int64_t dns_port = 0;
    status = config_get_int("dns_port", &dns_port);
    if (status == STATUS_SUCCESS && dns_port > 0) {
        config->dns_port = (uint16_t)dns_port;
    }
    
    char dns_domain[256] = {0};
    status = config_get_string("dns_domain", dns_domain, sizeof(dns_domain));
    if (status == STATUS_SUCCESS && dns_domain[0] != '\0') {
        if (config->dns_domain != NULL) {
            free(config->dns_domain);
        }
        config->dns_domain = strdup(dns_domain);
    }
    
    char pcap_device[256] = {0};
    status = config_get_string("pcap_device", pcap_device, sizeof(pcap_device));
    if (status == STATUS_SUCCESS && pcap_device[0] != '\0') {
        if (config->pcap_device != NULL) {
            free(config->pcap_device);
        }
        config->pcap_device = strdup(pcap_device);
    }
    
    int64_t http_api_port = 0;
    status = config_get_int("http_api_port", &http_api_port);
    if (status == STATUS_SUCCESS && http_api_port > 0) {
        config->http_api_port = (uint16_t)http_api_port;
    }
    
    char log_file[256] = {0};
    status = config_get_string("log_file", log_file, sizeof(log_file));
    if (status == STATUS_SUCCESS && log_file[0] != '\0') {
        if (config->log_file != NULL) {
            free(config->log_file);
        }
        config->log_file = strdup(log_file);
    }
    
    int64_t log_level = 0;
    status = config_get_int("log_level", &log_level);
    if (status == STATUS_SUCCESS) {
        config->log_level = (uint8_t)log_level;
    }
    
    bool enable_tcp = false;
    status = config_get_bool("enable_tcp", &enable_tcp);
    if (status == STATUS_SUCCESS) {
        config->enable_tcp = enable_tcp;
    }
    
    bool enable_udp = false;
    status = config_get_bool("enable_udp", &enable_udp);
    if (status == STATUS_SUCCESS) {
        config->enable_udp = enable_udp;
    }
    
    bool enable_ws = false;
    status = config_get_bool("enable_ws", &enable_ws);
    if (status == STATUS_SUCCESS) {
        config->enable_ws = enable_ws;
    }
    
    bool enable_icmp = false;
    status = config_get_bool("enable_icmp", &enable_icmp);
    if (status == STATUS_SUCCESS) {
        config->enable_icmp = enable_icmp;
    }
    
    bool enable_dns = false;
    status = config_get_bool("enable_dns", &enable_dns);
    if (status == STATUS_SUCCESS) {
        config->enable_dns = enable_dns;
    }
    
    bool enable_http_api = false;
    status = config_get_bool("enable_http_api", &enable_http_api);
    if (status == STATUS_SUCCESS) {
        config->enable_http_api = enable_http_api;
    }
    
    bool enable_console = false;
    status = config_get_bool("enable_console", &enable_console);
    if (status == STATUS_SUCCESS) {
        config->enable_console = enable_console;
    }
    
    // Free configuration
    config_shutdown();
    
    return STATUS_SUCCESS;
}

/**
 * @brief Free server configuration
 */
status_t server_free_config(server_config_t* config) {
    if (config == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Free allocated memory
    if (config->config_file) free(config->config_file);
    if (config->bind_address) free(config->bind_address);
    if (config->dns_domain) free(config->dns_domain);
    if (config->pcap_device) free(config->pcap_device);
    if (config->log_file) free(config->log_file);
    
    // Reset configuration
    memset(config, 0, sizeof(server_config_t));
    
    return STATUS_SUCCESS;
}

/**
 * @brief Message received callback
 */
static void on_message_received(protocol_listener_t* listener, client_t* client, protocol_message_t* message) {
    if (listener == NULL || client == NULL || message == NULL) {
        return;
    }
    
    // Update client last seen time
    client_update_info(client, NULL, NULL, NULL);
    
    // Check if this is a protocol switch message
    // Format: "SWITCH:PROTOCOL_TYPE"
    if (message->data_len >= 8 && memcmp(message->data, "SWITCH:", 7) == 0) {
        char id_str[37];
        uuid_to_string(client->id, id_str, sizeof(id_str));
        LOG_INFO("Protocol switch message received from client %s", id_str);
        
        // Extract protocol type from message
        char protocol_str[32] = {0};
        size_t protocol_len = message->data_len - 7 < sizeof(protocol_str) - 1 ? 
                             message->data_len - 7 : sizeof(protocol_str) - 1;
        memcpy(protocol_str, (char*)message->data + 7, protocol_len);
        protocol_str[protocol_len] = '\0';
        
        // TODO: Implement actual protocol switching logic
        LOG_INFO("Client %s requested switch to protocol %s", id_str, protocol_str);
        return;
    }
    
    // Process message based on type
    // TODO: Implement message processing
    
    char id_str[37];
    uuid_to_string(client->id, id_str, sizeof(id_str));
    LOG_INFO("Received message from client %s", id_str);
}

/**
 * @brief Client connected callback
 */
static void on_client_connected(protocol_listener_t* listener, client_t* client) {
    if (listener == NULL || client == NULL) {
        return;
    }
    
    char id_str[37];
    uuid_to_string(client->id, id_str, sizeof(id_str));
    LOG_INFO("Client %s connected via protocol type %d", id_str, listener->protocol_type);
}

/**
 * @brief Client disconnected callback
 */
static void on_client_disconnected(protocol_listener_t* listener, client_t* client) {
    if (listener == NULL || client == NULL) {
        return;
    }
    
    char id_str[37];
    uuid_to_string(client->id, id_str, sizeof(id_str));
    LOG_INFO("Client %s disconnected from protocol type %d", id_str, listener->protocol_type);
}
