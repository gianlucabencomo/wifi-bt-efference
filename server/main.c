#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 5000
#define BUFFER_SIZE 65535 

int main() {
    int sockfd;
    char *buffer = malloc(BUFFER_SIZE);
    struct sockaddr_in servaddr, cliaddr;
    unsigned long long packet_count = 0;

    if (!buffer) {
        perror("Malloc failed");
        return 1;
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        free(buffer);
        return 1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Bind failed");
        free(buffer);
        return 1;
    }

    printf("Reflector Server ready on port %d...\n", PORT);
    printf("Press Ctrl+C to stop.\n\n");

    while(1) {
        socklen_t len = sizeof(cliaddr);
        
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&cliaddr, &len);
        
        if (n > 0) {
            packet_count++;
            
            // \r moves cursor to start of line, fflush forces the print
            printf("\rPackets Bounced: %llu | Latest Size: %d bytes   ", packet_count, n);
            fflush(stdout);

            sendto(sockfd, buffer, n, 0, (const struct sockaddr *)&cliaddr, len);
        }
    }
    
    free(buffer);
    close(sockfd);
    return 0;
}