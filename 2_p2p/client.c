#include <inttypes.h>
#include <arpa/inet.h>
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
#include <sys/times.h>
#include <sys/time.h>
#include <time.h>
#include "const.h"


//Function prototypes
void add_file(char *filename);
void free_files();
void print_clients();
int read_setup(char *filename);
void read_filename(char *filename, int flag);
void create_server(void);
void process_request(void);
void *distribute_threads(void *i);
int prompt(void);
void create_threads(void);

char *files[MAXUSRFILES] = {NULL};
//Client port list
in_port_t clist[MAXCLIENTS] = {0};
in_port_t cs_port;
char hostport[MAXPORTCHARS+1];

//Server socket info
struct sockaddr_in csa;
int csfd;
int main(int argc, char **argv)
{
	//Parse client server port number
	if(argc != 3)
	{
		printf("Usage: ./client <portno> <setupfile-path>\n");
		return 1;
	}
	int port = atoi(argv[1]);
	if(port < 1 || port > MAXPORT || strlen(argv[1]) > MAXPORTCHARS)
	{
		printf("Port number invalid: Range is between 1-65535 \n");
		return 1;
	}
	cs_port = (in_port_t)atoi(argv[1]);
	//printf("Read host port %d\n",cs_port);
	//copy
	strcpy(hostport,argv[1]);
	if(!read_setup(argv[2]))
		return 1;
	print_clients();
	create_server();
	create_threads();

	close(csfd);
	free_files();
	return 0;
}
void process_request()
{
	struct sockaddr_in ca;
	int cfd;
	socklen_t len = sizeof(ca);
	printf("Server waiting for connection\n");
	cfd = accept(csfd,(struct sockaddr*)&ca,&len);
	printf("Connected\n");

	char cmdstr[2];
	char filename[MAXLINE];
	char peerid[HOSTNAMELENGTH];
	//Receive command #
	//1 = Register, 2 = lookup
	printf("Waiting next command\n");

	//Receive command from client to perform
	if(recv(cfd,(void *)cmdstr,2,0) == 0)
		return;
	printf("Received\n");

	//convert to integer
	int cmd = atoi(cmdstr);
	printf("Received Command %d\n",cmd);
	//Close the connection
	close(cfd);
	printf("Done\n");
}
/* User interface function which interacts with the central
 * indexing server and retrieves files from other clients.
 */
void client_user(void)
{
	struct sockaddr_in sa;
	int sfd;
	//Socket over localhost to central indexing server
	sfd = socket(AF_INET,SOCK_STREAM,0);
	if(sfd < 0)
		perror("Socket");

	//Set socket structure variables
	int err;
	//memset(&sa,0,sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(clist[0]);
	//Bind to 127.0.0.1 (Local)
	sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	//Connect to cental indexing server
	err = connect(sfd,(const struct sockaddr *)&sa,sizeof(sa));
	if(err < 0)
		perror("Connect");

	#ifdef TESTING
	printf("Testing 1000 sequential requests\n");

	/* Timing code privided by Dr.Sun in CS546*/

	/* Timing variables */
	struct timeval etstart, etstop;  /* Elapsed times using gettimeofday() */
	struct timezone tzdummy;
	clock_t etstart2, etstop2;	/* Elapsed times using times() */
	unsigned long long usecstart, usecstop;
	struct tms cputstart, cputstop;  /* CPU times for my processes */
	//Start clock
	gettimeofday(&etstart, &tzdummy);
	etstart2 = times(&cputstart);
	int i;
	for(i = 0; i < 1000;i++)
	{
		//Do a lookup request
		//Send Server command #
		char found[2];
		int found_int;
		char *filename = "testing.txt";
		send(sfd,"2",2,0);
		send(sfd,(void *)filename,MAXLINE,0);
		//Read if server found the file
		recv(sfd,(void *)found,2,0);
		found_int = atoi(found);
	}
	gettimeofday(&etstop, &tzdummy);
	etstop2 = times(&cputstop);
	usecstart = (unsigned long long)etstart.tv_sec * 1000000 + etstart.tv_usec;
	usecstop = (unsigned long long)etstop.tv_sec * 1000000 + etstop.tv_usec;
	/* Display timing results */
	//Divide to by 1000 to get milliseconds, and and another thousand for the
	//requests
	printf("\nAvg Response time = %g ms.\n",(float)(usecstop - usecstart)/(float)(1000*1000));
	done = 1;
	unlink(sa.sun_path);
	close(sfd);
	//Close client server as well
	unlink(csa.sun_path);
	close(csfd);
	return;
	#endif
	
	int cmd;
		//Get command from user
		cmd = prompt();
		char filename[MAXLINE];	
		char peerid[HOSTNAMELENGTH];
		char numpeers[PEERRECVNUMCHARS];
		int numpeersint;
		char found[2];
		int found_int;
		switch(cmd)
		{
			case 1:
				send(sfd,"1",2,0);
				break;
			case 2:
				send(sfd,"2",2,0);
				break;
			case 3: 
				send(sfd,"3",2,0);
		}
		/*
		switch(cmd){
			//Register a file to the central indexing server
			case 1: 
				//Read filename from user
				read_filename(filename,0);
				//Send Server command #
				send(sfd,"1",2,0);
				break;
				//Send Server filename and hostname
				send(sfd,(void *)filename,MAXLINE,0);
				//send(sfd,hostname,HOSTNAMELENGTH,0);
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
					//int input = prompt_receive(sfd,numpeersint,peerid);
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
				//send(sfd,(void *)hostname,HOSTNAMELENGTH,0);
				break;
		}
		*/
	//done = 1;
	//unlink(sa.sun_path);
	close(sfd);
	//Close client server as well
	//unlink(csa.sun_path);
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
	int valid;
	do{
		valid = 1;
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
//Create client server
void create_server(void)
{
	//Socket over localhost
	csfd = socket(AF_INET,SOCK_STREAM,0);
	if(csfd < 0)
		perror("Socket");

	//Set socket structure vars
	memset(&csa,0,sizeof(csa));
	csa.sin_family = AF_INET;
	csa.sin_port = htons(cs_port);
	//Bind to 127.0.0.1 (Local)
	csa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	//Bind the server to path
	int err;
	err = bind(csfd,(struct sockaddr*)&csa,sizeof(csa));	
	if(err < 0)
	{
		perror("Bind");
		exit(0);
	}

	//Listen for connections, maximum of numclients*2 clients
	err = listen(csfd,MAXCONNECTIONS);
	if(err < 0)
	{
		perror("Listen");
		exit(0);
	}

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
		if(flag == 0)
			printf("Enter the filename to register\n");
		else 
			printf("Enter the filename to retreive\n");	
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
				printf("File does not exist, try again\n");
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
/* Adds a file to this clients file listing */
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
/* Frees all files from file listing for this client */
void free_files()
{
	int i;
	for(i = 0; i < MAXUSRFILES;i++)
		if(files[i] != NULL)
			free(files[i]);
}
/* Reads in the setup file specified when the client
 * was executed. This contains the ports of neighboring
 * clients. These ports are added to a list (clist)
 */
int read_setup(char *filename)
{
	FILE *file;
	char *buf;
	char port[MAXPORTCHARS+1];
	if((buf=strchr(filename,'\n')) != NULL)
		*buf = '\0';
	//If registering
	//Check to make sure we can open the file
	file = fopen((const char *)filename,"r");
	if(file == NULL)
	{
		printf("Setup file does not exist\n");
		return 0;
	}
	else
	{
		int i = 0;
		//TODO File parsing here
		while(fgets(port,MAXPORTCHARS+1,file))
		{
			if((buf = strchr(port,'\n')) != NULL)
				*buf = '\0';
			printf("Read port: %s\n",port);
			clist[i] = (in_port_t)atoi(port);
			i++;
		}
		fclose(file);
		return 1;
	}
}
void print_clients()
{
	int i;
	for(i = 0; i < MAXCLIENTS; i++)
	{
		if(clist[i] == 0)
			break;
		printf("Client %d: %d\n",i,clist[i]);
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
			//file_checker();
			process_request();
			break;
		default:
			//retrieve server
			//retrieve_server();
			break;
	}
	return NULL;
}
