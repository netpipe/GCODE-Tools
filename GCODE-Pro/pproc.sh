#!/bin/bash
#Description: Launcher for Repetier-Host gcodes

echo "Launching Scripts\n"
dir=$(dirname "$0") # must be the script directory
#cd dir

#update=0
#echo "checking for updates " $update
#if [[ $update == 1 ]]; #enable this for github updater.
#then
 #   pwd=$PWD
 #   cd $dir
 #   git pull
  #  cd $pwd
#fi

btweakatz=0
bmusicPrint=0
brecover=0
szAnswer=0
test=""
sfile=""

midi2rtttl () {
                    echo "calling midi2rttl function"
                    ftname=$(basename "$1" | cut -d "." -f1 )
                    ftpath=$(dirname -- "$1")
                    #fext="${1##*.}"
                    echo "function args are $1\n"
                    echo "ftpath and name are $ftpath / $ftname\n"

                   # $dir/midi2rtttl/waon -n 1000 -i "$1" -o "$dir/midi2rtttl/$fname.mid"
                    php "$dir/midi2rtttl/midi2rtttl.php" "$1" > "$dir/midi2rtttl/$ftname.rtl"
                    test="$ftname:$(cat $dir/midi2rtttl/$ftname.rtl | cut -d ':' -f2) : $(cat $dir/midi2rtttl/$ftname.rtl | cut -d ':' -f3)"
                    echo $test > "$dir/midi2rtttl/$ftname.rtl"

}

sleep .1 && wmctrl -a Information -b add,above &
WINDOWID=$(xwininfo -root -int | awk '/xwininfo:/{print $4}') \
#  zenity --info --text="This --info class dialog is on top of the root window" &

#szDate=$(zenity --calendar --text "Pick a day" --title "Medical Leave" --day 23 --month 5 --year 2008); echo $szDate
#zenity  --notification  --window-icon=update.png  --text "Yes No"
#szSavePath=$(zenity --file-selection --save --confirm-overwrite);echo $szSavePath



#hello=(${hello//|/ }); for (( element = 0 ; element < ${#hello[@]}; element++ )); do somethingTo ${hello[$element]}; done
parseConfig () {


    if [[ $(echo $1| cut -d "|" -f1) == "TweakAtZ" ]]
    then
    stweakatz="TweakAtZ" #$(echo $hello| cut -d "|" -f1);
    elif [[ $(echo $1| cut -d "|" -f2) == "TweakAtZ" ]]
    then
    stweakatz="TweakAtZ" #$(echo $hello| cut -d "|" -f1);
    elif [[ $(echo $1| cut -d "|" -f3) == "TweakAtZ" ]]
    then
    stweakatz="TweakAtZ" #$(echo $hello| cut -d "|" -f1);
    else
    stweakatz="" 
    fi

    if [[ $(echo $1| cut -d "|" -f1) == "Recover-Print" ]]
    then
    sRecover="Recover-Print"
    elif [[ $(echo $1| cut -d "|" -f2) == "Recover-Print" ]]
    then
    sRecover="Recover-Print"
    elif [[ $(echo $1| cut -d "|" -f3) == "Recover-Print" ]]
    then
    sRecover="Recover-Print"
    else
    sRecover=""
    fi

    if [[ $(echo $1| cut -d "|" -f1) == "musicalPrint" ]]
    then
    smusicPrint="musicalPrint"
    elif [[ $(echo $1| cut -d "|" -f2) == "musicalPrint" ]]
    then
    smusicPrint="musicalPrint"
    elif [[ $(echo $1| cut -d "|" -f3) == "musicalPrint" ]]
    then
    smusicPrint="musicalPrint"
    else
    smusicPrint=""
    fi

}


        bstweakatz="True"
        bsRecover="False"
        bsmusicPrint="False"


    if [[ -e "$dir/pproc.conf" ]]
    then
     hello=$(cat "$dir/pproc.conf")
     parseConfig $hello


        bstweakatz="True"
        bsRecover="False"
        bsmusicPrint="True"

        if [[ $stweakatz == "" ]]
        then
        echo $stweakatz
        bstweakatz="False"
        fi
        if [[ $sRecover == "" ]]
        then
        echo $sRecover
        bsRecover="False"
        fi
        if [[ $smusicPrint == "" ]]
        then
        echo $smusicPrint
        bsmusicPrint="False"
        fi
    fi

bconf="False"
hello=$(zenity --list --checklist --title "Testing checkbox." --text "Checkbox test." --column "" --column "Nice" $bstweakatz TweakAtZ $bsmusicPrint musicalPrint $bsRecover Recover-Print $bconf Config )

parseConfig $hello

    if [[ $(echo $hello| cut -d "|" -f1) == "Config" ]]
    then
    bconf="True"
    elif [[ $(echo $hello| cut -d "|" -f2) == "Config" ]]
    then
    bconf="True"
    elif [[ $(echo $hello| cut -d "|" -f3) == "Config" ]]
    then
    bconf="True"
    elif [[ $(echo $hello| cut -d "|" -f4) == "Config" ]]
    then
    bconf="True"
    else
    bconf="False"
    fi


#save config
echo "$stweakatz|$smusicPrint|$sRecover" > "$dir/pproc.conf"

#hello=$(echo "$stweakatz|$smusicPrint|$sRecover")
#echo $hello
#stweakatz=$(echo $hello| cut -d "|" -f1)
#smusicPrint=$(echo $hello| cut -d "|" -f2)
#sRecover=$(echo $hello| cut -d "|" -f3)

tweakatzConfig(){

    if [[ -e "$dir/atz.conf" ]]
    then
        #    if [[ $(cat atz.conf) == "" ]]
        echo "TweakAtZ - found file settings"

    else
        echo "TweakAtZ - Creating Settings"
        iiZBed=50
        iiZhotend=255
        iatZposition=2.24
        iatZBed=50
        iatZhotend=230
        ifinish=80
        echo "$iiZBed|$iiZhotend|$iatZposition|$iatZBed|$iatZhotend|$ifinish" > "$dir/atz.conf"
    fi

        atzform=$(cat "$dir/atz.conf")
        iiZBed=$(echo $atzform| cut -d "|" -f1)
        iiZhotend=$(echo $atzform| cut -d "|" -f2)
        iatZposition=$(echo $atzform| cut -d "|" -f3)
        iatZBed=$(echo $atzform| cut -d "|" -f4)
        iatZhotend=$(echo $atzform| cut -d "|" -f5)
        ifinish=$(echo $atzform| cut -d "|" -f6)

    if [[ $1 == "config" ]]
    then
        echo "config values"
        atzform=$(zenity --forms --title="TweakAtZ" --text="Settings" \
       --add-entry="initial Bed Temperature- $iiZBed" \
       --add-entry="initial Hotend Temperature- $iiZhotend"  \
       --add-entry="atZ Height- $iatZposition"  \
       --add-entry="atZ Bed Temperature- $iatZBed" \
       --add-entry="atZ Hotend Temperature- $iatZhotend"  \
       --add-entry="print % to turn off bed- $ifinish" \
       --add-calendar="Time Stamp GCODE" )
    fi
        #grab form values
        iZBed=$(echo $atzform| cut -d "|" -f1)
        iZhotend=$(echo $atzform| cut -d "|" -f2)
        atZposition=$(echo $atzform| cut -d "|" -f3)
        atZBed=$(echo $atzform| cut -d "|" -f4)
        atZhotend=$(echo $atzform| cut -d "|" -f5)
        finish=$(echo $atzform| cut -d "|" -f6)

        #set old values if blank
        if [[ $iZBed == "" ]]
        then
            iZBed=$iiZBed
        fi
        if [[ $iZhotend == "" ]]
        then
            iZhotend=$iiZhotend
        fi
        if [[    $atZposition == "" ]]
        then
            atZposition=$iatZposition
        fi
        if [[ $atZBed == "" ]]
        then
            atZBed=$iatZBed
        fi
        if [[  $atZhotend == "" ]]
        then
            atZhotend=$iatZhotend
        fi
        if [[  $finish == "" ]]
        then
            finish=$ifinish
        fi
    
        echo "TweakAtZ - Saving Settings"
        echo "$iZBed|$iZhotend|$atZposition|$atZBed|$atZhotend|$finish"
        echo "$iZBed|$iZhotend|$atZposition|$atZBed|$atZhotend|$finish" > "$dir/atz.conf"

   # btweakatz=1
}



echo "$stweakatz|$smusicPrint|$sRecover"

if [[ $bconf == "False" ]]
then
    if [[ $stweakatz == "TweakAtZ" ]]
    then
        tweakatzConfig
        btweakatz=1
    fi
else
    tweakatzConfig "Config"
fi


if [[ $smusicPrint == "musicalPrint" ]]
    then
   # szAnswer2=$(zenity --entry --text "select file midi,wav,rtl file" --entry-text "$dir"); 
    szSavePath=$(zenity --file-selection --save);echo $szSavePath
    bmusicPrint=1
fi

if [[ $sconfigure == "Configure" ]]
    then
   # szAnswer2=$(zenity --entry --text "select file midi,wav,rtl file" --entry-text "$dir"); 
    szSavePath=$(zenity --file-selection --save);echo $szSavePath
    bmusicPrint=1
fi

if [[ $sRecover == "Recover-Print" ]]
    then
#if atz.conf not there then message to run
        atzform=$(cat "$dir/atz.conf")
        iiZBed=$(echo $atzform| cut -d "|" -f1)
        iiZhotend=$(echo $atzform| cut -d "|" -f2)

    szAnswer2=$(zenity --forms --title="TweakAtZ" --text="Settings" \
       --add-entry="z height " \
       --add-entry="Bed Temperature- $iiZBed" \
       --add-entry="Hotend Temperature- $iiZhotend"  \
       --add-entry="rehome Z - use 1 to enable" )  

        zStart=$(echo $szAnswer2| cut -d "|" -f1)
        zBED=$(echo $szAnswer2| cut -d "|" -f2)
        zHOTEND=$(echo $szAnswer2| cut -d "|" -f3)
        rehomeZ=$(echo $szAnswer2| cut -d "|" -f4)

    if [[ $zBED == "" ]]
    then
    zBED=$iiZBed
    fi

    if [[ $zHOTEND == "" ]]
    then
    zHOTEND=$iiZhotend
    fi

    echo $rehomeZ

    if [[ $rehomeZ == "" ]]
    then
    rehomeZ=0
    fi

   # szAnswer=$(zenity --entry --text "what z position to start at" --entry-text "0");
    if [[ $zStart != "0" ]]
    then
    brecover=1
    echo "valid height found" # parse to see or do it in python script?
   # echo $zStart
    fi
    echo "resume printing"
echo $zStart $zBED $zHOTEND $rehomeZ

#try searching for it or close match
fi



if [ $btweakatz -eq 1 ]
then
    #    echo "$iZBed|$iZhotend|$atZposition|$atZBed|$atZhotend|$finish"
    python "$dir/tweakatz-repetier3.py" $1 $iZBed $iZhotend $atZposition $atZBed $atZhotend $finish
   # python "$dir/tweakatz-repetier2.py" $1 $atzform
fi

if [ $bmusicPrint -eq 1 ]
then
file=$szSavePath
echo "$file"
#file="$PWD/midi2rtttl/t.mid";
fname=$(basename $file | cut -d "." -f1 )
fpath=$(dirname -- $file)
fext="${file##*.}"; #$(basename $file | cut -d "." -f2 )
#realpath
#$(readlink -f $file)
#fdir=$(dirname "$szSavePath")

    if [ "$(locate php)" != "" ]
    then
        echo "found php continuing"
        
        if [ "$fext" == "mid" ];
        then
            echo "MID found"
            midi2rtttl "$file" 
        fi

        if [ "$fext" == "rtl" ];
        then
            echo "decoding rtl file"
            midi2rtttl "$file" 
            echo "test"
        fi

        if [ -e "$dir/midi2rtttl/waon" ]; 
        then
        echo "found waon"

            if [ "$fext" == "wav" ] #test extension for mid if not then make one
            then
                        echo "decoding wav file"
                  #  if [ -e "$dir/midi2rtttl/$fname.mid" ]; #check if file exists use it instead
                  #  then
                        #midi2rtttl "$file"
                   # else
                        $dir/midi2rtttl/waon -n 1000 -i "$file" -o "$dir/midi2rtttl/$fname.mid"
                        midi2rtttl "$dir/midi2rtttl/$fname.mid"

                   if [ 0 == 1 ] #run it through for second pass or back to midi file for testinng.
                   then
                    python "$dir/midi2rtttl/rtttlMaker.py" midi -i "$dir/midi2rtttl/$fname.rtl" -o "$fname.mid"
                    timidity "$fname.mid" -Ow -o "$fname.wav"
                    $dir/midi2rtttl/waon -n 1000 -i "$file" -o "$dir/midi2rtttl/$fname.mid"
                    midi2rtttl "$dir/midi2rtttl/$fname.mid"
                    #fi
                    fi

            fi

            if [ "$fext" == "ogg" ] #test extension for mid if not then make one
            then
                        echo "decoding ogg file"
                        oggdec "$file" -o "$dir/midi2rtttl/$fname.wav"
                        $dir/midi2rtttl/waon -n 1000 -i "$dir/midi2rtttl/$fname.wav" -o "$dir/midi2rtttl/$fname.mid"
                        midi2rtttl "$dir/midi2rtttl/$fname.mid" 
            fi

            if [ "$fext" == "mp3" ] #test extension for mid if not then make one
            then
                    #try using convert too ?
                 echo "decoding mp3 file"
                    if [ "$(locate ffmpeg)" != "" ]
                    then
                        ffmpeg -i "$file" "$dir/midi2rtttl/$fname.wav"
                    elif [ "$(locate mpg123)" != "" ]
                    then
                        mpg123 -w "fname".wav "$dir/midi2rtttl/$fname".mp3
                    fi
                        $dir/midi2rtttl/waon -n 1000 -i "$dir/midi2rtttl/$fname.wav" -o "$dir/midi2rtttl/$fname.mid"
                        midi2rtttl "$dir/midi2rtttl/$fname.mid"
                        #"$fpath/$fname.wav"
            fi

            if [ "$fext" != "" ]
            then
            fname=""
            fi
        #./midi2rtttl/run.sh $1 $szAnswer2

        else
            echo "no waon - please compile or install the midi converter or choose one. wav2mid might work too."
        fi

        if [ "$fname" != "" ]
        then
                python "$dir/musicalPrint.py" $1 "$dir/midi2rtttl/$fname.rtl"
        else
                python "$dir/musicalPrint.py" $1 ""
        fi

    else
        echo "no php cannot use this feature"
    fi
fi

if [ $brecover -eq 1 ]
then
    python "$dir/resume-printZ.py" $1 $zStart $zBED $zHOTEND $rehomeZ
fi

