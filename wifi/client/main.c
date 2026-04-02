#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>

#define PORT 5000
// --- CHANGE THIS TO YOUR MACBOOK'S IP ADDRESS ---
#define SERVER_IP "192.168.1.155" 
// ------------------------------------------------

#define TRIALS 1000 // 1000 pings per size for a better average

typedef struct {
    double avg_latency;
    double jitter;
    double drop_rate;
} Result;

Result benchmark_size(int sockfd, struct sockaddr_in *dest_addr, int size) {
    char *buffer = malloc(size);
    char *recv_buf = malloc(size);
    
    if (!buffer || !recv_buf) {
        perror("Malloc failed");
        exit(1);
    }
    
    // Warm up the memory pages to avoid first-run allocation latency
    memset(buffer, 'A', size);
    
    struct timespec start, end;
    double total_latency = 0;
    double total_jitter = 0;
    double prev_latency = -1.0;
    
    int dropped = 0;
    int jitter_measurements = 0;

    for (uint32_t i = 0; i < TRIALS; i++) {
        // Embed the sequence number at the start of the payload
        memcpy(buffer, &i, sizeof(uint32_t));

        clock_gettime(CLOCK_MONOTONIC, &start);
        
        // Send packet
        sendto(sockfd, buffer, size, 0, (struct sockaddr *)dest_addr, sizeof(*dest_addr));
        
        // Receive packet using a throwaway struct to protect dest_addr
        struct sockaddr_storage from_addr;
        socklen_t from_len = sizeof(from_addr);
        
        int n = recvfrom(sockfd, recv_buf, size, 0, (struct sockaddr *)&from_addr, &from_len);
        
        // Extract sequence number from received packet
        uint32_t received_seq = 0xFFFFFFFF; // Default to invalid
        if (n >= sizeof(uint32_t)) {
            memcpy(&received_seq, recv_buf, sizeof(uint32_t));
        }

        // Determine if it was a valid response
        if (n < 0 || received_seq != i) {
            dropped++;
            prev_latency = -1.0; // Reset jitter tracking to avoid calculating across dropped packets
        } else {
            clock_gettime(CLOCK_MONOTONIC, &end);
            
            // Calculate latency in microseconds
            double current_latency = (end.tv_sec - start.tv_sec) * 1e6 + 
                                     (end.tv_nsec - start.tv_nsec) / 1e3;
            
            total_latency += current_latency;

            // Calculate Jitter
            if (prev_latency > 0) {
                double diff = current_latency - prev_latency;
                if (diff < 0) diff = -diff; // Absolute value
                total_jitter += diff;
                jitter_measurements++;
            }
            prev_latency = current_latency;
        }
        
        // Small sleep to prevent flooding the network switch (1ms)
        usleep(1000); 
    }

    Result res;
    int successful = TRIALS - dropped;
    res.avg_latency = (successful > 0) ? (total_latency / successful) : 0;
    res.jitter = (jitter_measurements > 0) ? (total_jitter / jitter_measurements) : 0;
    res.drop_rate = ((double)dropped / TRIALS) * 100.0;

    free(buffer);
    free(recv_buf);
    return res;
}

int main() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // Set a 100ms timeout. If it takes longer than 100ms on a local network, it's effectively lost.
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000; 
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Increase socket buffers for stability
    int buf_size = 2 * 1024 * 1024;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &buf_size, sizeof(buf_size));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &buf_size, sizeof(buf_size));

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, SERVER_IP, &servaddr.sin_addr) <= 0) {
        printf("Invalid address / Address not supported. Check SERVER_IP macro.\n");
        return 1;
    }

    // Notice we skip sizes smaller than 4 bytes now, because our sequence number requires 4 bytes.
    int payloads[] = {8, 64, 512, 1024, 2048, 4096, 8192};
    int n_payloads = sizeof(payloads) / sizeof(payloads[0]);

    printf("\nUDP Benchmark targeting %s: %d Trials per Size\n", SERVER_IP, TRIALS);
    printf("%-15s | %-15s | %-15s | %-10s\n", "Payload (Bytes)", "Avg RTT (us)", "Jitter (us)", "Drop %");
    printf("----------------------------------------------------------------------------\n");

    for (int i = 0; i < n_payloads; i++) {
        Result res = benchmark_size(sockfd, &servaddr, payloads[i]);
        printf("%-15d | %-15.2f | %-15.2f | %-10.1f%%\n", 
               payloads[i], res.avg_latency, res.jitter, res.drop_rate);
    }

    printf("----------------------------------------------------------------------------\n");
    close(sockfd);
    return 0;
}