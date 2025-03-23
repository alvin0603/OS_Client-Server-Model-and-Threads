all: server client
server: server.c pthread_implement.c 
	gcc -o server server.c pthread_implement.c -pthread
client: client.c 
	gcc -o client client.c -pthread
clean: 
	rm -f server client