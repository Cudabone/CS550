#!/bin/bash
make
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
rm ../c1/*.txt
rm ../c2/*.txt
rm ../c3/*.txt
rm ../c4/*.txt
rm ../c5/*.txt
cp test1.txt ../c1/
cp test2.txt ../c2/
cp test3.txt ../c3/
cp test3.txt ../c4/
cp test3.txt ../c5/
cd ..
