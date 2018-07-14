#Name: Post Process Launcher
#Info: Post Process Launcher
#Depend: GCode
#Type: postprocess
#Param: testing(string:1) command to run


import re
import math
import os
import sys

from itertools import tee, izip

#print ( "string" +sys.argv[1] + "\n")

#os.system('play -n synth 2 pluck "g"')
print "current directory is " + os.getcwd()
print "filename is " + filename + "\n"

os.system("xterm -e './GCODE-Tools/GCODE-Pro/pproc.sh' + filename")
#os.system("xterm -e 'for i in {G,E,C,A,G,C,E};do play -qn synth 2 pluck $i & sleep .25 ; done'")

#with open(filename, "r") as f:
#	lines = f.readlines()


			
			
