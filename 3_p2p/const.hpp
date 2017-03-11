//Port Constants
#define MAXPORT 65535
#define MAXPORTCHARS 5
#define MAXCONNECTIONS 2

//Client Config
//Number of seconds between file checks
#define UPDATETIME 1
//Size of file transfer chunk
#define BUFFSIZE 512

//TTL values
#define TTL 50
#define TTLCHARS 3

//Filename max string length-1 (Excluding \0)
#define MAXLINE 1024
#define PEERRECVNUMCHARS 11
//File size significant digits; Ex 1024 = 4
#define MAXFILESIZECHARS 256

//Client values
//Number of clients
#define NUMCLIENTS 10
//Number of threads for client, minimum = 3
//Need two threads for self, one for every other client
#define NTHREADS 4
