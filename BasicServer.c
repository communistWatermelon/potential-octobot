/*
	main / listener
	set up server
	bind to port
	accept connection
		add client to list
		create io worker
		
io worker / processing
	if client disconnects
		remove client from list
	wait for input
	echo input to client
	write send / recv results
*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define SERVER_PORT 7000
#define BUFLEN  80 
#define TRUE 1

typedef struct
{
    int * client_socket;
    struct sockaddr_in* client;
} ClientWrapper ;

void createSocket(int*);
void bindSocket(int*, struct sockaddr_in*, int*);
void acceptClient(int*, int*, struct sockaddr*, socklen_t*);
void SocketOptions(int *);
void *serviceClient(void*);

int main()
{
    int listenSocket = 0;
    int newSocket = 0;
    int port = SERVER_PORT;
    socklen_t clientLength = 0;
    pthread_t clientThread;

    struct sockaddr_in server, client_addr;
    ClientWrapper arguments;
    

    createSocket(&listenSocket);
    SocketOptions(&listenSocket);
    bindSocket(&listenSocket, &server, &port);
    
    listen(listenSocket, 5);
    
    while (TRUE)
    {
        acceptClient(&listenSocket, &newSocket, (struct sockaddr *)&client_addr, &clientLength);
        arguments.client_socket = &newSocket;
        arguments.client = &client_addr;
        
        printf("Got a connection!");
        
        if (pthread_create(&clientThread, NULL, &serviceClient, (void *) &arguments) != 0)
        {
            perror ("Can't create thread!");
            exit(1);
        }
    }
    
    close(listenSocket);
    return 0;
}

void createSocket(int * listen_socket)
{
    if ((*listen_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror ("Can't create a socket");
		exit(1);
	}
}

void bindSocket(int * socket, struct sockaddr_in * server, int * port)
{
    bzero((char *)server, sizeof(struct sockaddr_in));
	server->sin_family = AF_INET;
	server->sin_port = htons(*port);
	server->sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(*socket, (struct sockaddr *)server, sizeof(*server)) == -1)
	{
		perror("Can't bind name to socket");
		exit(1);
	}
}

void acceptClient(int * server_socket, int * new_socket, struct sockaddr * client, socklen_t * client_len)
{
	*client_len= sizeof(client);
	if ((*new_socket = accept (*server_socket, (struct sockaddr *)client, client_len)) == -1)
	{
		fprintf(stderr, "Can't accept client\n");
		exit(1);
	}
}

void SocketOptions(int * socket)
{
    int arg = 1;
    if (setsockopt (*socket, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)) == -1)
    {
		perror("SetSockOpt Failed!");
		exit(1);
    }
}

void *serviceClient(void *clientInfo)
{
    int n = 0, bytes_to_read = 0;
    char *bp = 0;
    char buf[BUFLEN] = { 0 };
    
    int socket = *((ClientWrapper *) (clientInfo))->client_socket;
    
    struct sockaddr_in* client = malloc(sizeof(struct sockaddr_in));
    memcpy(client, ((ClientWrapper *) clientInfo)->client, sizeof(struct sockaddr_in));    
   
    printf(" Remote Address:  %s\n", inet_ntoa(client->sin_addr));
	bp = buf;
	bytes_to_read = BUFLEN;
	while ((n = recv (socket, bp, bytes_to_read, 0)) < BUFLEN)
	{
		bp += n;
		bytes_to_read -= n;
	}
	printf ("sending:%s\n", buf);

	send (socket, buf, BUFLEN, 0);
	close (socket);
	return 0;
}
