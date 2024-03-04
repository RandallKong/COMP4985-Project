
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

    server_socket = socket_create(addr.ss_family, SOCK_STREAM, 0);
    socket_bind(server_socket, &addr, port);
    start_listening(server_socket, BASE_TEN);
    setup_signal_handler();

    // Create a pipe for communication between the admin server and the group chat server
    if(pipe2(pipe_fds, O_CLOEXEC) == -1)
    {
        perror("pipe2");
        exit(EXIT_FAILURE);
    }

    // Initialize the set of active sockets
    FD_ZERO(&readfds);
    FD_SET(server_socket, &readfds);
    FD_SET(pipe_fds[0], &readfds);    // Add the read end of the pipe to the set
    max_sd = server_socket > pipe_fds[0] ? server_socket : pipe_fds[0];

    while(!exit_flag)
    {
        // Wait for activity on the server socket or the pipe
        if(select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // Check if there is a new connection
        if(FD_ISSET(server_socket, &readfds))
        {
            // Handle new connections
            char passkey_buffer[TWO_FIFTY_SIX];
            int  attempts        = 0;
            bool passkey_matched = false;
            int  new_socket      = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);

            if(new_socket < 0)
            {
                perror("accept");
                continue;
            }

            printf("New connection from %s:%d\n", inet_ntoa(((struct sockaddr_in *)&client_addr)->sin_addr), ntohs(((struct sockaddr_in *)&client_addr)->sin_port));

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
                    break;
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
            }
            else
            {
                pid_t pid = fork();
                if(pid == 0)
                {                                                                       // initialization of group chat server
                    close(pipe_fds[0]);                                                 // This child end // Child process: Close the read end of the pipe
                    start_groupChat_server(addr, port + 1, new_socket, pipe_fds[1]);    // Start the group chat server
                    close(pipe_fds[1]);                                                 // Close the write end of the pipe after use
                    exit(EXIT_SUCCESS);
                }
                else if(pid > 0)
                {                          // parent process continuing
                    close(pipe_fds[1]);    // Parent process: Close the write end of the pipe

                    // Continuously read messages from the child process
                    while(!exit_flag)
                    {
                        int     received_client_count;
                        ssize_t bytes_read = read(pipe_fds[0], &received_client_count, sizeof(received_client_count));

                        if(bytes_read > 0)
                        {
                            ssize_t bytes_sent;
                            printf("Received client count from group chat server: %d\n", received_client_count);

                            // Send this information to the server manager
                            bytes_sent = send(new_socket, &received_client_count, sizeof(received_client_count), 0);
                            if(bytes_sent != sizeof(received_client_count))
                            {
                                perror("Failed to send client count to server manager");
                            }
                        }
                        else if(bytes_read == 0)
                        {
                            printf("Group chat server closed the pipe.\n");
                            break;    // Exit the loop if the child process has closed the pipe
                        }
                        else
                        {
                            perror("Failed to read from pipe");
                            break;    // Exit the loop if an error occurred
                        }
                    }

                    close(pipe_fds[0]);    // Close the read end of the pipe after use
                    wait(NULL);            // Wait for the child process to terminate
                }
                else
                {
                    perror("fork");
                    exit(EXIT_FAILURE);
                }
                close(new_socket);    // Close the server manager connection
            }
        }

        // Update the set of active sockets for the next iteration
        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);
        FD_SET(pipe_fds[0], &readfds);
    }

    // Close the server socket and clean up
    close(server_socket);
    close(pipe_fds[0]);    // Close the read end of the pipe
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
