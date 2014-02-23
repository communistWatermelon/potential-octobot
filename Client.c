/*---------------------------------------------------------------------------------------
--	SOURCE FILE:		tcp_clnt.c - A simple TCP client program.
--
--	PROGRAM:		tclnt.exe
--
--	FUNCTIONS:		Berkeley Socket API
--
--	DATE:			January 23, 2001
--
--	REVISIONS:		(Date and Description)
--				January 2005
--				Modified the read loop to use fgets.
--				While loop is based on the buffer length 
--
--
--	DESIGNERS:		Aman Abdulla
--
--	PROGRAMMERS:		Aman Abdulla
--
--	NOTES:
--	The program will establish a TCP connection to a user specifed server.
-- 	The server can be specified using a fully qualified domain name or and
--	IP address. After the connection has been established the user will be
-- 	prompted for date. The date string is then sent to the server and the
-- 	response (echo) back from the server is displayed.
---------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_TCP_PORT		7000	// Default port
#define BUFLEN				80  	// Buffer length

char * message;
int count;
int loop;
int threadsCreated;
int threadsServicing;
int threadsFinished;
int messageSize;
int clients;

void createSocket(int*);
void* sendData(void*);
void createMessage(int);
void initSocket(struct sockaddr_in*, int*, struct hostent*, char*);
void connectSocket(int*, struct sockaddr_in*, struct hostent*);
void updateStats();

int main (int argc, char **argv)
{
	messageSize = BUFLEN;
	clients = 10000;
    loop = 1;
	int sd[clients], port;
	struct hostent *hp;
	struct sockaddr_in server;
	char  *host;
    pthread_t clientThread[clients];
    size_t i  = 0;

    threadsCreated = 0;
	threadsServicing = 0;
	threadsFinished = 0;

	switch(argc)
	{
		case 2:
			host =	argv[1];	// Host name
			port =	SERVER_TCP_PORT;
		break;
		case 3:
			host =	argv[1];
			port =	atoi(argv[2]);	// User specified port
		break;
		case 4: 
			host =	argv[1];
			port =	atoi(argv[2]);	// User specified port
			messageSize = atoi(argv[3]); // message size
		break;
		case 5: 
			host =	argv[1];
			port =	atoi(argv[2]);	// User specified port
			messageSize = atoi(argv[3]);
			clients = atoi(argv[4]); // threads
		break;
		case 6: 
			host =	argv[1];
			port =	atoi(argv[2]);	// User specified port
			messageSize = atoi(argv[3]);
			clients = atoi(argv[4]); // threads
			loop = atoi(argv[5]);
		break;
		default:
			fprintf(stderr, "Usage: %s [host] [port] [messageSize] [threads] [loops]\n", argv[0]);
			exit(1);
	}

	createMessage(messageSize);

	for (i = 0; i < clients; i++)
	{
		createSocket(&sd[i]);
		initSocket(&server, &port, hp, host);
		connectSocket(&sd[i], &server, hp);

		if (pthread_create(&clientThread[i], NULL, &sendData, (void *) &sd[i]) != 0)
	    {	
	        perror ("Can't create thread!");
	        exit(1);
	    }
	    threadsCreated++;
    }


    while((threadsCreated - threadsFinished) > 0) 
    {
    	sleep(1);
		updateStats();
    }
	getchar();

	printf("Finished!\n");
	free(message);
	
	return (0);
}

void createMessage(int messageSize)
{
	char alphabet = 0;
	size_t i = 0;

	message = calloc(messageSize, sizeof(char));

	printf("Creating message of size %d\n", messageSize);
	for (i = 0; i < (messageSize - 1); i++)
	{
		alphabet = (i % 26) + 97;
		strncat(message, &alphabet, 1);
	}
	alphabet = '\n';
	strncat(message, &alphabet, 1);
	printf("...Complete!\n");
}

void createSocket(int * sd)
{
	if ((*sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Cannot create socket");
		exit(1);
	}
}

void initSocket(struct sockaddr_in *server, int * port, struct hostent *hp, char* host)
{
	bzero((char *)server, sizeof(struct sockaddr_in));
	server->sin_family = AF_INET;
	server->sin_port = htons(*port);
	if ((hp = gethostbyname(host)) == NULL)
	{
		fprintf(stderr, "Unknown server address\n");
		exit(1);
	}
	bcopy(hp->h_addr, (char *)&server->sin_addr, hp->h_length);

}

void connectSocket(int *sd, struct sockaddr_in *server, struct hostent *hp)
{
	if (connect (*sd, (struct sockaddr *)server, sizeof(*server)) == -1)
	{
		fprintf(stderr, "Can't connect to server\n");
		perror("connect");
		exit(1);
	}
}

void* sendData(void * args)
{
	int n = 0, bytes_to_read = BUFLEN;
	char *bp, rbuf[BUFLEN];
	int sd = *((int *) args);
	size_t i =0;

	for (i = 0; i < loop; i++)
	{
		send (sd, message, BUFLEN, 0);
		bp = rbuf;

		while ((n = recv (sd, bp, bytes_to_read, 0)) < BUFLEN)
		{
			bp += n;
			bytes_to_read -= n;
		}

		n = 0;
		bytes_to_read = BUFLEN;
	}

	threadsFinished++;
	
	//close(sd);
	return NULL;
}

void updateStats()
{
	printf("\033[2J\n");
	printf("Message size: %d\n", messageSize);
	printf("Threads Created: %d\n", threadsCreated);
	printf("Threads Finished: %d\n", threadsFinished);
	fflush(stdout);
}