# GNUtella-style Peer-to-Peer Network

Run using IP sockets

## Program Files
- client.c - peer client/server
- const.h - parameters used in client.c
- Makefile - builds the c programs using clang
- test0.txt - Test 3 clients and 1 server for all basic operations.
- test(1-4).txt - Test average response time for 1 to 4 clients to a server
- (1-10).txt - Files corresponding to 1~10MB files created by create_files.sh

## Scripts
- setup.sh - moves newly built client and test files into directories ./c(1-4)
- create_files.sh - creates files of size varying from 1MB~10MB
- test_star(1-4).sh - used to run test for 200 queries, not working!
- run_leaf(2-5).sh - used to run a star topology 
- mktestfiles.sh - creates some test files for transferring

## Build(On OSX):

mkdir c1 c2 c3 c4 c5

./setup.sh - will add files to each directory

## Running Instructions
Ensure const.h NUMCLIENTS set to the respective number of clients

(Create a directory for each client)

./client <portno> <config-file>

EX: in c1 directory, ./client ../cl1

The client should be in a separate directory from other clients

## Output file (output.txt)

mkdir c1 c2 c3 c4 c5

./setup.sh

(in c1 dir)
./run_center.sh

(in c2 dir)
./run_leaf2.sh

(in c3 dir)
./run_leaf3.sh

(in c4 dir)
./run_leaf4.sh

(in c5 dir)
./run_leaf5.sh

Exit all clients normally with command 3: Exit

Else may have to end server with killall or ctrl-c

## Tests / Output files
- 10 client Star topology, on issuing 200 requests for response time

Uncomment #define TESTING and #define TEST1 in const.h

run ./test_star1.sh

## Verification
The program was verified for both star and linear topologies for all
functionality and is working. However, it was not tested for 10 total clients,
but it would work. Registering files, retrieving files, queries, requests, and
the file checker is working as intended.
