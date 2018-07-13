#!/bin/bash
#Tecan
#Description:

file=$1;
fname=$(echo $file| cut -d "." -f1)

php midi2rtttl.php $file > "$fname.rtl"
test="$fname:$(cat $fname.rtl | cut -d ':' -f2) : $(cat $fname.rtl | cut -d ':' -f3)"
#test="test:$(cat test.txt | cut -d ":" -f2) : $(cat test.txt | cut -d ":" -f3)"
echo $test > "$fname.rtl"
#echo $test


