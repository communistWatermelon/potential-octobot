/*
	main / listener
	set up server
	bind to port
	wait for select call
    	accept connection
	    	add client to list
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
#include <string.h>
#include <arpa/inet.h>
#include <signal.h>

#define SERVER_PORT 7000
#define BUFLEN  80 
#define MAXLINE 4096
#define TRUE 1
#define THING 5000


int client[FD_SETSIZE];
int sockfd;
fd_set allset;
size_t i;

typedef struct
{
    int * client;
    int sockfd;
    fd_set allset;
    int i;
} SelectWrapper ;

void createSocket(int*);
void bindSocket(int*, struct sockaddr_in*, int*);
void acceptClient(int*, int*, struct sockaddr*, socklen_t*);
void SocketOptions(int*);
void *serviceClient(void*);
void ioworker(int *, int, int, int, fd_set, fd_set);
void addDescriptor(int*, int*, int*, int*, fd_set*);
static void SystemFatal(const char* message);
    
int main()
{
    //int client[FD_SETSIZE];
    int nready, /*sockfd = 0,*/ maxfd;
    int maxi = -1;
    int listenSocket, newSocket = 0;
    int port = SERVER_PORT;
    fd_set rset;
    //fd_set allset;
    i = 0;
    socklen_t clientLength = 0;    
    struct sockaddr_in server, client_addr;
    
    createSocket(&listenSocket);
    SocketOptions(&listenSocket);
    bindSocket(&listenSocket, &server, &port);
    
    listen(listenSocket, 5);
    maxfd = listenSocket;
    maxi = -1;
    
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
            //acceptClient(&listenSocket, &newSocket, (struct sockaddr *)&client_addr, &clientLength);
            //addDescriptor(client, &newSocket, &maxfd, &maxi, &allset);
            
//        	if (--nready <= 0)
  //      		continue;	/* no more readable descriptors */
            clientLength = sizeof(client_addr);
            if ((newSocket = accept(listenSocket, (struct sockaddr *) &client_addr, &clientLength)) == -1)
                SystemFatal("accept error");
            
            printf(" Remote Address:  %s\n", inet_ntoa(client_addr.sin_addr));

            for (i = 0; i < FD_SETSIZE; i++)
                if (client[i] < 0)
                {
                    client[i] = newSocket; // save descriptor
                    break;
                }
    
            if (i == FD_SETSIZE)
            {
                printf ("Too many clients\n");
                exit(1);
            }

            FD_SET (newSocket, &allset);     // add new descriptor to set
            if (newSocket > maxfd)
                maxfd = newSocket; // for select

            if (i > maxi)
                maxi = i;   // new max index in client[] array

            if (--nready <= 0)
                continue;   // no more readable descriptors
        }
        
        ioworker(client, sockfd, nready, maxi, rset, allset);
    }
    
    //free(info);
    close(listenSocket);
    return 0;
}

static void SystemFatal(const char* message)
{
    perror (message);
    exit (EXIT_FAILURE);
}

void ioworker(int *client, int sockfd, int nready, int maxi, fd_set rset, fd_set allset)
{
    size_t i = 0;
    pthread_t processingThread;
    pthread_attr_t tattr;
    SelectWrapper * info;
    
    for (i = 0; i <= maxi; i++)	/* check all clients for data */
	{
	    if ((sockfd = client[i]) < 0)
	    	continue;

	    if (FD_ISSET(sockfd, &rset))
 		{
 		
 		    info = malloc(sizeof(SelectWrapper));
 		    info->client = client;
 		    info->sockfd = sockfd;
 		    info->i      = i;
 		    info->allset = allset;
 		
    		/*if (pthread_attr_init(&tattr) != 0)
            {   
                perror ("Can't init thread attributes!");
                exit(1);    
            }

            if (pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED) != 0)
            {
                perror ("Can't detach thread!");
                exit(1);    
            }

            if (pthread_create(&processingThread, &tattr, &serviceClient, (void *) info) != 0)
            {
                perror ("Can't create thread!");
                exit(1);
            }*/

            serviceClient((void*) info);
                
            if (--nready <= 0)
    	    	    break;
	    }
    }
}

void *serviceClient(void *selectInfo)
{
    int n = 0, bytes_to_read = 0;
    char *bp = 0;
    char buf[BUFLEN] = { 0 };
  
    SelectWrapper * info = (SelectWrapper *)selectInfo;
   
	bp = buf;
    bytes_to_read = BUFLEN;
    
    while ((n = read(info->sockfd, bp, bytes_to_read)) > 0)
    {
	    bp += n;
	    bytes_to_read -= n;
    }
    
    write(info->sockfd, buf, BUFLEN);
    
    if (n == 0)
    {
        close(info->sockfd);
        FD_CLR(info->sockfd, &info->allset);
        info->client[info->i] = -1;
        free(info);
    }
    
    return NULL;
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
	printf(" Remote Address:  %s\n", inet_ntoa(((struct sockaddr_in *)client)->sin_addr));
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

void addDescriptor(int * client, int * newSocket, int * maxfd, int * maxi, fd_set * allset)
{
    size_t i = 0;
    
    for (i = 0; i < FD_SETSIZE; i++)
    {
		if (client[i] < 0)
		{
	        client[i] = *newSocket;	/* save descriptor */
            break;
		}
	}
	
	if (i == FD_SETSIZE)
	{
		printf ("Too many clients\n");
		exit(1);
	}
	
    FD_SET(*newSocket, allset);
    if (*newSocket > *maxfd)
    {
        *maxfd = *newSocket;
    }
       
    if (i > *maxi)
    {
		*maxi = i;	/* new max index in client[] array */
	}
}
