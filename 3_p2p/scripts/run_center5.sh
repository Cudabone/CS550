#!/bin/bash
echo ./client 10001 ../config/star0.txt
if [ "$1" == "push" ]; then
	./client 10001 ../config/star0.txt --push
elif [ "$1" == "pull" ]; then 
	./client 10001 ../config/star0.txt --pull
fi
