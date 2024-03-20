#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h> // For uint8_t and uint16_t types
#include <arpa/inet.h> // For htons function

// Constants for the protocol
#define PROTOCOL_VERSION 1

// Function prototypes
ssize_t send_byte(int sockfd, uint8_t byte);
ssize_t send_uint16(int sockfd, uint16_t value);
int send_header(int sockfd, uint8_t version, uint16_t content_size);
ssize_t read_byte(int sockfd, uint8_t *byte);
ssize_t read_uint16(int sockfd, uint16_t *value);
int read_header(int sockfd, uint8_t *version, uint16_t *content_size);

#endif // PROTOCOL_H
