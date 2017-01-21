#C Compliler
CC = clang
#Compiler Flags
CFLAGS = -g -Wall

all: server client 
	@echo build complete

server: server.c
	$(CC) $(CFLAGS) server.c -o server

client: client.c
	$(CC) $(CFLAGS) client.c -o client

clean: 
	@rm -f ./server ./client
	@rm -fr ./server.dSYM ./client.dSYM
