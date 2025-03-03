/**
 * @file server.c
 * @brief Server implementation for C2 server
 */

#include "../include/server.h"
#include "../include/protocol.h"
#include "../include/client.h"
#include "../include/task.h"
#include "../include/module.h"
#include "../include/console.h"
#include "../common/logger.h"
#include "../common/config.h"
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
    
    // Create protocol listeners
    status_t status;
    protocol_listener_config_t listener_config;
    
    // TCP listener
    if (server_config.enable_tcp) {
        memset(&listener_config, 0, sizeof(listener_config));
        listener_config.bind_address = server_config.bind_address;
        listener_config.port = server_config.tcp_port;
        
        status = protocol_manager_create_listener(PROTOCOL_TYPE_TCP, &listener_config, &tcp_listener);
        if (status != STATUS_SUCCESS) {
            LOG_ERROR("Failed to create TCP listener: %d", status);
            server_stop();
            pthread_mutex_unlock(&server_mutex);
            return status;
        }
        
        status = protocol_manager_register_callbacks(tcp_listener, on_message_received, on_client_connected, on_client_disconnected);
        if (status != STATUS_SUCCESS) {
            LOG_ERROR("Failed to register TCP listener callbacks: %d", status);
            server_stop();
            pthread_mutex_unlock(&server_mutex);
            return status;
        }
        
        status = protocol_manager_start_listener(tcp_listener);
        if (status != STATUS_SUCCESS) {
            LOG_ERROR("Failed to start TCP listener: %d", status);
            server_stop();
            pthread_mutex_unlock(&server_mutex);
            return status;
        }
        
        LOG_INFO("TCP listener started on port %d", server_config.tcp_port);
    }
    
    // UDP listener
    if (server_config.enable_udp) {
        memset(&listener_config, 0, sizeof(listener_config));
        listener_config.bind_address = server_config.bind_address;
        listener_config.port = server_config.udp_port;
        
        status = protocol_manager_create_listener(PROTOCOL_TYPE_UDP, &listener_config, &udp_listener);
        if (status != STATUS_SUCCESS) {
            LOG_ERROR("Failed to create UDP listener: %d", status);
            server_stop();
            pthread_mutex_unlock(&server_mutex);
            return status;
        }
        
        status = protocol_manager_register_callbacks(udp_listener, on_message_received, on_client_connected, on_client_disconnected);
        if (status != STATUS_SUCCESS) {
            LOG_ERROR("Failed to register UDP listener callbacks: %d", status);
            server_stop();
            pthread_mutex_unlock(&server_mutex);
            return status;
        }
        
        status = protocol_manager_start_listener(udp_listener);
        if (status != STATUS_SUCCESS) {
            LOG_ERROR("Failed to start UDP listener: %d", status);
            server_stop();
            pthread_mutex_unlock(&server_mutex);
            return status;
        }
        
        LOG_INFO("UDP listener started on port %d", server_config.udp_port);
    }
    
    // WebSocket listener
    if (server_config.enable_ws) {
        memset(&listener_config, 0, sizeof(listener_config));
        listener_config.bind_address = server_config.bind_address;
        listener_config.port = server_config.ws_port;
        listener_config.ws_path = "/";
        
        status = protocol_manager_create_listener(PROTOCOL_TYPE_WS, &listener_config, &ws_listener);
        if (status != STATUS_SUCCESS) {
            LOG_ERROR("Failed to create WebSocket listener: %d", status);
            server_stop();
            pthread_mutex_unlock(&server_mutex);
            return status;
        }
        
        status = protocol_manager_register_callbacks(ws_listener, on_message_received, on_client_connected, on_client_disconnected);
        if (status != STATUS_SUCCESS) {
            LOG_ERROR("Failed to register WebSocket listener callbacks: %d", status);
            server_stop();
            pthread_mutex_unlock(&server_mutex);
            return status;
        }
        
        status = protocol_manager_start_listener(ws_listener);
        if (status != STATUS_SUCCESS) {
            LOG_ERROR("Failed to start WebSocket listener: %d", status);
            server_stop();
            pthread_mutex_unlock(&server_mutex);
            return status;
        }
        
        LOG_INFO("WebSocket listener started on port %d", server_config.ws_port);
    }
    
    // ICMP listener
    if (server_config.enable_icmp) {
        memset(&listener_config, 0, sizeof(listener_config));
        listener_config.pcap_device = server_config.pcap_device;
        
        status = protocol_manager_create_listener(PROTOCOL_TYPE_ICMP, &listener_config, &icmp_listener);
        if (status != STATUS_SUCCESS) {
            LOG_ERROR("Failed to create ICMP listener: %d", status);
            server_stop();
            pthread_mutex_unlock(&server_mutex);
            return status;
        }
        
        status = protocol_manager_register_callbacks(icmp_listener, on_message_received, on_client_connected, on_client_disconnected);
        if (status != STATUS_SUCCESS) {
            LOG_ERROR("Failed to register ICMP listener callbacks: %d", status);
            server_stop();
            pthread_mutex_unlock(&server_mutex);
            return status;
        }
        
        status = protocol_manager_start_listener(icmp_listener);
        if (status != STATUS_SUCCESS) {
            LOG_ERROR("Failed to start ICMP listener: %d", status);
            server_stop();
            pthread_mutex_unlock(&server_mutex);
            return status;
        }
        
        LOG_INFO("ICMP listener started on device %s", server_config.pcap_device);
    }
    
    // DNS listener
    if (server_config.enable_dns) {
        memset(&listener_config, 0, sizeof(listener_config));
        listener_config.bind_address = server_config.bind_address;
        listener_config.port = server_config.dns_port;
        listener_config.domain = server_config.dns_domain;
        
        status = protocol_manager_create_listener(PROTOCOL_TYPE_DNS, &listener_config, &dns_listener);
        if (status != STATUS_SUCCESS) {
            LOG_ERROR("Failed to create DNS listener: %d", status);
            server_stop();
            pthread_mutex_unlock(&server_mutex);
            return status;
        }
        
        status = protocol_manager_register_callbacks(dns_listener, on_message_received, on_client_connected, on_client_disconnected);
        if (status != STATUS_SUCCESS) {
            LOG_ERROR("Failed to register DNS listener callbacks: %d", status);
            server_stop();
            pthread_mutex_unlock(&server_mutex);
            return status;
        }
        
        status = protocol_manager_start_listener(dns_listener);
        if (status != STATUS_SUCCESS) {
            LOG_ERROR("Failed to start DNS listener: %d", status);
            server_stop();
            pthread_mutex_unlock(&server_mutex);
            return status;
        }
        
        LOG_INFO("DNS listener started on port %d for domain %s", server_config.dns_port, server_config.dns_domain);
    }
    
    // Start console if enabled
    if (server_config.enable_console) {
        status = console_start();
        if (status != STATUS_SUCCESS) {
            LOG_ERROR("Failed to start console: %d", status);
            server_stop();
            pthread_mutex_unlock(&server_mutex);
            return status;
        }
        
        LOG_INFO("Console interface started");
    }
    
    // Start HTTP API if enabled
    if (server_config.enable_http_api) {
        // TODO: Implement HTTP API
        LOG_INFO("HTTP API not implemented yet");
    }
    
    server_running = true;
    pthread_mutex_unlock(&server_mutex);
    
    LOG_INFO("Server started");
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
    config_t cfg;
    status_t status = config_init(&cfg, config_file);
    if (status != STATUS_SUCCESS) {
        return status;
    }
    
    // Get configuration values
    char* bind_address = NULL;
    status = config_get_string(&cfg, "bind_address", &bind_address);
    if (status == STATUS_SUCCESS && bind_address != NULL) {
        if (config->bind_address != NULL) {
            free(config->bind_address);
        }
        config->bind_address = strdup(bind_address);
    }
    
    uint16_t tcp_port = 0;
    status = config_get_uint16(&cfg, "tcp_port", &tcp_port);
    if (status == STATUS_SUCCESS && tcp_port > 0) {
        config->tcp_port = tcp_port;
    }
    
    uint16_t udp_port = 0;
    status = config_get_uint16(&cfg, "udp_port", &udp_port);
    if (status == STATUS_SUCCESS && udp_port > 0) {
        config->udp_port = udp_port;
    }
    
    uint16_t ws_port = 0;
    status = config_get_uint16(&cfg, "ws_port", &ws_port);
    if (status == STATUS_SUCCESS && ws_port > 0) {
        config->ws_port = ws_port;
    }
    
    uint16_t dns_port = 0;
    status = config_get_uint16(&cfg, "dns_port", &dns_port);
    if (status == STATUS_SUCCESS && dns_port > 0) {
        config->dns_port = dns_port;
    }
    
    char* dns_domain = NULL;
    status = config_get_string(&cfg, "dns_domain", &dns_domain);
    if (status == STATUS_SUCCESS && dns_domain != NULL) {
        if (config->dns_domain != NULL) {
            free(config->dns_domain);
        }
        config->dns_domain = strdup(dns_domain);
    }
    
    char* pcap_device = NULL;
    status = config_get_string(&cfg, "pcap_device", &pcap_device);
    if (status == STATUS_SUCCESS && pcap_device != NULL) {
        if (config->pcap_device != NULL) {
            free(config->pcap_device);
        }
        config->pcap_device = strdup(pcap_device);
    }
    
    uint16_t http_api_port = 0;
    status = config_get_uint16(&cfg, "http_api_port", &http_api_port);
    if (status == STATUS_SUCCESS && http_api_port > 0) {
        config->http_api_port = http_api_port;
    }
    
    char* log_file = NULL;
    status = config_get_string(&cfg, "log_file", &log_file);
    if (status == STATUS_SUCCESS && log_file != NULL) {
        if (config->log_file != NULL) {
            free(config->log_file);
        }
        config->log_file = strdup(log_file);
    }
    
    uint8_t log_level = 0;
    status = config_get_uint8(&cfg, "log_level", &log_level);
    if (status == STATUS_SUCCESS) {
        config->log_level = log_level;
    }
    
    bool enable_tcp = false;
    status = config_get_bool(&cfg, "enable_tcp", &enable_tcp);
    if (status == STATUS_SUCCESS) {
        config->enable_tcp = enable_tcp;
    }
    
    bool enable_udp = false;
    status = config_get_bool(&cfg, "enable_udp", &enable_udp);
    if (status == STATUS_SUCCESS) {
        config->enable_udp = enable_udp;
    }
    
    bool enable_ws = false;
    status = config_get_bool(&cfg, "enable_ws", &enable_ws);
    if (status == STATUS_SUCCESS) {
        config->enable_ws = enable_ws;
    }
    
    bool enable_icmp = false;
    status = config_get_bool(&cfg, "enable_icmp", &enable_icmp);
    if (status == STATUS_SUCCESS) {
        config->enable_icmp = enable_icmp;
    }
    
    bool enable_dns = false;
    status = config_get_bool(&cfg, "enable_dns", &enable_dns);
    if (status == STATUS_SUCCESS) {
        config->enable_dns = enable_dns;
    }
    
    bool enable_http_api = false;
    status = config_get_bool(&cfg, "enable_http_api", &enable_http_api);
    if (status == STATUS_SUCCESS) {
        config->enable_http_api = enable_http_api;
    }
    
    bool enable_console = false;
    status = config_get_bool(&cfg, "enable_console", &enable_console);
    if (status == STATUS_SUCCESS) {
        config->enable_console = enable_console;
    }
    
    // Free configuration
    config_free(&cfg);
    
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
    client_update_last_seen(client);
    
    // Check if this is a protocol switch message
    if (protocol_switch_is_message(message->data, message->data_len)) {
        protocol_switch_process_message(client, message->data, message->data_len);
        return;
    }
    
    // Process message based on type
    // TODO: Implement message processing
    
    LOG_INFO("Received message from client %s", client->id);
}

/**
 * @brief Client connected callback
 */
static void on_client_connected(protocol_listener_t* listener, client_t* client) {
    if (listener == NULL || client == NULL) {
        return;
    }
    
    LOG_INFO("Client %s connected via %s", client->id, protocol_type_to_string(listener->protocol_type));
}

/**
 * @brief Client disconnected callback
 */
static void on_client_disconnected(protocol_listener_t* listener, client_t* client) {
    if (listener == NULL || client == NULL) {
        return;
    }
    
    LOG_INFO("Client %s disconnected from %s", client->id, protocol_type_to_string(listener->protocol_type));
}
