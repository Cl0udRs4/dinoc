/**
 * @file builder.c
 * @brief Client builder implementation
 */

#include "include/common.h"
#include "include/encryption.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

// Builder options
typedef struct {
    char* protocols;        // Comma-separated list of protocols
    char* domain;           // Domain name
    char* servers;          // Comma-separated list of server addresses
    char* modules;          // Comma-separated list of modules
    char* output;           // Output file name
    encryption_type_t encryption; // Encryption type
    bool debug;             // Debug flag
} builder_options_t;

/**
 * @brief Print usage information
 * 
 * @param program Program name
 */
void print_usage(const char* program) {
    printf("Usage: %s [options]\n", program);
    printf("Options:\n");
    printf("  -p, --protocol PROTOCOLS   Comma-separated list of protocols (tcp,udp,ws,icmp,dns)\n");
    printf("  -d, --domain DOMAIN        Domain name\n");
    printf("  -s, --servers SERVERS      Comma-separated list of server addresses\n");
    printf("  -m, --modules MODULES      Comma-separated list of modules\n");
    printf("  -o, --output FILE          Output file name\n");
    printf("  -e, --encryption TYPE      Encryption type (aes, chacha20)\n");
    printf("  -g, --debug                Enable debug mode\n");
    printf("  -h, --help                 Show this help message\n");
}

/**
 * @brief Parse command line arguments
 * 
 * @param argc Argument count
 * @param argv Argument values
 * @param options Options structure to fill
 * @return int 0 on success, non-zero on error
 */
int parse_arguments(int argc, char** argv, builder_options_t* options) {
    static struct option long_options[] = {
        {"protocol", required_argument, 0, 'p'},
        {"domain", required_argument, 0, 'd'},
        {"servers", required_argument, 0, 's'},
        {"modules", required_argument, 0, 'm'},
        {"output", required_argument, 0, 'o'},
        {"encryption", required_argument, 0, 'e'},
        {"debug", no_argument, 0, 'g'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;
    
    while ((c = getopt_long(argc, argv, "p:d:s:m:o:e:gh", long_options, &option_index)) != -1) {
        switch (c) {
            case 'p':
                options->protocols = strdup(optarg);
                break;
                
            case 'd':
                options->domain = strdup(optarg);
                break;
                
            case 's':
                options->servers = strdup(optarg);
                break;
                
            case 'm':
                options->modules = strdup(optarg);
                break;
                
            case 'o':
                options->output = strdup(optarg);
                break;
                
            case 'e':
                if (strcmp(optarg, "aes") == 0) {
                    options->encryption = ENCRYPTION_AES;
                } else if (strcmp(optarg, "chacha20") == 0) {
                    options->encryption = ENCRYPTION_CHACHA20;
                } else {
                    fprintf(stderr, "Invalid encryption type: %s\n", optarg);
                    return 1;
                }
                break;
                
            case 'g':
                options->debug = true;
                break;
                
            case 'h':
                print_usage(argv[0]);
                return 1;
                
            case '?':
                return 1;
                
            default:
                fprintf(stderr, "Unknown option: %c\n", c);
                return 1;
        }
    }
    
    // Check required options
    if (options->protocols == NULL) {
        fprintf(stderr, "Missing required option: --protocol\n");
        return 1;
    }
    
    if (options->servers == NULL) {
        fprintf(stderr, "Missing required option: --servers\n");
        return 1;
    }
    
    if (options->modules == NULL) {
        fprintf(stderr, "Missing required option: --modules\n");
        return 1;
    }
    
    // Set default output file name
    if (options->output == NULL) {
        options->output = strdup("client");
    }
    
    // Set default encryption type
    if (options->encryption == ENCRYPTION_NONE) {
        options->encryption = ENCRYPTION_AES;
    }
    
    return 0;
}

/**
 * @brief Free builder options
 * 
 * @param options Options to free
 */
void free_options(builder_options_t* options) {
    if (options->protocols != NULL) {
        free(options->protocols);
    }
    
    if (options->domain != NULL) {
        free(options->domain);
    }
    
    if (options->servers != NULL) {
        free(options->servers);
    }
    
    if (options->modules != NULL) {
        free(options->modules);
    }
    
    if (options->output != NULL) {
        free(options->output);
    }
}

/**
 * @brief Main function
 */
int main(int argc, char** argv) {
    // Initialize options
    builder_options_t options;
    memset(&options, 0, sizeof(options));
    
    // Parse arguments
    if (parse_arguments(argc, argv, &options) != 0) {
        return 1;
    }
    
    // Print builder configuration
    printf("Builder configuration:\n");
    printf("  Protocols: %s\n", options.protocols);
    printf("  Domain: %s\n", options.domain ? options.domain : "(none)");
    printf("  Servers: %s\n", options.servers);
    printf("  Modules: %s\n", options.modules);
    printf("  Output: %s\n", options.output);
    printf("  Encryption: %s\n", options.encryption == ENCRYPTION_AES ? "AES" : "ChaCha20");
    printf("  Debug: %s\n", options.debug ? "enabled" : "disabled");
    
    // TODO: Implement client building
    printf("Client building not implemented yet\n");
    
    // Free options
    free_options(&options);
    
    return 0;
}
