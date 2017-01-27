#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <rpc/rpc.h>
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
int registry(const char *peerid, const char *filename);
int remove_entry(const char *peerid, const char *filename);
void free_list(void);
void print_registry();

int main(int argc, char **argv)
{
	//IPv4 AF_INET socket
	//struct sockaddr_in sai;

	//Local AF_UNIX socket
	struct sockaddr_un sa,ca;
	int sfd,cfd;

	//Socket over IPv4
	//sockfd = socket(AF_INET,SOCK_SEQPACKET,0);	
	
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
	err = listen(sfd,3);
	if(err < 0)
		perror("Listen");

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
	recv(cfd,(void *)cmdstr,2,0);
	int cmd = atoi(cmdstr);
	if(cmd == 1)
	{
		recv(cfd,(void *)filename,MAXLINE,0);
		recv(cfd,(void *)peerid,HOSTNAMELENGTH,0);
		printf("Calling Registry with filename: \"%s\"\n",filename);
		registry(peerid,filename);
	}
	else if(cmd == 2)
	{
		recv(cfd,(void *)filename,MAXLINE,0);
		printf("Calling Registry with filename: \"%s\"\n",filename);
		char *temp = lookup(filename);
		if(temp != NULL)
		{
			//Tell client we found file (Next message file name)
			send(cfd,"1",2,0);
			send(cfd,(void *)temp,HOSTNAMELENGTH,0);
			printf("Server found host %s has file\n",temp);
		}
		else
		{
			//Tell client we did not find file
			send(cfd,"0",2,0);
			printf("Server did not find file\n");
		}
	}
	}
	printf("Done\n");
	/*
	char *s1 = "Sv";
	char *s2 = malloc(2*sizeof(char));
	send(cfd,(const void *)s1,2,0);
	recv(cfd,(void *)s2,2,0);
	printf("From client: %s\n",s2);
	free(s2);
	*/
	print_registry();
	unlink(sa.sun_path);
	close(sfd);
	free_list();
	return 0;
}
/* 0 on success, -1 on failure: list full */
int registry(const char *peerid, const char *filename)
{
	int i;
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
