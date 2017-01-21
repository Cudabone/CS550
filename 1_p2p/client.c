#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

int main(int argc, char **argv)
{
	int sockfd;
	//Socket over IPv4
	//sockfd = socket(AF_INET,SOCK_SEQPACKET,0);	
	
	//Socket over localhost
	sockfd = socket(AF_LOCAL,SOCK_STREAM,0);
	connect("SERV");
}
