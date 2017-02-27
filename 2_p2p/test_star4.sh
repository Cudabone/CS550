#!/bin/bash
./client 10001 ./config/star1.txt &
./client 10006 ./config/star2.txt &
./client 10007 ./config/star2.txt &
./client 10008 ./config/star2.txt &
./client 10009 ./config/star2.txt &
./client 10010 ./config/star2.txt &

sleep 1
./client 10002 ./config/star2.txt >> test_star4.txt &
./client 10003 ./config/star2.txt >> test_star4.txt &
./client 10004 ./config/star2.txt >> test_star4.txt & 
./client 10005 ./config/star2.txt >> test_star4.txt &

killall client
