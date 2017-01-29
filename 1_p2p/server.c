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
typedef struct
{
	char peerid[HOSTNAMELENGTH];
	char *filename;
} pfile;

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

	//Create threads
	pthread_t threads[NUMCLIENTS];
	int i;
	for(i = 0; i < NUMCLIENTS; i++)
	{
		/* Call gauss with each thread and paremeter i which
		 * will act like a rank in MPI */
		pthread_create(&threads[i],NULL,process_request,NULL);
	}
	/* Terminate all threads */
	for(i = 0; i < NUMCLIENTS; i++)
	{
		pthread_join(threads[i],NULL);
	}
	print_registry();
	unlink(sa.sun_path);
	close(sfd);
	free_list();
	//IPv4 AF_INET socket
	//struct sockaddr_in sai;

	//Local AF_UNIX socket

	//Socket over IPv4
	//sockfd = socket(AF_INET,SOCK_SEQPACKET,0);	
	
	//Socket over localhost

}
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
	int err;
	err = bind(sfd,(struct sockaddr*)&sa,sizeof(sa));	
	if(err < 0)
		perror("Bind");

	//Listen for connections, maximum of 3 clients
	err = listen(sfd,NUMCLIENTS);
	if(err < 0)
		perror("Listen");

}
/* 0 on success, -1 on failure: list full */
int registry(const char *peerid, const char *filename)
{
	int i;
	if(check_lookup(peerid,filename))
	{
		printf("File already registered\n");
		return 0;
	}
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
	printf("Server registry full\n");
	return -1;
}
/* 0 on success, -1 on failure: entry not in list */
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
void *process_request(void *i)
{
	struct sockaddr_un ca;
	int cfd;
	socklen_t len = sizeof(ca);
	printf("Server waiting for connection\n");
	cfd = accept(sfd,(struct sockaddr*)&ca,&len);
	printf("Connected\n");

	while(1)
	{
		char cmdstr[2];
		char filename[MAXLINE];
		char peerid[HOSTNAMELENGTH];
		//Receive command #
		//1 = Register, 2 = lookup
		printf("Waiting next command\n");

		//TODO, ensure that when client closes, this does not loop, should try
		//to accept again instead.
		if(recv(cfd,(void *)cmdstr,2,0) == 0)
			break;
		printf("Received\n");
		int cmd = atoi(cmdstr);
		switch(cmd)
		{
			//Register file for client
			case 1:	
				recv(cfd,(void *)filename,MAXLINE,0);
				recv(cfd,(void *)peerid,HOSTNAMELENGTH,0);
				printf("Calling Registry with filename: \"%s\"\n",filename);
				registry(peerid,filename);
				break;
			//Lookup file for client
			case 2:
				recv(cfd,(void *)filename,MAXLINE,0);
				printf("Calling Registry with filename: \"%s\"\n",filename);
				char *temp = lookup(filename);
				if(temp != NULL)
				{
					//Tell client we found file (Next message file name)
					send(cfd,"1",2,0);
					//send(cfd,(void *)temp,HOSTNAMELENGTH,0);
					printf("Sending list\n");
					sendlist(cfd,filename);
					printf("Server found host %s has file\n",temp);
				}
				else
				{
					//Tell client we did not find file
					send(cfd,"0",2,0);
					printf("Server did not find file\n");
				}
				break;
				//Client closing, unregister all files
			case 3:
				recv(cfd,(void *)peerid,HOSTNAMELENGTH,0);
				printf("Removing all entries for peerid %s\n",peerid);
				remove_all_entries(peerid);
				break;
		}
		/*
		if(cmd == 1)
		{
			recv(cfd,(void *)filename,MAXLINE,0);
			recv(cfd,(void *)peerid,HOSTNAMELENGTH,0);
			printf("Calling Registry with filename: \"%s\"\n",filename);
			registry(peerid,filename);
		}
		//Lookup file for client
		else if(cmd == 2)
		{
			recv(cfd,(void *)filename,MAXLINE,0);
			printf("Calling Registry with filename: \"%s\"\n",filename);
			char *temp = lookup(filename);
			if(temp != NULL)
			{
				//Tell client we found file (Next message file name)
				send(cfd,"1",2,0);
				//send(cfd,(void *)temp,HOSTNAMELENGTH,0);
				printf("Sending list\n");
				sendlist(cfd,filename);
				printf("Server found host %s has file\n",temp);
			}
			else
			{
				//Tell client we did not find file
				send(cfd,"0",2,0);
				printf("Server did not find file\n");
			}
		}
		*/
	}
	close(cfd);
	printf("Done\n");
	return NULL;
}
void sendlist(int cfd, char *filename)
{
	int i;
	int count = 0;
	char countstr[PEERRECVNUMCHARS];
	for(i = 0; i < MAXFILES; i++)
	{
		if(files[i] != NULL && strcmp(files[i]->filename,filename) == 0)
		{
			count++;
		}
	}
	printf("Found %d clients with file\n",count);
	snprintf(countstr,PEERRECVNUMCHARS,"%d",count);
	send(cfd,(void *)countstr,PEERRECVNUMCHARS,0);
	printf("Sending clients\n");
	for(i = 0; i < MAXFILES; i++)
	{
		if(files[i] != NULL && strcmp(files[i]->filename,filename) == 0)		
		{
			printf("Sending client for list: %s\n",files[i]->peerid);
			send(cfd,(files[i]->peerid),HOSTNAMELENGTH,0);
		}
	}
}
