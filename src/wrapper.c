#include "../include/protocol.h"
#include "../include/server.h"
#include <stdbool.h>

int main(void)
{
    in_port_t               port;
    char                   *address  = NULL;
    char                   *port_str = NULL;
    struct sockaddr_storage addr;
    char                    input[MAX_INPUT_LENGTH];
    char                   *endptr;
    long                    choice;

    printf("****%s****\n", WELCOME_STARTUP);
    printf("1: %s\n", OPTION_NO_SM);
    printf("2: %s\n", OPTION_WITH_SM);

    // Prompt user for choice
    printf("\nEnter your choice (1 or 2): ");
    fgets(input, sizeof(input), stdin);
    choice = strtol(input, &endptr, BASE_TEN);

    // Switch statement based on user choice
    switch(choice)
    {
        case 1:
            printf("You chose option 1: No session management\n");
            handle_prompt(&address, &port_str);
            handle_arguments(address, port_str, &port);
            convert_address(address, &addr);
            start_groupChat_server(addr, port, 0, 0);

            free(address);
            free(port_str);
            break;
        case 2:
            printf("You chose option 2: With session management\n");
            handle_prompt(&address, &port_str);
            handle_arguments(address, port_str, &port);
            convert_address(address, &addr);
            start_admin_server(addr, port);

            free(address);
            free(port_str);
            break;
        default:
            printf("Invalid choice. Please enter 1 or 2.\n");
            printf("\nRandall be cute\n");
            break;
    }
    return 0;
}

void start_admin_server(struct sockaddr_storage addr, in_port_t port)
{
    int                     server_socket;
    struct sockaddr_storage client_addr;
    socklen_t               client_addr_len;
    fd_set                  readfds;
    int                     max_sd;
    int                     pipe_fds[2];
    int                     server_manager_socket = 0;

    server_socket = socket_create(addr.ss_family, SOCK_STREAM, 0);
    socket_bind(server_socket, &addr, port);
    start_listening(server_socket, BASE_TEN);
    admin_setup_signal_handler();

    // Create a pipe for communication between the admin server and the group chat server
    if(pipe2(pipe_fds, O_CLOEXEC) == -1)    // use incase D'Arcy template
                                            //    if(pipe(pipe_fds) == -1) // use incase gcc
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Initialize the set of active sockets
    FD_ZERO(&readfds);
    FD_SET(server_socket, &readfds);
    FD_SET(pipe_fds[0], &readfds);    // Add the read end of the pipe to the set
    max_sd = server_socket > pipe_fds[0] ? server_socket : pipe_fds[0];

    while(!admin_exit_flag)
    {
        // Wait for activity on the server socket or the pipe
        if(select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0)
        {
            if(errno == EINTR)
            {
                continue;    // Interrupted by signal, continue the loop
            }
            perror("select");
            exit(EXIT_FAILURE);
        }

        // Check if there is a new connection and no active server manager connection
        if(FD_ISSET(server_socket, &readfds) && server_manager_socket == 0)
        {
            server_manager_socket = handle_new_server_manager(server_socket, &client_addr, &client_addr_len, pipe_fds, addr, port);
            if(server_manager_socket > 0)
            {
                FD_SET(server_manager_socket, &readfds);
                if(server_manager_socket > max_sd)
                {
                    max_sd = server_manager_socket;
                }
            }
        }

        // Check if there is activity on the server manager socket
        //        if(server_manager_socket > 0 && FD_ISSET(server_manager_socket, &readfds))
        //        {
        //            // Handle activity on the server manager socket (e.g., receiving commands)
        //            // If the connection is closed, remove the socket from the set and reset the variable
        //            FD_CLR(server_manager_socket, &readfds);
        //            close(server_manager_socket);
        //            server_manager_socket = 0;
        //        }

        // Check if there is data to read from the pipe
        if(FD_ISSET(pipe_fds[0], &readfds))
        {
            read_from_pipe(pipe_fds[0], server_manager_socket);    // Read messages from the child process
        }

        // Update the set of active sockets for the next iteration
        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);
        FD_SET(pipe_fds[0], &readfds);
        if(server_manager_socket > 0)
        {
            FD_SET(server_manager_socket, &readfds);
        }
    }

    // Close the server socket and clean up
    close(server_socket);
    close(pipe_fds[0]);    // Close the read end of the pipe
    if(server_manager_socket > 0)
    {
        close(server_manager_socket);    // Close the server manager socket if it's still open
    }
}

void handle_prompt(char **address, char **port_str)
{
    printf("\nEnter the IP address to bind the server (default 127.0.0.1): ");
    *address = malloc(MAX_INPUT_LENGTH * sizeof(char));
    fgets(*address, MAX_INPUT_LENGTH, stdin);
    (*address)[strcspn(*address, "\n")] = 0;    // Remove newline character

    // Default to 127.0.0.1 if no input is detected
    if(strlen(*address) == 0)
    {
        free(*address);
        *address = strdup("127.0.0.1");
        printf("No input detected. Defaulting to IP address: %s\n", *address);
    }

    printf("\nEnter the port to bind the server (default 8080): ");
    *port_str = malloc(MAX_INPUT_LENGTH * sizeof(char));
    fgets(*port_str, MAX_INPUT_LENGTH, stdin);
    (*port_str)[strcspn(*port_str, "\n")] = 0;    // Remove newline character

    // Default to 8080 if no input is detected
    if(strlen(*port_str) == 0)
    {
        free(*port_str);
        *port_str = strdup("8080");
        printf("No input detected. Defaulting to port: %s\n", *port_str);
    }

    // Print the inputted IP address and port
    printf("IP Address: %s\n", *address);
    printf("Port: %s\n", *port_str);
}

void admin_setup_signal_handler(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));

#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#endif
    sa.sa_handler = admin_sigint_handler;
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

void admin_sigint_handler(int signum)
{
    (void)signum;
    admin_exit_flag = 1;
}

int handle_new_server_manager(int server_socket, struct sockaddr_storage *client_addr, socklen_t *client_addr_len, const int pipe_fds[2], struct sockaddr_storage addr, in_port_t port)
{
    char  passkey_buffer[TWO_FIFTY_SIX];
    int   attempts        = 0;
    bool  passkey_matched = false;
    int   new_socket      = accept(server_socket, (struct sockaddr *)client_addr, client_addr_len);
    pid_t pid;

    if(new_socket < 0)
    {
        perror("accept");
        return -1;
    }

    printf("New connection from %s:%d\n", inet_ntoa(((struct sockaddr_in *)client_addr)->sin_addr), ntohs(((struct sockaddr_in *)client_addr)->sin_port));

    // Authenticate the server manager connection
    while(attempts < 3 && !passkey_matched)
    {
        ssize_t bytes_read = recv(new_socket, passkey_buffer, sizeof(passkey_buffer) - 1, 0);
        if(bytes_read > 0 && passkey_buffer[bytes_read - 1] == '\n')
        {
            passkey_buffer[bytes_read - 1] = '\0';
        }

        if(bytes_read <= 0)
        {
            printf("Connection closed or error occurred\n");
            close(new_socket);
            return -1;
        }

        if(strcmp(passkey_buffer, PASSKEY) == 0)
        {
            passkey_matched = true;
            printf("Passkey matched. Connection authorized.\n");
        }
        else
        {
            printf("Incorrect passkey. Attempts remaining: %d\n", 2 - attempts);
            attempts++;
        }
    }

    if(!passkey_matched)
    {
        printf("Passkey authentication failed. Closing connection.\n");
        close(new_socket);
        return -1;
    }

    // Start the group chat server as a child process
    pid = fork();
    if(pid == 0)
    {
        // Child process: Start the group chat server
        close(pipe_fds[0]);                                                 // Close the read end of the pipe in the child
        start_groupChat_server(addr, port + 1, new_socket, pipe_fds[1]);    // Pass the write end of the pipe
        close(pipe_fds[1]);                                                 // Close the write end of the pipe after use
        exit(EXIT_SUCCESS);
    }
    else if(pid > 0)
    {
        close(pipe_fds[1]);    // Close the write end of the pipe in the parent
        // Optionally close new_socket if it's not used in the parent process
        // close(new_socket);
    }
    else
    {
        perror("fork");
        close(new_socket);
        exit(EXIT_FAILURE);
    }

    return new_socket;    // Return the server manager socket
}

void read_from_pipe(int pipe_fd, int server_manager_socket)
{
    int     received_client_count;
    ssize_t bytes_read = read(pipe_fd, &received_client_count, sizeof(received_client_count));

    if(bytes_read > 0)
    {
        char    count_str[BASE_TEN];    // Enough to hold all digits of an int
        ssize_t bytes_sent;
        printf("Received client count from group chat server: %d\n", received_client_count);

        // Send this information to the server manager
        sprintf(count_str, "%d", received_client_count);
        bytes_sent = send(server_manager_socket, count_str, strlen(count_str), 0);
        if(bytes_sent < 0)
        {
            perror("Failed to send client count to server manager");
        }
    }
    else if(bytes_read == 0)
    {
        printf("Group chat server closed the pipe.\n");
    }
    else
    {
        if(errno != EINTR)
        {
            perror("Failed to read from pipe");
        }
    }
}
