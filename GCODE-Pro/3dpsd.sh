#!/bin/bash
#3d printer gcode uploader for sdcard using bash only
#untested sofar
#todo list and delete files from sdcard and initialize print

#extra credits
#https://unix.stackexchange.com/questions/42376/reading-from-serial-from-linux-command-line/42377
#od -x < /dev/ttyS0  #hexdump from serialport
#chown o+rw /dev/ttyUSB

#3dpSD.sh linear-slide.gco test.gcode
printerAddress='/dev/ttyACM0'
filename=$2
#filename="linear-slide.gco"
gcode=$(cat $1)
#gcode=$(cat test.gcode)

function readCommand {
#waits
while [ $timer -lt 350 ];do
	if [ $(cat $printerAddress) != "" ]; then
		cat $printerAddress > tmpfile.txt
		return $(cat $printerAddress)
		break #extra measure
	fi
	#sleep 1
	timer=$[$timer+1]
done
}

function sendCommand {
	echo $1 >> $printerAddress

	return 1;
}

#initial sdcard setup for write codes
sendCommand("M21;"); #get sdcard ready
sendCommand("M28 $filename.txt"); #write to file

#send to SD card
iLineNum=0;
echo $gcode | while in;do
	tmpGcode="N$iLineNum $in"
      #  sendCommand($tmpGcode); 
	#check for valid response, errors during send when it does not get valid line number.
	while [ readCommand() != "ok N$iLineNum" ]; do #just guessing here for now
		#check that sent line and read line match before sending next.
		echo "ok"
		#else
		#resend command
        	sendCommand($tmpGcode);
	done
	iLineNum=$[$iLineNum+1]
done

#function finishWrite{}
#if connected and code sent fine then send finish commands
        sendCommand("M29;"); //stop writing
        sendCommand("M23 $filename;")
