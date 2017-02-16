#C Compliler
CC = clang
#Compiler Flags
CFLAGS = -g -Wall

all: client 
	@echo build complete

client: client.c const.h
	$(CC) $(CFLAGS) client.c -o client

clean: 
	@rm -f ./server ./client
	@rm -fr ./server.dSYM ./client.dSYM
