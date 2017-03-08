#C Compliler
CC = clang++
#Compiler Flags
CFLAGS = -g -Wall -pthread -std=c++0x

all: client 
	@echo build complete

client: client.cpp const.h
	$(CC) $(CFLAGS) client.cpp -o client

clean: 
	@rm -f ./server ./client
	@rm -fr ./server.dSYM ./client.dSYM
