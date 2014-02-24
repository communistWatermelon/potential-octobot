/*---------------------------------------------------------------------------------------
--  SOURCE FILE:    BasicServer.c - A robust tcp load testing client.
--
--  PROGRAM:        BasicServer
--
--  FUNCTIONS:      void createSocket(int*)
--                  void bindSocket(int*, struct sockaddr_in*, int*)
--                  void acceptClient(int*, int*, struct sockaddr*, socklen_t*)
--                  void SocketOptions(int *)
--                  void *serviceClient(void*)
--
--  DATE:           February 14, 2014
--
--  DESIGNERS:      Jacob Miner
--
--  PROGRAMMERS:    Jacob Miner
--
--  NOTES:
--  A TCP echo server using multithreading.
--  
---------------------------------------------------------------------------------------*/
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
void printstats();

int connected;
int connections;

/*------------------------------------------------------------------------------
--
--  FUNCTION:   main
--
--  DATE:       February 14, 2014
--
--  DESIGNERS:  Jacob Miner  
--
--  PROGRAMMER: Jacob Miner 
--
--  INTERFACE: main()
--
--  RETURNS:  int - 0 on success
--
--  NOTES: The main thread of the program. Calls all other functions.
--  
------------------------------------------------------------------------------*/
int main()
{
    int listenSocket = 0;
    int newSocket = 0;
    int port = SERVER_PORT;
    socklen_t clientLength = 0;
    pthread_t clientThread;
    pthread_attr_t tattr;

    struct sockaddr_in server, client_addr;
    ClientWrapper arguments;

    connected = 0;
    connections = 0;
    createSocket(&listenSocket);
    SocketOptions(&listenSocket);
    bindSocket(&listenSocket, &server, &port);
    
    listen(listenSocket, 5);
    
    while (TRUE)
    {
        printstats();
        acceptClient(&listenSocket, &newSocket, (struct sockaddr *)&client_addr, &clientLength);
        arguments.client_socket = &newSocket;
        arguments.client = &client_addr;
        
		if (pthread_attr_init(&tattr) != 0)
        {	
        	perror ("Can't init thread attributes!");
            exit(1);	
        }

        if (pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED) != 0)
        {
        	perror ("Can't detach thread!");
            exit(1);	
        }

        if (pthread_create(&clientThread, &tattr, &serviceClient, (void *) &arguments) != 0)
        {
            perror ("Can't create thread!");
            exit(1);
        }
    }
    
    close(listenSocket);
    return 0;
}

/*------------------------------------------------------------------------------
--
--  FUNCTION:   printstats
--
--  DATE:       February 14, 2014
--
--  DESIGNERS:  Jacob Miner  
--
--  PROGRAMMER: Jacob Miner 
--
--  INTERFACE: printstats()
--
--  RETURNS:  void
--
--  NOTES: Prints the current stats of the connections 
--  
------------------------------------------------------------------------------*/
void printstats()
{
    printf("Servicing: %d\n", connected);
    printf("Total Connections: %d\n", connections);
}

/*------------------------------------------------------------------------------
--
--  FUNCTION:   createSocket
--
--  DATE:       February 14, 2014
--
--  DESIGNERS:  Jacob Miner  
--
--  PROGRAMMER: Jacob Miner 
--
--  INTERFACE: createSocket(int * listen_socket)
--                  listen_socket - the socket to listen on
--
--  RETURNS:  void
--
--  NOTES: Wrapper for socket(), that handles errors
--  
------------------------------------------------------------------------------*/
void createSocket(int * listen_socket)
{
    if ((*listen_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror ("Can't create a socket");
		exit(1);
	}
}

/*------------------------------------------------------------------------------
--
--  FUNCTION:   bindSocket
--
--  DATE:       February 14, 2014
--
--  DESIGNERS:  Jacob Miner  
--
--  PROGRAMMER: Jacob Miner 
--
--  INTERFACE: bindSocket(int * socket, struct sockaddr_in * server, int * port)
--                      socket - the socket to bind
--                      server - the server information
--                      port - the port to bind
--
--  RETURNS:  void
--
--  NOTES: Wrapper for bind that handles errors
--  
------------------------------------------------------------------------------*/
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

/*------------------------------------------------------------------------------
--
--  FUNCTION:   acceptClient
--
--  DATE:       February 14, 2014
--
--  DESIGNERS:  Jacob Miner  
--
--  PROGRAMMER: Jacob Miner 
--
--  INTERFACE: acceptClient(int * server_socket, int * new_socket, struct sockaddr * client, socklen_t * client_len)
--                      server_socket - the servers listening socket
--                      new_socket - the new socket for a client
--                      client - client sockaddr structure
--                      client_len - client length
--
--  RETURNS:  void
--
--  NOTES: Weapper for accept() that handles errors
--  
------------------------------------------------------------------------------*/
void acceptClient(int * server_socket, int * new_socket, struct sockaddr * client, socklen_t * client_len)
{
	*client_len= sizeof(client);
	if ((*new_socket = accept (*server_socket, (struct sockaddr *)client, client_len)) == -1)
	{
		fprintf(stderr, "Can't accept client\n");
		exit(1);
	}
}

/*------------------------------------------------------------------------------
--
--  FUNCTION:   SocketOptions
--
--  DATE:       February 14, 2014
--
--  DESIGNERS:  Jacob Miner  
--
--  PROGRAMMER: Jacob Miner 
--
--  INTERFACE: SocketOptions(int * socket)
--                      socket - the socket to set the options on
--
--  RETURNS:  void
--
--  NOTES: Sets relavent socket options, like reuse addr
--  
------------------------------------------------------------------------------*/
void SocketOptions(int * socket)
{
    int arg = 1;
    if (setsockopt (*socket, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)) == -1)
    {
		perror("SetSockOpt Failed!");
		exit(1);
    }
}

/*------------------------------------------------------------------------------
--
--  FUNCTION:   serviceClient
--
--  DATE:       February 14, 2014
--
--  DESIGNERS:  Jacob Miner  
--
--  PROGRAMMER: Jacob Miner 
--
--  INTERFACE: serviceClient(void *clientInfo)
--                      clientInfo - all the information needed to echo back to the client
--
--  RETURNS:  void
--
--  NOTES: The thread created after a client connects to service them.
--  
------------------------------------------------------------------------------*/
void *serviceClient(void *clientInfo)
{
    int n = 0, bytes_to_read = 0;
    char *bp = 0;
    char buf[BUFLEN] = { 0 };
    
    int socket = *((ClientWrapper *) (clientInfo))->client_socket;
    
    struct sockaddr_in* client = malloc(sizeof(struct sockaddr_in));
    memcpy(client, ((ClientWrapper *) clientInfo)->client, sizeof(struct sockaddr_in));    
   
    //printf(" Remote Address:  %s\n", inet_ntoa(client->sin_addr));
    connected++;
    connections++;
	bp = buf;
	bytes_to_read = BUFLEN;
	do
    {
        n = 0;

        while ((n = recv (socket, bp, bytes_to_read, 0)) < BUFLEN)
    	{
            if (n <= 0)
                break;
    		bp += n;
    		bytes_to_read -= n;
    	}

    	if (n > 0)
        {
            //printf ("sending message of size: %d\n", sizeof(buf));
            send (socket, buf, BUFLEN, 0);
        }
    } while (n > 0);
	
    free(client);
	close (socket);
    connected--;
	return NULL;
}
