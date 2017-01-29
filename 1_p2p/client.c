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
//TODO
//Be able to select from a list of peers to receive from
//Check if file is modified

//Can set path for where to look for files
//Remove entries from server once client is closed

//Function Prototypes
int prompt(void);
int prompt_receive(int sfd,int numpeers,char *peerid);
void read_filename(char *filename, int flag);
void *distribute_threads(void *i);
void create_threads(void);
void retrieve_server(void);
void create_server(void);
void client_user(void);
void retrieve(char *filename, char *peerid);
void add_file(char *filename);
void free_files();
void file_checker();

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
//hostname for client server
char *files[MAXUSRFILES] = {NULL};
char hostname[HOSTNAMELENGTH];
char *peerlist[NUMCLIENTS] = {NULL};
//Parameter to close file checker
int done = 0;
int main(int argc, char **argv)
{
	//Parse client server name
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

	//Create the client server, socket file descriptor csfd
	create_server();
	//Create the threads for client and client-server
	create_threads();
	//close the server
	free_files();
	unlink(csa.sun_path);
	close(csfd);
}
/* User interface function which interacts with the central
 * indexing server and retrieves files from other clients.
 */
void client_user(void)
{
	struct sockaddr_un sa;
	int sfd;
	//Socket over localhost to central indexing server
	sfd = socket(AF_UNIX,SOCK_STREAM,0);
	if(sfd < 0)
		perror("Socket");

	//Set socket structure variables
	int err;
	memset(&sa,0,sizeof(sa));
	sa.sun_family = AF_UNIX;
	strcpy(sa.sun_path, "../SERV");
	//Connect to cental indexing server
	err = connect(sfd,(const struct sockaddr *)&sa,sizeof(sa));
	if(err < 0)
		perror("Connect");

	int cmd;
	do{
		//Get command from user
		cmd = prompt();
		char filename[MAXLINE];	
		char peerid[HOSTNAMELENGTH];
		char numpeers[PEERRECVNUMCHARS];
		int numpeersint;
		char found[2];
		int found_int;
		switch(cmd){
			//Register a file to the central indexing server
			case 1: 
				//Read filename from user
				read_filename(filename,0);
				//Send Server command #
				send(sfd,"1",2,0);
				//Send Server filename and hostname
				send(sfd,(void *)filename,MAXLINE,0);
				send(sfd,hostname,HOSTNAMELENGTH,0);
				add_file(filename);
				break;
			//Do a lookup and then can retrieve
			case 2: 
				//Read filename from user
				read_filename(filename,1);
				//Send Server command #
				send(sfd,"2",2,0);
				send(sfd,(void *)filename,MAXLINE,0);
				//Read if server found the file
				recv(sfd,(void *)found,2,0);
				found_int = atoi(found);
			 	if(found_int)	
				{
					printf("File found on server\n");
					//send/recv from other client
					//recv(sfd,(void *)peerid,HOSTNAMELENGTH,0);
					//Receive number of peer file holders
					recv(sfd,(void *)numpeers,PEERRECVNUMCHARS,0);
					//Convert it to an integer
					numpeersint = atoi(numpeers);
					//printf("Received peerid: %s\n",peerid);
					int input = prompt_receive(sfd,numpeersint,peerid);
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
			//Close the client, unregister all files
			case 3:
				send(sfd,"3",2,0);	
				send(sfd,(void *)hostname,HOSTNAMELENGTH,0);
				break;
		}
	}while(cmd != 3);
	done = 1;
	unlink(sa.sun_path);
	close(sfd);
	//Close client server as well
	unlink(csa.sun_path);
	close(csfd);
}
/* Prompt the user for the command they would like to do,
 * these include register, lookup/receive, and exit
 */
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
		//Read input
		fgets(str,MAXLINE,stdin);
		//Parse input to integer
		input = atoi(str);
		//Ensure input number is valid
		if(input > 3 || input < 1 || input == 0)
		{
			valid = 0;
			printf("Invalid input, try again\n");
		}
	}while(!valid);
	return input;
}
/* Prompt the user as to whether they want to receive, 
 * then which user they want to receive from
 */
int prompt_receive(int sfd, int numpeers,char *peerid)
{
	int valid = 1;
	char peers[NUMCLIENTS][HOSTNAMELENGTH];
	int input;

	do{
		valid = 1;
		char str[MAXLINE];
		printf("Retrieve from a peer?\n");
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
	//If want to retrieve
	int peernum;
	if(input == 1)
	{
		//Choose which peer
		int i;
		printf("Select which peer to retrieve file from\n");
		for(i = 0; i < numpeers; i++)
		{
			recv(sfd,(void *)&peers[i][0],HOSTNAMELENGTH,0);	
			printf("%d: %s\n",i,peers[i]);
		}
		do{
			valid = 1;
			char str[MAXLINE];
			fgets(str,MAXLINE,stdin);
			peernum = atoi(str);
			printf("Numpeers: %d, Peernum chosen: %d\n",numpeers,peernum);
			if(peernum < 0 || peernum > numpeers)
			{
				valid = 0;
				printf("Invalid input, try again\n");
			}
			if(peers[peernum] == hostname)
			{
				valid = 0;
				printf("Cannot select self\n");
			}
		}while(!valid);
		strcpy(peerid,peers[peernum]);
		printf("Selected peerid: %s\n",peerid);
	}
	return input;
}
//flag = 0 : Register, flag = 1 : Lookup
/* Reads a filename from the user, can be called
 * with two flags as to whether the read
 * will correspond to registering a file or
 * a lookup/receive.
 */
void read_filename(char *filename, int flag)
{
	FILE *file;
	do{
		printf("Enter the filename to lookup/register\n");
		//Read input
		fgets(filename,MAXLINE,stdin);
		char *buf;
		if((buf=strchr(filename,'\n')) != NULL)
			*buf = '\0';
		//If registering
		if(flag == 0)
		{
			//Check to make sure we can open the file
			file = fopen((const char *)filename,"r");
			if(file == NULL)
			{
				printf("File does not exist!\n");
			}
			else
			{
				fclose(file);
			}
			//Close the file
		}
	}while(file == NULL && flag == 0);
	//Close the file
	//printf("\"%s\"\n",filename);
}
/* Checks if any registered files from client were moved
 * or can no longer be opened. If so remove from central
 * indexing server
 */
void file_checker()
{
	struct sockaddr_un sa;
	int sfd;
	printf("File checker running\n");
	//Socket over localhost to central indexing server
	sfd = socket(AF_UNIX,SOCK_STREAM,0);
	if(sfd < 0)
		perror("Socket");

	//Set socket structure variables
	int err;
	memset(&sa,0,sizeof(sa));
	sa.sun_family = AF_UNIX;
	strcpy(sa.sun_path, "../SERV");
	//Connect to cental indexing server
	err = connect(sfd,(const struct sockaddr *)&sa,sizeof(sa));
	if(err < 0)
		perror("Connect");

	int i;
	while(!done)
	{
		FILE *file;
		for(i = 0; i < MAXUSRFILES; i++)
		{
			if(files[i] != NULL)	
			{
				file = fopen((const char *)files[i],"r");
				if(file == NULL)
				{
					printf("File %s moved or deleted: Updating server\n",files[i]);
					//Tell server to remove from list, if available
					if(send(sfd,"4",2,0) < 0)
					{
						close(sfd);
						return;
					}
					//Send file name and host name
					send(sfd,(void *)files[i],MAXLINE,0);
					send(sfd,(void *)hostname,HOSTNAMELENGTH,0);
					free(files[i]);
					files[i] = NULL;
				}
				else
					fclose(file);
			}
		}
	}
}
/* Set up a server to retrieve from other client
 * and receive the file filename from peerid
 */
void retrieve(char *filename, char *peerid)
{
	if(strcmp(peerid,hostname) == 0)
	{
		printf("Cannot retrieve from self\n");
		return;
	}
	struct sockaddr_un sa;
	int sfd;
	//Create socket for client-client transaction
	sfd = socket(AF_UNIX,SOCK_STREAM,0);
	if(sfd < 0)
		perror("Socket");

	int err;
	//Set socket structure variables
	memset(&sa,0,sizeof(sa));
	sa.sun_family = AF_UNIX;
	strcpy(sa.sun_path, peerid);
	//Connect to the other client
	err = connect(sfd,(const struct sockaddr *)&sa,sizeof(sa));
	if(err < 0)
		perror("Connect");
	char filesize[MAXFILESIZECHARS];
	//Send client the file name wanted
	send(sfd,(void *)filename,MAXLINE,0);
	//Receive the filesize
	recv(sfd,(void *)filesize,MAXFILESIZECHARS,0);
	//File size and remaining bytes
	int rem = atoi(filesize);
	char buf[BUFFSIZE];
	FILE *file = fopen(filename,"w");
	int recvb = 0;
	//Write to a new file
	while(((recvb = recv(sfd,buf,BUFFSIZE,0)) > 0) && (rem > 0))
	{
		fwrite(buf,sizeof(char),recvb,file);
		rem -= recvb;
	}
	printf("File received\n");
	fclose(file);
	//unlink(sa.sun_path);
	close(sfd);
}
/* Set up client server for sending files 
 * socketfd = csfd 
 * socket structure = csa 
 */
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
/* Run the client file server to send files
 * to other clients 
 */
void retrieve_server(void)
{
	struct sockaddr_un cca;
	int ccfd;
	//Set up server
	//Listen for connections
	//Send file
	//End connection
	socklen_t len = sizeof(cca);
	//Loop
	while(1)
	{
		printf("Server waiting for connection\n");
		ccfd = accept(csfd,(struct sockaddr*)&cca,&len);
		if(ccfd < 0)
		{
			close(ccfd);
			return;
		}
		printf("Connected\n");
		char filename[MAXLINE];
		//Receive the filename, if no clients to accept, return
		if(recv(ccfd,(void *)filename,MAXLINE,0) == 0)
		{
			close(ccfd);
			return;
		}
		printf("Sending file: %s\n",filename);

		//Send file
		printf("File would be sent here\n");
		//Open the file
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
		//Convert fstat filesize to string
		sprintf(filesize, "%d",(int)filestat.st_size);
		//Send the file size
		send(ccfd,(void *)filesize,MAXFILESIZECHARS,0);
		off_t len = 0;
		//Send the entire file
		if(sendfile(fd,ccfd,0,&len,NULL,0) < 0)
		{
			printf("Error sending file\n");
			close(ccfd);
			return;
		}
		printf("File sent\n");
		//close file 
		close(fd);
		//close client connection
		close(ccfd);
	}
}
/* Create the threads which will execute client user and
 * client server side as well as file checking
 */
void create_threads(void)
{
	//Create the n threads
	pthread_t threads[NTHREADS];
	int i;
	int num[NTHREADS] = {0};
	int *p = num;
	for(i = 0; i < NTHREADS; i++)
	{
		num[i] = i;
		//Create threads, and send their index in num using p
		pthread_create(&threads[i],NULL,distribute_threads,p);
		p++;
	}
	/* Terminate all threads */
	for(i = 0; i < NTHREADS; i++)
	{
		pthread_join(threads[i],NULL);
	}
}
/* Distributes the threads to their designated function,
 * which includes client user, file checking, and the client
 * server
 */
void *distribute_threads(void *i)
{
	int num = *((int *)i);
	//printf("Thread #: %d\n",num);
	switch(num)
	{
		case 0:
			//Client user
			client_user();
			break;
		case 1:
			//File checking
			file_checker();
			break;
		default:
			//retrieve server
			retrieve_server();
			break;
	}
	return NULL;
}
void add_file(char *filename)
{
	int i;
	for(i = 0; i < MAXUSRFILES;i++)
	{
		if(files[i] == NULL)
		{
			files[i] = malloc(MAXLINE);
			strcpy(files[i],filename);
			break;
		}
	}
}
void free_files()
{
	int i;
	for(i = 0; i < MAXUSRFILES;i++)
		if(files[i] != NULL)
			free(files[i]);
}
