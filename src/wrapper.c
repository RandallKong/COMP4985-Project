
#include "../include/server.h"

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
            //            handle_prompt(&address, &port_str);
            //            handle_arguments(address, port_str, &port);
            //            convert_address(address, &addr);
            //            start_client_server(addr, port);
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
    start_server(addr, port);
}

void handle_prompt(char **address, char **port_str)
{
    printf("\nEnter the IP address to bind the server: ");
    *address = malloc(MAX_INPUT_LENGTH * sizeof(char));
    fgets(*address, MAX_INPUT_LENGTH, stdin);
    (*address)[strcspn(*address, "\n")] = 0;    // Remove newline character

    printf("\nEnter the port to bind the server: ");
    *port_str = malloc(MAX_INPUT_LENGTH * sizeof(char));
    fgets(*port_str, MAX_INPUT_LENGTH, stdin);
    (*port_str)[strcspn(*port_str, "\n")] = 0;    // Remove newline character

    // Print the inputted IP address and port
    printf("IP Address: %s\n", *address);
    printf("Port: %s\n", *port_str);
}
