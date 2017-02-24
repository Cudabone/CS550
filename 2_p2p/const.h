//Maximum filename length
#define MAXLINE 1024
//Maximum hostname length
#define HOSTNAMELENGTH 16
//Number of clients
#define NUMCLIENTS 3
//Number of threads for client, minimum = 3
#define NTHREADS NUMCLIENTS+2
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
#define MAXCONNECTIONS 2
#define MAXPATH 260
#define TTL 25
