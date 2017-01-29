# Napster-style Peer-to-Peer Network

Central indexing server based peer-to-peer network

# Build(On OSX):
make
./setup.sh

# Running Instructions
Run server first:
./server
./client <path-to-central-server>/<client-hostname>
The client should be in a separate directory from other clients

# Scripts
create_files.sh - creates files of size varying from 1MB~10MB
setup.sh - sets up client executables and files in separate directories

# Normal run
make
./setup.sh
(in separete shells)
./server
./c1/client ../cl1
./c2/client ../cl2
./c3/client ../cl3
