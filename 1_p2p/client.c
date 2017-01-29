#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "const.h"

int prompt(void);
void read_filename(char *filename);
void retrieve(char *filename);
void *distribute_threads(void *i);
void create_threads(void);
void retrieve_server(void);
void create_server(void);
void client_user(void);

/* NOTES
 * Threads: 1 thread for user
 * 			2 threads (one per other client)
 * 			1 thread to check files
 */

//Central server connection
//Client-Server connection
//
//Client server and client client addr
struct sockaddr_un csa;
int csfd;
char hostname[HOSTNAMELENGTH];
int main(int argc, char **argv)
{
	//Socket over IPv4
	//sockfd = socket(AF_INET,SOCK_SEQPACKET,0);	
	if(argc != 2)
	{
		printf("Usage: ./client clientid\n");
		return 1;
	}
	if(strlen(argv[1])+1 >= HOSTNAMELENGTH)
	{
		printf("Host name too large\n");
		return 1;
	}
	strcpy(hostname,argv[1]);
	printf("Hostname: %s\n",hostname);

	//Create the threads for client and client-server
	create_server();
	create_threads();
}
void client_user(void)
{
	struct sockaddr_un sa;
	int sfd;
	//Socket over localhost
	sfd = socket(AF_UNIX,SOCK_STREAM,0);
	if(sfd < 0)
		perror("Socket");

	//Set socket structure vars
	int err;
	memset(&sa,0,sizeof(sa));
	sa.sun_family = AF_UNIX;
	strcpy(sa.sun_path, "SERV");
	err = connect(sfd,(const struct sockaddr *)&sa,sizeof(sa));
	if(err < 0)
		perror("Connect");

	int cmd;
	do{
		cmd = prompt();
		char filename[MAXLINE];	
		char peerid[HOSTNAMELENGTH];
		char found[2];
		int found_int;
		switch(cmd){
			case 1: 
				read_filename(filename);
				//Send Server command #
				send(sfd,"1",2,0);
				//Send Server filename
				send(sfd,(void *)filename,MAXLINE,0);
				send(sfd,hostname,HOSTNAMELENGTH,0);
				break;
			case 2: 
				read_filename(filename);
				//Send Server command #
				send(sfd,"2",2,0);
				send(sfd,(void *)filename,MAXLINE,0);
				recv(sfd,(void *)found,2,0);
				found_int = atoi(found);
			 	if(found_int)	
				{
					//send/recv from other client
					recv(sfd,(void *)peerid,HOSTNAMELENGTH,0);
					printf("Received peerid: %s\n",peerid);
				}
				else
				{
					printf("File does not exist on server\n");
				}
				break;
			case 3: 
				break;
		}
	}while(cmd != 3);
	unlink(sa.sun_path);
	close(sfd);
}
int prompt(void)
{
	int input;
	//Scan input
	//scanf("%d",&input);
	char str[MAXLINE];
	int valid = 1;
	do{
		printf("Enter the number for the desired command\n");
		printf("1: Register a file to the server\n");
		printf("2: Look up / Retreive a file\n");
		printf("3: Exit\n");
		fgets(str,MAXLINE,stdin);
		//Consume end of line character
		input = atoi(str);
		if(input > 3 || input < 1 || input == 0)
		{
			valid = 0;
			printf("Invalid input, try again\n");
		}
	}while(!valid);
	return input;
}
void read_filename(char *filename)
{
	FILE *file;
	do{
		printf("Enter the filename to register\n");
		fgets(filename,MAXLINE,stdin);
		char *buf;
		if((buf=strchr(filename,'\n')) != NULL)
			*buf = '\0';
		file = fopen((const char *)filename,"r");
		if(file == NULL)
		{
			printf("File does not exist!\n");
		}
	}while(file == NULL);
	//printf("\"%s\"\n",filename);
}
void retrieve(char *filename)
{
	struct sockaddr_un sa,ca;
	int sfd,cfd;
	sfd = socket(AF_UNIX,SOCK_STREAM,0);
	cfd = socket(AF_UNIX,SOCK_STREAM,0);
	if(sfd < 0)
		perror("Socket");

	//Set socket structure vars
	int err;
	memset(&sa,0,sizeof(sa));
	sa.sun_family = AF_UNIX;
	strcpy(sa.sun_path, "SERV");
	err = connect(sfd,(const struct sockaddr *)&sa,sizeof(sa));
	if(err < 0)
		perror("Connect");

}
void create_server(void)
{
	//Create client server
	csfd = socket(AF_UNIX,SOCK_STREAM,0);
	if(csfd < 0)
		perror("Socket");

	//Set socket structure vars
	int err;
	memset(&csa,0,sizeof(csa));
	csa.sun_family = AF_UNIX;
	strcpy(csa.sun_path,hostname);
	unlink(csa.sun_path);

	err = bind(csfd,(struct sockaddr*)&csa,sizeof(csa));	
	if(err < 0)
		perror("Bind");
	//Socket over localhost
	//Listen for connections, maximum of 1 client (1 per thread)
	err = listen(csfd,2);
	if(err < 0)
		perror("Listen");
}
void retrieve_server(void)
{
	struct sockaddr_un cca;
	int ccfd;
	//Set up server
	//Listen for connections
	//Send file
	//End connection
	while(1)
	{
		socklen_t len = sizeof(cca);
		printf("Server waiting for connection\n");
		ccfd = accept(csfd,(struct sockaddr*)&cca,&len);
		printf("Connected\n");
		char filename[MAXLINE];
		recv(ccfd,(void *)filename,MAXLINE,0);
		printf("Sending file: %s\n",filename);

		//Send file

		unlink(csa.sun_path);
		close(ccfd);
	}
	close(csfd);
}
void create_threads(void)
{
	//Create the n threads
	pthread_t threads[NTHREADS];
	int i;
	int num[NTHREADS];
	void *p = num;
	for(i = 0; i < NTHREADS; i++)
	{
		num[i] = i;
		/* Call gauss with each thread and paremeter i which
		 * will act like a rank in MPI */
		pthread_create(&threads[i],NULL,distribute_threads,p);
		p++;
	}
	/* Terminate all threads */
	for(i = 0; i < NTHREADS; i++)
	{
		pthread_join(threads[i],NULL);
	}
}
void *distribute_threads(void *i)
{
	int num = *((int *)i);
	//int *num = (int)*i;
	switch(num)
	{
		case 0:
			//Client user
			client_user();
			break;
		case 1:
			//File checking
			break;
		default:
			//retrieve server
			//retrieve_server();
			break;
	}
	return NULL;
}
