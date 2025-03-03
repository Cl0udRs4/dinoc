#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

int main() {
    printf("Starting simple server test\n");
    
    // Create TCP socket
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket < 0) {
        perror("TCP socket creation failed");
        return 1;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("TCP setsockopt failed");
        close(tcp_socket);
        return 1;
    }
    
    // Bind TCP socket
    struct sockaddr_in tcp_addr;
    memset(&tcp_addr, 0, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_addr.s_addr = INADDR_ANY;
    tcp_addr.sin_port = htons(7080);
    
    if (bind(tcp_socket, (struct sockaddr*)&tcp_addr, sizeof(tcp_addr)) < 0) {
        perror("TCP bind failed");
        close(tcp_socket);
        return 1;
    }
    
    if (listen(tcp_socket, 5) < 0) {
        perror("TCP listen failed");
        close(tcp_socket);
        return 1;
    }
    
    printf("TCP server listening on port 7080\n");
    
    // Create UDP socket
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        perror("UDP socket creation failed");
        close(tcp_socket);
        return 1;
    }
    
    // Set socket options
    if (setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("UDP setsockopt failed");
        close(tcp_socket);
        close(udp_socket);
        return 1;
    }
    
    // Bind UDP socket
    struct sockaddr_in udp_addr;
    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    udp_addr.sin_port = htons(7081);
    
    if (bind(udp_socket, (struct sockaddr*)&udp_addr, sizeof(udp_addr)) < 0) {
        perror("UDP bind failed");
        close(tcp_socket);
        close(udp_socket);
        return 1;
    }
    
    printf("UDP server listening on port 7081\n");
    
    // Keep the program running until Ctrl+C
    printf("Press Ctrl+C to exit\n");
    
    // Set up signal handler
    signal(SIGINT, SIG_DFL);
    
    // Wait for signal
    pause();
    
    // Close sockets
    close(tcp_socket);
    close(udp_socket);
    
    return 0;
}
