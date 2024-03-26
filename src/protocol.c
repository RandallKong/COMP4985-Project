#include "../include/protocol.h"

// Function to send a single byte
ssize_t send_byte(int sockfd, uint8_t byte)
{
    return send(sockfd, &byte, sizeof(byte), 0);
}

// Function to send a 16-bit integer in network byte order
ssize_t send_uint16(int sockfd, uint16_t value)
{
    uint16_t net_value = htons(value);    // Convert to network byte order
    return send(sockfd, &net_value, sizeof(net_value), 0);
}

// Function to send the protocol header
int send_header(int sockfd, uint8_t version, uint16_t content_size)
{
    if(send_byte(sockfd, version) == -1 || send_uint16(sockfd, content_size) == -1)
    {
        return -1;
    }
    return 0;
}

// Function to send message with protocol
int send_with_protocol(int sockfd, uint8_t version, const char *message)
{
    uint16_t content_size;
    content_size = (uint16_t)strlen(message);

    if(send_header(sockfd, version, content_size) == -1)
    {
        return -1;
    }

    if(send(sockfd, message, content_size, 0) < content_size)
    {
        return -1;
    }

    return 0;
}

// Function to read a single byte
ssize_t recv_byte(int sockfd, uint8_t *byte)
{
    return recv(sockfd, byte, sizeof(*byte), 0);
}

// Function to read a 16-bit integer in network byte order
ssize_t recv_uint16(int sockfd, uint16_t *value)
{
    uint16_t net_value;
    ssize_t  result = recv(sockfd, &net_value, sizeof(net_value), 0);
    if(result > 0)
    {
        *value = ntohs(net_value);    // Convert from network byte order
    }
    return result;
}

// Function to read the protocol header
int read_header(int sockfd, uint8_t *version, uint16_t *content_size)
{
    if(recv_byte(sockfd, version) == -1 || recv_uint16(sockfd, content_size) == -1)
    {
        return -1;
    }
    return 0;
}

// Function to read message with protocol
ssize_t read_with_protocol(int sockfd, uint8_t *version, char *buffer, size_t buffer_size)
{
    uint16_t content_size = 0;
    ssize_t  bytes_received;

    if(read_header(sockfd, version, &content_size) == -1)
    {
        return -1;
    }

    if(content_size >= buffer_size)
    {
        fprintf(stderr, "Buffer too small for incoming message\n");
        return -1;
    }

    bytes_received = recv(sockfd, buffer, content_size, 0);
    if(bytes_received <= 0)
    {
        return -1;
    }

    buffer[bytes_received] = '\0';    // Null-terminate the message
    return bytes_received;            // Return the length of the received message
}
