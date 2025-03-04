/**
 * @file template_generator.c
 * @brief Template generator implementation for builder
 */

#define _GNU_SOURCE  /* For strdup */

#include "template_generator.h"
#include "builder.h"
#include "client_template.h"
#include "../include/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Template file path
#define TEMPLATE_FILE_PATH "../../builder/client_template.c.template"

// Builder version
#define BUILDER_VERSION_MAJOR 1
#define BUILDER_VERSION_MINOR 0
#define BUILDER_VERSION_PATCH 0

// Helper functions for template generation
static char* read_template_file(const char* template_path);
static char* replace_template_placeholder(char* template_content, const char* placeholder, const char* replacement);
static char* generate_protocol_definitions(const protocol_type_t* protocols, size_t protocol_count);
static char* generate_server_definitions(const char** servers, size_t server_count);
static char* generate_domain_definition(const char* domain);
static char* generate_encryption_definition(encryption_algorithm_t algorithm);
static char* generate_module_definitions(const char** modules, size_t module_count);
static char* generate_protocol_fallback_code(const protocol_type_t* protocols, size_t protocol_count);
static char* generate_protocol_support_check(const protocol_type_t* protocols, size_t protocol_count);
static char* generate_protocol_connection_implementations(const protocol_type_t* protocols, size_t protocol_count, 
                                                         const char** servers, size_t server_count, const char* domain);
static char* generate_heartbeat_implementation(const protocol_type_t* protocols, size_t protocol_count);
static char* generate_module_forward_declarations(const char** modules, size_t module_count);
static char* generate_module_implementations(const char** modules, size_t module_count);

/**
 * @brief Initialize template generator
 */
status_t template_generator_init(void) {
    // Nothing to initialize for now
    return STATUS_SUCCESS;
}

/**
 * @brief Shutdown template generator
 */
status_t template_generator_shutdown(void) {
    // Nothing to clean up for now
    return STATUS_SUCCESS;
}

/**
 * @brief Read template file into memory
 */
static char* read_template_file(const char* template_path) {
    FILE* file = fopen(template_path, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Failed to open template file: %s\n", template_path);
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate memory for file content
    char* content = (char*)malloc(file_size + 1);
    if (content == NULL) {
        fclose(file);
        return NULL;
    }
    
    // Read file content
    size_t read_size = fread(content, 1, file_size, file);
    fclose(file);
    
    if (read_size != (size_t)file_size) {
        free(content);
        return NULL;
    }
    
    // Null-terminate content
    content[file_size] = '\0';
    
    return content;
}

/**
 * @brief Replace placeholder in template with replacement
 */
static char* replace_template_placeholder(char* template_content, const char* placeholder, const char* replacement) {
    if (template_content == NULL || placeholder == NULL || replacement == NULL) {
        return template_content;
    }
    
    // Find placeholder in template
    char* placeholder_pos = strstr(template_content, placeholder);
    if (placeholder_pos == NULL) {
        return template_content;
    }
    
    // Calculate new size
    size_t template_len = strlen(template_content);
    size_t placeholder_len = strlen(placeholder);
    size_t replacement_len = strlen(replacement);
    size_t new_len = template_len - placeholder_len + replacement_len;
    
    // Allocate memory for new content
    char* new_content = (char*)malloc(new_len + 1);
    if (new_content == NULL) {
        return template_content;
    }
    
    // Copy content before placeholder
    size_t prefix_len = placeholder_pos - template_content;
    memcpy(new_content, template_content, prefix_len);
    
    // Copy replacement
    memcpy(new_content + prefix_len, replacement, replacement_len);
    
    // Copy content after placeholder
    size_t suffix_len = template_len - prefix_len - placeholder_len;
    memcpy(new_content + prefix_len + replacement_len, placeholder_pos + placeholder_len, suffix_len);
    
    // Null-terminate new content
    new_content[new_len] = '\0';
    
    // Free old content
    free(template_content);
    
    return new_content;
}

/**
 * @brief Generate protocol definitions
 */
static char* generate_protocol_definitions(const protocol_type_t* protocols, size_t protocol_count) {
    if (protocols == NULL || protocol_count == 0) {
        return strdup("// No protocols defined");
    }
    
    // Calculate buffer size
    size_t buffer_size = 1024;  // Start with 1KB
    char* buffer = (char*)malloc(buffer_size);
    if (buffer == NULL) {
        return NULL;
    }
    
    // Initialize buffer
    buffer[0] = '\0';
    
    // Add protocol type definitions
    strcat(buffer, "// Protocol types\n");
    strcat(buffer, "typedef enum {\n");
    strcat(buffer, "    PROTOCOL_NONE = 0,\n");
    
    for (size_t i = 0; i < protocol_count; i++) {
        switch (protocols[i]) {
            case PROTOCOL_TYPE_TCP:
                strcat(buffer, "    PROTOCOL_TCP = 1,\n");
                break;
            case PROTOCOL_TYPE_UDP:
                strcat(buffer, "    PROTOCOL_UDP = 2,\n");
                break;
            case PROTOCOL_TYPE_WS:
                strcat(buffer, "    PROTOCOL_WS = 3,\n");
                break;
            case PROTOCOL_TYPE_ICMP:
                strcat(buffer, "    PROTOCOL_ICMP = 4,\n");
                break;
            case PROTOCOL_TYPE_DNS:
                strcat(buffer, "    PROTOCOL_DNS = 5,\n");
                break;
            default:
                break;
        }
    }
    
    strcat(buffer, "} protocol_type_t;\n\n");
    
    // Add protocol support flags
    strcat(buffer, "// Protocol support flags\n");
    
    for (size_t i = 0; i < protocol_count; i++) {
        switch (protocols[i]) {
            case PROTOCOL_TYPE_TCP:
                strcat(buffer, "#define SUPPORT_TCP 1\n");
                break;
            case PROTOCOL_TYPE_UDP:
                strcat(buffer, "#define SUPPORT_UDP 1\n");
                break;
            case PROTOCOL_TYPE_WS:
                strcat(buffer, "#define SUPPORT_WS 1\n");
                break;
            case PROTOCOL_TYPE_ICMP:
                strcat(buffer, "#define SUPPORT_ICMP 1\n");
                break;
            case PROTOCOL_TYPE_DNS:
                strcat(buffer, "#define SUPPORT_DNS 1\n");
                break;
            default:
                break;
        }
    }
    
    return buffer;
}

/**
 * @brief Generate server definitions
 */
static char* generate_server_definitions(const char** servers, size_t server_count) {
    if (servers == NULL || server_count == 0) {
        return strdup("// No servers defined");
    }
    
    // Calculate buffer size
    size_t buffer_size = 1024;  // Start with 1KB
    char* buffer = (char*)malloc(buffer_size);
    if (buffer == NULL) {
        return NULL;
    }
    
    // Initialize buffer
    buffer[0] = '\0';
    
    // Add server count definition
    char server_count_str[32];
    snprintf(server_count_str, sizeof(server_count_str), "%zu", server_count);
    strcat(buffer, "// Server count\n");
    strcat(buffer, "#define SERVER_COUNT ");
    strcat(buffer, server_count_str);
    strcat(buffer, "\n\n");
    
    // Add server definitions
    strcat(buffer, "// Server addresses\n");
    strcat(buffer, "static const char* server_addresses[SERVER_COUNT] = {\n");
    
    for (size_t i = 0; i < server_count; i++) {
        strcat(buffer, "    \"");
        strcat(buffer, servers[i]);
        strcat(buffer, "\"");
        
        if (i < server_count - 1) {
            strcat(buffer, ",");
        }
        
        strcat(buffer, "\n");
    }
    
    strcat(buffer, "};\n");
    
    return buffer;
}

/**
 * @brief Generate domain definition
 */
static char* generate_domain_definition(const char* domain) {
    if (domain == NULL) {
        return strdup("// No domain defined");
    }
    
    // Calculate buffer size
    size_t buffer_size = 256;  // Start with 256 bytes
    char* buffer = (char*)malloc(buffer_size);
    if (buffer == NULL) {
        return NULL;
    }
    
    // Initialize buffer
    buffer[0] = '\0';
    
    // Add domain definition
    strcat(buffer, "// Domain\n");
    strcat(buffer, "#define DOMAIN \"");
    strcat(buffer, domain);
    strcat(buffer, "\"\n");
    
    return buffer;
}

/**
 * @brief Generate encryption definition
 */
/**
 * @brief Generate module definitions
 */
static char* generate_module_definitions(const char** modules, size_t module_count) {
    if (modules == NULL || module_count == 0) {
        return strdup("// No modules defined");
    }
    
    // Calculate buffer size
    size_t buffer_size = 1024;  // Start with 1KB
    char* buffer = (char*)malloc(buffer_size);
    if (buffer == NULL) {
        return NULL;
    }
    
    // Initialize buffer
    buffer[0] = '\0';
    
    // Add module count definition
    char module_count_str[32];
    snprintf(module_count_str, sizeof(module_count_str), "%zu", module_count);
    strcat(buffer, "// Module count\n");
    strcat(buffer, "#define MODULE_COUNT ");
    strcat(buffer, module_count_str);
    strcat(buffer, "\n\n");
    
    // Add module definitions
    strcat(buffer, "// Module names\n");
    strcat(buffer, "static const char* module_names[MODULE_COUNT] = {\n");
    
    for (size_t i = 0; i < module_count; i++) {
        strcat(buffer, "    \"");
        strcat(buffer, modules[i]);
        strcat(buffer, "\"");
        
        if (i < module_count - 1) {
            strcat(buffer, ",");
        }
        
        strcat(buffer, "\n");
    }
    
    strcat(buffer, "};\n");
    
    return buffer;
}

/**
 * @brief Generate protocol fallback code
 */
static char* generate_protocol_fallback_code(const protocol_type_t* protocols, size_t protocol_count) {
    if (protocols == NULL || protocol_count <= 1) {
        return strdup("// No protocol fallback needed");
    }
    
    // Calculate buffer size
    size_t buffer_size = 1024;  // Start with 1KB
    char* buffer = (char*)malloc(buffer_size);
    if (buffer == NULL) {
        return NULL;
    }
    
    // Initialize buffer
    buffer[0] = '\0';
    
    // Add fallback code
    for (size_t i = 1; i < protocol_count; i++) {
        char protocol_name[32];
        
        switch (protocols[i]) {
            case PROTOCOL_TYPE_TCP:
                snprintf(protocol_name, sizeof(protocol_name), "PROTOCOL_TCP");
                break;
            case PROTOCOL_TYPE_UDP:
                snprintf(protocol_name, sizeof(protocol_name), "PROTOCOL_UDP");
                break;
            case PROTOCOL_TYPE_WS:
                snprintf(protocol_name, sizeof(protocol_name), "PROTOCOL_WS");
                break;
            case PROTOCOL_TYPE_ICMP:
                snprintf(protocol_name, sizeof(protocol_name), "PROTOCOL_ICMP");
                break;
            case PROTOCOL_TYPE_DNS:
                snprintf(protocol_name, sizeof(protocol_name), "PROTOCOL_DNS");
                break;
            default:
                continue;
        }
        
        strcat(buffer, "if (status != 0) {\n");
        strcat(buffer, "    #if CLIENT_DEBUG_MODE\n");
        strcat(buffer, "    printf(\"[DEBUG] Trying fallback protocol: ");
        strcat(buffer, protocol_name);
        strcat(buffer, "\\n\");\n");
        strcat(buffer, "    #endif\n");
        strcat(buffer, "    \n");
        strcat(buffer, "    status = client_connect_with_protocol(");
        strcat(buffer, protocol_name);
        strcat(buffer, ");\n");
        strcat(buffer, "}\n");
    }
    
    return buffer;
}

/**
 * @brief Generate protocol support check
 */
static char* generate_protocol_support_check(const protocol_type_t* protocols, size_t protocol_count) {
    if (protocols == NULL || protocol_count == 0) {
        return strdup("// No protocols to check");
    }
    
    // Calculate buffer size
    size_t buffer_size = 1024;  // Start with 1KB
    char* buffer = (char*)malloc(buffer_size);
    if (buffer == NULL) {
        return NULL;
    }
    
    // Initialize buffer
    buffer[0] = '\0';
    
    // Add support check
    for (size_t i = 0; i < protocol_count; i++) {
        char protocol_name[32];
        
        switch (protocols[i]) {
            case PROTOCOL_TYPE_TCP:
                snprintf(protocol_name, sizeof(protocol_name), "PROTOCOL_TCP");
                break;
            case PROTOCOL_TYPE_UDP:
                snprintf(protocol_name, sizeof(protocol_name), "PROTOCOL_UDP");
                break;
            case PROTOCOL_TYPE_WS:
                snprintf(protocol_name, sizeof(protocol_name), "PROTOCOL_WS");
                break;
            case PROTOCOL_TYPE_ICMP:
                snprintf(protocol_name, sizeof(protocol_name), "PROTOCOL_ICMP");
                break;
            case PROTOCOL_TYPE_DNS:
                snprintf(protocol_name, sizeof(protocol_name), "PROTOCOL_DNS");
                break;
            default:
                continue;
        }
        
        if (i > 0) {
            strcat(buffer, "    \n");
        }
        
        strcat(buffer, "    if (protocol_type == ");
        strcat(buffer, protocol_name);
        strcat(buffer, ") {\n");
        strcat(buffer, "        protocol_supported = 1;\n");
        strcat(buffer, "    }\n");
    }
    
    return buffer;
}

/**
 * @brief Generate protocol connection implementations
 */
static char* generate_protocol_connection_implementations(const protocol_type_t* protocols, size_t protocol_count, 
                                                         const char** servers, size_t server_count, const char* domain) {
    // Suppress unused parameter warnings
    (void)servers;
    (void)server_count;
    (void)domain;
    
    if (protocols == NULL || protocol_count == 0) {
        return strdup("// No protocols to implement");
    }
    
    // Calculate buffer size
    size_t buffer_size = 4096;  // Start with 4KB
    char* buffer = (char*)malloc(buffer_size);
    if (buffer == NULL) {
        return NULL;
    }
    
    // Initialize buffer
    buffer[0] = '\0';
    
    // Add connection implementations
    for (size_t i = 0; i < protocol_count; i++) {
        char protocol_name[32];
        
        switch (protocols[i]) {
            case PROTOCOL_TYPE_TCP:
                snprintf(protocol_name, sizeof(protocol_name), "PROTOCOL_TCP");
                strcat(buffer, "if (protocol_type == ");
                strcat(buffer, protocol_name);
                strcat(buffer, ") {\n");
                strcat(buffer, "    // TCP connection implementation\n");
                strcat(buffer, "    struct sockaddr_in server_addr;\n");
                strcat(buffer, "    int sockfd;\n");
                strcat(buffer, "    \n");
                strcat(buffer, "    // Create socket\n");
                strcat(buffer, "    sockfd = socket(AF_INET, SOCK_STREAM, 0);\n");
                strcat(buffer, "    if (sockfd < 0) {\n");
                strcat(buffer, "        #if CLIENT_DEBUG_MODE\n");
                strcat(buffer, "        perror(\"socket\");\n");
                strcat(buffer, "        #endif\n");
                strcat(buffer, "        return -1;\n");
                strcat(buffer, "    }\n");
                strcat(buffer, "    \n");
                strcat(buffer, "    // Connect to server\n");
                strcat(buffer, "    for (size_t j = 0; j < SERVER_COUNT; j++) {\n");
                strcat(buffer, "        char host[256];\n");
                strcat(buffer, "        int port = 0;\n");
                strcat(buffer, "        \n");
                strcat(buffer, "        // Parse host:port\n");
                strcat(buffer, "        if (sscanf(server_addresses[j], \"%255[^:]:%d\", host, &port) != 2) {\n");
                strcat(buffer, "            continue;\n");
                strcat(buffer, "        }\n");
                strcat(buffer, "        \n");
                strcat(buffer, "        // Get server address\n");
                strcat(buffer, "        struct hostent* server = gethostbyname(host);\n");
                strcat(buffer, "        if (server == NULL) {\n");
                strcat(buffer, "            continue;\n");
                strcat(buffer, "        }\n");
                strcat(buffer, "        \n");
                strcat(buffer, "        // Set up server address\n");
                strcat(buffer, "        memset(&server_addr, 0, sizeof(server_addr));\n");
                strcat(buffer, "        server_addr.sin_family = AF_INET;\n");
                strcat(buffer, "        memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);\n");
                strcat(buffer, "        server_addr.sin_port = htons(port);\n");
                strcat(buffer, "        \n");
                strcat(buffer, "        // Connect\n");
                strcat(buffer, "        if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == 0) {\n");
                strcat(buffer, "            // Connection successful\n");
                strcat(buffer, "            client_connected = 1;\n");
                strcat(buffer, "            \n");
                strcat(buffer, "            #if CLIENT_DEBUG_MODE\n");
                strcat(buffer, "            printf(\"[DEBUG] Connected to %s:%d using TCP\\n\", host, port);\n");
                strcat(buffer, "            #endif\n");
                strcat(buffer, "            \n");
                strcat(buffer, "            return 0;\n");
                strcat(buffer, "        }\n");
                strcat(buffer, "    }\n");
                strcat(buffer, "    \n");
                strcat(buffer, "    // No server available\n");
                strcat(buffer, "    close(sockfd);\n");
                strcat(buffer, "    return -1;\n");
                strcat(buffer, "}\n");
                break;
                
            case PROTOCOL_TYPE_UDP:
                snprintf(protocol_name, sizeof(protocol_name), "PROTOCOL_UDP");
                strcat(buffer, "if (protocol_type == ");
                strcat(buffer, protocol_name);
                strcat(buffer, ") {\n");
                strcat(buffer, "    // UDP connection implementation\n");
                strcat(buffer, "    #if CLIENT_DEBUG_MODE\n");
                strcat(buffer, "    printf(\"[DEBUG] UDP protocol not fully implemented yet\\n\");\n");
                strcat(buffer, "    #endif\n");
                strcat(buffer, "    \n");
                strcat(buffer, "    return -1;\n");
                strcat(buffer, "}\n");
                break;
                
            case PROTOCOL_TYPE_WS:
                snprintf(protocol_name, sizeof(protocol_name), "PROTOCOL_WS");
                strcat(buffer, "if (protocol_type == ");
                strcat(buffer, protocol_name);
                strcat(buffer, ") {\n");
                strcat(buffer, "    // WebSocket connection implementation\n");
                strcat(buffer, "    #if CLIENT_DEBUG_MODE\n");
                strcat(buffer, "    printf(\"[DEBUG] WebSocket protocol not fully implemented yet\\n\");\n");
                strcat(buffer, "    #endif\n");
                strcat(buffer, "    \n");
                strcat(buffer, "    return -1;\n");
                strcat(buffer, "}\n");
                break;
                
            case PROTOCOL_TYPE_ICMP:
                snprintf(protocol_name, sizeof(protocol_name), "PROTOCOL_ICMP");
                strcat(buffer, "if (protocol_type == ");
                strcat(buffer, protocol_name);
                strcat(buffer, ") {\n");
                strcat(buffer, "    // ICMP connection implementation\n");
                strcat(buffer, "    #if CLIENT_DEBUG_MODE\n");
                strcat(buffer, "    printf(\"[DEBUG] ICMP protocol not fully implemented yet\\n\");\n");
                strcat(buffer, "    #endif\n");
                strcat(buffer, "    \n");
                strcat(buffer, "    return -1;\n");
                strcat(buffer, "}\n");
                break;
                
            case PROTOCOL_TYPE_DNS:
                snprintf(protocol_name, sizeof(protocol_name), "PROTOCOL_DNS");
                strcat(buffer, "if (protocol_type == ");
                strcat(buffer, protocol_name);
                strcat(buffer, ") {\n");
                strcat(buffer, "    // DNS connection implementation\n");
                strcat(buffer, "    #if CLIENT_DEBUG_MODE\n");
                strcat(buffer, "    printf(\"[DEBUG] DNS protocol not fully implemented yet\\n\");\n");
                strcat(buffer, "    #endif\n");
                strcat(buffer, "    \n");
                strcat(buffer, "    return -1;\n");
                strcat(buffer, "}\n");
                break;
                
            default:
                continue;
        }
    }
    
    return buffer;
}

/**
 * @brief Generate heartbeat implementation
 */
static char* generate_heartbeat_implementation(const protocol_type_t* protocols, size_t protocol_count) {
    // Calculate buffer size
    size_t buffer_size = 1024;  // Start with 1KB
    char* buffer = (char*)malloc(buffer_size);
    if (buffer == NULL) {
        return NULL;
    }
    
    // Initialize buffer
    buffer[0] = '\0';
    
    // Add heartbeat implementation
    strcat(buffer, "// Implement heartbeat based on current protocol\n");
    strcat(buffer, "switch (current_protocol) {\n");
    
    for (size_t i = 0; i < protocol_count; i++) {
        switch (protocols[i]) {
            case PROTOCOL_TYPE_TCP:
                strcat(buffer, "    case PROTOCOL_TCP:\n");
                strcat(buffer, "        // TCP heartbeat implementation\n");
                strcat(buffer, "        // TODO: Implement actual TCP heartbeat\n");
                strcat(buffer, "        break;\n");
                break;
                
            case PROTOCOL_TYPE_UDP:
                strcat(buffer, "    case PROTOCOL_UDP:\n");
                strcat(buffer, "        // UDP heartbeat implementation\n");
                strcat(buffer, "        // TODO: Implement actual UDP heartbeat\n");
                strcat(buffer, "        break;\n");
                break;
                
            case PROTOCOL_TYPE_WS:
                strcat(buffer, "    case PROTOCOL_WS:\n");
                strcat(buffer, "        // WebSocket heartbeat implementation\n");
                strcat(buffer, "        // TODO: Implement actual WebSocket heartbeat\n");
                strcat(buffer, "        break;\n");
                break;
                
            case PROTOCOL_TYPE_ICMP:
                strcat(buffer, "    case PROTOCOL_ICMP:\n");
                strcat(buffer, "        // ICMP heartbeat implementation\n");
                strcat(buffer, "        // TODO: Implement actual ICMP heartbeat\n");
                strcat(buffer, "        break;\n");
                break;
                
            case PROTOCOL_TYPE_DNS:
                strcat(buffer, "    case PROTOCOL_DNS:\n");
                strcat(buffer, "        // DNS heartbeat implementation\n");
                strcat(buffer, "        // TODO: Implement actual DNS heartbeat\n");
                strcat(buffer, "        break;\n");
                break;
                
            default:
                break;
        }
    }
    
    strcat(buffer, "    default:\n");
    strcat(buffer, "        break;\n");
    strcat(buffer, "}\n");
    
    return buffer;
}

/**
 * @brief Generate module forward declarations
 */
static char* generate_module_forward_declarations(const char** modules, size_t module_count) {
    if (modules == NULL || module_count == 0) {
        return strdup("// No module forward declarations needed");
    }
    
    // Calculate buffer size
    size_t buffer_size = 1024;  // Start with 1KB
    char* buffer = (char*)malloc(buffer_size);
    if (buffer == NULL) {
        return NULL;
    }
    
    // Initialize buffer
    buffer[0] = '\0';
    
    // Add forward declarations
    for (size_t i = 0; i < module_count; i++) {
        strcat(buffer, "static int module_");
        strcat(buffer, modules[i]);
        strcat(buffer, "_init(void);\n");
        
        strcat(buffer, "static int module_");
        strcat(buffer, modules[i]);
        strcat(buffer, "_shutdown(void);\n");
        
        strcat(buffer, "static int module_");
        strcat(buffer, modules[i]);
        strcat(buffer, "_execute(const uint8_t* data, size_t data_len);\n");
    }
    
    return buffer;
}

/**
 * @brief Generate module implementations
 */
static char* generate_module_implementations(const char** modules, size_t module_count) {
    if (modules == NULL || module_count == 0) {
        return strdup("// No module implementations needed");
    }
    
    // Calculate buffer size
    size_t buffer_size = 4096;  // Start with 4KB
    char* buffer = (char*)malloc(buffer_size);
    if (buffer == NULL) {
        return NULL;
    }
    
    // Initialize buffer
    buffer[0] = '\0';
    
    // Add module implementations
    for (size_t i = 0; i < module_count; i++) {
        // Module load function
        strcat(buffer, "/**\n");
        strcat(buffer, " * @brief Load ");
        strcat(buffer, modules[i]);
        strcat(buffer, " module\n");
        strcat(buffer, " */\n");
        strcat(buffer, "int client_load_module_");
        strcat(buffer, modules[i]);
        strcat(buffer, "(void) {\n");
        strcat(buffer, "    // Check if initialized\n");
        strcat(buffer, "    if (!client_initialized) {\n");
        strcat(buffer, "        return -1;\n");
        strcat(buffer, "    }\n");
        strcat(buffer, "    \n");
        strcat(buffer, "    // Initialize module\n");
        strcat(buffer, "    if (module_");
        strcat(buffer, modules[i]);
        strcat(buffer, "_init() != 0) {\n");
        strcat(buffer, "        return -1;\n");
        strcat(buffer, "    }\n");
        strcat(buffer, "    \n");
        strcat(buffer, "    #if CLIENT_DEBUG_MODE\n");
        strcat(buffer, "    printf(\"[DEBUG] Module '");
        strcat(buffer, modules[i]);
        strcat(buffer, "' loaded\\n\");\n");
        strcat(buffer, "    #endif\n");
        strcat(buffer, "    \n");
        strcat(buffer, "    return 0;\n");
        strcat(buffer, "}\n");
        strcat(buffer, "\n");
        
        // Module unload function
        strcat(buffer, "/**\n");
        strcat(buffer, " * @brief Unload ");
        strcat(buffer, modules[i]);
        strcat(buffer, " module\n");
        strcat(buffer, " */\n");
        strcat(buffer, "int client_unload_module_");
        strcat(buffer, modules[i]);
        strcat(buffer, "(void) {\n");
        strcat(buffer, "    // Check if initialized\n");
        strcat(buffer, "    if (!client_initialized) {\n");
        strcat(buffer, "        return -1;\n");
        strcat(buffer, "    }\n");
        strcat(buffer, "    \n");
        strcat(buffer, "    // Shutdown module\n");
        strcat(buffer, "    if (module_");
        strcat(buffer, modules[i]);
        strcat(buffer, "_shutdown() != 0) {\n");
        strcat(buffer, "        return -1;\n");
        strcat(buffer, "    }\n");
        strcat(buffer, "    \n");
        strcat(buffer, "    #if CLIENT_DEBUG_MODE\n");
        strcat(buffer, "    printf(\"[DEBUG] Module '");
        strcat(buffer, modules[i]);
        strcat(buffer, "' unloaded\\n\");\n");
        strcat(buffer, "    #endif\n");
        strcat(buffer, "    \n");
        strcat(buffer, "    return 0;\n");
        strcat(buffer, "}\n");
        strcat(buffer, "\n");
        
        // Module execute function
        strcat(buffer, "/**\n");
        strcat(buffer, " * @brief Execute ");
        strcat(buffer, modules[i]);
        strcat(buffer, " module\n");
        strcat(buffer, " */\n");
        strcat(buffer, "int client_execute_module_");
        strcat(buffer, modules[i]);
        strcat(buffer, "(const uint8_t* data, size_t data_len) {\n");
        strcat(buffer, "    // Check if initialized\n");
        strcat(buffer, "    if (!client_initialized) {\n");
        strcat(buffer, "        return -1;\n");
        strcat(buffer, "    }\n");
        strcat(buffer, "    \n");
        strcat(buffer, "    // Execute module\n");
        strcat(buffer, "    if (module_");
        strcat(buffer, modules[i]);
        strcat(buffer, "_execute(data, data_len) != 0) {\n");
        strcat(buffer, "        return -1;\n");
        strcat(buffer, "    }\n");
        strcat(buffer, "    \n");
        strcat(buffer, "    #if CLIENT_DEBUG_MODE\n");
        strcat(buffer, "    printf(\"[DEBUG] Module '");
        strcat(buffer, modules[i]);
        strcat(buffer, "' executed\\n\");\n");
        strcat(buffer, "    #endif\n");
        strcat(buffer, "    \n");
        strcat(buffer, "    return 0;\n");
        strcat(buffer, "}\n");
        strcat(buffer, "\n");
        
        // Module implementation
        strcat(buffer, "/**\n");
        strcat(buffer, " * @brief Initialize ");
        strcat(buffer, modules[i]);
        strcat(buffer, " module\n");
        strcat(buffer, " */\n");
        strcat(buffer, "static int module_");
        strcat(buffer, modules[i]);
        strcat(buffer, "_init(void) {\n");
        
        if (strcmp(modules[i], "shell") == 0) {
            strcat(buffer, "    // Shell module initialization\n");
            strcat(buffer, "    // Nothing to initialize for now\n");
        } else {
            strcat(buffer, "    // Module initialization\n");
            strcat(buffer, "    // TODO: Implement module initialization\n");
        }
        
        strcat(buffer, "    return 0;\n");
        strcat(buffer, "}\n");
        strcat(buffer, "\n");
        
        strcat(buffer, "/**\n");
        strcat(buffer, " * @brief Shutdown ");
        strcat(buffer, modules[i]);
        strcat(buffer, " module\n");
        strcat(buffer, " */\n");
        strcat(buffer, "static int module_");
        strcat(buffer, modules[i]);
        strcat(buffer, "_shutdown(void) {\n");
        
        if (strcmp(modules[i], "shell") == 0) {
            strcat(buffer, "    // Shell module shutdown\n");
            strcat(buffer, "    // Nothing to clean up for now\n");
        } else {
            strcat(buffer, "    // Module shutdown\n");
            strcat(buffer, "    // TODO: Implement module shutdown\n");
        }
        
        strcat(buffer, "    return 0;\n");
        strcat(buffer, "}\n");
        strcat(buffer, "\n");
        
        strcat(buffer, "/**\n");
        strcat(buffer, " * @brief Execute ");
        strcat(buffer, modules[i]);
        strcat(buffer, " module\n");
        strcat(buffer, " */\n");
        strcat(buffer, "static int module_");
        strcat(buffer, modules[i]);
        strcat(buffer, "_execute(const uint8_t* data, size_t data_len) {\n");
        
        if (strcmp(modules[i], "shell") == 0) {
            strcat(buffer, "    // Shell module execution\n");
            strcat(buffer, "    // Execute shell command\n");
            strcat(buffer, "    if (data == NULL || data_len == 0) {\n");
            strcat(buffer, "        return -1;\n");
            strcat(buffer, "    }\n");
            strcat(buffer, "    \n");
            strcat(buffer, "    // Null-terminate command\n");
            strcat(buffer, "    char* command = (char*)malloc(data_len + 1);\n");
            strcat(buffer, "    if (command == NULL) {\n");
            strcat(buffer, "        return -1;\n");
            strcat(buffer, "    }\n");
            strcat(buffer, "    \n");
            strcat(buffer, "    memcpy(command, data, data_len);\n");
            strcat(buffer, "    command[data_len] = '\\0';\n");
            strcat(buffer, "    \n");
            strcat(buffer, "    #if CLIENT_DEBUG_MODE\n");
            strcat(buffer, "    printf(\"[DEBUG] Executing shell command: %s\\n\", command);\n");
            strcat(buffer, "    #endif\n");
            strcat(buffer, "    \n");
            strcat(buffer, "    // Execute command\n");
            strcat(buffer, "    FILE* fp = popen(command, \"r\");\n");
            strcat(buffer, "    if (fp == NULL) {\n");
            strcat(buffer, "        free(command);\n");
            strcat(buffer, "        return -1;\n");
            strcat(buffer, "    }\n");
            strcat(buffer, "    \n");
            strcat(buffer, "    // Read output\n");
            strcat(buffer, "    char buffer[1024];\n");
            strcat(buffer, "    while (fgets(buffer, sizeof(buffer), fp) != NULL) {\n");
            strcat(buffer, "        // TODO: Send output back to server\n");
            strcat(buffer, "        #if CLIENT_DEBUG_MODE\n");
            strcat(buffer, "        printf(\"%s\", buffer);\n");
            strcat(buffer, "        #endif\n");
            strcat(buffer, "    }\n");
            strcat(buffer, "    \n");
            strcat(buffer, "    // Close command\n");
            strcat(buffer, "    pclose(fp);\n");
            strcat(buffer, "    free(command);\n");
        } else {
            strcat(buffer, "    // Module execution\n");
            strcat(buffer, "    // TODO: Implement module execution\n");
        }
        
        strcat(buffer, "    return 0;\n");
        strcat(buffer, "}\n");
        
        if (i < module_count - 1) {
            strcat(buffer, "\n");
        }
    }
    
    return buffer;
}

static char* generate_encryption_definition(encryption_algorithm_t algorithm) {
    // Calculate buffer size
    size_t buffer_size = 256;  // Start with 256 bytes
    char* buffer = (char*)malloc(buffer_size);
    if (buffer == NULL) {
        return NULL;
    }
    
    // Initialize buffer
    buffer[0] = '\0';
    
    // Add encryption definition
    strcat(buffer, "// Encryption algorithm\n");
    strcat(buffer, "typedef enum {\n");
    strcat(buffer, "    ENCRYPTION_NONE = 0,\n");
    strcat(buffer, "    ENCRYPTION_AES_128_GCM = 1,\n");
    strcat(buffer, "    ENCRYPTION_AES_256_GCM = 2,\n");
    strcat(buffer, "    ENCRYPTION_CHACHA20_POLY1305 = 3\n");
    strcat(buffer, "} encryption_algorithm_t;\n\n");
    
    // Add encryption algorithm
    strcat(buffer, "#define ENCRYPTION_ALGORITHM ");
    
    switch (algorithm) {
        case ENCRYPTION_NONE:
            strcat(buffer, "ENCRYPTION_NONE");
            break;
        case ENCRYPTION_AES_128_GCM:
            strcat(buffer, "ENCRYPTION_AES_128_GCM");
            break;
        case ENCRYPTION_AES_256_GCM:
            strcat(buffer, "ENCRYPTION_AES_256_GCM");
            break;
        case ENCRYPTION_CHACHA20_POLY1305:
            strcat(buffer, "ENCRYPTION_CHACHA20_POLY1305");
            break;
        default:
            strcat(buffer, "ENCRYPTION_NONE");
            break;
    }
    
    strcat(buffer, "\n");
    
    return buffer;
}

/**
 * @brief Generate client from template
 */
status_t template_generator_generate(const builder_config_t* builder_config, const char* output_file) {
    if (builder_config == NULL || output_file == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Create client configuration from builder configuration
    client_config_t client_config;
    memset(&client_config, 0, sizeof(client_config_t));
    
    // Copy protocols
    client_config.protocols = (protocol_type_t*)malloc(builder_config->protocol_count * sizeof(protocol_type_t));
    if (client_config.protocols == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    memcpy(client_config.protocols, builder_config->protocols, builder_config->protocol_count * sizeof(protocol_type_t));
    client_config.protocol_count = builder_config->protocol_count;
    
    // Copy servers
    client_config.servers = (char**)malloc(builder_config->server_count * sizeof(char*));
    if (client_config.servers == NULL) {
        free(client_config.protocols);
        return STATUS_ERROR_MEMORY;
    }
    
    for (size_t i = 0; i < builder_config->server_count; i++) {
        client_config.servers[i] = strdup(builder_config->servers[i]);
        if (client_config.servers[i] == NULL) {
            for (size_t j = 0; j < i; j++) {
                free(client_config.servers[j]);
            }
            
            free(client_config.servers);
            free(client_config.protocols);
            return STATUS_ERROR_MEMORY;
        }
    }
    
    client_config.server_count = builder_config->server_count;
    
    // Copy domain
    if (builder_config->domain != NULL) {
        client_config.domain = strdup(builder_config->domain);
        if (client_config.domain == NULL) {
            for (size_t i = 0; i < client_config.server_count; i++) {
                free(client_config.servers[i]);
            }
            
            free(client_config.servers);
            free(client_config.protocols);
            return STATUS_ERROR_MEMORY;
        }
    }
    
    // Copy modules
    if (builder_config->module_count > 0) {
        client_config.modules = (char**)malloc(builder_config->module_count * sizeof(char*));
        if (client_config.modules == NULL) {
            free(client_config.domain);
            
            for (size_t i = 0; i < client_config.server_count; i++) {
                free(client_config.servers[i]);
            }
            
            free(client_config.servers);
            free(client_config.protocols);
            return STATUS_ERROR_MEMORY;
        }
        
        for (size_t i = 0; i < builder_config->module_count; i++) {
            client_config.modules[i] = strdup(builder_config->modules[i]);
            if (client_config.modules[i] == NULL) {
                for (size_t j = 0; j < i; j++) {
                    free(client_config.modules[j]);
                }
                
                free(client_config.modules);
                free(client_config.domain);
                
                for (size_t j = 0; j < client_config.server_count; j++) {
                    free(client_config.servers[j]);
                }
                
                free(client_config.servers);
                free(client_config.protocols);
                return STATUS_ERROR_MEMORY;
            }
        }
        
        client_config.module_count = builder_config->module_count;
    }
    
    // Set other configuration
    client_config.heartbeat_interval = 60;  // Default: 1 minute
    client_config.heartbeat_jitter = 10;    // Default: 10 seconds
    client_config.encryption_algorithm = builder_config->encryption_algorithm;
    client_config.version_major = builder_config->version_major;
    client_config.version_minor = builder_config->version_minor;
    client_config.version_patch = builder_config->version_patch;
    client_config.debug_mode = builder_config->debug_mode;
    
    // Read template file
    char* template_content = read_template_file(TEMPLATE_FILE_PATH);
    if (template_content == NULL) {
        fprintf(stderr, "Error: Failed to read template file\n");
        
        // Clean up
        for (size_t i = 0; i < client_config.module_count; i++) {
            free(client_config.modules[i]);
        }
        
        free(client_config.modules);
        free(client_config.domain);
        
        for (size_t i = 0; i < client_config.server_count; i++) {
            free(client_config.servers[i]);
        }
        
        free(client_config.servers);
        free(client_config.protocols);
        
        return STATUS_ERROR;
    }
    
    // Replace placeholders
    
    // Builder version
    char builder_version[32];
    snprintf(builder_version, sizeof(builder_version), "%d.%d.%d", 
             BUILDER_VERSION_MAJOR, BUILDER_VERSION_MINOR, BUILDER_VERSION_PATCH);
    template_content = replace_template_placeholder(template_content, "{{BUILDER_VERSION}}", builder_version);
    
    // Version
    char version_major[16], version_minor[16], version_patch[16];
    snprintf(version_major, sizeof(version_major), "%u", client_config.version_major);
    snprintf(version_minor, sizeof(version_minor), "%u", client_config.version_minor);
    snprintf(version_patch, sizeof(version_patch), "%u", client_config.version_patch);
    
    template_content = replace_template_placeholder(template_content, "{{VERSION_MAJOR}}", version_major);
    template_content = replace_template_placeholder(template_content, "{{VERSION_MINOR}}", version_minor);
    template_content = replace_template_placeholder(template_content, "{{VERSION_PATCH}}", version_patch);
    
    // Debug mode
    template_content = replace_template_placeholder(template_content, "{{DEBUG_MODE}}", 
                                                  client_config.debug_mode ? "1" : "0");
    
    // Protocol definitions
    char* protocol_definitions = generate_protocol_definitions(client_config.protocols, client_config.protocol_count);
    if (protocol_definitions != NULL) {
        template_content = replace_template_placeholder(template_content, "{{PROTOCOL_DEFINITIONS}}", protocol_definitions);
        free(protocol_definitions);
    }
    
    // Server definitions
    char* server_definitions = generate_server_definitions(
        (const char**)client_config.servers, client_config.server_count);
    if (server_definitions != NULL) {
        template_content = replace_template_placeholder(template_content, "{{SERVER_DEFINITIONS}}", server_definitions);
        free(server_definitions);
    }
    
    // Domain definition
    char* domain_definition = generate_domain_definition(client_config.domain);
    if (domain_definition != NULL) {
        template_content = replace_template_placeholder(template_content, "{{DOMAIN_DEFINITION}}", domain_definition);
        free(domain_definition);
    }
    
    // Encryption definition
    char* encryption_definition = generate_encryption_definition(client_config.encryption_algorithm);
    if (encryption_definition != NULL) {
        template_content = replace_template_placeholder(template_content, "{{ENCRYPTION_DEFINITION}}", encryption_definition);
        free(encryption_definition);
    }
    
    // Heartbeat configuration
    char heartbeat_interval[16], heartbeat_jitter[16];
    snprintf(heartbeat_interval, sizeof(heartbeat_interval), "%u", client_config.heartbeat_interval);
    snprintf(heartbeat_jitter, sizeof(heartbeat_jitter), "%u", client_config.heartbeat_jitter);
    
    template_content = replace_template_placeholder(template_content, "{{HEARTBEAT_INTERVAL}}", heartbeat_interval);
    template_content = replace_template_placeholder(template_content, "{{HEARTBEAT_JITTER}}", heartbeat_jitter);
    
    // Module definitions
    char* module_definitions = generate_module_definitions(
        (const char**)client_config.modules, client_config.module_count);
    if (module_definitions != NULL) {
        template_content = replace_template_placeholder(template_content, "{{MODULE_DEFINITIONS}}", module_definitions);
        free(module_definitions);
    }
    
    // Default protocol
    char default_protocol[32];
    if (client_config.protocol_count > 0) {
        switch (client_config.protocols[0]) {
            case PROTOCOL_TYPE_TCP:
                snprintf(default_protocol, sizeof(default_protocol), "PROTOCOL_TCP");
                break;
            case PROTOCOL_TYPE_UDP:
                snprintf(default_protocol, sizeof(default_protocol), "PROTOCOL_UDP");
                break;
            case PROTOCOL_TYPE_WS:
                snprintf(default_protocol, sizeof(default_protocol), "PROTOCOL_WS");
                break;
            case PROTOCOL_TYPE_ICMP:
                snprintf(default_protocol, sizeof(default_protocol), "PROTOCOL_ICMP");
                break;
            case PROTOCOL_TYPE_DNS:
                snprintf(default_protocol, sizeof(default_protocol), "PROTOCOL_DNS");
                break;
            default:
                snprintf(default_protocol, sizeof(default_protocol), "PROTOCOL_NONE");
                break;
        }
    } else {
        snprintf(default_protocol, sizeof(default_protocol), "PROTOCOL_NONE");
    }
    
    template_content = replace_template_placeholder(template_content, "{{DEFAULT_PROTOCOL}}", default_protocol);
    
    // Protocol fallback code
    char* protocol_fallback_code = generate_protocol_fallback_code(client_config.protocols, client_config.protocol_count);
    if (protocol_fallback_code != NULL) {
        template_content = replace_template_placeholder(template_content, "{{PROTOCOL_FALLBACK_CODE}}", protocol_fallback_code);
        free(protocol_fallback_code);
    }
    
    // Protocol support check
    char* protocol_support_check = generate_protocol_support_check(client_config.protocols, client_config.protocol_count);
    if (protocol_support_check != NULL) {
        template_content = replace_template_placeholder(template_content, "{{PROTOCOL_SUPPORT_CHECK}}", protocol_support_check);
        free(protocol_support_check);
    }
    
    // Protocol connection implementations
    char* protocol_connection_implementations = generate_protocol_connection_implementations(
        client_config.protocols, client_config.protocol_count,
        (const char**)client_config.servers, client_config.server_count,
        client_config.domain);
    if (protocol_connection_implementations != NULL) {
        template_content = replace_template_placeholder(template_content, "{{PROTOCOL_CONNECTION_IMPLEMENTATIONS}}", 
                                                      protocol_connection_implementations);
        free(protocol_connection_implementations);
    }
    
    // Heartbeat implementation
    char* heartbeat_implementation = generate_heartbeat_implementation(client_config.protocols, client_config.protocol_count);
    if (heartbeat_implementation != NULL) {
        template_content = replace_template_placeholder(template_content, "{{HEARTBEAT_IMPLEMENTATION}}", heartbeat_implementation);
        free(heartbeat_implementation);
    }
    
    // Module forward declarations
    char* module_forward_declarations = generate_module_forward_declarations(
        (const char**)client_config.modules, client_config.module_count);
    if (module_forward_declarations != NULL) {
        template_content = replace_template_placeholder(template_content, "{{MODULE_FORWARD_DECLARATIONS}}", 
                                                      module_forward_declarations);
        free(module_forward_declarations);
    }
    
    // Module implementations
    char* module_implementations = generate_module_implementations(
        (const char**)client_config.modules, client_config.module_count);
    if (module_implementations != NULL) {
        template_content = replace_template_placeholder(template_content, "{{MODULE_IMPLEMENTATIONS}}", module_implementations);
        free(module_implementations);
    }
    
    // Write output file
    FILE* file = fopen(output_file, "w");
    if (file == NULL) {
        free(template_content);
        // Clean up
        for (size_t i = 0; i < client_config.module_count; i++) {
            free(client_config.modules[i]);
        }
        
        free(client_config.modules);
        free(client_config.domain);
        
        for (size_t i = 0; i < client_config.server_count; i++) {
            free(client_config.servers[i]);
        }
        
        free(client_config.servers);
        free(client_config.protocols);
        
        return STATUS_ERROR;
    }
    
    // Write template content to file
    fprintf(file, "%s", template_content);
    
    // Close file
    fclose(file);
    
    // Free template content
    free(template_content);
    
    // Clean up
    for (size_t i = 0; i < client_config.module_count; i++) {
        free(client_config.modules[i]);
    }
    
    free(client_config.modules);
    free(client_config.domain);
    
    for (size_t i = 0; i < client_config.server_count; i++) {
        free(client_config.servers[i]);
    }
    
    free(client_config.servers);
    free(client_config.protocols);
    
    return STATUS_SUCCESS;
}
