#include "protocol.h"

// Function to send a single byte
ssize_t send_byte(int sockfd, uint8_t byte) {
    return send(sockfd, &byte, sizeof(byte), 0);
}

// Function to send a 16-bit integer in network byte order
ssize_t send_uint16(int sockfd, uint16_t value) {
    uint16_t net_value = htons(value); // Convert to network byte order
    return send(sockfd, &net_value, sizeof(net_value), 0);
}

// Function to send the protocol header
int send_header(int sockfd, uint8_t version, uint16_t content_size) {
    if (send_byte(sockfd, version) == -1 ||
       send_uint16(sockfd, content_size) == -1) {
        return -1;
    }
    return 0;
}

// Function to read a single byte
ssize_t read_byte(int sockfd, uint8_t *byte) {
    return recv(sockfd, byte, sizeof(*byte), 0);
}

// Function to read a 16-bit integer in network byte order
ssize_t read_uint16(int sockfd, uint16_t *value) {
    uint16_t net_value;
    ssize_t result = recv(sockfd, &net_value, sizeof(net_value), 0);
    if (result > 0) {
        *value = ntohs(net_value); // Convert from network byte order
    }
    return result;
}

// Function to read the protocol header
int read_header(int sockfd, uint8_t *version, uint16_t *content_size) {
    if (read_byte(sockfd, version) == -1 ||
       read_uint16(sockfd, content_size) == -1) {
        return -1;
    }
    return 0;
}
