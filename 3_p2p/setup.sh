#!/bin/bash
make
echo Creating Client Directories
rm -r ./c1 ./c2 ./c3 ./c4 ./c5
mkdir c1 c2 c3 c4 c5
mkdir ./c1/User ./c1/Downloaded
mkdir ./c2/User ./c2/Downloaded
mkdir ./c3/User ./c3/Downloaded
mkdir ./c4/User ./c4/Downloaded
mkdir ./c5/User ./c5/Downloaded

echo Moving Client into directories
cp ./client ./c1/
cp ./config/line1.txt ./c1/
cp ./client ./c2/
cp ./config/line2.txt ./c2/
cp ./client ./c3/
cp ./config/line3.txt ./c3/
cp ./client ./c4/
cp ./client ./c5/

echo Moving run scripts into directories
cd scripts
cp ./run_center.sh ../c1/
cp ./run_leaf2.sh ../c2/
cp ./run_leaf3.sh ../c3/
cp ./run_leaf4.sh ../c4/
cp ./run_leaf5.sh ../c5/
cd ..

echo Copying test files
cd testfiles
cp test1.txt ../c1/User/
cp test2.txt ../c2/User/
cp test3.txt ../c3/User/
cp test3.txt ../c4/User/
cp test3.txt ../c5/User/
cd ..
