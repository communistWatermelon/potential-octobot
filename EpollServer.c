/*
	main / listener
	set up server
	bind to port
	accept connection
		add client to list
		create io worker

io worker
	receive connection
	add to epoll
	epoll_wait call
		create processing worker

processing worker
	if client disconnects
		remove client from list
	wait for input 
	echo input to client
	write send / recv results
*/
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <strings.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#define SERVER_PORT 7000
#define BUFLEN  80 
#define EPOLL_QUEUE_LEN	256
#define TRUE  1
#define FALSE 0

int acceptClient(int*, struct epoll_event*);
void createSocket(int*);
void bindSocket(int*, struct sockaddr_in*, int*);
void SocketOptions(int*);
void *serviceClient(void*);
void ioworker(int *, int, int, int, fd_set, fd_set);
void initEpoll(int*, struct epoll_event*);
void close_fd(int);
void signalHandle();
static void SystemFatal(const char* message);
static int ClearSocket(int fd);

int listenSocket;

int main()
{
    int port = SERVER_PORT;
    size_t i = 0;
	int num_fds, epoll_fd;
	static struct epoll_event events[EPOLL_QUEUE_LEN], event;
    struct sockaddr_in server;

    signalHandle();
    createSocket(&listenSocket);
    SocketOptions(&listenSocket);
    bindSocket(&listenSocket, &server, &port);    
    initEpoll(&epoll_fd, &event);
    
    while (TRUE)
    {
        num_fds = epoll_wait (epoll_fd, events, EPOLL_QUEUE_LEN, -1);
		if (num_fds < 0) 
			SystemFatal ("Error in epoll_wait!");
        
        for (i = 0; i < num_fds; i++) 
		{
    		/* Case 1: Error condition */
    		if (events[i].events & (EPOLLHUP | EPOLLERR)) 
			{
				fputs("epoll: EPOLLERR", stderr);
				close(events[i].data.fd);
				continue;
    		}
    		assert (events[i].events & EPOLLIN);

    		/* Case 2: Server is receiving a connection request */
    		if (events[i].data.fd == listenSocket) 
			{
				if (acceptClient(&epoll_fd, &event))
				    continue;
    		}

    		/* Case 3: One of the sockets has read data */
    		if (!ClearSocket(events[i].data.fd)) 
			{
				/* epoll will remove the fd from its set automatically when the fd is closed */
				close (events[i].data.fd);
    		}
	    }
	}   
    close(listenSocket);
    return 0;
}

void *ioworker(void *arg)
{
    return NULL;
}

void *serviceClient(void *arg
{   
    return NULL;
}

static int ClearSocket (int fd) 
{
	int	n, bytes_to_read;
	char *bp, buf[BUFLEN];

	while (TRUE)
	{	
		bp = buf;
		bytes_to_read = BUFLEN;
		while ((n = recv (fd, bp, bytes_to_read, 0)) < BUFLEN)
		{
			bp += n;
			bytes_to_read -= n;
		}
		printf ("sending:%s\n", buf);

		send (fd, buf, BUFLEN, 0);
		close (fd);
		return TRUE;
	}
	close(fd);
	return(0);

}

static void SystemFatal(const char* message) 
{
    perror (message);
    exit (EXIT_FAILURE);
}

void close_fd (int signo)
{
    close(listenSocket);
	exit (EXIT_SUCCESS);
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

int acceptClient(int * epoll_fd, struct epoll_event * event)
{
    int fd_new;
	struct sockaddr_in remote_addr;
	socklen_t addr_size = sizeof(struct sockaddr_in);
    
    fd_new = accept (listenSocket, (struct sockaddr*) &remote_addr, &addr_size);
	if (fd_new == -1) 
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK) 
		{
			perror("accept");
		}
		return 1;
	}

	/* Make the fd_new non-blocking */
	if (fcntl (fd_new, F_SETFL, O_NONBLOCK | fcntl(fd_new, F_GETFL, 0)) == -1) 
		SystemFatal("fcntl");
	
	/* Add the new socket descriptor to the epoll loop */
	event->data.fd = fd_new;
	if (epoll_ctl (*epoll_fd, EPOLL_CTL_ADD, fd_new, event) == -1) 
		SystemFatal ("epoll_ctl");
	
	printf(" Remote Address:  %s\n", inet_ntoa(remote_addr.sin_addr));
	return 1;
}

void SocketOptions(int * socket)
{
    int arg = 1;
    if (setsockopt (*socket, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)) == -1)
    {
		perror("SetSockOpt Failed!");
		exit(1);
    }
    
    if (fcntl (*socket, F_SETFL, O_NONBLOCK | fcntl (*socket, F_GETFL, 0)) == -1) 
		SystemFatal("fcntl");
}

void initEpoll(int * epoll_fd, struct epoll_event* event)
{
    if (listen (listenSocket, SOMAXCONN) == -1) 
		SystemFatal("listen");
    
    *epoll_fd = epoll_create(EPOLL_QUEUE_LEN);
	if (*epoll_fd == -1) 
		SystemFatal("epoll_create");
		
	event->events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET;
	event->data.fd = listenSocket;
	if (epoll_ctl (*epoll_fd, EPOLL_CTL_ADD, listenSocket, event) == -1) 
		SystemFatal("epoll_ctl");
}


void signalHandle()
{
	struct sigaction act;
    act.sa_handler = close_fd;
    act.sa_flags = 0;
    if ((sigemptyset (&act.sa_mask) == -1 || sigaction (SIGINT, &act, NULL) == -1))
    {
        perror ("Failed to set SIGINT handler");
        exit (EXIT_FAILURE);
    }
}

