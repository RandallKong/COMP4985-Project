#ifndef SERVER_SERVER_H
#define SERVER_SERVER_H
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

void      setup_signal_handler(void);
void      sigint_handler(int signum);
void      handle_arguments(const char *ip_address, const char *port_str, in_port_t *port);
in_port_t parse_in_port_t(const char *port_str);
void      convert_address(const char *address, struct sockaddr_storage *addr);
int       socket_create(int domain, int type, int protocol);
void      socket_bind(int sockfd, struct sockaddr_storage *addr, in_port_t port);
void      start_listening(int server_fd, int backlog);
int       socket_accept_connection(int server_fd, struct sockaddr_storage *client_addr, socklen_t *client_addr_len);
void      socket_close(int sockfd);

// Wrapper Prototypes
void start_admin_server(struct sockaddr_storage addr, in_port_t port);
void handle_prompt(char **address, char **port_str);

// Client Server Prototypes
void *handle_client(void *arg);
void  start_server(struct sockaddr_storage addr, in_port_t port, int sm_socket);
void  free_usernames(void);
// void         print_users(void);
void connect_to_server_manager(const char *sm_ip, int sm_port);
void handle_message(const char *buffer, int sender_fd);
void send_user_list(int sender_fd);
void set_username(int sender_fd, const char *buffer);
void direct_message(int sender_fd, const char *buffer);

// GENERAL USE
#define BASE_TEN 10
#define MAX_USERNAME_SIZE 15
#define MAX_CLIENTS 32
#define TWO_FIFTY_SIX 256
#define BUFFER_SIZE 1024
#define MESSAGE_SIZE (BUFFER_SIZE + MAX_USERNAME_SIZE + BASE_TEN)
// #define UINT16_MAX 65535

// SERVER MANAGER WRAPPER MESSAGES

#define PASSKEY "hellyabrother"
#define WELCOME_STARTUP "Initializing Server Wrapper"
#define OPTION_NO_SM "Start the Client Server without SERVER MANAGER"
#define OPTION_WITH_SM "Listen for SERVER MANAGER"
#define MAX_INPUT_LENGTH 256

// CLIENT SERVER MESSAGES
#define WELCOME_MESSAGE "\nWelcome to the chat, "
#define COMMAND_LIST "COMMAND LIST\n/h ----------------------> list of commands\n/ul ---------------------> list of users\n/u <username> -----------> set username (MAX 15 chars, no spaces)\n/w <receiver username> <message> -> whisper\n\n"
#define SHUTDOWN_MESSAGE "Server is now offline. Please join back later.\n"
#define SERVER_FULL "Server: server is full, please join back later\n"
#define USERNAME_FAILURE "Server: Sorry that username is already taken\n"
#define USERNAME_SUCCESS "Server: Success! You will now go by "
#define COMMAND_NOT_FOUND "Server: Invalid Command. /h for help\n"
#define INVALID_NUM_ARGS "Server: Error! Invalid # Arguments. /h for command list.\n"
#define INVALID_RECEIVER "Server: Non Existent Receiver\n"
#define USERNAME_TOO_LONG "Server: Error, username too long. 15 is the MAX.\n"

struct ClientInfo
{
    int   client_socket;
    int   client_index;
    char *username;
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static struct ClientInfo clients[MAX_CLIENTS];

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static int client_count = 0;

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static volatile sig_atomic_t exit_flag = 0;

#endif    // SERVER_SERVER_H
