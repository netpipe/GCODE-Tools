#Name: Post Process Launcher
#Info: Post Process Launcher
#Depend: GCode
#Type: postprocess
#Param: testing(integer:100) command to run

# put this as a shortcut into the cura folder after checking out the repo to the plugins folder of ~/.cura/


import re
import math
import os
import sys

from itertools import tee, izip


#print ( "string" +sys.argv[1] + "\n")

#os.system('play -n synth 2 pluck "g"')

#string0="xterm -e \"+ os.getenv("HOME") + "/.cura/plugins/GCODE-Tools/GCODE-Pro/pproc.sh " + filename + "\""
#string0="xterm -e "+ os.getenv("HOME") + '/.cura/plugins/GCODE-Tools/GCODE-Pro/pproc.sh ' + filename
print "current directory is " + os.getcwd()
print "filename is " + filename + "\n"

#print ("test"+os.getenv("HOME") )
with open( os.getenv("HOME") + "/.cura/plugins/ready","r") as f:
    for r in f:
        print("ready is " + r )
        if int(r) == "1": #if ready
            print ("Ready - launching pproc")
            os.system("xterm -e "+ os.getenv("HOME") + '/.cura/plugins/GCODE-Tools/GCODE-Pro/pproc.sh ' + filename)
            
        else:
            print ("ready file was bool 0")
            os.system("xterm -e \"python " + os.getenv("HOME") + '/.cura/plugins/GCODE-Tools/GCODE-Pro/cura-ready-button.py ' + filename + "\"&" )
            #os.system('xterm -e "python ' + os.getenv("HOME") + '/.cura/plugins/GCODE-Tools/GCODE-Pro/cura-ready-button.py "')
            #os.system("xterm -e 'mate-calculator'&")

        
#os.system("xterm -e 'mate-calculator'&")



#os.system("xterm -e 'for i in {G,E,C,A,G,C,E};do play -qn synth 2 pluck $i & sleep .25 ; done'")

#with open(filename, "r") as f:
#	lines = f.readlines()

			
			
