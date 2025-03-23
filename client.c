#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "server.h"

// Receive message from server
int client_fd = -1;
void* receiver_thread(void* arg) 
{
    char str_received[1024];
    while (1)
    { 
        int bytes_received = recv(client_fd, str_received, sizeof(str_received)-1, 0);
        if (bytes_received <= 0)
        {
            printf("\n<Server connected interrupted>\n");
            close(client_fd);
            exit(1); 
        }
        str_received[bytes_received] = '\0';
        printf("%s", str_received);
        fflush(stdout);
    }
    return NULL;
}

int main() 
{
    int connected = 0; 
    char command[1000];
    pthread_t thread_id;

    printf("(1) connect <server_ip> <port> <username>\n");
    printf("(2) chat <name> \"message content\"\n");
    printf("(3) bye\n");

    while (1) 
    {
        if (!fgets(command, sizeof(command), stdin))
            break;

        // clear '\n'
        char *tmp = strchr(command, '\n');
        if (tmp)
            *tmp = '\0';

        if (strncmp(command, "connect ", 8) == 0)
        {
            if (connected) 
            {
                printf("<Don't connected again.>\n");
                continue;
            }
            
            // parsing connect command
            char server_ip[100];
            int port = 0;
            char username[100];
            if (sscanf(command + 8, "%s %d %s", server_ip, &port, username) != 3) 
            {
                printf("<connect command error>\n");
                continue;
            }

            client_fd = socket(AF_INET, SOCK_STREAM, 0);
            // Set up target IP
            struct sockaddr_in server_address;
            memset(&server_address, 0, sizeof(server_address));
            server_address.sin_family = AF_INET;
            server_address.sin_port = htons(port);
            if (inet_pton(AF_INET, server_ip, &server_address.sin_addr) <= 0) 
            {
                printf("<IP error>\n");
                close(client_fd);
                client_fd = -1;
                continue;
            }

            // connect to server
            if (connect(client_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) 
            {
                printf("<Connect error>\n");
                close(client_fd);
                client_fd = -1;
                continue;
            }
            connected = 1;
            printf("<Connect to server %s:%d successfully>\n", server_ip, port);

            send(client_fd, username, strlen(username), 0); // send username to server
            // Start execution thread
            pthread_create(&thread_id, NULL, receiver_thread, NULL);
            pthread_detach(thread_id);

        }
        else if (strncmp(command, "chat ", 5) == 0)
        {
            if (!connected) 
            {
                printf("<Please connect to server first>\n");
                continue;
            }
            // send command to server directly(server is reponsible for handle it)
            strcat(command, "\n"); 
            send(client_fd, command, strlen(command), 0);
        }
        else if (strcmp(command, "bye") == 0) 
        {
            if (!connected) 
            {
                printf("<Please connect to server first>\n");
                continue;
            }
            // send it to server too
            send(client_fd, "bye\n", 4, 0);
            close(client_fd);
            client_fd = -1;
            connected = 0;
            printf("<You has been off-line>\n");
            break;
        }
        else 
        {
            printf("<Please follow the instruction>\n");
        }
    }
    close(client_fd);
    return 0;
}