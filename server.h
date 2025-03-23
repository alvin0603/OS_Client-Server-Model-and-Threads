#ifndef SERVER_H
#define SERVER_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <time.h>
#define PORT 6666
#define MAX_client 10
#define MAX_size 100000


extern pthread_mutex_t whiteboard_mutex;
extern pthread_mutex_t clients_mutex; // Prevent two threads from read/write the same shared resources
extern int client_socket[MAX_client];
extern char* client_usernames[MAX_client];
extern char whiteboard[MAX_size];

void* serving_client(void* arg);
void broadcast(char* message, int sender);
void get_Time(char* time_string, size_t len);
#endif