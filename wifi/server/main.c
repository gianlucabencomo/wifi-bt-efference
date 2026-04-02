#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 5000
#define BUFFER_SIZE 65536 // 2 ** 16

int main() {
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr, client_addr;
    unsigned long int n_packets = 0;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (const struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Socket failed to bind.");
        return -1;
    }

    printf("Reflector Server ready on port %d...\n", PORT);
    printf("Press Ctrl+C to stop.\n");
    printf("------------------------------------\n");

    while(1) {
        socklen_t len = sizeof(client_addr);
        
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &client_addr, &len);
        
        if (n > 0) {
            n_packets++;
            
            // \r moves cursor to start of line, fflush forces the print
            printf("\rPackets Bounced: %lu | Latest Size: %d bytes   ", n_packets, n);
            fflush(stdout);

            sendto(sockfd, buffer, n, 0, (const struct sockaddr *) &client_addr, len);
        }
    }
    
    close(sockfd);
    return 0;
}

