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
#include <sys/time.h>
#include <cmath>
#include <utime.h>

//Network Includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <uuid/uuid.h>

//Multithreading
#include <thread>
#include <functional>

//TODO NOTES
//When a file is downloaded, must also receive origin ID, and TTR value, and
//modification time, sent by server who is not origin
//
//Directories: "User" "Downloaded"
//Downloaded files will be auto-registered
//Only can register user files
//
//TODO 
//-Modify file checker to check outdated files - DONE CHECK
//-Add mtime of source, mtime of receiving - DONE CHECK
//-Send file update - DONE CHECK
//
//TODO
//-Regular requests with both lists, modify file_check - DONE
//-Ensure enough threads - DONE
//-Registering files from either directory, adding to correct list - DONE

typedef std::list<in_port_t> port_list;

//Query, including UUID and a port number for requester
typedef struct
{
	uuid_t uuid;
	in_port_t port;
} query;

//File entries, including filename and modification time
typedef struct
{
	std::string filename;
	time_t mtime;
} file_entry;

//File entry for pull approach, filename, modification time, and origin port
typedef struct
{
	std::string filename;
	time_t rtime; //retrieve time
	time_t mtime; //modification time
	in_port_t origin;
	int TTR;
} file_entry_dl;

bool read_setup(std::string filename,port_list &ports);
bool valid_port_range(in_port_t port);
void print_port_list(const port_list &ports);
void create_threads();
void int_to_string(int val,char *string);
int prompt();
void port_to_string(in_port_t port, char *string);
bool read_filename(char *filename, int flag);
int prompt_receive(int numpeers,char *peerid,std::list<std::string> recvports);
void print_plist(const std::list<std::string> &plist);
bool exit_cmd(int cmd);
//int usr_dl_prompt();

//Servers
void client_user();
bool create_server();
void *process_request();
void retrieve_file(int sfd, const char *filename, in_port_t port);
void send_file(int cfd);
void update();
void send_update_request(file_entry_dl *fe,time_t ctime);

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
bool file_check(const char *filename);
void add_file(char *filename);
//void file_checker();
void file_checker_push();
void file_checker_pull();
void remove_file(char *filename);
//void create_directories();
bool check_mtime(char *filename,time_t mtime);
void add_dl_file(const char *filename,in_port_t origin,time_t rtime,time_t mtime, int TTR);
void delete_files();
void create_directories();
file_entry_dl *get_dl_file(const char *filename);
file_entry *get_file(const char *filename);

//testing
void modify_files();
void send_query(const char *filename);
void push_test();

//Query listing
std::list<query *> qlist;
//File listing
std::list<file_entry *> flist;
//Update listing (Downloaded files list)
std::list<file_entry_dl *> ulist;

//Client Server Info
int lsfd;
struct sockaddr_in lsa;
in_port_t ls_port;
port_list plist;

//Hostname and directories for each client
std::string hostname; //Port number as identifier
std::string usrdir;
std::string dldir;
std::string cwdir; //Default working directory

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
	hostname = std::string(argv[1]);
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
	char cwdstr[MAXLINE];
	if(getcwd(cwdstr,sizeof(cwdstr)) != NULL)
	{
		cwdir = std::string(cwdstr);
		usrdir = cwdir+"/User/";
		dldir = cwdir+"/Downloaded/";
		//usrdir = cwdir
	}
	printf("Current working directory: %s\n", cwdir.c_str());
	//create_directories();
	print_port_list(plist);
	if(!create_server())
		return 4;
	create_threads();
	free_qlist();
	delete_files();
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

							//printf("Forwarding request to port: %d\n",nextport);
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
				//Recv Update Check
			case 4:
				{
					char mtimestr[MTIMECHARS] = {"0"};
					time_t mtime;
					recv(cfd,filename,MAXLINE,0);
					recv(cfd,mtimestr,MTIMECHARS,0);
					mtime = (time_t)atoi(mtimestr);
					if(check_mtime(filename,mtime))
					{
						//If file needs to be updated
						send(cfd,"1",2,0);
						send_file(cfd);
					}
					else
					{
						//File does not need to be updated
						send(cfd,"0",2,0);
						int TTR = TTRVAL;
						char ttrstr[TTRCHARS+1] = {"0"};
						int_to_string(TTR,ttrstr);
						send(cfd,ttrstr,TTRCHARS+1,0);
					}
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

	int cmd = prompt();
	bool exit = exit_cmd(cmd);
	while(!exit)
	{
		//Get command from user

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
					if(!read_filename(filename,0))
						break;
					add_file(filename);
					break;
				}
			//Do a lookup and then can retrieve
			case 2: 
				{
					//Read filename from user
					if(!read_filename(filename,1))
						break;

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
					int input = 0;
					if(numpeersint != 0)
						input = prompt_receive(numpeersint,peerid,recvports);
					//Call recv file here
					if(input == 1)
					{
						//TODO may not need new connection if in list
						in_port_t port = (in_port_t)atoi(peerid);
						sa.sin_port = htons(port);
						err = connect(sfd,(const struct sockaddr *)&sa,sizeof(sa));
						if(err < 0)
							perror("Connect (File Receive)");
						send(sfd,"2",2,0);
						retrieve_file(sfd,filename,port);
						//printf("Would have received file here\n");
						printf("File transfer done!\n");
					}
					remove_query(uuid);
					//print_queries();
					close(sfd);
					break;
				}
				//Check for updating files
			case 3:
				{
					update();
					//TODO we can spread word to close all servers
					break;
				}
		}
		//Get next input, check if exiting
		cmd = prompt();
		exit = exit_cmd(cmd);
	}
	done = 1;
	close(sfd);
	//Close client server as well
	close(lsfd);
}
//Reads the setup file given in the argv, this contains the port list of
//neighboring clients
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
//Checks if a given port is within the valid port range
bool valid_port_range(in_port_t port)
{
	if(port < 1 || port > 65535)
		return false;
	return true;
}
//Prints the port list for neighboring clients. Used for debugging.
void print_plist(const std::list<std::string> &plist)
{
	std::cout << "Recv Ports: " << "[ ";
	for(std::list<std::string>::const_iterator it = plist.begin(); it != plist.end(); it++)
	{
		std::cout << *it << " ";
	}
	std::cout << "]" << std::endl;
}
//Prints the port list for neighboring clients. Used for debugging.
void print_port_list(const port_list &ports)
{
	std::cout << "Port list: [ ";
	for(port_list::const_iterator it = ports.begin(); it != ports.end(); it++)
	{
		std::cout << *it << " ";
	}
	std::cout << "]" << std::endl;
}
//Creates the client's server with specified port in argv
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
			if(push)
				file_checker_push();
			else file_checker_pull();
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
//Adds a query to the query listing
void add_query(uuid_t uuid, in_port_t up_port)
{
	query *q = new query;
	uuid_copy(q->uuid,uuid);
	q->port = up_port;
	qlist.push_back(q);
}
//Removes a query from the query listing
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
//Frees the query listing
void free_qlist()
{
	while(!qlist.empty())
	{
		delete qlist.front();
		qlist.pop_front();
	}
}
//Checks if a query is in the query list. True if so, false otherwise.
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
//Adds file to the user file list, not downloaded
void add_file(char *filename)
{
	std::string file = usrdir+filename;
	int fd = open(file.c_str(),O_RDONLY);
	if(fd < 0)
	{
		printf("File does not exist, cannot add\n");
		return;
	}
	close(fd);
	struct stat filestat;
	fstat(fd,&filestat);
	file_entry *fe = new file_entry;
	fe->filename = std::string(filename);
	fe->mtime = filestat.st_mtime;
	flist.push_back(fe);
}
//Creates and adds a downloaded file entry to the file list
void add_dl_file(const char *filename,in_port_t origin,time_t rtime,time_t mtime, int TTR)
{
	file_entry_dl *fe = new file_entry_dl;
	fe->filename = std::string(filename);
	fe->rtime = rtime;
	fe->mtime = mtime;
	fe->origin = origin;
	fe->TTR = TTR;
	ulist.push_back(fe);
}
//Finds a returns a downloaded file entry, else returns null
file_entry_dl *get_dl_file(const char *filename)
{
	for(std::list<file_entry_dl *>::const_iterator it = ulist.begin(); it != ulist.end(); it++)
	{
		if(strcmp(filename,(*it)->filename.c_str()) == 0)
			return (*it);
	}
	return NULL;
}
//Finds and returns a file entry, else returns null
file_entry *get_file(const char *filename)
{
	for(std::list<file_entry *>::const_iterator it = flist.begin(); it != flist.end(); it++)
	{
		if(strcmp(filename,(*it)->filename.c_str()) == 0)
			return (*it);
	}
	return NULL;
}
//Removes the files from file and downloaded file list.
void delete_files()
{
	while(!flist.empty())
	{
		delete(flist.front());
		flist.pop_front();
	}
	while(!ulist.empty())
	{
		delete(ulist.front());
		flist.pop_front();
	}
}
//Checks both file list and downloaded file list, returns if the file exists
bool file_check(const char *filename)
{
	for(std::list<file_entry *>::const_iterator it = flist.begin(); it != flist.end(); it++)
	{
		if(strcmp(filename,(*it)->filename.c_str()) == 0)
			return true;
	}
	for(std::list<file_entry_dl *>::const_iterator it = ulist.begin(); it != ulist.end(); it++)
	{
		if(strcmp(filename,(*it)->filename.c_str()) == 0)
			return true;
	}
	return false;
}
//Checks if any files need updating, requests updates for 
//all files, and updates the listing.
void update()
{
	for(std::list<file_entry_dl *>::iterator it = ulist.begin(); it != ulist.end(); it++)
	{
		file_entry_dl *fe = *it;
		struct timeval tv;	
		gettimeofday(&tv,NULL);
		time_t ctime = tv.tv_sec;
		if((int)floor(((double)(ctime - fe->rtime)/60)) >= fe->TTR)
		{
			//request update
			send_update_request(fe,ctime);
		}
	}
}
//send update request to socket cfd
void send_update_request(file_entry_dl *fe,time_t ctime)
{
	printf("Sending file update request\n");
	struct sockaddr_in ca;
	int cfd;
	cfd = socket(AF_INET,SOCK_STREAM,0);
	if(cfd < 0)
	{
		perror("Socket");
		return;
	}
	ca.sin_family = AF_INET;
	ca.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	ca.sin_port = htons(fe->origin);
	//TODO create socket here
	char mtimestr[MTIMECHARS] = {"0"};
	char updatestr[2] = {"0"};
	int update;
	int_to_string(fe->mtime,mtimestr);
	int err = connect(cfd,(const struct sockaddr *)&ca,sizeof(ca));
	if (err < 0)
	{
		perror("Connect (User Update)\n");
		return;
	}

	send(cfd,"4",2,0);
	send(cfd,fe->filename.c_str(),MAXLINE,0);
	send(cfd,mtimestr,MTIMECHARS,0);
	recv(cfd,updatestr,2,0);
	update = atoi(updatestr);
	if(update == 1)
	{
		//TODO
		//fileupdate
		printf("File changed, updating file\n");
		retrieve_file(cfd,fe->filename.c_str(),fe->origin);
	}
	else
	{
		printf("File not changed, updating TTR\n");
		char ttrstr[TTRCHARS] = {"0"};
		//TTR update
		recv(cfd,ttrstr,TTRCHARS,0);
		fe->TTR = atoi(ttrstr);
		fe->rtime = ctime;
	}
	printf("File update complete\n");
	close(cfd);
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
		bool dl_file = false;

		if(get_file(filename) != NULL)
			dl_file = false;
		else if(get_dl_file(filename) != NULL)
			dl_file = true;

		std::string file;
		if(dl_file)
			file = dldir+filename;
		else file = usrdir+filename;

		int fd = open(file.c_str(),O_RDONLY); 
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
		char mtimestr[MTIMECHARS] = {"0"};
		time_t mtime = filestat.st_mtime;
		int_to_string(mtime,mtimestr);
		send(cfd,mtimestr,MTIMECHARS,0);

		int TTR = TTRVAL;
		char ttrstr[TTRCHARS] = {"0"};
		int_to_string(TTR,ttrstr);
		send(cfd,ttrstr,TTRCHARS,0);

		char originstr[MAXPORTCHARS] = {"0"};
		if(!dl_file)
		{
			int_to_string(ls_port,originstr);
			send(cfd,originstr,MAXPORTCHARS,0);
		}
		else 
		{
			file_entry_dl *fe = get_dl_file(filename);
			int_to_string(fe->origin,originstr);
			send(cfd,originstr,MAXPORTCHARS,0);	
		}

		printf("File sent\n");
		//close file 
		close(fd);
		//close client connection
		close(cfd);
}
//Modifies a file for testing purposes
void modify_files()
{
	int fd;
	for(std::list<file_entry *>::iterator it = flist.begin(); it != flist.end(); it++)
	{
		std::string file = usrdir+(*it)->filename;
		struct stat filestat;
		time_t mtime;
		struct utimbuf update_time;
		fd = open(file.c_str(),O_APPEND);
		if(!(fd < 0))
		{
			fstat(fd,&filestat);	
			mtime = filestat.st_mtime;
			update_time.actime = filestat.st_atime;
			update_time.modtime = filestat.st_mtime+1;
			close(fd);
			utime(file.c_str(),&update_time);
		}
	}
	sleep(UPDATETIME);
}
/* Checks if any registered files from client were moved
 * or can no longer be opened. If so remove from central
 * indexing server
 */
void remove_file(char *filename)
{
	for(std::list<file_entry *>::iterator it = flist.begin(); it != flist.end(); it++)
	{
		if(strcmp(filename,(*it)->filename.c_str()) == 0)
		{
			printf("Removing invalidated file: %s\n",(*it)->filename.c_str());
			delete *it;
			flist.erase(it);
		}
	}
	for(std::list<file_entry_dl *>::iterator it = ulist.begin(); it != ulist.end(); it++)
	{
		if(strcmp(filename,(*it)->filename.c_str()) == 0)
		{
			printf("Removing invalidated file: %s\n",(*it)->filename.c_str());
			delete *it;
			ulist.erase(it);
		}
	}
}
/*
void file_checker()
{
	while(!done)
	{
		FILE *file;
		int fd;
		if(push)
		{
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
		}
		else
		{
			update();
		}
		sleep(UPDATETIME);
	}
}
*/
//Checks file listing for push based servers. If a file is deleted or
//modified, an invalidation request is sent across the network.
void file_checker_push()
{
	printf("File checker (push) started\n");
	while(!done)
	{
		int fd;
		for(std::list<file_entry *>::iterator it = flist.begin(); it != flist.end(); it++)
		{
			std::string file = usrdir+(*it)->filename;
			fd = open(file.c_str(),O_RDONLY);
			if(fd < 0)
			{
				printf("File %s moved or deleted: Updating list\n",(*it)->filename.c_str());
				delete(*it);
				flist.erase(it);
				send_invalidation((*it)->filename.c_str());
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
				close(fd);
			}
		}
		sleep(UPDATETIME);
	}
}
//Checks file listings for pull based servers. If a file is deleted, it is
//removed from the file listing. If a file expires, a request to update the
//file is sent. This is seen in update() and send_update_request().
void file_checker_pull()
{
	printf("File checker (pull) started\n");
	while(!done)
	{
		int fd;
		for(std::list<file_entry *>::iterator it = flist.begin(); it != flist.end(); it++)
		{
			std::string file = usrdir+(*it)->filename;
			fd = open(file.c_str(),O_RDONLY);
			if(fd < 0)
			{
				printf("File %s moved or deleted: Updating list\n",(*it)->filename.c_str());
				//Tell server to remove from list, if available
				delete(*it);
				flist.erase(it);
			}
			else
			{
				close(fd);
			}
		}
		update();
		sleep(UPDATETIME);
	}
}
//Compare the modification of filename to mtime
//If mtime is older, returns true
//else return false
bool check_mtime(char *filename,time_t mtime)
{
	for(std::list<file_entry *>::const_iterator it = flist.begin(); it != flist.end(); it++)
	{
		if(strcmp((*it)->filename.c_str(),filename) == 0)
		{
			if((*it)->mtime > mtime)
				return true;
			return false;
		}
	}
	//Default that file is most up to date, even if deleted by origin
	return false;
}
//Sends an invalidation request for a file to all neighboring peers. If a peer
//has a file, they invalidate it by removing it from their list.
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
//Upon receiving an invalidation request, this forwards the invalidation to
//all neighboring peers. If the client has the file, then it is removed from the file listing.
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
		printf("2: Look up / Retrieve a file\n");
		if(push)
		{
			printf("3: Exit\n");
		}
		else
		{
			printf("3: Update downloaded files\n");
			printf("4: Exit\n");
		}
		//Read input
		fgets(str,MAXLINE,stdin);
		//Parse input to integer
		input = atoi(str);
		//Ensure input number is valid
		if(push)
		{
			if(input > 3 || input < 1 || input == 0)
			{
				valid = 0;
				printf("Invalid input, try again\n");
			}
		}
		else
		{
			if(input > 4 || input < 1 || input == 0)
			{
				valid = 0;
				printf("Invalid input, try again\n");
			}
		}
	}while(!valid);
	return input;
}
//Helper function to check if a command should exit user program
bool exit_cmd(int cmd)
{
	if((push && cmd == 3) || (!push && cmd == 4))
	{
		return true;	
	}
	return false;
}
/*
int usr_dl_prompt()
{
	int input;
	char str[MAXLINE];
	int valid;
	do
	{
		valid = 1;
		printf("Register a User file or Downloaded file?\n");
		printf("0: User file\n");
		printf("1: Downloaded file\n");
		//read input
		fgets(str,MAXLINE,stdin);
		input = atoi(str);
		if(input < 0 || input > 1)
		{
			valid = 0;
			printf("Invalid input, try again\n");
		}
	}while(!valid);
	return input;
}
*/
//flag = 0 : Register, flag = 1 : Lookup
/* Reads a filename from the user, can be called
 * with two flags as to whether the read
 * will correspond to registering a file or
 * a lookup/receive.
 */
bool read_filename(char *filename, int flag)
{
	int fd;
	if(flag == 0)
		printf("Enter the filename to register\n");
	else 
		printf("Enter the filename to retrieve\n");	
	//Read input
	fgets(filename,MAXLINE,stdin);
	char *buf;
	if((buf=strchr(filename,'\n')) != NULL)
		*buf = '\0';
	//If registering
	if(flag == 0)
	{
		std::string file = usrdir+filename;
		//Check to make sure we can open the file
		//printf("Attempting to open: %s\n",file.c_str());
		fd = open(file.c_str(),O_RDONLY);
		if(fd < 0)
		{
			printf("File does not exist, cancelling\n");
			return false;
		}
		else
		{
			close(fd);
			return true;
		}
		//Close the file
	}
	return true;
}
/* Set up a server to retrieve from other client
 * and receive the file filename from peerid
 */
void retrieve_file(int sfd, const char *filename, in_port_t port)
{
	char filesize[MAXFILESIZECHARS];
	//Send client the file name wanted
	send(sfd,(void *)filename,MAXLINE,0);
	//Receive the filesize
	recv(sfd,(void *)filesize,MAXFILESIZECHARS,0);
	//File size and remaining bytes
	int rem = atoi(filesize);
	char buf[BUFFSIZE];
	std::string filestr = dldir+filename;
	int fd = open(filestr.c_str(),O_RDWR | O_CREAT);
	FILE *file = fdopen(fd,"w+");
	int recvb = 0;
	int totalb = atoi(filesize);
	//Write to a new file
	printf("Display file '%s'",filename);
	//Set file to read only
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
	/*
	if(chmod(filestr.c_str(),S_IRUSR | S_IRGRP | S_IROTH) < 0)
		printf("chmod failed\n");
		*/
	struct timeval tv;
	gettimeofday(&tv,NULL);
	time_t rtime = tv.tv_sec;

	char mtimestr[MTIMECHARS] = {"0"};
	recv(sfd,mtimestr,MTIMECHARS,0);
	time_t mtime = atoi(mtimestr);

	char ttrstr[TTRCHARS] = {"0"};
	recv(sfd,ttrstr,TTRCHARS,0);
	int TTR = atoi(ttrstr);

	char originstr[MAXPORTCHARS] = {"0"};
	recv(sfd,originstr,MAXPORTCHARS,0);
	in_port_t origin = (in_port_t)atoi(originstr);
	
	printf("Adding dl file\n");
	add_dl_file(filename,origin,rtime,mtime,TTR);

	printf("File received\n");
	//Display file if less than 1KB
	fclose(file);
	close(fd);
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
/*
void create_directories()
{
	usrdir = "User";
	dldir = "Downloaded";
	int err;
	err = chdir(usrdir.c_str());
	if(err < 0)
		mkdir(usrdir.c_str(),0777);
	else
		chdir(cwdir.c_str());
	err = chdir(dldir.c_str());
	if(err < 0)
		mkdir(dldir.c_str(),0777);
	else 
		chdir(cwdir.c_str());
}
*/
void send_query(const char *filename)
{
	//Read filename from user
	int sfd;
	struct sockaddr_in sa;
	int err;
	char portstr[MAXPORTCHARS+1];
	port_to_string(ls_port,portstr);
	char numpeers[PEERRECVNUMCHARS];
	int numpeersint = 0;
	std::list<std::string> recvports;
	char peerid[MAXPORTCHARS+1];

	int ttlval = TTL;
	char ttlstr[TTLCHARS];
	int_to_string(ttlval,ttlstr);

	sa.sin_family = AF_INET;
	//Bind to 127.0.0.1 (Local)
	sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
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
		send(sfd,portstr,MAXPORTCHARS+1,0);
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
	if(numpeersint != 0)
	{
		strcpy(peerid,recvports.front().c_str());
		//Call recv file here
		//TODO may not need new connection if in list
		in_port_t port = (in_port_t)atoi(peerid);
		sa.sin_port = htons(port);
		err = connect(sfd,(const struct sockaddr *)&sa,sizeof(sa));
		if(err < 0)
			perror("Connect (File Receive)");
		send(sfd,"2",2,0);
		retrieve_file(sfd,filename,port);
		//printf("Would have received file here\n");
		printf("File transfer done!\n");
	}

	remove_query(uuid);
	//print_queries();
	close(sfd);
}
void push_test()
{
	int invalidations = 0;
	int total = 0;
	std::list<std::string> files;
	files.push_back(std::string("1kb.txt"));
	files.push_back(std::string("2kb.txt"));	
	files.push_back(std::string("3kb.txt"));	
	files.push_back(std::string("4kb.txt"));	
	files.push_back(std::string("5kb.txt"));	
	files.push_back(std::string("6kb.txt"));	
	files.push_back(std::string("7kb.txt"));	
	files.push_back(std::string("8kb.txt"));	
	files.push_back(std::string("9kb.txt"));	
	files.push_back(std::string("10kb.txt"));	
	//Get all files initially
	for(std::list<std::string>::const_iterator it = files.begin(); it != files.end(); it++)
	{
		send_query((*it).c_str());
	}
	for(int i = 0; i < 100; i++)
	{
		for(std::list<std::string>::const_iterator it = files.begin(); it != files.end(); it++)
		{
			total++;
			//If you dont have the file, it was updated
			if(!file_check((*it).c_str()))
			{
				invalidations++;
				if(rand() % 10 < 5)
					send_query((*it).c_str());
			}
		}
	}
	float percentinvalid = (float)(invalidations/total);
	printf("Number of invalidations: %d\n",invalidations);
	printf("Percent invalid: %%%2f\n",percentinvalid);
	printf("Percent valid: %%%2f\n",100-percentinvalid);
}
