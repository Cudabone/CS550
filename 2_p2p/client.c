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
#include <uuid/uuid.h>
#include <assert.h>

//Function prototypes
void add_file(char *filename);
void free_files();
void print_clients();
int read_setup(char *filename);
void read_filename(char *filename, int flag);
void create_server(void);
void *process_request();
void *distribute_threads(void *i);
int prompt(void);
void create_threads(void);
//void initialize_list(in_port_t *pathlist);
void add_client(in_port_t *pathlist);
void to_string(int val,char *string);
int to_int(char *string);
int prompt_receive(int numpeers,char *peerid,char recvports[MAXCLIENTS][MAXPORTCHARS+1]);
int have_query(uuid_t uuid);
void add_query(uuid_t uuid, in_port_t up_port);
void free_query_list(void);
void remove_query(uuid_t uuid);
in_port_t find_query_port(uuid_t uuid);
void add_port(in_port_t port, in_port_t plist[MAXPORTLIST]);
int file_check(char *filename);
void send_file(int cfd);
void retrieve(int sfd, char *filename);
void file_checker();
void print_queries();
void print_plist(char recvports[MAXCLIENTS][MAXPORTCHARS+1]);
void client_user(void);

char *files[MAXUSRFILES] = {NULL};
//Client port list
in_port_t clist[MAXCLIENTS] = {0};
in_port_t cs_port;
char hostport[MAXPORTCHARS+1];
int done = 0;

//Each query has a uuid (universally unique identifier) and an upstream port
typedef struct query
{
	uuid_t uuid;
	in_port_t up_port;
} query;

//List of queries
query *qlist[MAXQUERIES] = {NULL};


/*
typedef struct query
{
	int cmd;
	char filename[MAXPATH];
	in_port_t pathlist[TTL];
	int pathindex;
	int timetolive;
	char found[2];
} query;
*/


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
	strcpy(hostport,argv[1]);
	if(!read_setup(argv[2]))
		return 1;

	/*
#ifdef TESTING
#if defined TEST1
	if(cs_port == 10002)
#elif defined TEST2
	if(cs_port == 10002 || cs_port == 10003)
#elif defined TEST3
	if(cs_port == 10002 || cs_port == 10003 || cs_port == 10004)
#elif defined TEST4
	if(cs_port == 10002 || cs_port == 10003 || cs_port == 10004 || cs_port == 10005)
#endif
	{
		client_user();
		return 0;
	}
#endif
*/

	print_clients();
	create_server();
	create_threads();

	close(csfd);
	free_files();
	return 0;
}
//Process a request from a client, or client file checker
void *process_request()
{
	//Create a socket addr for client
	struct sockaddr_in ca;
	int cfd;
	socklen_t len = sizeof(ca);

	//Loop until all clients disconnected
	while(!done)
	{
		//Wait for new request
		//printf("Server waiting for connection\n");
		cfd = accept(csfd,(struct sockaddr*)&ca,&len);
		if(cfd < 0)
		{
			break;
			//perror("Accept");
		}

		//printf("Connected\n");
		//print_queries();

		/*
		size_t querysize = sizeof(query);
		query *q = malloc(querysize);
		//Receive command #
		//1 = Register, 2 = lookup
		printf("Waiting next query\n");

		//Receive command from client to perform
		if(recv(cfd,(void *)q,querysize,0) == 0)
			break;
			*/
		char cmdstr[2];
		char filename[MAXLINE];
		uuid_t uuid;
		uuid_string_t ustr;
		//in_port_t plist[MAXPORTLIST];
		char recvports[MAXCLIENTS][MAXPORTCHARS+1];
		int numpeersint = 0;
		char numpeers[PEERRECVNUMCHARS];
		int ttlval;
		char ttlstr[TTLCHARS];
		in_port_t up_port;
		char upportstr[MAXPORTCHARS+1] = {"0"};

		struct sockaddr_in sa;

		//Set socket structure variables
		int err;
		//memset(&sa,0,sizeof(sa));
		sa.sin_family = AF_INET;
		//Bind to 127.0.0.1 (Local)
		sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

		//Initialize port list to 0;
		int j;
		for(j = 0; j < MAXCLIENTS; j++)
			strcpy(recvports[j],"0");

		if(recv(cfd,(void *)cmdstr,2,0) == 0)
			break;

		printf("Received query\n");
		//convert to integer
		int cmd = to_int(cmdstr);
		//printf("Received command: %d\n",cmd);

		
		switch(cmd)
		{
			//Outgoing query
			case 1:	
				//Recv uuid of msg
				recv(cfd,ustr,sizeof(uuid_string_t),0);
				uuid_parse(ustr,uuid);
				recv(cfd,upportstr,MAXPORTCHARS+1,0);
				up_port = (in_port_t)atoi(upportstr);
				printf("Received query with UUID: %s, up-port %d\n",ustr,up_port);
				//printf("Received up_port: %d\n",up_port);

				//Recv ttl value and decrement
				recv(cfd,ttlstr,TTLCHARS,0);
				ttlval = to_int(ttlstr);
				ttlval--;
				//printf("Received TTL, str: %s, val: %d\n",ttlstr,ttlval);
				char newttlstr[TTLCHARS];
				to_string(ttlval,newttlstr);

				//Get file name and peerid(hostname)
				recv(cfd,(void *)filename,MAXLINE,0);
				//If alread processed query
				//print_queries();
				//printf("Result of have query: %d\n",have_query(uuid));
				if(have_query(uuid))
				{
					//printf("Already have query, uuid: %s\n",ustr);
					//Dont forward
					//remove_query(uuid);		
					//Send not found none
					char numpeersout[PEERRECVNUMCHARS] = {"0"};
					send(cfd,numpeersout,PEERRECVNUMCHARS,0);
					//remove_query(uuid);
				}
				else if(ttlval == 0)
				{
					//printf("Adding query, uuid: %s\n",ustr);
					add_query(uuid,up_port);
					//Only check self for file
					if(file_check(filename) == 1)
					{
						numpeersint++;
						assert(numpeersint == 1);
						char portno[MAXPORTCHARS+1] = {"0"};
						to_string(cs_port,portno);
						char numpeersout[PEERRECVNUMCHARS] = {"0"};
						to_string(numpeersint,numpeersout);
						send(cfd,numpeersout,PEERRECVNUMCHARS,0);
						send(cfd,portno,MAXPORTCHARS+1,0);
					}
					//We dont have the file
					else
					{
						//Send that we nor anyone else has the file
						char numpeersout[PEERRECVNUMCHARS] = {"0"};
						send(cfd,numpeersout,PEERRECVNUMCHARS,0);
					}
					remove_query(uuid);
					printf("Completed query, uuid: %s\n",ustr);
				}
				//Forward check to all client if not handled already
				else
				{
					//printf("Adding query, uuid: %s\n",ustr);
					add_query(uuid,up_port);
					if(file_check(filename) == 1)				
					{
						numpeersint++;
						char portno[MAXPORTCHARS+1] = {"0"};
						//add_port(cs_port,plist);
						to_string(cs_port,portno);
						//strcpy(recvports[0],portno);

						int j;
						for(j = 0; j < MAXCLIENTS; j++)
						{
							if(strcmp(recvports[j],"0") == 0)
							{
								strcpy(recvports[j],portno);
								break;
							}
						}
						//printf("Client added: %s to port list\n",portno);
					}
					//Forward to all clients
					int i;
					for(i = 0; i < MAXCLIENTS; i++)
					{
						if(clist[i] == 0)
						{
							//printf("Gone through entire client list\n");
							break;
						}
						else if(clist[i] != up_port)
						{
							int sfd;
							//Socket over localhost to each node
							sfd = socket(AF_INET,SOCK_STREAM,0);
							if(sfd < 0)
								perror("Socket");

							printf("Forwarding request to port: %d\n",clist[i]);
							//Start request to each server
							char port[MAXPORTCHARS+1] = {"0"};
							to_string(cs_port,port);
							sa.sin_port = htons(clist[i]);
							err = connect(sfd,(const struct sockaddr *)&sa,sizeof(sa));
							if(err < 0)
								perror("Connect (Server)");
							//Send Lookup Command and filename
							send(sfd,"1",2,0);
							send(sfd,(void *)ustr,sizeof(uuid_string_t),0);
							send(sfd,port,MAXPORTCHARS+1,0);
							send(sfd,newttlstr,TTLCHARS,0);
							send(sfd,filename,MAXLINE,0);

							printf("Waiting for peer to respond\n"); 

							//Receive number of peers with file 
							recv(sfd,(void *)numpeers,PEERRECVNUMCHARS,0); 
							numpeersint += to_int(numpeers); 
							//printf("Found %d more peers with file\n",to_int(numpeers));
							//If there are peers to recv from
							if(numpeersint != 0)
							{
								int temp;
								for(temp = 0; temp < numpeersint; temp++)
								{
									char portno[MAXPORTCHARS+1] = {"0"};
									//Receive port of each available client with file
									recv(sfd,(void *)portno,MAXPORTCHARS+1,0);
									int j;
									for(j = 0; j < MAXCLIENTS; j++)
									{
										if(strcmp(recvports[j],"0") == 0)
										{
											strcpy(recvports[j],portno);
											break;
										}
									}
								}
							}
							close(sfd);
						}
					}
					//close(sfd);
					//Return numpeersint->char
					char numpeersout[PEERRECVNUMCHARS] = {"0"};
					to_string(numpeersint,numpeersout);
					send(cfd,numpeersout,PEERRECVNUMCHARS,0);
					//Return recv port list
					//print_plist(recvports);
					int j = 0;
					for(i = 0; i < numpeersint; i++)
					{
						if(j < MAXCLIENTS && (strcmp(recvports[j],"0") != 0))
						{
							send(cfd,recvports[j],MAXPORTCHARS+1,0);
							j++;
						}
					}
					assert(numpeersint == j);
					//remove query
					remove_query(uuid);
					printf("Completed Query!\n");
					//print_queries();
				}
				break;
				//file retrieval
			case 2:
				send_file(cfd);
				break;
			//Client closing, unregister all files
			default: break;
				//printf("Control should never reach here!");
		}
		close(cfd);
		//close(sfd);
	}
	done = 1;
	//Close the connection
	close(cfd);
	//close(csfd);
	printf("Done\n");
	return NULL;
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
	//Bind to 127.0.0.1 (Local)
	sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	/*
	//Connect to cental indexing server
	err = connect(sfd,(const struct sockaddr *)&sa,sizeof(sa));
	if(err < 0)
		perror("Connect");
		*/
#ifdef TESTING
#if defined TEST1
	if(cs_port == 10002)
#elif defined TEST2
	if(cs_port == 10002 || cs_port == 10003)
#elif defined TEST3
	if(cs_port == 10002 || cs_port == 10003 || cs_port == 10004)
#elif defined TEST4
	if(cs_port == 10002 || cs_port == 10003 || cs_port == 10004 || cs_port == 10005)
#endif
	{

		//Set socket structure variables
		int err;
		//memset(&sa,0,sizeof(sa));
		sa.sin_family = AF_INET;
		//Bind to 127.0.0.1 (Local)
		sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		//Bind to port
		sa.sin_port = htons(clist[0]);

		char recvports[MAXCLIENTS][MAXPORTCHARS+1];
		char numpeers[PEERRECVNUMCHARS];
		int numpeersint = 0;
		char port[MAXPORTCHARS+1];
		to_string(cs_port,port);
		/*
		char found[2];
		int found_int;
		*/
		int ttlval = TTL;
		char ttlstr[TTLCHARS];
		to_string(ttlval,ttlstr);

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
		for(i = 0; i < 200;i++)
		{
			/*
			sfd = socket(AF_INET,SOCK_STREAM,0);
			if(sfd < 0)
				perror("Socket");
			err = connect(sfd,(const struct sockaddr *)&sa,sizeof(sa));
			if(err < 0)
				perror("Connect (User)");
				*/
			//Do a lookup request
			//Send Server command #
			char *filename = "testing.txt";
			uuid_t uuid;
			uuid_generate(uuid);
			uuid_string_t ustr;
			uuid_unparse(uuid,ustr);
			//printf("Generated UUID: %s\n",uuid);
			add_query(uuid,cs_port);	

			//Send query
			int i;
			for(i = 0; i < MAXCLIENTS; i++)
			{
				//create new socket
				sfd = socket(AF_INET,SOCK_STREAM,0);
				if(sfd < 0)
					perror("Socket");

				if(clist[i] == 0)
				{
					//printf("Gone through entire client list\n");
					break;
				}
				//Start request to each server
				sa.sin_port = htons(clist[i]);
				err = connect(sfd,(const struct sockaddr *)&sa,sizeof(sa));
				if(err < 0)
					perror("Connect (User)");
				//Send Lookup Command and filename
				send(sfd,"1",2,0);
				send(sfd,(void *)ustr,sizeof(uuid_string_t),0);
				send(sfd,port,MAXPORTCHARS+1,0);
				send(sfd,ttlstr,TTLCHARS,0);
				send(sfd,filename,MAXLINE,0);

				//Receive number of peers with file 
				recv(sfd,(void *)numpeers,PEERRECVNUMCHARS,0); 
				numpeersint += to_int(numpeers); 
				//printf("Found %d more peers with file\n",numpeersint);
				//If there are peers to recv from
				if(numpeersint != 0)
				{
					int temp;
					for(temp = 0; temp < numpeersint; temp++)
					{
						char portno[MAXPORTCHARS+1];
						//Receive port of each available client with file
						recv(sfd,(void *)portno,MAXPORTCHARS+1,0);
						int j;
						for(j = 0; j < MAXCLIENTS; j++)
						{
							if(strcmp(recvports[j],"0") == 0)
							{
								strcpy(recvports[j],portno);
								break;
							}
						}
					}
				}
				close(sfd);
			}
			remove_query(uuid);
		}
		gettimeofday(&etstop, &tzdummy);
		etstop2 = times(&cputstop);
		usecstart = (unsigned long long)etstart.tv_sec * 1000000 + etstart.tv_usec;
		usecstop = (unsigned long long)etstop.tv_sec * 1000000 + etstop.tv_usec;
		/* Display timing results */
		//Divide to by 1000 to get milliseconds, and and another thousand for the
		//requests
		printf("\nAvg Response time = %g ms.\n",(float)(usecstop - usecstart)/(float)(1000*200));
		done = 1;
		close(sfd);
	}
	//close(csfd);
	return;
#endif
	
	int cmd;
	do{
		//Get command from user

		cmd = prompt();
		char filename[MAXLINE];
		char peerid[MAXPORTCHARS+1];
		char recvports[MAXCLIENTS][MAXPORTCHARS+1];
		char numpeers[PEERRECVNUMCHARS];
		int numpeersint = 0;
		char port[MAXPORTCHARS+1];
		to_string(cs_port,port);
		/*
		char found[2];
		int found_int;
		*/
		int ttlval = TTL;
		char ttlstr[TTLCHARS];
		to_string(ttlval,ttlstr);
		//printf("TTL string: %s\n",ttlstr);

		//Initialize port list to 0;
		int j;
		for(j = 0; j < MAXCLIENTS; j++)
			strcpy(recvports[j],"0");

		switch(cmd){
			//Register a file
			case 1: 
				//Read filename from user
				read_filename(filename,0);
				//printf("Read filename: %s\n",filename);
				add_file(filename);
				break;
			//Do a lookup and then can retrieve
			case 2: 
				//Read filename from user
				read_filename(filename,1);

				//Generate UUID for Request
				uuid_t uuid;
				uuid_generate(uuid);
				uuid_string_t ustr;
				uuid_unparse(uuid,ustr);
				//printf("Generated UUID: %s\n",uuid);
				add_query(uuid,cs_port);	

				/*
				query q;
				size_t querysize = sizeof(query);

				//Initialize query request
				initialize_list(q.pathlist);
				q.cmd = 1;
				q.timetolive = TTL;
				strcpy(q.found,"1");
				strcpy(q.filename,filename);
				add_client(q.pathlist);
				*/

				int i;
				for(i = 0; i < MAXCLIENTS; i++)
				{
					//create new socket
					sfd = socket(AF_INET,SOCK_STREAM,0);
					if(sfd < 0)
						perror("Socket");

					if(clist[i] == 0)
					{
						//printf("Gone through entire client list\n");
						break;
					}
					//Start request to each server
					sa.sin_port = htons(clist[i]);
					err = connect(sfd,(const struct sockaddr *)&sa,sizeof(sa));
					if(err < 0)
						perror("Connect (User)");
					//Send Lookup Command and filename
					send(sfd,"1",2,0);
					send(sfd,(void *)ustr,sizeof(uuid_string_t),0);
					send(sfd,port,MAXPORTCHARS+1,0);
					send(sfd,ttlstr,TTLCHARS,0);
					send(sfd,filename,MAXLINE,0);

					printf("Waiting for peers to respond...\n"); 
					
					//Receive number of peers with file 
					recv(sfd,(void *)numpeers,PEERRECVNUMCHARS,0); 
					numpeersint += to_int(numpeers); 
					//printf("Found %d more peers with file\n",numpeersint);
					//If there are peers to recv from
					if(numpeersint != 0)
					{
						int temp;
						for(temp = 0; temp < numpeersint; temp++)
						{
							char portno[MAXPORTCHARS+1] = {"0"};
							//Receive port of each available client with file
							recv(sfd,(void *)portno,MAXPORTCHARS+1,0);
							int j;
							for(j = 0; j < MAXCLIENTS; j++)
							{
								if(strcmp(recvports[j],"0") == 0)
								{
									strcpy(recvports[j],portno);
									break;
								}
							}
						}
					}
					close(sfd);
				}
				//print_plist(recvports);
				//close socket
				//close(sfd);
				sfd = socket(AF_INET,SOCK_STREAM,0);
				printf("Found %d total peers with file\n",numpeersint);
				int recv = 0;
				if(numpeersint != 0)
					recv = prompt_receive(numpeersint,peerid,recvports);
				//Call recv file here
				if(recv == 1)
				{
					//TODO may not need new connection if in list
					in_port_t port = (in_port_t)atoi(peerid);
					sa.sin_port = htons(port);
					err = connect(sfd,(const struct sockaddr *)&sa,sizeof(sa));
					if(err < 0)
						perror("Connect (File Receive)");
					send(sfd,"2",2,0);
					retrieve(sfd,filename);
					printf("Would have received file here\n");
					printf("File transfer done!\n");
				}
				remove_query(uuid);
				//print_queries();
				close(sfd);
				sfd = socket(AF_INET,SOCK_STREAM,0);
				//TODO choose peer here and begin file transfer
			//Close the client, unregister all files
			case 3:
				//TODO we can spread word to close all servers
				break;
		}
	}while(cmd != 3);
	done = 1;
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
		printf("1: Register a file for network\n");
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
/* Prompt the user as to whether they want to receive, 
 * then which user they want to receive from
 * Return: 0 if not receiving, 1 if receiving
 */
int prompt_receive(int numpeers,char *peerid,char recvports[MAXCLIENTS][MAXPORTCHARS+1])
{
	int valid = 1;
	//char peers[NUMCLIENTS][HOSTNAMELENGTH];
	int input;
    //recvports[MAXCLIENTS][MAXPORTCHARS+1];

	do{
		valid = 1;
		char str[MAXLINE];
		printf("Retrieve from a peer?\n");
		printf("1: Yes\n");
		printf("2: No\n");
		fgets(str,MAXLINE,stdin);
		input = atoi(str);
		if(input != 1 && input != 2)
		{
			valid = 0;
			printf("Invalid input, try again\n");
		}
	}while(!valid);
	//If want to retrieve
	int peernum;
	if(input == 1)
	{
		//Choose which peer
		int i;
		printf("Select which peer to retrieve file from\n");
		for(i = 0; i < numpeers; i++)
		{
			printf("%d: port %s\n",i,recvports[i]);
		}
		do{
			valid = 1;
			char str[MAXLINE];
			fgets(str,MAXLINE,stdin);
			peernum = atoi(str);
			//printf("Numpeers: %d, Peernum chosen: %d\n",numpeers,peernum);
			if(peernum < 0 || peernum > numpeers)
			{
				valid = 0;
				printf("Invalid input, try again\n");
			}
		}while(!valid);
		strcpy(peerid,recvports[peernum]);
		printf("Selected peerid: %s\n",peerid);
	}
	return input;
}
void send_file(int cfd)
{
		char filename[MAXLINE];
		//Receive the filename, if no clients to accept, return
		if(recv(cfd,(void *)filename,MAXLINE,0) == 0)
		{
			close(cfd);
			return;
		}
		printf("Sending file: %s\n",filename);

		//Send file
		//printf("File would be sent here\n");
		//Open the file
		int fd = open(filename,O_RDONLY); 
		if(fd < 0)
		{
			printf("File couldnt be opened\n");
			close(cfd);
			return;
		}
		struct stat filestat;
		fstat(fd,&filestat);
		char filesize[MAXFILESIZECHARS];
		//Convert fstat filesize to string
		sprintf(filesize, "%d",(int)filestat.st_size);
		//Send the file size
		send(cfd,(void *)filesize,MAXFILESIZECHARS,0);
		off_t len = 0;
		//Send the entire file
		if(sendfile(fd,cfd,0,&len,NULL,0) < 0)
		{
			printf("Error sending file\n");
			close(cfd);
			return;
		}
		printf("File sent\n");
		//close file 
		close(fd);
		//close client connection
		close(cfd);
}
/* Set up a server to retrieve from other client
 * and receive the file filename from peerid
 */
void retrieve(int sfd, char *filename)
{
	char filesize[MAXFILESIZECHARS];
	//Send client the file name wanted
	send(sfd,(void *)filename,MAXLINE,0);
	//Receive the filesize
	recv(sfd,(void *)filesize,MAXFILESIZECHARS,0);
	//File size and remaining bytes
	int rem = atoi(filesize);
	char buf[BUFFSIZE];
	FILE *file = fopen(filename,"w");
	int recvb = 0;
	int totalb = atoi(filesize);
	//Write to a new file
	printf("Display file '%s'",filename);
	if(totalb < 1000)
		printf("\n");
	else
		printf("...too large\n");
	while(((recvb = recv(sfd,buf,BUFFSIZE,0)) > 0) && (rem > 0))
	{
		fwrite(buf,sizeof(char),recvb,file);
		rem -= recvb;
		//write if file less that 1K
		if(totalb < 1000)
		{
			fwrite(buf,sizeof(char),recvb,stdout);
		}
	}
	printf("File received\n");
	//Display file if less than 1KB
	fclose(file);
	file = fopen(filename,"r");
	if(atoi(filesize) < 1000)
	{
	}
	fclose(file);
	//unlink(sa.sun_path);
	close(sfd);
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
/*
void initialize_list(in_port_t *pathlist)
{
	int i;
	for(i = 0; i < TTL; i++)
	{
		pathlist[i] = 0;
	}
}
*/
//Adds a port who has file to port list
void add_port(in_port_t port, in_port_t plist[MAXPORTLIST])
{
	int i;
	for(i = 0; i < MAXPORTLIST; i++)
	{
		if(plist[i] == 0)
		{
			plist[i] = port;
			break;
		}
	}
}
//Finds query port in list
in_port_t find_query_port(uuid_t uuid)
{
	int i;
	for(i = 0; i < MAXQUERIES; i++)
	{
		if(qlist[i] != NULL && uuid_compare(uuid,qlist[i]->uuid) == 0)
		{
			return qlist[i]->up_port;
		}
	}
	return 0;
}
//Removes a query from the list of queries by UUID
void remove_query(uuid_t uuid)
{
	int i; 
	int found = 0;
	for(i = 0; i < MAXQUERIES; i++)
	{
		if(qlist[i] != NULL && uuid_compare(uuid,qlist[i]->uuid) == 0)
		{
			found = 1;
			free(qlist[i]);
			qlist[i] = NULL;
			break;
		}
	}
	/*
	if(found)
		printf("Found query in list,removing.\n");
		*/
}
//Checks if the query already exists in the list
int have_query(uuid_t uuid)
{
	int i; 
	for(i = 0; i < MAXQUERIES; i++)
	{
		if(qlist[i] != NULL && uuid_compare(uuid,qlist[i]->uuid) == 0)
			return 1;
	}
	return 0;
}
//Prints all queries in the list, for debugging purposes
void print_queries()
{
	int i;
	for(i = 0; i < MAXQUERIES; i++)
	{
		if(qlist[i] != NULL)
		{
			uuid_t uuid;
			uuid_copy(uuid,qlist[i]->uuid);
			uuid_string_t ustr;
			uuid_unparse(uuid,ustr);
			printf("UUID: %s\n",ustr);
		}
	}
}
//Adds a query to the list by UUID and upstream port
void add_query(uuid_t uuid, in_port_t up_port)
{
	int i;
	for(i = 0; i < MAXQUERIES; i++)
	{
		if(qlist[i] == NULL)
		{
			query *q = malloc(sizeof(query));
			uuid_copy(q->uuid,uuid);
			q->up_port = up_port;
			qlist[i] = q;
			break;
		}
	}
}
//Frees the query list
void free_query_list(void)
{
	int i;
	for(i = 0; i < MAXQUERIES; i++)
	{
		if(qlist[i] != NULL)
			free(qlist[i]);
	}
}
/* Adds a file to this clients file listing */
void add_file(char *filename)
{
	if(file_check(filename) == 1)
	{
		printf("File already registered\n");
		return;
	}
	int i;
	for(i = 0; i < MAXUSRFILES;i++)
	{
		if(files[i] == NULL)
		{
			files[i] = malloc(MAXLINE);
			strcpy(files[i],filename);
			break;
		}
		else if(i == MAXUSRFILES-1)
			printf("File list full\n");
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
//Checks if the file is in the file list
int file_check(char *filename)
{
	int i;
	for(i = 0; i < MAXUSRFILES;i++)
	{
		if(files[i] != NULL)
		{
			if(strcmp(filename,files[i]) == 0)	
				return 1;
		}
	}
	return 0;
}
//Converts a port number to a string
void port_to_string(in_port_t port, char *string)
{
	sprintf(string,"%u",port);
}
//Converts an integer to a string
void to_string(int val,char *string)
{
	sprintf(string,"%d",val);	
}
//Converts a string to an integer
int to_int(char *string)
{
	return atoi(string);
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
		while(fgets(port,MAXPORTCHARS+2,file) != NULL)
		{
			if((buf = strchr(port,'\n')) != NULL)
				*buf = '\0';
			/*
			if((buf = strchr(port,' ')) != NULL)
				*buf = '\0';
				*/
			//printf("Read port: %s\n",port);
			clist[i] = (in_port_t)atoi(port);
			i++;
		}
		fclose(file);
		return 1;
	}
}
//Prints all clients in the client list
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
			file_checker();
			break;
		default:
			process_request();
			//retrieve server
			//retrieve_server();
			break;
	}
	return NULL;
}
/* Checks if any registered files from client were moved
 * or can no longer be opened. If so remove from central
 * indexing server
 */
void file_checker()
{
	int i;
	while(!done)
	{
		FILE *file;
		for(i = 0; i < MAXUSRFILES; i++)
		{
			if(files[i] != NULL)	
			{
				file = fopen((const char *)files[i],"r");
				if(file == NULL)
				{
					printf("File %s moved or deleted: Updating list\n",files[i]);
					//Tell server to remove from list, if available
					free(files[i]);
					files[i] = NULL;
				}
				else
					fclose(file);
			}
		}
		sleep(UPDATETIME);
	}
}
//Prints the port list in the query response, for debugging
void print_plist(char recvports[MAXCLIENTS][MAXPORTCHARS+1])
{
	int i;
	for(i = 0; i < MAXCLIENTS; i++)
	{
		if(strcmp(recvports[i],"0") != 0)
		{
			printf("Port: %s\n",recvports[i]);
		}
	}
}
