/*
	main / listener
	set up server
	bind to port
	accept connection
		add client to list
		create io worker

io worker
	receive connection
	add to select
	wait for select call
		create processing worker

processing worker
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
#include <strings.h>
#include <arpa/inet.h>

#define SERVER_PORT 7000
#define BUFLEN  80 
#define MAXLINE 4096
#define TRUE 1
typedef struct
{
    int * maxi;
    int * client;
    int * sockfd;
    fd_set * rset;
    fd_set * allset;
    int * nready;
    int i;
} SelectWrapper ;

void createSocket(int*);
void bindSocket(int*, struct sockaddr_in*, int*);
void acceptClient(int*, int*, struct sockaddr*, socklen_t*);
void SocketOptions(int*);
void *serviceClient(void*);
void *ioworker(void*);
void addDescriptor(int*, int*, fd_set *, int*, int*, int*);

int main()
{
    int listenSocket = 0;
    int nready = 0;
    int newSocket = 0;
    int port = SERVER_PORT;
    int sockfd = 0, maxfd = 0, new_sd = 0;
    int client[FD_SETSIZE] = { 0 };
    int maxi = -1;
    fd_set rset, allset;
    socklen_t clientLength = 0;
    pthread_t ioThread = 0;
        
    struct sockaddr_in server, client_addr;
    SelectWrapper arguments;
    
    createSocket(&listenSocket);
    SocketOptions(&listenSocket);
    bindSocket(&listenSocket, &server, &port);
    
    listen(listenSocket, 5);
    maxfd = listenSocket;
    
    size_t i = 0;
    for (i = 0; i < FD_SETSIZE; i++)
        client[i] = -1;
    
    FD_ZERO(&allset);
    FD_SET(listenSocket, &allset);
    
    while (TRUE)
    {
        rset = allset;
        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
    
        if (FD_ISSET(listenSocket, &rset)) /* New client connection */
        {
            acceptClient(&listenSocket, &newSocket, (struct sockaddr *)&client_addr, &clientLength);
            addDescriptor(client, &newSocket, &allset, &maxfd, &maxi, &new_sd);
        	if (--nready <= 0)
        		continue;	// no more readable descriptors
        }
        
        arguments.maxi = &maxi;
        arguments.client = client;
        arguments.sockfd = &sockfd;
        arguments.rset = &rset;
        arguments.allset = &allset;
        arguments.nready = &nready;
    
        if (pthread_create(&ioThread, NULL, &ioworker, (void *) &arguments) != 0)
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

void addDescriptor(int * client, int * newSocket, fd_set * allset, int * maxfd, int * maxi, int * new_sd)
{
    size_t i = 0;
    for (i = 0; i < FD_SETSIZE; i++)
		if (client[i] < 0)
		{
	        client[i] = *new_sd;	// save descriptor
            break;
		}
	
	if (i == FD_SETSIZE)
	{
		printf ("Too many clients\n");
		exit(1);
	}
	
    FD_SET(*newSocket, allset);
    if (*newSocket > *maxfd)
        *maxfd = *newSocket;
    if (i > *maxi)
		*maxi = i;	// new max index in client[] array
}

void *ioworker(void *selectInfo)
{
    size_t i = 0;
    SelectWrapper * info = (SelectWrapper *) selectInfo;
    pthread_t processingThread = 0;
    
    for (i = 0; i <= *(info->maxi); i++)	// check all clients for data
	{
	    if ((*(info->sockfd) = info->client[i]) < 0)
	    	continue;

	    if (FD_ISSET(*(info->sockfd), info->rset))
 		{
 		    info->i = i;
 			if (pthread_create(&processingThread, NULL, &serviceClient, (void *) info) != 0)
            {
                perror ("Can't create thread!");
                exit(1);
            }
            
            if (--(*(info->nready)) <= 0)
	    	    break;
	    }
    }
}

void *serviceClient(void *selectInfo)
{
    int n = 0, bytes_to_read = 0;
    char *bp = 0;
    char buf[BUFLEN] = { 0 };
    
    SelectWrapper * info = (SelectWrapper *) selectInfo;
   
	if (FD_ISSET(*(info->sockfd), info->rset))
	{
		bp = buf;
	    bytes_to_read = BUFLEN;
	    while ((n = read(*(info->sockfd), bp, bytes_to_read)) > 0)
	    {
		    bp += n;
		    bytes_to_read -= n;
	    }
	    
	    write(*(info->sockfd), buf, BUFLEN);
	
	    if (n == 0)
		{
//		    printf(" Remote Address:  %s closed connection\n", inet_ntoa(client_addr.sin_addr));
		    close(*(info->sockfd));
		    FD_CLR(*(info->sockfd), info->allset);
   				info->client[info->i] = -1;
		}
    }
    
	return 0;
}
