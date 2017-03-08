#!/bin/bash
echo -n Making testfiles... 
#touch test1.txt test2.txt test3.txt
echo test1 >> test1.txt
echo test2 >> test2.txt
echo test3 >> test3.txt
echo complete

#Making 
echo -n Making star topology files...
echo 10002 >> star0.txt
echo 10003 >> star0.txt
echo 10004 >> star0.txt
echo 10005 >> star0.txt

echo 10001 >> star2.txt
echo 10001 >> star3.txt
echo 10001 >> star4.txt
echo 10001 >> star5.txt
echo complete
