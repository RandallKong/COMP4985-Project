
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
            start_server(addr, port);

            free(address);
            free(port_str);
            break;
        case 2:
            printf("You chose option 2: With session management\n");
            handle_prompt(&address, &port_str);
            handle_arguments(address, port_str, &port);
            convert_address(address, &addr);
            start_client_server(addr, port);

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

void start_client_server(struct sockaddr_storage addr, in_port_t port)
{
    int                     server_socket;
    struct sockaddr_storage client_addr;
    socklen_t               client_addr_len;
    fd_set                  readfds;
    int                     max_sd;

    server_socket = socket_create(addr.ss_family, SOCK_STREAM, 0);
    socket_bind(server_socket, &addr, port);
    start_listening(server_socket, BASE_TEN);
    setup_signal_handler();

    // Initialize the set of active sockets
    FD_ZERO(&readfds);
    FD_SET(server_socket, &readfds);
    max_sd = server_socket;

    while(!exit_flag)
    {
        // Wait for activity on the server socket
        if(select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // Check if there is a new connection
        if(FD_ISSET(server_socket, &readfds))
        {
            char passkey_buffer[TWO_FIFTY_SIX];
            int  attempts;
            bool passkey_matched;
            int  new_socket;
            client_addr_len = sizeof(client_addr);
            new_socket      = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);

            if(new_socket < 0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            // Print details of the client (IP/Port)
            printf("New connection from %s:%d\n", inet_ntoa(((struct sockaddr_in *)&client_addr)->sin_addr), ntohs(((struct sockaddr_in *)&client_addr)->sin_port));

            // Handle the server manager connection
            attempts        = 0;
            passkey_matched = false;

            while(attempts < 3 && !passkey_matched)
            {
                ssize_t bytes_read;
                memset(passkey_buffer, 0, sizeof(passkey_buffer));
                bytes_read = recv(new_socket, passkey_buffer, sizeof(passkey_buffer) - 1, 0);
                printf("Received passkey: %s\n", passkey_buffer);

                // Remove the newline character if present
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
                // Passkey matched, you can proceed with further communication
                // start_server(addr, port); // Uncomment if you need to start another server
                // Passkey matched, you can proceed with further communication
                // start_server(addr, port); // Uncomment if you need to start another server

                // Close the socket after handling the connection

                // Close the socket after handling the connection
                close(new_socket);
            }
        }
    }

    // Close the server socket and clean up
    close(server_socket);
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
