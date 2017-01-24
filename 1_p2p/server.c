#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

/*TODO:
 * Figure out why I need a new string to receive valid data
 */

#define MAXFILES 64
typedef struct
{
	int peerid;
	char *filename;
} pfile;

pfile *files[MAXFILES] = {NULL}; 
int lookup(const char *filename);
int registry(const int peerid, const char *filename);
int remove_entry(const int peerid, const char *filename);
void free_list(void);

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
	char *s1 = "Sv";
	char *s2 = malloc(2*sizeof(char));
	send(cfd,(const void *)s1,2,0);
	recv(cfd,(void *)s2,2,0);
	printf("From client: %s\n",s2);
	unlink(sa.sun_path);
	close(sfd);
	free_list();
	free(s2);
	return 0;
}
/* 0 on success, -1 on failure: list full */
int registry(const int peerid, const char *filename)
{
	int i;
	for(i = 0; i < MAXFILES; i++)
	{
		if(files[i] != NULL)
		{
			files[i] = malloc(sizeof(pfile));
			files[i]->peerid = peerid;
			files[i]->filename = malloc(sizeof(*filename)); 
			strcpy(files[i]->filename,filename);
			return 0;
		}
	}
	return -1;
}
/* 0 on success, -1 on failure: entry not in list */
int remove_entry(const int peerid, const char *filename)
{
	int i;
	for(i = 0; i < MAXFILES; i++)
	{
		if(files[i]!= NULL && files[i]->peerid == peerid && strcmp(files[i]->filename,filename) == 0)
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
int lookup(const char *filename)
{
	int i;
	for(i = 0; i < MAXFILES; i++)
	{
		if(files[i] != NULL && strcmp(files[i]->filename,filename) == 0)
			return files[i]->peerid;
	}
	return -1;
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
