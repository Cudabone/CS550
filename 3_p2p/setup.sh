#!/bin/bash
make
echo Creating Client Directories
rm -r ./c1 ./c2 ./c3 ./c4 ./c5 ./c6 ./c7 ./c8 ./c9 ./c10
mkdir c1 c2 c3 c4 c5 c6 c7 c8 c9 c10
mkdir ./c1/User ./c1/Downloaded
mkdir ./c2/User ./c2/Downloaded
mkdir ./c3/User ./c3/Downloaded
mkdir ./c4/User ./c4/Downloaded
mkdir ./c5/User ./c5/Downloaded
mkdir ./c6/User ./c6/Downloaded
mkdir ./c7/User ./c7/Downloaded
mkdir ./c8/User ./c8/Downloaded
mkdir ./c9/User ./c9/Downloaded
mkdir ./c10/User ./c10/Downloaded
cp *kb.txt ./c1/User
cp *kb.txt ./c1/User 
cp *kb.txt ./c2/User 
cp *kb.txt ./c3/User 
cp *kb.txt ./c4/User 
cp *kb.txt ./c5/User 
cp *kb.txt ./c6/User 
cp *kb.txt ./c7/User 
cp *kb.txt ./c8/User 
cp *kb.txt ./c9/User 
cp *kb.txt ./c10/User 

echo Moving Client into directories
cp ./client ./push_test_query ./c1/
cp ./config/line1.txt ./c1/
cp ./client ./push_test ./c2/
cp ./config/line2.txt ./c2/
cp ./client ./push_test ./c3/
cp ./config/line3.txt ./c3/
cp ./client ./push_test ./c4/
cp ./client ./push_test ./c5/
cp ./client ./push_test ./c6/
cp ./client ./push_test ./c7/
cp ./client ./push_test ./c8/
cp ./client ./push_test ./c9/
cp ./client ./push_test ./c10/

echo Moving run scripts into directories
cd scripts
cp ./run_center5.sh ../c1/
cp ./run_center10.sh ../c1/
cp ./run_leaf2.sh ../c2/
cp ./run_leaf3.sh ../c3/
cp ./run_leaf4.sh ../c4/
cp ./run_leaf5.sh ../c5/
cp ./run_leaf6.sh ../c6/
cp ./run_leaf7.sh ../c7/
cp ./run_leaf8.sh ../c8/
cp ./run_leaf9.sh ../c9/
cp ./run_leaf10.sh ../c10/
cd ..

echo Copying test files
cd testfiles
cp test1.txt ../c1/User/
cp test2.txt ../c2/User/
cp test3.txt ../c3/User/
cp test3.txt ../c4/User/
cp test3.txt ../c5/User/
cp test3.txt ../c6/User/
cp test3.txt ../c7/User/
cp test3.txt ../c8/User/
cp test3.txt ../c9/User/
cp test3.txt ../c10/User/
cd ..
