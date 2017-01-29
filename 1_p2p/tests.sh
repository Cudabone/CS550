#!/bin/bash
./server &
echo test 1
cd ./c1
./client ../cl1 | tee ../test1.txt
cd ..
killall -c server
echo test 2
./server &
cd ./c1
./client ../cl1 &
cd ../c2
./client ../cl2 | tee ../test2.txt 
cd ..
killall -c server
echo test 3
./server &
cd ./c1
./client ../cl1 &
cd ../c2
./client ../cl2 &
cd ../c3
./client ../cl3 | tee ../test3.txt 
cd ..
killall -c server
echo test 4
./server &
cd ./c1
./client ../cl1 &
cd ../c2
./client ../cl2 &
cd ../c3
./client ../cl3 &
cd ../c4
./client ../cl4 | tee ../test4.txt
cd ..
killall -c server
