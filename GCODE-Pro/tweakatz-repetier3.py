#!/usr/bin/python
#
#Tweak At Z plugin - also has warmup commands for initial temperatures to 
#
#perl /mnt/mnt2/projects/3D-CNC/3DPrinting/DA-VINCI/tweakatz-repetier.pl #out
#python /mnt/mnt2/projects/3D-CNC/3DPrinting/DA-VINCI/tweakatz-repetier.py #out
#
# ChangeLog -  added % print v2
#
#

import sys
import re

import os
import shutil
from sys import argv


#Initial heatup temperature
if len(sys.argv) >= 1:
    fnf = sys.argv[1]
    print ("looks good for file " + sys.argv[1])
else:
    fnf = 'file not found'

print fnf

if sys.argv[2]=="":
    ibedTemp=60
else:
    ibedTemp=sys.argv[2]

if sys.argv[3]=="":
    iextruderTemp=223
else:
    iextruderTemp=sys.argv[3]

#Temp At Z
if sys.argv[4]:
    height=sys.argv[4] #2.24 #try and get a value that matches the layers you use otherwise it may not find it.
    sea = re.compile("Z"+str(height)+"*")
    print("found argv for string height")
else:
    height=2.24 #try and get a value that matches the layers you use otherwise it may not find it.
    sea = re.compile("Z"+str(height)+"*")
    print("default height of 2.24" + str(height))

if sys.argv[5]:
    bedTemp=sys.argv[5]
else:
    bedTemp=50

if sys.argv[6]:
    extruderTemp=sys.argv[6]
else:
    extruderTemp=225

#finishing Temps
fbedTemp=0
#fextruderTemp=225

# percent finished - turn off bed
if sys.argv[6:]:
    fpercent=float(sys.argv[7])*0.01
else:
    fpercent=0.80

print ("string to use  bed:"+str(ibedTemp) + "iextruder:" + str(iextruderTemp) + "height:" + str(height) + "bedtemp:" + str(bedTemp) + "extruderTemp:" + str(extruderTemp) + "fpercent:" + str(fpercent)+ "\n" )



print ("Tweak At Z\n")
#sea = re.compile("M106 S[1-9]+[0-9]*")
print (sys.argv[1]+"\n")
#rep = re.compile("M106 S255\n\g<0>")
out = open(sys.argv[1]+"_fixed", 'w')

#out = open(sys.argv[1], 'w')

#iterators for linecounts and %
i=0
i2=0

with open(sys.argv[1]) as f:
    for r in f:
        i=i+1
#print ("this many lines"+str(i)+"\n")

with open(sys.argv[1]) as f:
    for r in f:
      if re.search(sea, r) is not None:
        print ("Found Z - Cooling down a bit.\n")
        out.write(";Cooling Print\n")
        out.write("M140 S"+str(bedTemp)+"\n")
        out.write("M104 S"+str(extruderTemp)+"\n")
        out.write(r)
      elif i2 == int(i*fpercent) :
        print ("Finishing Print Temps.\n")
        out.write(";Finishing Print\n")
        out.write("M140 S"+str(fbedTemp)+"\n")
        out.write(r)
      #  out.write("M104 S"+str(fextruderTemp)+"\n")
 #     elif re.search(re.compile("G28*"), r) is not None:
 #       print ("Found Homing Command - Inserting initial warmup.\n")
 #       out.write(";start tweakatz write\n")
#        out.write("M140 S"+str(ibedTemp)+"\n")
#        out.write("M104 S"+str(iextruderTemp)+"\n")
    #  elif bool(re.search(re.compile(";tweakatz"), r)):
	#print("start gcode not found, Skipping code insertion\n please put ";tweakatz" string into the starting gcode from repetierhost for script to work")
      elif re.search(re.compile(";tweakatz"), r) is not None:
        print ("Found Homing Command - Inserting initial warmup.\n")
        out.write(";start tweakatz write\n")
	out.write(";set Z to 4 very slow to give time to pull out larger parts, should be homed already\n")
	out.write("G1 Z4 F10\n")
	out.write(";initial warm temps 41deg\n")
        out.write("M140 S41\n")
        out.write("M104 S41\n")
        out.write(";homing command G28\n")
        out.write("G28\n")
	out.write("G90\n")
	out.write("M82\n")
	out.write(";initial temps\n")
        out.write("M140 S"+str(ibedTemp)+"\n")
        out.write("M104 S"+str(iextruderTemp)+"\n")
	out.write(";waiting temps\n")
	out.write("M190 S"+str(ibedTemp)+"\n")
	out.write("G92 E0\n")
        out.write("M109 S"+str(iextruderTemp)+"\n")
	out.write(";clear the corners using y40 x20\n")
	out.write("G1 Y40 x20 F4800\n")

        out.write(r)
      else:
       out.write(r)
      i2=i2+1

#copying file back to original.
#os.rename(sys.argv[1]+"_fixed", sys.argv[1])
shutil.move(sys.argv[1]+"_fixed", sys.argv[1])
