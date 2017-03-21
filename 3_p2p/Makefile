#C Compliler
CC = clang++
#Compiler Flags
CFLAGS = -g -Wall -pthread -std=c++0x

all: client 
	@echo Build Complete

client: client.cpp const.hpp
	$(CC) $(CFLAGS) client.cpp -o client

test: push_test.cpp push_test_query.cpp const.hpp
	$(CC) $(CFLAGS) push_test.cpp -o push_test
	$(CC) $(CFLAGS) push_test_query.cpp -o push_test_query

clean: 
	@rm -f ./client ./push_test
	@rm -fr ./client.dSYM ./push_test.dSYM
