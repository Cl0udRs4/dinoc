#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int create_and_bind(const char* bind_address, int port, const char* protocol) {
    int sock = -1;
    struct sockaddr_in addr;
    
    // Create socket
    if (strcmp(protocol, "tcp") == 0) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
    } else if (strcmp(protocol, "udp") == 0) {
        sock = socket(AF_INET, SOCK_DGRAM, 0);
    } else {
        fprintf(stderr, "Unknown protocol: %s\n", protocol);
        return -1;
    }
    
    if (sock < 0) {
        perror("socket");
        return -1;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(sock);
        return -1;
    }
    
    // Bind socket
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(bind_address);
    if (addr.sin_addr.s_addr == INADDR_NONE) {
        addr.sin_addr.s_addr = INADDR_ANY;
    }
    addr.sin_port = htons(port);
    
    printf("Binding %s socket to %s:%d\n", protocol, bind_address, port);
    
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sock);
        return -1;
    }
    
    printf("Successfully bound %s socket to %s:%d\n", protocol, bind_address, port);
    
    // Listen if TCP
    if (strcmp(protocol, "tcp") == 0) {
        if (listen(sock, 5) < 0) {
            perror("listen");
            close(sock);
            return -1;
        }
        printf("Listening on %s:%d\n", bind_address, port);
    }
    
    return sock;
}

int main(int argc, char** argv) {
    const char* bind_address = "0.0.0.0";
    int tcp_port = 8080;
    int udp_port = 8081;
    int ws_port = 8082;
    int dns_port = 5353;
    int http_port = 8083;
    
    // Create and bind TCP socket
    int tcp_sock = create_and_bind(bind_address, tcp_port, "tcp");
    if (tcp_sock < 0) {
        fprintf(stderr, "Failed to create and bind TCP socket\n");
        return 1;
    }
    
    // Create and bind UDP socket
    int udp_sock = create_and_bind(bind_address, udp_port, "udp");
    if (udp_sock < 0) {
        fprintf(stderr, "Failed to create and bind UDP socket\n");
        close(tcp_sock);
        return 1;
    }
    
    // Create and bind WebSocket socket (TCP)
    int ws_sock = create_and_bind(bind_address, ws_port, "tcp");
    if (ws_sock < 0) {
        fprintf(stderr, "Failed to create and bind WebSocket socket\n");
        close(tcp_sock);
        close(udp_sock);
        return 1;
    }
    
    // Create and bind DNS socket (UDP)
    int dns_sock = create_and_bind(bind_address, dns_port, "udp");
    if (dns_sock < 0) {
        fprintf(stderr, "Failed to create and bind DNS socket\n");
        close(tcp_sock);
        close(udp_sock);
        close(ws_sock);
        return 1;
    }
    
    // Create and bind HTTP API socket (TCP)
    int http_sock = create_and_bind(bind_address, http_port, "tcp");
    if (http_sock < 0) {
        fprintf(stderr, "Failed to create and bind HTTP API socket\n");
        close(tcp_sock);
        close(udp_sock);
        close(ws_sock);
        close(dns_sock);
        return 1;
    }
    
    printf("All sockets bound successfully. Press Enter to exit...\n");
    getchar();
    
    // Close sockets
    close(tcp_sock);
    close(udp_sock);
    close(ws_sock);
    close(dns_sock);
    close(http_sock);
    
    return 0;
}
