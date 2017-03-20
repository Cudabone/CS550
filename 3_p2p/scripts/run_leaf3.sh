#!/bin/bash
echo ./client 10003 ../config/star2.txt 
if [ "$1" == "push" ]; then
	./client 10003 ../config/star2.txt --push
elif [ "$1" == "pull" ]; then 
	./client 10003 ../config/star2.txt --pull
fi
