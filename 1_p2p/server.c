#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

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
	int berr;
	berr = bind(sfd,(struct sockaddr*)&sa,sizeof(sa));	
	if(berr < 0)
		perror("Bind");

	cfd = accept(sfd,(struct sockaddr*)&ca,sizeof(ca));
	//Listen for connections, maximum of 3 clients
	//listen(sfd,3);
	unlink(sa.sun_path);
	close(sfd);
	return 0;
}
