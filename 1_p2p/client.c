#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define MAXLINE 1024
int prompt(void);
void read_filename(char *filename);

/* NOTES
 * Threads: 1 thread for user
 * 			2 threads (one per other client)
 * 			1 thread to check files
 */
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

	int cmd;
	do{
		cmd = prompt();
		char filename[MAXLINE];	
		char peer_num[2];
		switch(cmd){
			case 1: 
				read_filename(filename);
				//Send Server command #
				send(sfd,"1",2,0);
				//Send Server filename
				send(sfd,(void *)filename,MAXLINE,0);
			case 2: 
				read_filename(filename);
				//Send Server command #
				send(sfd,"2",2,0);
				send(sfd,(void *)filename,MAXLINE,0);
				recv(sfd,(void *)peer_num,2,0);
			 	if(peer_num < 0)	
				{
					printf("File does not exist on server\n");
					break;
				}
				else
				{
					//send/recv from other client
				}
				break;
			case 3: 
				break;
		}
	}while(cmd != 3);
	unlink(sa.sun_path);
	close(sfd);
	free(s1);
}
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
		printf("3: Exit\n");
		fgets(str,MAXLINE,stdin);
		//Consume end of line character
		input = atoi(str);
		if(input > 3 || input < 1 || input == 0)
		{
			valid = 0;
			printf("Invalid input, try again\n");
		}
	}while(!valid);
	return input;
}
void read_filename(char *filename)
{
	printf("Enter the filename to register\n");
	fgets(filename,MAXLINE,stdin);
	char *buf;
	if((buf=strchr(filename,'\n')) != NULL)
		*buf = '\0';
	printf("\"%s\"\n",filename);
}
