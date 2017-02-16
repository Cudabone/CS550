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
void create_server(void);

char *files[MAXUSRFILES] = {NULL};
//Client port list
in_port_t clist[MAXCLIENTS] = {0};
in_port_t cs_port;
char hostport[MAXPORTCHARS+1];

//Server socket info
struct sockaddr_in sa;
int sfd;
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

	close(sfd);
	free_files();
	return 0;
}
//Create client server
void create_server(void)
{
	//Socket over localhost
	sfd = socket(AF_INET,SOCK_STREAM,0);
	if(sfd < 0)
		perror("Socket");

	//Set socket structure vars
	memset(&sa,0,sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(cs_port);
	//Bind to 127.0.0.1 (Local)
	sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	//Bind the server to path
	int err;
	err = bind(sfd,(struct sockaddr*)&sa,sizeof(sa));	
	if(err < 0)
	{
		perror("Bind");
		exit(0);
	}

	//Listen for connections, maximum of numclients*2 clients
	err = listen(sfd,MAXCONNECTIONS);
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
