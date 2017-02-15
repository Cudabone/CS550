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
#define PEERRECVNUMCHARS 2
//Maximum files user can register
#define MAXUSRFILES 10
#define SERVPORT 10000
#define MAXPORT 65535
#define MAXPORTCHARS 5
