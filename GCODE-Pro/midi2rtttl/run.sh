#!/bin/bash
#Tecan
#Description:

mfile=$1;
wfile=$2;
fname=$(echo $mfile| cut -d "." -f1)

if [ $wfile ] #test extension for mid if not then make one
then
    ./waon -i $wfile -o $fname.mid
   # mfile=$wfile
fi

php midi2rtttl.php $mfile > "$fname.rtl"
test="$fname:$(cat $fname.rtl | cut -d ':' -f2) : $(cat $fname.rtl | cut -d ':' -f3)"
#test="test:$(cat test.txt | cut -d ":" -f2) : $(cat test.txt | cut -d ":" -f3)"
echo $test > "$fname.rtl"
#echo $test

    python rtttlMaker.py midi -i "$fname.rtl" -o "$fname.mid"
    timidity "$fname.mid" -Ow -o "$fname.wav"
    aplay "$fname.wav"

