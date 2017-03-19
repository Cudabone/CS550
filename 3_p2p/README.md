# GNUtella-style Distributed Peer-to-Peer File Sharing System w/ Push/Pull
# File Updates

## Program Files
- client.cpp - peer client/server
- const.hpp - parameters used in client.cpp
- Makefile - builds the client using clang++
- test0.txt - Test 3 clients and 1 server for all basic operations.
- test(1-4).txt - Test average response time for 1 to 4 clients to a server
- (1-10).txt - Files corresponding to 1~10MB files created by create_files.sh

## Scripts
- setup.sh - creates testing environment for clients, creates separate client
  directories c(1-10).
- create_files.sh - creates files of size varying from 1MB~10MB
- run_center5.sh - Used as center of a 5 node star topology
- run_center10.sh - Used as center of a 10 node star topology
- run_leaf(2-10).sh - Used to run leaf nodes of star topology
- mktestfiles.sh - creates some test files for transferring

## Build(On OSX):
Run: ./setup.sh 

## Running Instructions
Run: ./setup.sh
	
	Change directory into client directory

./client <portno> <config-file> --push

or

./client <portno> <config-file> --pull

EX: in c1 directory, ./client ../cl1

The client should be in a separate directory from other clients

## Output file (output.txt)

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

(in c6 dir)
./run_leaf6.sh

(in c7 dir)
./run_leaf7.sh

(in c8 dir)
./run_leaf8.sh

(in c9 dir)
./run_leaf9.sh

(in c10 dir)
./run_leaf10.sh

Exit all clients normally with command 3: Exit

Else may have to end server with killall or ctrl-c

## Tests / Output files
- 10 client Star topology, on issuing 200 requests for response time

run ./test_star1.sh

## Verification
The program was verified for both star and linear topologies for all
functionality and is working. However, it was not tested for 10 total clients,
but it would work. Registering files, retrieving files, queries, requests, and
the file checker is working as intended.
