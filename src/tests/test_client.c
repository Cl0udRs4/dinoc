/**
 * @file test_client.c
 * @brief Test client for DinoC server
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Protocol header structure
typedef struct {
    uint8_t magic;          // Magic byte for encryption type
    uint8_t version;        // Protocol version
    uint16_t flags;         // Protocol flags
    uint32_t payload_len;   // Payload length
} __attribute__((packed)) protocol_header_t;

// Magic bytes for encryption type
#define AES_MAGIC_BYTE      0xA3
#define CHACHA20_MAGIC_BYTE 0xC2

// Encryption types
typedef enum {
    ENCRYPTION_NONE = 0,
    ENCRYPTION_AES = 1,
    ENCRYPTION_CHACHA20 = 2
} encryption_type_t;

/**
 * @brief Create a protocol message
 */
void create_message(encryption_type_t type, const char* payload, uint8_t** message, size_t* message_len) {
    size_t payload_len = strlen(payload);
    *message_len = sizeof(protocol_header_t) + payload_len;
    *message = (uint8_t*)malloc(*message_len);
    
    // Create header
    protocol_header_t* header = (protocol_header_t*)*message;
    
    // Set magic byte based on encryption type
    switch (type) {
        case ENCRYPTION_AES:
            header->magic = AES_MAGIC_BYTE;
            break;
            
        case ENCRYPTION_CHACHA20:
            header->magic = CHACHA20_MAGIC_BYTE;
            break;
            
        default:
            header->magic = 0xFF;
            break;
    }
    
    // Set other fields
    header->version = 0x01;
    header->flags = 0x0001;  // Registration message
    header->payload_len = payload_len;
    
    // Copy payload
    memcpy(*message + sizeof(protocol_header_t), payload, payload_len);
}

/**
 * @brief Connect to server and send message
 */
int connect_and_send(const char* server_ip, int server_port, encryption_type_t encryption) {
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
    
    // Create registration message
    const char* payload = "REGISTER|client_id=test_client|hostname=testhost|os=linux|version=1.0";
    uint8_t* message;
    size_t message_len;
    
    create_message(encryption, payload, &message, &message_len);
    
    // Send message
    printf("Sending registration message with %s header...\n", 
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
        
        // Print response header
        if (response_len >= sizeof(protocol_header_t)) {
            protocol_header_t* header = (protocol_header_t*)response;
            printf("Response header: magic=0x%02x, version=0x%02x, flags=0x%04x, payload_len=%u\n",
                   header->magic, header->version, header->flags, header->payload_len);
            
            // Check if server responded with the same encryption type
            if ((encryption == ENCRYPTION_AES && header->magic == AES_MAGIC_BYTE) ||
                (encryption == ENCRYPTION_CHACHA20 && header->magic == CHACHA20_MAGIC_BYTE)) {
                printf("Server correctly responded with the same encryption type\n");
            } else {
                printf("Server responded with a different encryption type!\n");
            }
        }
        
        // Print response payload
        if (response_len > sizeof(protocol_header_t)) {
            printf("Response payload: ");
            for (size_t i = sizeof(protocol_header_t); i < response_len && i < 64; i++) {
                printf("%c", response[i]);
            }
            printf("\n");
        }
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
    
    encryption_type_t encryption = ENCRYPTION_NONE;
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
    
    return connect_and_send(server_ip, server_port, encryption);
}
