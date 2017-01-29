# Napster-style Peer-to-Peer Network

Central indexing server based peer-to-peer network
Run using UNIX sockets: Can be easily switched to IPv4 sockets
Central indexing server in main directory
Clients held in subdirectory of main directory

# Build(On OSX):
make
makedir c1
makedir c2
makedir c3
makedir c4
./setup.sh - will add files

# Running Instructions
Run server first
./server
Create a directory for each client
In respective directory, server must be in previous directory.
(In sub directory of main) 
./client ../<client-hostname>
EX: in c1 directory, ./client ../cl1
The client should be in a separate directory from other clients

# Scripts
create_files.sh - creates files of size varying from 1MB~10MB
setup.sh - sets up client executables and files in separate directories
tests.sh - response time for 1,2,3,and 4 clients respectivelin into test(1-4).txt

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
