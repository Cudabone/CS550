#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	struct sockaddr_un sa;
	int sfd;
	//Socket over IPv4
	//sockfd = socket(AF_INET,SOCK_SEQPACKET,0);	
	
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
	char *s1 = malloc(2*sizeof(char));
	char *s2 = "Cl";
	recv(sfd,(void *)s1,2,0);
	printf("From server: %s\n",s1);
	strcpy(s1,"Cl");	
	send(sfd,(const void *)s2,2,0);
	unlink(sa.sun_path);
	close(sfd);
	free(s1);
}
