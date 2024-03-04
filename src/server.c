#include "../include/server.h"

void *handle_client(void *arg)
{
    char buffer[BUFFER_SIZE];
    /// char                     sent_message[BUFFER_SIZE];
    const struct ClientInfo *client_info     = (struct ClientInfo *)arg;
    int                      client_socket   = client_info->client_socket;
    int                      client_index    = client_info->client_index;
    const char              *client_username = client_info->username;    // Change to pointer
                                                                         // ssize_t                  bytes_sent;// Change bytes_sent to ssize_t

    while(1)
    {
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if(bytes_received <= 0)
        {
            printf("%s left the chat.\n", client_username);
            close(client_socket);
            client_count--;
            printf("Population: %d/%d\n", client_count, MAX_CLIENTS);
            fflush(stdout);
            pthread_mutex_lock(&clients_mutex);         // Lock the mutex before modifying the clients array
            clients[client_index].client_socket = 0;    // Mark client socket as closed
            pthread_mutex_unlock(&clients_mutex);       // Unlock the mutex after modifying the clients array

            break;    // Exit the loop when client disconnects
        }

        buffer[bytes_received] = '\0';
        printf("Received from %s: %s", client_username, buffer);

        // snprintf(sent_message, sizeof(sent_message), "%s: %s", client_username, buffer);

        //        pthread_mutex_lock(&clients_mutex);    // Lock the mutex before accessing the clients array

        handle_message(buffer, client_socket);

        //        // Broadcast the message to all other connected clients
        //        for(int i = 0; i < MAX_CLIENTS; ++i)
        //        {
        //            if(clients[i].client_socket != 0 && i != client_index)
        //            {
        //                bytes_sent = send(clients[i].client_socket, sent_message, strlen(sent_message), 0);
        //                if(bytes_sent != (ssize_t)strlen(sent_message))    // Cast strlen to ssize_t
        //                {
        //                    //                    fprintf(stderr, "Error sending message to client %d\n", i);
        //
        //                    // Close the connection to the client
        //                    close(clients[i].client_socket);
        //
        //                    // Mark the client socket as closed
        //                    pthread_mutex_lock(&clients_mutex);
        //                    clients[i].client_socket = 0;
        //                    pthread_mutex_unlock(&clients_mutex);
        //
        //                    // TODO: MIGHT NEED TO FREE MEM HERE
        //
        //                    // Optionally, you can continue processing other clients or break out of the loop
        //                    continue;
        //                    // break;
        //                }
        //
        //                printf("%d <-------- %s", clients[i].client_socket, buffer);
        //            }
        //        }
        //
        //        pthread_mutex_unlock(&clients_mutex);    // Unlock the mutex after accessing the clients array

        // print_users();
    }

    //    pthread_exit(NULL);    // Exit the thread when the loop breaks

    pthread_exit(NULL);
}

void start_groupChat_server(struct sockaddr_storage addr, in_port_t port, int sm_socket, int pipe_write_fd)
{
    int                     server_socket;
    struct sockaddr_storage client_addr;
    socklen_t               client_addr_len;
    pthread_t               tid;

    server_socket = socket_create(addr.ss_family, SOCK_STREAM, 0);
    socket_bind(server_socket, &addr, port);
    start_listening(server_socket, BASE_TEN);
    setup_signal_handler();

    // Allocate memory for usernames
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        clients[i].username = malloc(MAX_USERNAME_SIZE);
        if(clients[i].username == NULL)
        {
            perror("Memory allocation failed");
            free_usernames();
            exit(EXIT_FAILURE);
        }
    }

    while(!exit_flag)
    {
        int    max_sd;
        int    activity;
        fd_set readfds;
        memset(&readfds, 0, sizeof(readfds));
        FD_SET(server_socket, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sm_socket, &readfds);    // Add the Server Manager socket to the read set
        max_sd = server_socket;
        for(int i = 0; i < MAX_CLIENTS; ++i)
        {
            if(clients[i].client_socket > 0)    // Check if the client socket is valid
            {
                FD_SET(clients[i].client_socket, &readfds);
                if(clients[i].client_socket > max_sd)
                {
                    max_sd = clients[i].client_socket;
                }
            }
        }

        // Wait for activity on one of the sockets
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if(activity == -1)
        {
            //             perror("select");
            continue;    // Keep listening for connections
        }

        // New connection
        if(FD_ISSET(server_socket, &readfds))
        {
            int                client_socket;
            int                client_index = -1;
            struct ClientInfo *client_info;
            char               welcome_message[BUFFER_SIZE];
            ssize_t            bytes_written;

            client_addr_len = sizeof(client_addr);
            client_socket   = socket_accept_connection(server_socket, &client_addr, &client_addr_len);

            if(client_socket == -1)
            {
                // TODO: error hand
                continue;    // Continue listening for connections
            }

            pthread_mutex_lock(&clients_mutex);

            for(int i = 0; i < MAX_CLIENTS; ++i)
            {
                if(clients[i].client_socket == 0)
                {
                    client_index = i;
                    client_count++;
                    break;
                }
            }

            if(client_index == -1)
            {
                const char *rejection_message = SERVER_FULL;
                send(client_socket, rejection_message, strlen(rejection_message), 0);
                close(client_socket);
                pthread_mutex_unlock(&clients_mutex);
                continue;    // Continue listening for connections
            }

            printf("New connection from %s:%d, assigned to Client%d\n", inet_ntoa(((struct sockaddr_in *)&client_addr)->sin_addr), ntohs(((struct sockaddr_in *)&client_addr)->sin_port), client_index + 1);
            printf("Population: %d/%d\n", client_count, MAX_CLIENTS);
            fflush(stdout);

            clients[client_index].client_socket = client_socket;
            clients[client_index].client_index  = client_index;
            snprintf(clients[client_index].username, MAX_USERNAME_SIZE, "Client%d", client_index + 1);    // Use snprintf to avoid buffer overflow

            pthread_mutex_unlock(&clients_mutex);

            sprintf(welcome_message, "%s%s!\n\n", WELCOME_MESSAGE, clients[client_index].username);

            send(client_socket, welcome_message, strlen(welcome_message), 0);
            send(client_socket, COMMAND_LIST, strlen(COMMAND_LIST), 0);

            // Send the updated client count to the admin server
            bytes_written = write(pipe_write_fd, &client_count, sizeof(client_count));
            if(bytes_written != sizeof(client_count))
            {
                perror("Failed to write client count to pipe");
            }

            // welcome message

            // Create a new thread to handle the client
            client_info = &clients[client_index];    // Pass the address of the struct in the array
            if(pthread_create(&tid, NULL, handle_client, (void *)client_info) != 0)
            {
                perror("Thread creation failed");
                close(client_socket);
                continue;
            }

            pthread_detach(tid);
        }

        // Check for messages from the Server Manager
        if(FD_ISSET(sm_socket, &readfds))
        {
            char    buffer[BUFFER_SIZE];
            ssize_t bytes_read;
            bytes_read = recv(sm_socket, buffer, sizeof(buffer), 0);
            if(bytes_read > 0)
            {
                // Handle the message from the Server Manager
                printf("Message from Server Manager: %s\n", buffer);
                // You can add logic here to process the message and take appropriate actions
            }
        }
    }

    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(clients[i].client_socket != 0)
        {
            send(clients[i].client_socket, SHUTDOWN_MESSAGE, strlen(SHUTDOWN_MESSAGE), 0);
        }
    }

    // Close server socket
    shutdown(server_socket, SHUT_RDWR);
    socket_close(server_socket);

    free_usernames();
}

void handle_message(const char *buffer, int sender_fd)
{
    if(buffer[0] == '/')
    {
        // Extract command
        char command[BUFFER_SIZE];    // Assuming command length is not more than 20 characters
        sscanf(buffer, "/%19s", command);

        // printf("command: %s\n", command);

        // Check command and call corresponding function
        if(strcmp(command, "h") == 0)
        {
            send(sender_fd, COMMAND_LIST, strlen(COMMAND_LIST), 0);
        }
        else if(strcmp(command, "ul") == 0)
        {
            send_user_list(sender_fd);
        }
        else if(strcmp(command, "u") == 0)
        {
            set_username(sender_fd, buffer);
        }
        else if(strcmp(command, "w") == 0)
        {
            direct_message(sender_fd, buffer);
        }
        else
        {
            // printf("Command not found\n");
            send(sender_fd, COMMAND_NOT_FOUND, sizeof(COMMAND_NOT_FOUND), 0);
        }
    }
    else
    {
        char message_with_sender[MESSAGE_SIZE];

        for(int i = 0; i < MAX_CLIENTS; ++i)
        {
            // Check if the client socket is valid and username is not NULL
            if(clients[i].client_socket == sender_fd)
            {
                sprintf(message_with_sender, "[All] %s: %s", clients[i].username, buffer);
                break;
            }
        }

        pthread_mutex_lock(&clients_mutex);

        for(int i = 0; i < MAX_CLIENTS; ++i)
        {
            if(clients[i].client_socket != 0 && clients[i].client_socket != sender_fd)
            {
                ssize_t bytes_sent;

                bytes_sent = send(clients[i].client_socket, message_with_sender, strlen(message_with_sender), 0);
                if(bytes_sent != (ssize_t)strlen(message_with_sender))
                {
                    // Handle error sending message
                    fprintf(stderr, "Error sending message to client %d\n", i);

                    // Close the connection to the client
                    close(clients[i].client_socket);

                    // Mark the client socket as closed
                    pthread_mutex_lock(&clients_mutex);
                    clients[i].client_socket = 0;
                    pthread_mutex_unlock(&clients_mutex);

                    // Optionally, you can continue processing other clients or break out of the loop
                    continue;
                }

                printf("%d <-------- %s", clients[i].client_socket, buffer);
            }
        }

        pthread_mutex_unlock(&clients_mutex);    // Unlock the mutex after accessing the clients array
    }
}

void send_user_list(int sender_fd)
{
    char user_list[BUFFER_SIZE];
    memset(user_list, 0, sizeof(user_list));    // Initialize user_list

    // Copy "USER LIST" to user_list
    strncpy(user_list, "USER LIST\n", sizeof(user_list) - 1);    // Use strncpy to avoid buffer overflow

    // Concatenate each user name to the message
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        // Check if the client socket is valid and username is not NULL
        if(clients[i].client_socket != 0 && clients[i].username != NULL)
        {
            // Concatenate username to user_list
            strncat(user_list, clients[i].username, sizeof(user_list) - strlen(user_list) - 1);    // Use strncat to avoid buffer overflow

            if(sender_fd == clients[i].client_socket)
            {
                strncat(user_list, "(you)", sizeof(user_list) - strlen(user_list) - 1);
            }

            strncat(user_list, "\n", sizeof(user_list) - strlen(user_list) - 1);    // Add newline character
        }
    }

    // Send user_list to the sender_fd
    send(sender_fd, user_list, strlen(user_list), 0);
}

void set_username(int sender_fd, const char *buffer)
{
    char response[BUFFER_SIZE];
    char command[BASE_TEN];
    char username[BUFFER_SIZE];
    char nothing[BUFFER_SIZE];

    if(sscanf(buffer, "/%9s %50s %100s", command, username, nothing) != 2)
    {
        send(sender_fd, INVALID_NUM_ARGS, strlen(INVALID_NUM_ARGS), 0);
        return;
    }

    if(strlen(username) > MAX_USERNAME_SIZE)
    {
        send(sender_fd, USERNAME_TOO_LONG, strlen(USERNAME_TOO_LONG), 0);
        return;
    }

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(strcmp(clients[i].username, username) == 0)
        {
            send(sender_fd, USERNAME_FAILURE, strlen(USERNAME_FAILURE), 0);
            return;
        }
    }

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(sender_fd == clients[i].client_socket)
        {
            snprintf(clients[i].username, MAX_USERNAME_SIZE, "%s", username);
            break;
        }
    }

    sprintf(response, "%s%s.\n", USERNAME_SUCCESS, username);
    send(sender_fd, response, strlen(response), 0);
}

void direct_message(int sender_fd, const char *buffer)
{
    // char response[BUFFER_SIZE];
    char command[BASE_TEN];
    char receiver[MAX_USERNAME_SIZE + 1];
    char message[BUFFER_SIZE];
    char sent_message[BUFFER_SIZE];
    //    int  receiver_fd = 0;
    //    int  receiver_fd = 0;
    int sender_id;

    if(sscanf(buffer, "/%9s %14s %1023[^\n]", command, receiver, message) != 3)
    {
        send(sender_fd, INVALID_NUM_ARGS, strlen(INVALID_NUM_ARGS), 0);
        return;
    }

    for(sender_id = 0; sender_id < MAX_CLIENTS; sender_id++)
    {
        if(clients[sender_id].client_socket == sender_fd)
        {
            break;
        }
    }

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(strcmp(clients[i].username, receiver) == 0)
        {
            if(sender_id == i)
            {
                sprintf(sent_message, "[Note] %s: %s\n", clients[sender_id].username, message);
            }
            else
            {
                sprintf(sent_message, "[Direct] %s: %s\n", clients[sender_id].username, message);
            }

            send(clients[i].client_socket, sent_message, strlen(sent_message), 0);
            return;
        }
    }

    send(sender_fd, INVALID_RECEIVER, strlen(INVALID_RECEIVER), 0);
}

void free_usernames(void)
{
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(clients[i].username != NULL)
        {
            free(clients[i].username);
            clients[i].username = NULL;    // Optional: Set the pointer to NULL after freeing
        }
    }
}

void handle_arguments(const char *ip_address, const char *port_str, in_port_t *port)
{
    if(ip_address == NULL)
    {
        printf("ip is null\n");
        exit(EXIT_FAILURE);
    }

    if(port_str == NULL)
    {
        printf("port str is null\n");
        exit(EXIT_FAILURE);
    }

    *port = parse_in_port_t(port_str);
}

in_port_t parse_in_port_t(const char *str)
{
    char     *endptr;
    uintmax_t parsed_value;

    errno        = 0;
    parsed_value = strtoumax(str, &endptr, BASE_TEN);

    if(errno != 0)
    {
        perror("Error parsing in_port_t\n");
        exit(EXIT_FAILURE);
    }

    if(*endptr != '\0')
    {
        printf("non-numerics inside port\n");
        exit(EXIT_FAILURE);
    }

    if(parsed_value > UINT16_MAX)
    {
        printf("port out of range\n");
        exit(EXIT_FAILURE);
    }

    return (in_port_t)parsed_value;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

void sigint_handler(int signum)
{
    exit_flag = 1;
}

#pragma GCC diagnostic pop

void convert_address(const char *address, struct sockaddr_storage *addr)
{
    memset(addr, 0, sizeof(*addr));

    if(inet_pton(AF_INET, address, &(((struct sockaddr_in *)addr)->sin_addr)) == 1)
    {
        addr->ss_family = AF_INET;
    }
    else if(inet_pton(AF_INET6, address, &(((struct sockaddr_in6 *)addr)->sin6_addr)) == 1)
    {
        addr->ss_family = AF_INET6;
    }
    else
    {
        fprintf(stderr, "%s is not an IPv4 or an IPv6 address\n", address);
        exit(EXIT_FAILURE);
    }
}

int socket_create(int domain, int type, int protocol)
{
    int sockfd;
    int opt = 1;

    sockfd = socket(domain, type, protocol);

    if(sockfd == -1)
    {
        perror("Socket creation failed\n");
        exit(EXIT_FAILURE);
    }

    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

void socket_bind(int sockfd, struct sockaddr_storage *addr, in_port_t port)
{
    char      addr_str[INET6_ADDRSTRLEN];
    socklen_t addr_len;
    void     *vaddr;
    in_port_t net_port;

    net_port = htons(port);

    if(addr->ss_family == AF_INET)
    {
        struct sockaddr_in *ipv4_addr;

        ipv4_addr           = (struct sockaddr_in *)addr;
        addr_len            = sizeof(*ipv4_addr);
        ipv4_addr->sin_port = net_port;
        vaddr               = (void *)&(((struct sockaddr_in *)addr)->sin_addr);
    }
    else if(addr->ss_family == AF_INET6)
    {
        struct sockaddr_in6 *ipv6_addr;

        ipv6_addr            = (struct sockaddr_in6 *)addr;
        addr_len             = sizeof(*ipv6_addr);
        ipv6_addr->sin6_port = net_port;
        vaddr                = (void *)&(((struct sockaddr_in6 *)addr)->sin6_addr);
    }
    else
    {
        fprintf(stderr,
                "Internal error: addr->ss_family must be AF_INET or AF_INET6, was: "
                "%d\n",
                addr->ss_family);
        exit(EXIT_FAILURE);
    }

    if(inet_ntop(addr->ss_family, vaddr, addr_str, sizeof(addr_str)) == NULL)
    {
        perror("inet_ntop\n");
        exit(EXIT_FAILURE);
    }

    printf("Binding to: %s:%u\n", addr_str, port);

    if(bind(sockfd, (struct sockaddr *)addr, addr_len) == -1)
    {
        perror("Binding failed");
        fprintf(stderr, "Error code: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    printf("Bound to socket: %s:%u\n", addr_str, port);
}

void start_listening(int server_fd, int backlog)
{
    if(listen(server_fd, backlog) == -1)
    {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Listening for incoming connections...\n");
}

int socket_accept_connection(int server_fd, struct sockaddr_storage *client_addr, socklen_t *client_addr_len)
{
    int  client_fd;
    char client_host[NI_MAXHOST];
    char client_service[NI_MAXSERV];

    errno     = 0;
    client_fd = accept(server_fd, (struct sockaddr *)client_addr, client_addr_len);

    if(client_fd == -1)
    {
        if(errno != EINTR)
        {
            perror("accept failed\n");
        }

        return -1;
    }

    if(getnameinfo((struct sockaddr *)client_addr, *client_addr_len, client_host, NI_MAXHOST, client_service, NI_MAXSERV, 0) == 0)
    {
        // printf("Received new request from -> %s:%s\n\n", client_host, client_service);
    }
    else
    {
        printf("Unable to get client information\n");
    }

    return client_fd;
}

void setup_signal_handler(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));

#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#endif
    sa.sa_handler = sigint_handler;
#if defined(__clang__)
    #pragma clang diagnostic pop
#endif

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if(sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

void socket_close(int sockfd)
{
    if(close(sockfd) == -1)
    {
        perror("Error closing socket\n");
        exit(EXIT_FAILURE);
    }
}
