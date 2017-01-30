# Napster-style Peer-to-Peer Network

Central indexing server based peer-to-peer network
Run using UNIX sockets: Can be easily switched to IPv4 sockets
Central indexing server in main directory
Clients held in subdirectory of main directory

# Program Files
server.c - central indexing server
client.c - peer client/server
const.h - parameters used in both server.c and client.c
Makefile - builds the c programs using clang
test0.txt - Test 3 clients and 1 server for all basic operations.
test(1-4).txt - Test average response time for 1 to 4 clients to a server
(1-10).txt - Files corresponding to 1~10MB files created by create_files.sh

# Scripts
setup.sh - moves newly built client and test files into directories ./c(1-4)
create_files.sh - creates files of size varying from 1MB~10MB
tests.sh - response time for 1,2,3,and 4 clients respectivelin into test(1-4).txt

# Build(On OSX):
make
makedir c1
makedir c2
makedir c3
makedir c4
./setup.sh - will add files

# Running Instructions
Ensure const.h NUMCLIENTS set to the respective number of clients
Run server first
./server
Create a directory for each client
In respective directory, server must be in previous directory.
(In sub directory of main) 
./client ../<client-hostname>
EX: in c1 directory, ./client ../cl1
The client should be in a separate directory from other clients

# Normal run
Ensure const.h value of NUMCLIENTS is equal to the number of clients
make
./setup.sh
(in separete shells)
(in main directory)
./server
(in c1 dir)
./client ../cl1
(in c2 dir)
./client ../cl2
(in c3 dir)
./client ../cl3
Exit all clients normally with command 3: Exit
Else may have to end server with killall or ctrl-c

# Tests / Output files
test0.txt - Terminal output from running 3 clients, one server
For Test
In client.c, must #define TESTING. Uncomment it.
test(1-4).txt - Output files which calculates average response time for 1 to 4
clients.

#Output file (Test 0)
Set NUMCLIENTS to 3 in const.h
make
./create_files.sh
mkdir c1
mkdir c2
mkdir c3
./setup.sh
(Remove unwanted files in each directory)
rm ./c1/1.txt
(In separate terminals)
./server
(In dir ./c1)
./client ../cl1
(In dir ./c2)
./client ../cl2
(In dir ./c3)
./client ../cl3
(Register 1.txt in both clients 2 and 3)
(Follow the rest of test0.txt)
(Exit all clients using input 3)

# Output files (test(1-4).txt)
Change NUMCLIENTS in const.h to 4.
Create directories c1-c4 if not done already
make
./setup.sh
./tests.sh

# Verification
The program was tested for all functions including registering files, lookups,
retreival, sending, file updating/checking, exiting, and multiple clients to
one server. This is summarized and shown in test case 1 (test0.txt).
