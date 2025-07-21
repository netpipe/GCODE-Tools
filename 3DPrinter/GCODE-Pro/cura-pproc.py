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
if 1 == 1:
    out = open(os.getenv("HOME") + "/.cura/plugins/test", 'w')

    with open(os.getenv("HOME") + '/.cura/plugins/test') as f:
        for r in f:
            if r == "":
                print("no gcode file string")
                out.write(sys.argv[1])
            else:
                print('setting file')
                file=r

i=0
#print ("test"+os.getenv("HOME") )
with open( os.getenv("HOME") + "/.cura/plugins/ready","r") as f:
    for r in f:
        if[ i == "0" ]: #only one loop to read ready state
            i = "1" 
            print("ready is " + r )
            print(int(r) )
            if r == "1": #if ready
            #if os.stat( os.getenv("HOME") + "/.cura/plugins/ready").st_size == 0:
                print ("Ready - launching pproc")
                os.system("xterm -e "+ os.getenv("HOME") + '/.cura/plugins/GCODE-Tools/GCODE-Pro/pproc.sh ' + filename)
            #write filename as ""
                
            else:
                print ("ready file was bool 0")
                os.system("gedit " + filename)
                button="xterm -e \"python " + os.getenv("HOME") + '/.cura/plugins/GCODE-Tools/GCODE-Pro/cura-button.py ' + filename + "\"&"
                print("launching " + button)
                os.system(button )
                #os.system('xterm -e "python ' + os.getenv("HOME") + '/.cura/plugins/GCODE-Tools/GCODE-Pro/cura-ready-button.py "')
                #os.system("xterm -e 'mate-calculator'&")
            

#with open( os.getenv("HOME") + "/.cura/plugins/ready","r") as f:
#    for r in f:
#        os.getenv("HOME") + '/.cura/plugins/test'

        
#os.system("xterm -e 'mate-calculator'&")



#os.system("xterm -e 'for i in {G,E,C,A,G,C,E};do play -qn synth 2 pluck $i & sleep .25 ; done'")

#with open(filename, "r") as f:
#	lines = f.readlines()

			
			
