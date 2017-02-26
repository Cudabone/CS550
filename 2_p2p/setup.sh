#!/bin/bash
rm -r c1 c2 c3 c4 c5
mkdir c1 c2 c3 c4 c5
make
cp ./client ./c1/
cp ./config/line1.txt ./c1/
cp ./client ./c2/
cp ./config/line2.txt ./c2/
cp ./client ./c3/
cp ./config/line3.txt ./c3/
cp ./client ./c4/
cp ./client ./c5/

echo Copying test files
cd testfiles
cp test1.txt ../c1/
cp test2.txt ../c2/
cp test3.txt ../c3/
cd ..
