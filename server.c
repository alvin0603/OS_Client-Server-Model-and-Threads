#include "server.h"

int client_socket[MAX_client] = {0};
char whiteboard[MAX_size] = {0};
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t whiteboard_mutex = PTHREAD_MUTEX_INITIALIZER;
int main()
{
    // Build Socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0); // IPv4 TCP
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // allow repeated occupancy
    if(server_fd == -1)
    {
        printf("Socket created failed\n");
        close(server_fd);
        exit(0);
    }

    // Bind Socket to IP and PORT
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;  // IPv4
    server_address.sin_port = htons(PORT);  // Set up port
    server_address.sin_addr.s_addr = INADDR_ANY;  // Linsten all IP
    if(bind(server_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0)
    {
        printf("Bind failed\n");
        close(server_fd);
        exit(0);
    }
    printf("Socket successfully bound to port %d.\n", PORT);

    // Start listening
    if(listen(server_fd, 10) < 0)
    {
        printf("Listen failed\n");
        close(server_fd);
        exit(0);
    }
    printf("Server start listening on port %d...\n", PORT);
    
    while(1)
    {
        // Accept connection from clent
        struct sockaddr_in client_address;
        socklen_t address_len = sizeof(client_address);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_address, &address_len);
        if(client_fd == -1)
        {
            printf("Accept error\n");
            close(server_fd);
            exit(0);
        }
        printf("Client connected: %s\n", inet_ntoa(client_address.sin_addr));
        
        /*
        // Test communication with client after success
        char str_received[1024];
        if(recv(client_fd, str_received, sizeof(str_received), 0))
        {
            printf("Received massage: %s\n",str_received);
            send(client_fd, "Massage received in server\n",28, 0);
        }
        */
        int* new_client_fd = malloc(sizeof(int));
        *new_client_fd = client_fd; // ensure all has it own fd
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, serving_client, new_client_fd);
        pthread_detach(thread_id); // seperate from main program
    }
    close(server_fd); 
}