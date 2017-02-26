//Maximum filename length
#define MAXLINE 1024
//Maximum hostname length
#define HOSTNAMELENGTH 16
//Number of clients
#define NUMCLIENTS 3
//Number of threads for client, minimum = 3
//Need two threads for self, one for every other client
#define NTHREADS NUMCLIENTS+1
//File size significant digits; Ex 1024 = 4
#define MAXFILESIZECHARS 256
//Size of file transfer chunk
#define BUFFSIZE 512
//Number of bytes to hold number of clients to retrieve from
//MAX = PEERRECVNUMCHARS*10^8. Now its 99999999
#define PEERRECVNUMCHARS 11
//Maximum files user can register
#define MAXUSRFILES 10
#define SERVPORT 10000
//Maximum port number
#define MAXPORT 65535
//Number of port characters
#define MAXPORTCHARS 5
//Maximum number of total clients (Range 1-65535)
#define MAXCLIENTS 1000 
//Number of things for client to listen for
#define MAXCONNECTIONS NUMCLIENTS
#define MAXPATH 260
//TTL Range 1-100
#define TTL 2
#define TTLCHARS 4
//Maximum number of queries client can handle at one time
#define MAXQUERIES NUMCLIENTS
#define MAXPORTLIST 1000
