#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/uio.h>
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
//TODO append .. to client servers to connect to

int prompt(void);
int prompt_receive(void);
void read_filename(char *filename, int flag);
void *distribute_threads(void *i);
void create_threads(void);
void retrieve_server(void);
void create_server(void);
void client_user(void);
void retrieve(char *filename, char *peerid);

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
	close(csfd);
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
	strcpy(sa.sun_path, "../SERV");
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
				//register check
				read_filename(filename,0);
				//Send Server command #
				send(sfd,"1",2,0);
				//Send Server filename
				send(sfd,(void *)filename,MAXLINE,0);
				send(sfd,hostname,HOSTNAMELENGTH,0);
				break;
			case 2: 
				//do a lookup
				read_filename(filename,1);
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
					int input = prompt_receive();
					//Receive
					if(input == 1)
					{
						retrieve(filename,peerid);				
					}
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
		//printf("3: Retrieve a file\n");
		printf("3: Exit\n");
		fgets(str,MAXLINE,stdin);
		//Consume end of line character
		input = atoi(str);
		if(input > 4 || input < 1 || input == 0)
		{
			valid = 0;
			printf("Invalid input, try again\n");
		}
	}while(!valid);
	return input;
}
int prompt_receive(void)
{
	int input;
	int valid = 1;
	char str[MAXLINE];

	do{
		printf("Retrieve from peer?\n");
		printf("1: Yes\n");
		printf("2: No\n");
		fgets(str,MAXLINE,stdin);
		input = atoi(str);
		if(input != 1 && input != 2)
		{
			valid = 0;
			printf("Invalid input, try again\n");
		}
	}while(!valid);
	return input;
}
//flag = 0 : Register, flag = 1 : Lookup
void read_filename(char *filename, int flag)
{
	FILE *file;
	do{
		printf("Enter the filename to lookup/register\n");
		fgets(filename,MAXLINE,stdin);
		char *buf;
		if((buf=strchr(filename,'\n')) != NULL)
			*buf = '\0';
		if(flag == 0)
		{
			file = fopen((const char *)filename,"r");
			if(file == NULL)
			{
				printf("File does not exist!\n");
			}
		}
	}while(file == NULL && flag == 0);
	//printf("\"%s\"\n",filename);
}
//Get the server name to retrieve
void read_server(char *server)
{
	printf("Enter the server\n");
}
void retrieve(char *filename, char *peerid)
{
	struct sockaddr_un sa;
	int sfd;
	sfd = socket(AF_UNIX,SOCK_STREAM,0);
	if(sfd < 0)
		perror("Socket");

	//Set socket structure vars
	int err;
	memset(&sa,0,sizeof(sa));
	sa.sun_family = AF_UNIX;
	strcpy(sa.sun_path, peerid);
	err = connect(sfd,(const struct sockaddr *)&sa,sizeof(sa));
	if(err < 0)
		perror("Connect");
	char filesize[MAXFILESIZECHARS];
	send(sfd,(void *)filename,MAXLINE,0);
	recv(sfd,(void *)filesize,MAXFILESIZECHARS,0);
	//File size and remaining bytes
	int rem = atoi(filesize);
	printf("file would be received here\n");
	//send file size
	//retrieve file
	char buf[BUFFSIZE];
	FILE *file = fopen(filename,"r+");
	int recvb = 0;
	while(((recvb = recv(sfd,buf,BUFFSIZE,0)) > 0) && (rem > 0))
	{
		fwrite(buf,sizeof(char),recvb,file);
		rem -= recvb;
	}
	fclose(file);
	unlink(sa.sun_path);
	close(sfd);
}
void create_server(void)
{
	//Create client server
	csfd = socket(AF_UNIX,SOCK_STREAM,0);
	if(csfd < 0)
	{
		perror("Socket");
		exit(0);
	}

	//Set socket structure vars
	memset(&csa,0,sizeof(csa));
	csa.sun_family = AF_UNIX;
	strcpy(csa.sun_path,hostname);
	unlink(csa.sun_path);

	int err;
	err = bind(csfd,(struct sockaddr*)&csa,sizeof(csa));	
	if(err < 0)
	{
		perror("Bind");
		exit(0);
	}
	//Socket over localhost
	//Listen for connections, maximum of 1 client (1 per thread)
	err = listen(csfd,NUMCLIENTS-1);
	if(err < 0)
	{
		perror("Listen");
		exit(0);
	}
}
void retrieve_server(void)
{
	struct sockaddr_un cca;
	int ccfd;
	//Set up server
	//Listen for connections
	//Send file
	//End connection
	socklen_t len = sizeof(cca);
	while(1)
	{
		printf("Server waiting for connection\n");
		ccfd = accept(csfd,(struct sockaddr*)&cca,&len);
		printf("Connected\n");
		char filename[MAXLINE];
		if(recv(ccfd,(void *)filename,MAXLINE,0) == 0)
		{
			close(ccfd);
			return;
		}
		printf("Sending file: %s\n",filename);

		//Send file
		printf("File would be sent here\n");
		int fd = open(filename,O_RDONLY); 
		if(fd < 0)
		{
			printf("File couldnt be opened\n");
			close(ccfd);
			return;
		}
		struct stat filestat;
		fstat(fd,&filestat);
		char filesize[MAXFILESIZECHARS];
		sprintf(filesize, "%d",(int)filestat.st_size);
		send(ccfd,(void *)filesize,MAXFILESIZECHARS,0);
		off_t len = 0;
		if(sendfile(fd,ccfd,0,&len,NULL,0) < 0)
		{
			printf("Error sending file\n");
			close(ccfd);
			return;
		}

		close(fd);
		close(ccfd);
	}
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
			retrieve_server();
			break;
	}
	return NULL;
}
