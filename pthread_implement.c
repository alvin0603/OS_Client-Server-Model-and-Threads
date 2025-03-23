#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "server.h"

extern char whiteboard[MAX_size];
char* client_usernames[MAX_client] = {NULL};

void broadcast(char* massage, int sender) // sender used to distinguish server itself
{
    pthread_mutex_lock(&clients_mutex);
    for(int i = 0; i < MAX_client; i++)
    {
        if(client_socket[i] && client_socket[i] != sender) 
            send(client_socket[i], massage, strlen(massage), 0);
    }
    pthread_mutex_unlock(&clients_mutex); // after accessing shared variable
}
void* serving_client(void* arg) // handle the interaction between server and client
{
    int client_fd = *(int*)arg;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char str_received[1024];
    char username[50] = {0};

    // get client IP
    getpeername(client_fd, (struct sockaddr*)&client_addr, &addr_len);
    char* client_ip = inet_ntoa(client_addr.sin_addr);
    int client_port = ntohs(client_addr.sin_port);
    // get client name
    int received_bytes = recv(client_fd, username, sizeof(username) - 1, 0);
    if(received_bytes <= 0)
    {
        printf("<User register failed>\n");
        return NULL;
    }
    username[received_bytes] = '\0'; 
    // clear new line
    char* nl = strchr(username, '\n');
    if (nl)
        *nl = '\0'; 
    // register the client
    pthread_mutex_lock(&clients_mutex);
    for(int i = 0; i < MAX_client; i++)
    {
        if(!client_socket[i]) 
        {
            client_socket[i] = client_fd;
            client_usernames[i] = strdup(username);
            break; 
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    // write on-line message to whiteborad
    pthread_mutex_lock(&whiteboard_mutex);
    char time[100];
    get_Time(time, sizeof(time));
    snprintf(whiteboard + strlen(whiteboard), MAX_size - strlen(whiteboard),"%s %s connected.\n", time, username);
    pthread_mutex_unlock(&whiteboard_mutex);

    // broadcast to all client someone on-line
    char login_massage[256];
    snprintf(login_massage, sizeof(login_massage), "<User %s is on-line, socket address: %s/%d>\n",username, client_ip, client_port);
    broadcast(login_massage, client_fd);
    // keeping receiving message from server & sending message to server
    while(1)
    {
        received_bytes = recv(client_fd, str_received, sizeof(str_received)-1, 0);
        if(received_bytes <= 0) // receiving failed -> disconnection
        {
            pthread_mutex_lock(&clients_mutex);
            for(int i = 0; i < MAX_client; i++) 
            {
                if(client_socket[i] == client_fd) 
                {
                    client_socket[i] = 0;
                    break;
                }
            }
            pthread_mutex_unlock(&clients_mutex);

            char logout_msg[256];
            snprintf(logout_msg, sizeof(logout_msg),"<User %s is off-line>\n", username);
            broadcast(logout_msg, 0);  // 0 -> server 

            // write off-line message to whiteborad
            pthread_mutex_lock(&whiteboard_mutex);
            char time[100];
            get_Time(time, sizeof(time));
            snprintf(whiteboard + strlen(whiteboard), MAX_size - strlen(whiteboard),"%s %s disconnected.\n", time, username);
            pthread_mutex_unlock(&whiteboard_mutex);

            close(client_fd);
            return NULL;
        }

        str_received[received_bytes] = '\0';
        //printf("%s\n",str_received);
        if (strncmp(str_received, "chat ", 5) == 0) 
        {
            // divide the command into receiver and message
            char *input = str_received + 5;
            char receiver[50];
            char message[1000];
            int length_receiver = 0;
            while (*input != '\0' && *input != ' ' && *input != '\"') 
            {
                receiver[length_receiver++] = *input;
                input++;
            }
            receiver[length_receiver] = '\0';

            while(*input == ' ')
                input++;
            if(*input != '\"')
            {
                send(client_fd, "<Message format error\nPlease start with '\"'>\n", 22, 0);
                continue;
            }
            input++; // skip "
            int length_message = 0;
            while(*input != '\0' && *input != '\"')
            {
                message[length_message++] = *input;
                input++;
            }
            message[length_message] = '\0';
            if(*input != '\"')
            {
                send(client_fd, "<Message format error\nPlease start with '\"'>\n", 22, 0);
                continue;
            }
            
            pthread_mutex_lock(&clients_mutex);
            // send message to receiver
            int found = 0;
            int offline = 0;
            for(int i = 0; i < MAX_client; i++)
            {
                if (client_usernames[i] &&  strcmp(client_usernames[i], receiver) == 0)
                {
                    if(client_socket[i] != 0) // user exist
                    {
                        char message_for_sending[1056];
                        snprintf(message_for_sending, sizeof(message_for_sending), "[%s]: %s\n", username, message);
                        send(client_socket[i], message_for_sending, strlen(message_for_sending), 0);
                        found = 1;
                    }
                    else // user exist but off-line
                        offline = 1;
                }
            }
            pthread_mutex_unlock(&clients_mutex);
            if(!found)
            {
                if(offline)
                {
                    char error_message[1000];
                    snprintf(error_message, sizeof(error_message), "<User %s is off-line>\n", receiver);
                    send(client_fd, error_message, strlen(error_message), 0);
                }
                else
                {
                    char error_message[1000];
                    snprintf(error_message, sizeof(error_message), "<User %s does not exist>\n", receiver);
                    send(client_fd, error_message, strlen(error_message), 0);
                }
                continue;
            }
            // write message to whiteboard
            pthread_mutex_lock(&whiteboard_mutex);
            get_Time(time, sizeof(time));
            snprintf(whiteboard + strlen(whiteboard), MAX_size,  "%s %s is using the whiteboard.\n", time, username);
            snprintf(whiteboard + strlen(whiteboard), MAX_size,  "<To %s> %s\n", receiver, message);
            printf("\n=== Whiteboard Updated ===\n%s\n", whiteboard);
            fflush(stdout); 
            pthread_mutex_unlock(&whiteboard_mutex);
        } 
        else if(strncmp(str_received, "bye", 3) == 0) 
        {
            // clear the off-line user
            pthread_mutex_lock(&clients_mutex);
            for (int i = 0; i < MAX_client; i++) 
            {
                if (client_socket[i] == client_fd) 
                {
                    client_socket[i] = 0;          
                    break;
                }
            }
            pthread_mutex_unlock(&clients_mutex);
            // broadcast to all client someone on-line
            char logout_message[256];
            snprintf(logout_message, sizeof(logout_message), "<User %s is off-line>\n", username);
            broadcast(logout_message, 0);
            
            // write off-line message to whiteborad
            pthread_mutex_lock(&whiteboard_mutex);
            char time[100];
            get_Time(time, sizeof(time));
            snprintf(whiteboard + strlen(whiteboard), MAX_size - strlen(whiteboard),"%s %s disconnected.\n", time, username);
            pthread_mutex_unlock(&whiteboard_mutex);

            close(client_fd);
            return NULL;
        }
    }
    return NULL;
}
void get_Time(char* time_string, size_t len) // get the string storing timer
{
    time_t now = time(NULL);
    struct tm* time_info = localtime(&now);
    strftime(time_string, len, "[%Y %B %d, %H:%M:%S]", time_info);
}