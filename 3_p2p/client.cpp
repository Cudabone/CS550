//General Includes
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <list>
#include <cassert>
#include "const.hpp"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//Network Includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <uuid/uuid.h>

//Multithreading
#include <thread>
#include <functional>

//TODO
//Create separate directories for own and other files
//Own files are added to own directory
//Other files are added to other directory

typedef std::list<in_port_t> port_list;

void create_directories();
bool read_setup(std::string filename,port_list &ports);
bool valid_port_range(in_port_t port);
void print_port_list(const port_list &ports);
void create_threads();
void int_to_string(int val,char *string);
void send_file(int cfd);
int prompt();
void port_to_string(in_port_t port, char *string);
void read_filename(char *filename, int flag);
int prompt_receive(int numpeers,char *peerid,std::list<std::string> recvports);
void print_plist(const std::list<std::string> &plist);

//Servers
void client_user();
bool create_server();
void *process_request();
void retrieve(int sfd, char *filename);

//Multithreading functions
void *distribute_threads(void *i);
void create_threads(void);

//Queries
void add_query(uuid_t uuid, in_port_t port);
void free_qlist();
bool have_query(uuid_t uuid);
void remove_query(uuid_t uuid);
void send_invalidation(const char *fname);
void forward_invalidation(int cfd);

//Files
bool file_check(char *filename);
void add_file(char *filename);
void file_checker();
void remove_file(char *filename);
void create_directories();

typedef struct
{
	uuid_t uuid;
	in_port_t port;
} query;

typedef struct
{
	std::string filename;
	time_t mtime;
} file_entry;

//Query listing
std::list<query *> qlist;
//File listing
std::list<file_entry *> flist;

//Client Server Info
int lsfd;
struct sockaddr_in lsa;
in_port_t ls_port;
port_list plist;

int done = 0;
bool push = false;
int main(int argc, char **argv)
{
	if(argc != 4)
	{
		//std::cout << "Usage: ./client <portno> <setupfile-path>" << std::endl;
		printf("Usage: ./client <portno> <setupfile-path> <update-option>\n");
		printf("\tUpdate option(s):\n");
		printf("\t\t--push\n\t\t\tPush updates to other clients\n");
		printf("\t\t--pull\n\t\t\tPull updates from other clients\n");
		return 1;
	}
	int port = atoi(argv[1]);
	if(port < 1 || port > MAXPORT || strlen(argv[1]) > MAXPORTCHARS)
	{
		//std::cout << "Port number invalid: Range is between 1-65535" << std::endl;
		printf("Port number invalid: Range is between 1-65535\n");
		return 2;
	}
	ls_port = (in_port_t)atoi(argv[1]);
	std::string setup_file = argv[2];
	std::string option = argv[3];
	if(!read_setup(setup_file,plist))
		return 3;
	if(option == std::string("--push"))
		push = true;
	else if(option == std::string("--pull"))
		push = false;
	else
	{
		printf("Choose an update option\n");
		return 4;
	}
	print_port_list(plist);
	if(!create_server())
		return 4;
	create_threads();
	free_qlist();

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
		cfd = accept(lsfd,(struct sockaddr*)&ca,&len);
		if(cfd < 0)
		{
			break;
			//perror("Accept");
		}

		//printf("Connected\n");
		//print_queries();

		char cmdstr[2];
		char filename[MAXLINE];
		uuid_t uuid;
		uuid_string_t ustr;
		unsigned int numpeersint = 0;
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

		std::list<std::string> recvports;

		if(recv(cfd,(void *)cmdstr,2,0) == 0)
			break;

		printf("Received query\n");
		//std::cout << "Received query" << std::endl;
		//convert to integer
		int cmd = atoi(cmdstr);
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
				//std::cout <<"Received query with UUID: "<<ustr<<", up-port: "<<up_port<<std::endl;

				//Recv ttl value and decrement
				recv(cfd,ttlstr,TTLCHARS,0);
				ttlval = atoi(ttlstr);
				ttlval--;
				//printf("Received TTL, str: %s, val: %d\n",ttlstr,ttlval);
				char newttlstr[TTLCHARS];
				int_to_string(ttlval,newttlstr);

				//Get file name and peerid(hostname)
				recv(cfd,(void *)filename,MAXLINE,0);
				//print_queries();
				
				//If already processed query
				if(have_query(uuid))
				{
					//Dont forward
					//Send not found none
					char numpeersout[PEERRECVNUMCHARS] = {"0"};
					send(cfd,numpeersout,PEERRECVNUMCHARS,0);
				}
				else if(ttlval == 0)
				{
					add_query(uuid,up_port);
					//Only check self for file
					if(file_check(filename))
					{
						numpeersint++;
						assert(numpeersint == 1);
						char portno[MAXPORTCHARS+1] = {"0"};
						int_to_string(ls_port,portno);
						char numpeersout[PEERRECVNUMCHARS] = {"0"};
						int_to_string(numpeersint,numpeersout);
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
					//std::cout <<"Completed query, UUID: "<<ustr<<std::endl; 
				}
				//Forward check to all client if not handled already
				else
				{
					//printf("Adding query, uuid: %s\n",ustr);
					add_query(uuid,up_port);
					if(file_check(filename))				
					{
						numpeersint++;
						char portno[MAXPORTCHARS+1] = {"0"};
						int_to_string(ls_port,portno);
						std::string portstr(portno);
						recvports.push_back(portstr);
						//printf("Client added: %s to port list\n",portno);
					}
					//Forward to all clients
					for(port_list::const_iterator it = plist.begin(); it != plist.end(); it++)
					{
						if(*it != up_port)
						{
							int sfd;
							int numpeersin = 0;
							in_port_t nextport = *it;
							//Socket over localhost to each node
							sfd = socket(AF_INET,SOCK_STREAM,0);
							if(sfd < 0)
								perror("Socket");

							printf("Forwarding request to port: %d\n",nextport);
							//Start request to each server
							char port[MAXPORTCHARS+1] = {"0"};
							int_to_string(ls_port,port);
							sa.sin_port = htons(nextport);
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
							numpeersint += atoi(numpeers); 
							numpeersin = atoi(numpeers);
							//printf("Found %d more peers with file\n",to_int(numpeers));
							//If there are peers to recv from
							if(numpeersin != 0)
							{
								int temp;
								for(temp = 0; temp < numpeersin; temp++)
								{
									char portno[MAXPORTCHARS+1] = {"0"};
									//Receive port of each available client with file
									recv(sfd,(void *)portno,MAXPORTCHARS+1,0);
									std::string portstr(portno);
									recvports.push_back(portstr);
								}
							}
							close(sfd);
						}
					}

					char numpeersout[PEERRECVNUMCHARS] = {"0"};
					int_to_string(numpeersint,numpeersout);
					send(cfd,numpeersout,PEERRECVNUMCHARS,0);
					//Return recv port list
					print_plist(recvports);
					printf("Numpeersint: %u, Recvports size: %lu\n",numpeersint,recvports.size());
					assert((int)numpeersint == (int)recvports.size());
					while(!recvports.empty())
					{
						send(cfd,recvports.front().c_str(),MAXPORTCHARS+1,0);
						recvports.pop_front();
					}
					//remove query
					remove_query(uuid);
					printf("Completed Query!\n");
					//print_queries();
				}
				break;
				//file retrieval
			case 2:
				{
					send_file(cfd);
					break;
				}
				//Invalidate file request
			case 3:
				{
					forward_invalidation(cfd);
					break;
				}
			default: 
				{
					break;
				}
				//printf("Control should never reach here!");
		}
		close(cfd);
		//close(sfd);
	}
	done = 1;
	//Close the connection
	close(cfd);
	//close(lsfd);
	printf("Done\n");
	return NULL;
}
/* User interface function which interacts with the central
 * indexing server and retrieves files from other clients.
 */
void client_user()
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

	int cmd;
	do{
		//Get command from user

		cmd = prompt();
		char filename[MAXLINE];
		char peerid[MAXPORTCHARS+1];
		std::list<std::string> recvports;
		char numpeers[PEERRECVNUMCHARS];
		int numpeersint = 0;
		char port[MAXPORTCHARS+1];
		port_to_string(ls_port,port);
		int ttlval = TTL;
		char ttlstr[TTLCHARS];
		int_to_string(ttlval,ttlstr);
		//printf("TTL string: %s\n",ttlstr);

		switch(cmd)
		{
			//Register a file
			case 1: 
				{
					//Read filename from user
					read_filename(filename,0);
					add_file(filename);
					break;
				}
			//Do a lookup and then can retrieve
			case 2: 
				{
					//Read filename from user
					read_filename(filename,1);

					//Generate UUID for Request
					uuid_t uuid;
					uuid_generate(uuid);
					uuid_string_t ustr;
					uuid_unparse(uuid,ustr);
					//printf("Generated UUID: %s\n",uuid);
					add_query(uuid,ls_port);	

					for(port_list::const_iterator it = plist.begin(); it != plist.end(); it++)
					{
						//create new socket
						sfd = socket(AF_INET,SOCK_STREAM,0);
						if(sfd < 0)
							perror("Socket");
						in_port_t nextport = *it;

						//Start request to each server
						sa.sin_port = htons(nextport);
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
						numpeersint += atoi(numpeers); 
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
								std::string portstr(portno);
								recvports.push_back(portstr);
							}
						}
						close(sfd);
					}

					//print_plist(recvports);
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
					break;
				}
			//Close the client, unregister all files
			case 3:
				{
					//TODO we can spread word to close all servers
					break;
				}
		}
	}while(cmd != 3);
	done = 1;
	close(sfd);
	//Close client server as well
	close(lsfd);
}
bool read_setup(std::string filename,port_list &ports)
{
	std::ifstream setup_file(filename.c_str());	
	if(!setup_file)
	{
		std::cout << "No such input file: " << filename << std::endl;
		return false;
	}
	std::string line;
	for(int line_no = 1; std::getline(setup_file,line); line_no++)
	{
		in_port_t port = atoi(line.c_str());
		if(!valid_port_range(port))
		{
			std::cout << "Port '" << port << "' not in range 1-65535" << std::endl;
			return false;
		}
		ports.push_back(port);
	}
	return true;
}
bool valid_port_range(in_port_t port)
{
	if(port < 1 || port > 65535)
		return false;
	return true;
}
void print_port_list(const port_list &ports)
{
	std::cout << "Port list: [ ";
	for(port_list::const_iterator it = ports.begin(); it != ports.end(); it++)
	{
		std::cout << *it << " ";
	}
	std::cout << "]" << std::endl;
}
bool create_server()
{
	lsfd = socket(AF_INET,SOCK_STREAM,0);
	if(lsfd < 0)
	{
		perror("Socket");
		return false;
	}

	//Set socket structure vars
	memset(&lsa,0,sizeof(lsa));
	lsa.sin_family = AF_INET;
	lsa.sin_port = htons(ls_port);

	//Set as local address
	lsa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	int err;
	err = bind(lsfd,(struct sockaddr*)&lsa,sizeof(lsa));	
	if(err < 0)
	{
		perror("Bind");
		return false;
	}

	err = listen(lsfd,MAXCONNECTIONS);
	if(err < 0)
	{
		perror("Listen");
		return false;
	}
	return true;
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
//Converts an integer to string
void int_to_string(int val,char *string)
{
	sprintf(string,"%d",val);	
}
//Converts a port number to a string
void port_to_string(in_port_t port, char *string)
{
	sprintf(string,"%u",port);
}
void add_query(uuid_t uuid, in_port_t up_port)
{
	query *q = new query;
	uuid_copy(q->uuid,uuid);
	q->port = up_port;
	qlist.push_back(q);
}
void remove_query(uuid_t uuid)
{
	for(std::list<query *>::iterator it = qlist.begin(); it != qlist.end(); it++)
	{
		query *q = *it;
		if(uuid_compare(uuid,q->uuid) == 0)
		{
			delete *it;
			qlist.erase(it);
			break;
		}
	}
}
void free_qlist()
{
	for(std::list<query *>::iterator it = qlist.begin(); it != qlist.end(); it++)
		delete *it;
}
bool have_query(uuid_t uuid)
{
	for(std::list<query *>::iterator it = qlist.begin(); it != qlist.end(); it++)
	{
		query *q = *it;
		if(uuid_compare(uuid,q->uuid) == 0)
			return true;
	}
	return false;
}
void add_file(char *filename)
{
	int fd = open(filename,O_RDONLY);
	if(fd < 0)
	{
		printf("File does not exist, cannot add\n");
		return;
	}
	struct stat filestat;
	fstat(fd,&filestat);
	file_entry *fe = new file_entry;
	fe->filename = std::string(filename);
	fe->mtime = filestat.st_mtime;
	flist.push_back(fe);
}
bool file_check(char *filename)
{
	for(std::list<file_entry *>::const_iterator it = flist.begin(); it != flist.end(); it++)
	{
		if(strcmp(filename,(*it)->filename.c_str()) == 0)
			return true;
	}
	return false;
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
/* Checks if any registered files from client were moved
 * or can no longer be opened. If so remove from central
 * indexing server
 */
void remove_file(char *filename)
{
	for(std::list<file_entry *>::const_iterator it = flist.begin(); it != flist.end(); it++)
	{
		if(strcmp(filename,(*it)->filename.c_str()) == 0)
		{
			printf("Removing invalidated file: %s\n",(*it)->filename.c_str());
			delete *it;
			flist.erase(it);
		}
	}
}
void file_checker()
{
	while(!done)
	{
		FILE *file;
		int fd;
		for(std::list<file_entry *>::iterator it = flist.begin(); it != flist.end(); it++)
		{
			file = fopen((const char *)(*it)->filename.c_str(),"r");
			if(file == NULL)
			{
				printf("File %s moved or deleted: Updating list\n",(*it)->filename.c_str());
				//Tell server to remove from list, if available
				delete(*it);
				flist.erase(it);
				send_invalidation((*it)->filename.c_str());
			}
			else
			{
				fclose(file);
				fd = open((const char *)(*it)->filename.c_str(),O_RDONLY);
				if(fd < 0)
				{
					printf("File deleted, sending invalidation request\n");
				}
				else
				{
					struct stat filestat;
					fstat(fd,&filestat);
					//If the file has been modified
					if(filestat.st_mtime > (*it)->mtime)
					{
						//Send invalidation request
						send_invalidation((*it)->filename.c_str());
						(*it)->mtime = filestat.st_mtime;
					}
				}
			}
		}
		sleep(UPDATETIME);
	}
}
void send_invalidation(const char *fname)
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

	uuid_t uuid;
	uuid_generate(uuid);
	uuid_string_t ustr;
	uuid_unparse(uuid,ustr);
	//printf("Generated UUID: %s\n",uuid);
	add_query(uuid,ls_port);	

	std::list<std::string> recvports;
	char port[MAXPORTCHARS+1];
	port_to_string(ls_port,port);
	int ttlval = TTL;
	char ttlstr[TTLCHARS];
	int_to_string(ttlval,ttlstr);
	char filename[MAXLINE];
	strcpy(filename,fname);

	for(port_list::const_iterator it = plist.begin(); it != plist.end(); it++)
	{
		//create new socket
		sfd = socket(AF_INET,SOCK_STREAM,0);
		if(sfd < 0)
			perror("Socket");
		in_port_t nextport = *it;

		//Start request to each server
		sa.sin_port = htons(nextport);
		err = connect(sfd,(const struct sockaddr *)&sa,sizeof(sa));
		if(err < 0)
			perror("Connect (User)");
		//Send Lookup Command and filename
		send(sfd,"3",2,0);
		send(sfd,(void *)ustr,sizeof(uuid_string_t),0);
		send(sfd,port,MAXPORTCHARS+1,0);
		send(sfd,ttlstr,TTLCHARS,0);
		send(sfd,filename,MAXLINE,0);
		close(sfd);
	}
}
void forward_invalidation(int cfd)
{
	struct sockaddr_in sa;
	sa.sin_family = AF_INET;
	//Bind to 127.0.0.1 (Local)
	sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	char filename[MAXLINE];
	uuid_t uuid;
	uuid_string_t ustr;
	int ttlval;
	char ttlstr[TTLCHARS];
	in_port_t up_port;
	char upportstr[MAXPORTCHARS+1] = {"0"};

	recv(cfd,ustr,sizeof(uuid_string_t),0);
	uuid_parse(ustr,uuid);
	recv(cfd,upportstr,MAXPORTCHARS+1,0);
	up_port = (in_port_t)atoi(upportstr);
	printf("Received invalidation request with UUID: %s, up-port %d\n",ustr,up_port);
	//std::cout <<"Received query with UUID: "<<ustr<<", up-port: "<<up_port<<std::endl;

	//Recv ttl value and decrement
	recv(cfd,ttlstr,TTLCHARS,0);
	ttlval = atoi(ttlstr);
	ttlval--;
	//printf("Received TTL, str: %s, val: %d\n",ttlstr,ttlval);
	char newttlstr[TTLCHARS];
	int_to_string(ttlval,newttlstr);
	//Get file name and peerid(hostname)
	recv(cfd,(void *)filename,MAXLINE,0);

	if(have_query(uuid))
	{
		//Do nothing
	}
	else if(ttlval == 0)
	{
		//printf("Adding query, uuid: %s\n",ustr);
		add_query(uuid,up_port);
		//Only check self for file
		if(file_check(filename))
		{
			remove_file(filename);
		}
		//We dont have the file
		remove_query(uuid);
		printf("Completed query, uuid: %s\n",ustr);
		//std::cout <<"Completed query, UUID: "<<ustr<<std::endl; 
	}
	//Forward check to all client if not handled already
	else
	{
		//printf("Adding query, uuid: %s\n",ustr);
		add_query(uuid,up_port);
		if(file_check(filename))				
		{
			remove_file(filename);
		}
		//Forward to all clients
		for(port_list::const_iterator it = plist.begin(); it != plist.end(); it++)
		{
			if(*it != up_port)
			{
				int sfd;
				int err;
				in_port_t nextport = *it;
				//Socket over localhost to each node
				sfd = socket(AF_INET,SOCK_STREAM,0);
				if(sfd < 0)
					perror("Socket");

				printf("Forwarding invalidation to port: %d\n",nextport);
				//Start request to each server
				char port[MAXPORTCHARS+1] = {"0"};
				int_to_string(ls_port,port);
				sa.sin_port = htons(nextport);
				err = connect(sfd,(const struct sockaddr *)&sa,sizeof(sa));
				if(err < 0)
					perror("Connect (Server)");

				//Send Lookup Command and filename
				send(sfd,"3",2,0);
				send(sfd,(void *)ustr,sizeof(uuid_string_t),0);
				send(sfd,port,MAXPORTCHARS+1,0);
				send(sfd,ttlstr,TTLCHARS,0);
				send(sfd,filename,MAXLINE,0);
				close(sfd);
			}
		}
		remove_query(uuid);
		printf("Completed Query!\n");
		//print_queries();
	}
}
/* Prompt the user for the command they would like to do,
 * these include register, lookup/receive, and exit
 */
int prompt()
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
/* Prompt the user as to whether they want to receive, 
 * then which user they want to receive from
 * Return: 0 if not receiving, 1 if receiving
 */
int prompt_receive(int numpeers,char *peerid,std::list<std::string> recvports)
{
	int valid = 1;
	int input;

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
		int i = 0;
		printf("Select which peer to retrieve file from\n");
		for(std::list<std::string>::const_iterator it = recvports.begin(); it != recvports.end(); it++)
		{
			printf("%d: port %s\n",i,(*it).c_str());
			i++;
		}
		do{
			valid = 1;
			char str[MAXLINE];
			fgets(str,MAXLINE,stdin);
			peernum = atoi(str);
			if(peernum < 0 || peernum >= numpeers)
			{
				valid = 0;
				printf("Invalid input, try again\n");
			}
		}while(!valid);
		
		i = 0;
		for(std::list<std::string>::const_iterator it = recvports.begin(); it != recvports.end(); it++)
		{
			if(i == peernum)	
			{
				strcpy(peerid,(*it).c_str());
				break;
			}
			i++;
		}
		printf("Selected peerid: %s\n",peerid);
	}
	return input;
}
void print_plist(const std::list<std::string> &plist)
{
	std::cout << "Recv Ports: " << "[ ";
	for(std::list<std::string>::const_iterator it = plist.begin(); it != plist.end(); it++)
	{
		std::cout << *it << " ";
	}
	std::cout << "]" << std::endl;
}
void create_directories()
{
	int err;
	err = chdir("User");
	if(err < 0)
		mkdir("User",0777);
	chdir("..");
	err = chdir("Received");
	if(err < 0)
		mkdir("Retreived",0777);
	chdir("..");
}
