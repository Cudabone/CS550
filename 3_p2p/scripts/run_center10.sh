#!/bin/bash
echo ./client 10001 ../config/star1.txt
if [ "$1" == "push" ]; then
	./client 10001 ../config/star1.txt --push
elif [ "$1" == "pull" ]; then 
	./client 10001 ../config/star1.txt --pull
fi
