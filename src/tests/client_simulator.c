/**
 * @file client_simulator.c
 * @brief Simple client simulator for testing protocol header detection
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Magic bytes for encryption type
#define AES_MAGIC_BYTE      0xA3
#define CHACHA20_MAGIC_BYTE 0xC2

// Encryption types
typedef enum {
    ENCRYPTION_NONE = 0,
    ENCRYPTION_AES = 1,
    ENCRYPTION_CHACHA20 = 2,
    ENCRYPTION_UNKNOWN = 255
} encryption_type_t;

// Protocol header structure
typedef struct {
    uint8_t magic;          // Magic byte for encryption type
    uint8_t version;        // Protocol version
    uint16_t flags;         // Protocol flags
    uint32_t payload_len;   // Payload length
} __attribute__((packed)) protocol_header_t;

/**
 * @brief Create a protocol header
 */
void create_header(encryption_type_t type, uint8_t version, uint16_t flags,
                  uint32_t payload_len, uint8_t* header) {
    protocol_header_t* hdr = (protocol_header_t*)header;
    
    // Set magic byte based on encryption type
    switch (type) {
        case ENCRYPTION_AES:
            hdr->magic = AES_MAGIC_BYTE;
            break;
            
        case ENCRYPTION_CHACHA20:
            hdr->magic = CHACHA20_MAGIC_BYTE;
            break;
            
        default:
            hdr->magic = 0xFF;
            break;
    }
    
    // Set other fields
    hdr->version = version;
    hdr->flags = flags;
    hdr->payload_len = payload_len;
}

/**
 * @brief Main function
 */
int main(int argc, char** argv) {
    if (argc < 4) {
        printf("Usage: %s <server_ip> <server_port> <encryption_type>\n", argv[0]);
        printf("  encryption_type: 1 = AES, 2 = ChaCha20\n");
        return 1;
    }
    
    const char* server_ip = argv[1];
    int server_port = atoi(argv[2]);
    int enc_type = atoi(argv[3]);
    
    encryption_type_t encryption = ENCRYPTION_UNKNOWN;
    switch (enc_type) {
        case 1:
            encryption = ENCRYPTION_AES;
            printf("Using AES encryption\n");
            break;
            
        case 2:
            encryption = ENCRYPTION_CHACHA20;
            printf("Using ChaCha20 encryption\n");
            break;
            
        default:
            printf("Invalid encryption type: %d\n", enc_type);
            return 1;
    }
    
    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Failed to create socket");
        return 1;
    }
    
    // Connect to server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid server IP address");
        close(sock);
        return 1;
    }
    
    printf("Connecting to %s:%d...\n", server_ip, server_port);
    
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to connect to server");
        close(sock);
        return 1;
    }
    
    printf("Connected to server\n");
    
    // Create message with protocol header
    const char* payload = "Hello, server!";
    size_t payload_len = strlen(payload);
    
    size_t message_len = sizeof(protocol_header_t) + payload_len;
    uint8_t* message = (uint8_t*)malloc(message_len);
    if (message == NULL) {
        perror("Failed to allocate memory for message");
        close(sock);
        return 1;
    }
    
    // Create header
    create_header(encryption, 0x01, 0x0001, payload_len, message);
    
    // Copy payload
    memcpy(message + sizeof(protocol_header_t), payload, payload_len);
    
    // Send message
    printf("Sending message with %s header...\n", 
           encryption == ENCRYPTION_AES ? "AES" : "ChaCha20");
    
    if (send(sock, message, message_len, 0) != message_len) {
        perror("Failed to send message");
        free(message);
        close(sock);
        return 1;
    }
    
    printf("Message sent\n");
    
    // Wait for response
    printf("Waiting for response...\n");
    
    uint8_t response[1024];
    ssize_t response_len = recv(sock, response, sizeof(response), 0);
    
    if (response_len > 0) {
        printf("Received response: %zd bytes\n", response_len);
        
        // Print response as hex
        printf("Response: ");
        for (ssize_t i = 0; i < response_len && i < 32; i++) {
            printf("%02x ", response[i]);
        }
        if (response_len > 32) {
            printf("...");
        }
        printf("\n");
    } else if (response_len == 0) {
        printf("Server closed connection\n");
    } else {
        perror("Failed to receive response");
    }
    
    // Clean up
    free(message);
    close(sock);
    
    return 0;
}
