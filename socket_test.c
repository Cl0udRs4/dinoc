#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

int main() {
    printf("Testing socket binding on port 8080\n");
    
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        printf("Socket creation failed: %s\n", strerror(errno));
        return 1;
    }
    
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        printf("Setsockopt failed: %s\n", strerror(errno));
        close(server_socket);
        return 1;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Bind failed: %s\n", strerror(errno));
        close(server_socket);
        return 1;
    }
    
    if (listen(server_socket, 5) < 0) {
        printf("Listen failed: %s\n", strerror(errno));
        close(server_socket);
        return 1;
    }
    
    printf("Socket bound and listening on port 8080\n");
    
    // Close socket
    close(server_socket);
    
    return 0;
}
