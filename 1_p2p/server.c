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

/*TODO:
 * Figure out why I need a new string to receive valid data
 */

#define MAXFILES 64
//structure to hold peers with files
typedef struct
{
	char peerid[HOSTNAMELENGTH];
	char *filename;
} pfile;

//Function prototypes
pfile *files[MAXFILES] = {NULL}; 
char *lookup(const char *filename);
int check_lookup(const char *peerid,const char *filename);
int registry(const char *peerid, const char *filename);
int remove_entry(const char *peerid, const char *filename);
void free_list(void);
void print_registry();
void create_server(void);
void *process_request(void *i);
void sendlist(int cfd, char *filename);
void remove_all_entries(const char *peerid);

//Server socket info
struct sockaddr_un sa;
int sfd;
int main(int argc, char **argv)
{
	create_server();

	//Create threads, two per client for user and file checker
	pthread_t threads[NUMCLIENTS*2];
	int i;
	for(i = 0; i < NUMCLIENTS*2; i++)
	{
		/* Call gauss with each thread and paremeter i which
		 * will act like a rank in MPI */
		pthread_create(&threads[i],NULL,process_request,NULL);
	}
	/* Terminate all threads */
	for(i = 0; i < NUMCLIENTS*2; i++)
	{
		pthread_join(threads[i],NULL);
	}
	//Print all registered files
	print_registry();
	unlink(sa.sun_path);
	close(sfd);
	//free all files
	free_list();
}
//Creates the central indexing server with file descriptor sfd
void create_server(void)
{
	//Socket over localhost
	sfd = socket(AF_UNIX,SOCK_STREAM,0);
	if(sfd < 0)
		perror("Socket");

	//Set socket structure vars
	memset(&sa,0,sizeof(sa));
	sa.sun_family = AF_UNIX;
	strcpy(sa.sun_path, "SERV");
	unlink(sa.sun_path);
	/*
	char *filename;
	strncpy(sa.sun_path, filename, sizeof(sa.sun_path));
	*/
	//Bind the server to path
	int err;
	err = bind(sfd,(struct sockaddr*)&sa,sizeof(sa));	
	if(err < 0)
		perror("Bind");

	//Listen for connections, maximum of numclients*2 clients
	err = listen(sfd,NUMCLIENTS*2);
	if(err < 0)
		perror("Listen");

}
/* 0 on success, -1 on failure: list full */
//Registers a file from user to list 
int registry(const char *peerid, const char *filename)
{
	int i;
	//Check if file already registered
	if(check_lookup(peerid,filename))
	{
		printf("File already registered\n");
		return 0;
	}
	//Else find a space and add it
	for(i = 0; i < MAXFILES; i++)
	{
		if(files[i] == NULL)
		{
			files[i] = malloc(sizeof(pfile));
			strcpy(files[i]->peerid,peerid);
			files[i]->filename = malloc(sizeof(*filename)); 
			strcpy(files[i]->filename,filename);
			return 0;
		}
	}
	//If control reaches here, list is full
	printf("Server registry full\n");
	return -1;
}
/* 0 on success, -1 on failure: entry not in list */
//Removes a specific entry from the list, used by the clients
//file status checker
int remove_entry(const char *peerid, const char *filename)
{
	int i;
	for(i = 0; i < MAXFILES; i++)
	{
		if(files[i]!= NULL && strcmp(files[i]->peerid,peerid) == 0 && strcmp(files[i]->filename,filename) == 0)
		{
			free(files[i]->filename);
			free(files[i]);
			files[i] = NULL;
			return 0;
		}
	}
	return -1;
}
//Removes all entries for a client, called on disconnecting
void remove_all_entries(const char *peerid)
{
	int i;
	for(i = 0; i < MAXFILES; i++)
	{
		if(files[i]!= NULL && strcmp(files[i]->peerid,peerid) == 0)
		{
			free(files[i]->filename);
			free(files[i]);
			files[i] = NULL;
		}
	}
}
/* peerid on success, -1 on failure: file not in list*/
//Lookup a file on the server
char *lookup(const char *filename)
{
	int i;
	for(i = 0; i < MAXFILES; i++)
	{
		if(files[i] != NULL && strcmp(files[i]->filename,filename) == 0)
			return files[i]->peerid;
	}
	return NULL;
}
/* Checks if already registered
 * 1 = already registered, 0 = not
 */
int check_lookup(const char *peerid,const char *filename)
{
	int i;
	for(i = 0; i < MAXFILES; i++)
	{
		if(files[i] != NULL && strcmp(files[i]->filename,filename) == 0 && strcmp(files[i]->peerid,peerid) == 0)
			return 1;
	}
	return 0;
}
//Free the entire file list
void free_list(void)
{
	int i;
	for(i = 0; i < MAXFILES; i++)
	{
		if(files[i] != NULL)
		{
			free(files[i]->filename);
			free(files[i]);
		}
	}
}
//Print the file registry, used for debugging
void print_registry()
{
	int i;
	for(i = 0; i < MAXFILES; i++)
	{
		if(files[i] != NULL)
		{
			printf("File %d : %s | Client : %s\n",i,files[i]->filename,files[i]->peerid);
		}
	}
}
//Process a request from a client, or client file checker
void *process_request(void *i)
{
	//Create a socket addr for client
	struct sockaddr_un ca;
	int cfd;
	socklen_t len = sizeof(ca);
	printf("Server waiting for connection\n");
	cfd = accept(sfd,(struct sockaddr*)&ca,&len);
	printf("Connected\n");

	//Loop until all clients disconnected
	while(1)
	{
		char cmdstr[2];
		char filename[MAXLINE];
		char peerid[HOSTNAMELENGTH];
		//Receive command #
		//1 = Register, 2 = lookup
		printf("Waiting next command\n");

		//Receive command from client to perform
		if(recv(cfd,(void *)cmdstr,2,0) == 0)
			break;
		printf("Received\n");
		//convert to integer
		int cmd = atoi(cmdstr);
		switch(cmd)
		{
			//Register file for client
			case 1:	
				//Get file name and peerid(hostname)
				recv(cfd,(void *)filename,MAXLINE,0);
				recv(cfd,(void *)peerid,HOSTNAMELENGTH,0);
				printf("Calling Registry with filename: \"%s\"\n",filename);
				//Register the file 
				registry(peerid,filename);
				break;
			//Lookup file for client
			case 2:
				//Get file name from client
				recv(cfd,(void *)filename,MAXLINE,0);
				printf("Calling Registry with filename: \"%s\"\n",filename);
				//Check if file is held in registry
				char *temp = lookup(filename);
				//If file found
				if(temp != NULL)
				{
					//Tell client we found file (Next message file name)
					send(cfd,"1",2,0);
					//send(cfd,(void *)temp,HOSTNAMELENGTH,0);
					printf("Sending list\n");
					sendlist(cfd,filename);
					//printf("Server found host %s has file\n",temp);
				}
				//If file not found
				else
				{
					//Tell client we did not find file
					send(cfd,"0",2,0);
					printf("Server did not find file\n");
				}
				break;
			//Client closing, unregister all files
			case 3:
				//Get client peerid to remove all files
				recv(cfd,(void *)peerid,HOSTNAMELENGTH,0);
				printf("Removing all entries for peerid %s\n",peerid);
				//remove all files for peerid
				remove_all_entries(peerid);
				break;
			//Called by client file checker when file gone
			case 4:
				printf("Server called to remove file\n");
				//Get file name and peerid
				recv(cfd,(void *)filename,MAXLINE,0);
				recv(cfd,(void *)peerid,HOSTNAMELENGTH,0);
				//Remove the entry for the file
				remove_entry(peerid,filename);
				printf("Removing file %s, file removed from client %s\n",filename,peerid);
				break;
		}
	}
	//Close the connection
	close(cfd);
	printf("Done\n");
	return NULL;
}
//Send an entire client listing for a specific file
void sendlist(int cfd, char *filename)
{
	int i;
	int count = 0;
	char countstr[PEERRECVNUMCHARS];
	//Count the number of clients who have file
	for(i = 0; i < MAXFILES; i++)
	{
		if(files[i] != NULL && strcmp(files[i]->filename,filename) == 0)
		{
			count++;
		}
	}
	printf("Found %d clients with file\n",count);
	snprintf(countstr,PEERRECVNUMCHARS,"%d",count);
	//Send number of clients who have file
	send(cfd,(void *)countstr,PEERRECVNUMCHARS,0);
	printf("Sending clients\n");
	for(i = 0; i < MAXFILES; i++)
	{
		if(files[i] != NULL && strcmp(files[i]->filename,filename) == 0)		
		{
			//Send each client who has file
			printf("Sending client for list: %s\n",files[i]->peerid);
			send(cfd,(files[i]->peerid),HOSTNAMELENGTH,0);
		}
	}
}
