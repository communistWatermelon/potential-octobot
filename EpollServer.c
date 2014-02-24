/*---------------------------------------------------------------------------------------
--	SOURCE FILE:		epoll_svr.c -   A simple echo server using the epoll API
--
--	PROGRAM:		epolls
--				gcc -Wall -ggdb -o epolls epoll_svr.c  
--
--	FUNCTIONS:		Berkeley Socket API
--
--	DATE:			February 2, 2008
--
--	REVISIONS:		(Date and Description)
--
--	DESIGNERS:		Design based on various code snippets found on C10K links
--				Modified and improved: Aman Abdulla - February 2008
--
--	PROGRAMMERS:		Aman Abdulla
--
--	NOTES:
--	The program will accept TCP connections from client machines.
-- 	The program will read data from the client socket and simply echo it back.
--	Design is a simple, single-threaded server using non-blocking, edge-triggered
--	I/O to handle simultaneous inbound connections. 
--	Test with accompanying client application: epoll_clnt.c
---------------------------------------------------------------------------------------*/

#include <assert.h>
#include <errno.h>
#include <pthread.h>
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
#include <unistd.h>
#include <signal.h>

#define TRUE 			1
#define FALSE 			0
#define EPOLL_QUEUE_LEN	256
#define BUFLEN			80
#define SERVER_PORT		7000

typedef struct {
	int epoll_fd;
	struct epoll_event events[EPOLL_QUEUE_LEN];
} epollWrapper;

//Globals
int fd_server;
int connected;
int servicing;
int finished;
epollWrapper info;

// Function prototypes
static void SystemFatal (const char* message);
static int ClearSocket (int fd);
void close_fd (int);
void updateStats();
void handleData(int*);
void setupSignal();
void setupListenSocket(int);
int handleError(int * i);
void setupFD(struct epoll_event * event);
int handleConnection(struct epoll_event * event, int * i);
void handleData(int * i);

int main (int argc, char* argv[]) 
{
	int i; 
	int num_fds;
	static struct epoll_event event;
	int port = SERVER_PORT;
    connected = 0;
    servicing = 0;
    finished  = 0;

    setupSignal();
   
	setupListenSocket(port);

	setupFD(&event);
    
	// Execute the epoll event loop
	while (TRUE) 
	{
		//struct epoll_event events[MAX_EVENTS];
		num_fds = epoll_wait (info.epoll_fd, info.events, EPOLL_QUEUE_LEN, -1);
		if (num_fds < 0) 
			SystemFatal ("Error in epoll_wait!");

		updateStats();
		for (i = 0; i < num_fds; i++) 
		{
    		// Case 1: Error condition
			if (handleError(&i))
				continue;

    		// Case 2: Server is receiving a connection request
    		if (handleConnection(&event, &i))
    			continue;
			
			handleData(&i);
		}
	}
	close(fd_server);
	exit (EXIT_SUCCESS);
}

void setupSignal()
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

void setupListenSocket(int port)
{
	int arg;
	struct sockaddr_in addr;

	fd_server = socket (AF_INET, SOCK_STREAM, 0);
	if (fd_server == -1) 
		SystemFatal("socket");
	
	// set SO_REUSEADDR so port can be resused imemediately after exit, i.e., after CTRL-c
	arg = 1;
	if (setsockopt (fd_server, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)) == -1) 
		SystemFatal("setsockopt");
	
	// Make the server listening socket non-blocking
	if (fcntl (fd_server, F_SETFL, O_NONBLOCK | fcntl (fd_server, F_GETFL, 0)) == -1) 
		SystemFatal("fcntl");
	
	// Bind to the specified listening port
	memset (&addr, 0, sizeof (struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);
	
	if (bind (fd_server, (struct sockaddr*) &addr, sizeof(addr)) == -1) 
		SystemFatal("bind");
}

void setupFD(struct epoll_event * event)
{
	// Listen for fd_news; SOMAXCONN is 128 by default
	if (listen (fd_server, SOMAXCONN) == -1) 
		SystemFatal("listen");
	
	// Create the epoll file descriptor
	info.epoll_fd = epoll_create(EPOLL_QUEUE_LEN);
	if (info.epoll_fd == -1) 
		SystemFatal("epoll_create");
	
	// Add the server socket to the epoll event loop
	event->events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET;
	event->data.fd = fd_server;
	if (epoll_ctl (info.epoll_fd, EPOLL_CTL_ADD, fd_server, event) == -1) 
		SystemFatal("epoll_ctl");
}

int handleError(int * i)
{
	if (info.events[*i].events & (EPOLLHUP | EPOLLERR)) 
	{
		fputs("epoll: EPOLLERR", stderr);
		close(info.events[*i].data.fd);
		return 1;
	}
	assert (info.events[*i].events & EPOLLIN);
	return 0;
}

int handleConnection(struct epoll_event * event, int * i)
{
	int fd_new;
	struct sockaddr_in remote_addr;
	socklen_t addr_size = sizeof(struct sockaddr_in);

	if (info.events[*i].data.fd == fd_server) 
	{
		//socklen_t addr_size = sizeof(remote_addr);
		fd_new = accept (fd_server, (struct sockaddr*) &remote_addr, &addr_size);
		if (fd_new == -1) 
		{
			if (errno != EAGAIN && errno != EWOULDBLOCK) 
			{
				perror("accept");
			}
			return 1;
		}

		// Make the fd_new non-blocking
		if (fcntl (fd_new, F_SETFL, O_NONBLOCK | fcntl(fd_new, F_GETFL, 0)) == -1) 
			SystemFatal("fcntl");
		
		// Add the new socket descriptor to the epoll loop
		event->data.fd = fd_new;
		if (epoll_ctl (info.epoll_fd, EPOLL_CTL_ADD, fd_new, event) == -1) 
			SystemFatal ("epoll_ctl");
		
		//printf(" Remote Address:  %s\n", inet_ntoa(remote_addr.sin_addr));
		connected++;
		return 1;
	}

	return 0;
}

void handleData(int * i)
{
	if (!ClearSocket(info.events[*i].data.fd)) 
	{
		// epoll will remove the fd from its set
		// automatically when the fd is closed
		finished++;
		close (info.events[*i].data.fd);
	}
}

void updateStats()
{
	printf("\033[2J\n");
	printf("total connections %d\n", connected);
	printf("servicing %d\n", connected - finished);
	printf("finished %d\n", finished);
}

static int ClearSocket (int fd) 
{
	int	n, bytes_to_read;
	char	*bp, buf[BUFLEN];

	bp = buf;
	bytes_to_read = BUFLEN;

	do
    {
        n = 0;
        while ((n = recv (fd, bp, bytes_to_read, 0)) < BUFLEN)
    	{
            if (n <= 0)
                break;

    		bp += n;
    		bytes_to_read -= n;
    	}

    	if (n > 0)
        {
            send (fd, buf, BUFLEN, 0);
        }
    } while (n > 0);
    return FALSE;
}

// Prints the error stored in errno and aborts the program.
static void SystemFatal(const char* message) 
{
    perror (message);
    exit (EXIT_FAILURE);
}

// close fd
void close_fd (int signo)
{
    close(fd_server);
	exit (EXIT_SUCCESS);
}