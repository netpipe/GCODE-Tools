#!/usr/bin/python
#
#Tweak At Z plugin - also has warmup commands for initial temperatures to 
#
#perl /mnt/mnt2/projects/3D-CNC/3DPrinting/DA-VINCI/tweakatz-repetier.pl #out
#python /mnt/mnt2/projects/3D-CNC/3DPrinting/DA-VINCI/tweakatz-repetier.py #out
#use pproc.sh to launch these better.
#
# ChangeLog -  added % print v2
#
#use M114 to find Z if its in the proper spot still



#

import sys
import re

import os
import shutil

#Initial Temperature
ibedTemp=60
iextruderTemp=230

#Temp At Z
height=sys.argv[2] #try and get a value that matches the layers you use otherwise it may not find it.
if height==0:
    height=2.7 #just testing




sea = re.compile("Z"+str(height)+"*")
bedTemp=50
extruderTemp=225

#finishing Temps
fbedTemp=0
#fextruderTemp=225

# percent finished - turn off bed
fpercent=0.80



print ("Resume At Z\n")
#sea = re.compile("M106 S[1-9]+[0-9]*")
print (sys.argv[1]+"\n")
#rep = re.compile("M106 S255\n\g<0>")
out = open(sys.argv[1]+"_fixed", 'w')


#iterators for linecounts and %
i=0

with open(sys.argv[1],"r") as f:
    for r in f:
        i=i+1
#print ("this many lines"+str(i)+"\n")
found = 0

with open(sys.argv[1]) as f:
    for r in f:
      if found ==0:
        if re.search(sea, r) is not None:
            found=2;
            print ("Found Z - search\n")
      if found==2:
        found=1
        print ("Found Z - Setting Up GCode\n")
        out.write(";Resume Printing Heating\n")
        out.write("G28 X0 Y0")
        out.write("M140 S"+str(bedTemp)+"\n")
        out.write("M104 S"+str(extruderTemp)+"\n")
        out.write("M190 S"+str(bedTemp)+"\n")
        out.write("M109 S"+str(extruderTemp)+"\n")
        out.write("G1 Y40 x20 F1000")
        out.write("G0 F3000 Z"+str((float(height)+4)))
      #  out.write("G90")
        out.write(";Resume Print - Code\n")
        out.write(r)
      if found==1:
        out.write(r)
        

#os.rename(sys.argv[1]+"_fixed", sys.argv[1])
shutil.move(sys.argv[1]+"_fixed", sys.argv[1])
