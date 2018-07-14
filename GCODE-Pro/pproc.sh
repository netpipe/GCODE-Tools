#!/bin/bash
#Description: Launcher for Repetier-Host gcodes

echo "Launching Scripts\n"
dir=$(dirname "$0")
#cd dir

btweakatz=0
bmusicPrint=0
brecover=0
szAnswer=0

sleep .1 && wmctrl -a Information -b add,above &
WINDOWID=$(xwininfo -root -int | awk '/xwininfo:/{print $4}') \
#  zenity --info --text="This --info class dialog is on top of the root window" &

#szDate=$(zenity --calendar --text "Pick a day" --title "Medical Leave" --day 23 --month 5 --year 2008); echo $szDate
#zenity  --notification  --window-icon=update.png  --text "Yes No"
#szSavePath=$(zenity --file-selection --save --confirm-overwrite);echo $szSavePath


hello=$(zenity --list --checklist --title "Testing checkbox." --text "Checkbox test." --column "" --column "Nice" True TweakAtZ True musicalPrint False Recover-Print)

#hello=(${hello//|/ }); for (( element = 0 ; element < ${#hello[@]}; element++ )); do somethingTo ${hello[$element]}; done


stweakatz=$(echo $hello| cut -d "|" -f1)
smusicPrint=$(echo $hello| cut -d "|" -f2)
sRecover=$(echo $hello| cut -d "|" -f3)

if [[ $stweakatz == "TweakAtZ" ]]
    then
    btweakatz=1
fi

if [[ $smusicPrint == "musicalPrint" ]]
    then
   # szAnswer2=$(zenity --entry --text "select file midi,wav,rtl file" --entry-text "$dir"); 
    szSavePath=$(zenity --file-selection --save);echo $szSavePath
    bmusicPrint=1
fi

if [[ $sRecover == "Recover-Print" ]]
    then
    brecover=1
    szAnswer=$(zenity --entry --text "what z position to start at" --entry-text "0");
    echo $szAnswer
#try searching for it or close match
fi



if [ $btweakatz -eq 1 ]
then
python "$dir/tweakatz-repetier2.py" $1
fi

if [ $bmusicPrint -eq 1 ]
then
file=$szSavePath
#file="$dir/midi2rtttl/t.mid";
fname=$(basename $file | cut -d "." -f1 )
fpath=$(dirname -- $file)
fext="${file##*.}"; #$(basename $file | cut -d "." -f2 )
#realpath
#$(readlink -f $file)


if [ $fext == "mid" ];
then
    echo "MID found"
    php "$dir/midi2rtttl/midi2rtttl.php" $file > "$dir/midi2rtttl/$fname.rtl"
    test="$fname:$(cat $dir/midi2rtttl/$fname.rtl | cut -d ':' -f2) : $(cat $dir/midi2rtttl/$fname.rtl | cut -d ':' -f3)"
    echo $test > "$dir/midi2rtttl/$fname.rtl"
fi

if [ $fext == "rtl" ];
then
  #  php "$dir/midi2rtttl/midi2rtttl.php" $file > "$dir/midi2rtttl/$fname.rtl"
 #   test="$fname:$(cat $dir/midi2rtttl/$fname.rtl | cut -d ':' -f2) : $(cat $dir/midi2rtttl/$fname.rtl | cut -d ':' -f3)"
  #  echo $test > "$dir/midi2rtttl/$fname.rtl"
    echo "test"
fi

if [ $fext == "wav" ] #test extension for mid if not then make one
then
    ./midi2rtttl/waon -i $1 -o $fname.mid
   # mfile=$wfile
    php "$dir/midi2rtttl/midi2rtttl.php" $file > "$dir/midi2rtttl/$fname.rtl"
    test="$fname:$(cat $dir/midi2rtttl/$fname.rtl | cut -d ':' -f2) : $(cat $dir/midi2rtttl/$fname.rtl | cut -d ':' -f3)"
    echo $test > "$dir/midi2rtttl/$fname.rtl"

fi

#./midi2rtttl/run.sh $1 $szAnswer2

#python "$dir/RTTTL2GCODE2.py" $1
python "$dir/musicalPrint.py" $1 "$dir/midi2rtttl/$fname.rtl"


fi

if [ $brecover -eq 1 ]
then
    python "$dir/resume-printZ.py" $1 $szAnswer
fi

