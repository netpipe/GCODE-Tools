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

#Initial Temperature
ibedTemp=60
iextruderTemp=230

#Temp At Z
height=2.24 #try and get a value that matches the layers you use otherwise it may not find it.
sea = re.compile("Z"+str(height)+"*")
bedTemp=50
extruderTemp=225

#finishing Temps
fbedTemp=0
#fextruderTemp=225

# percent finished - turn off bed
fpercent=0.80



print ("Tweak At Z\n")
#sea = re.compile("M106 S[1-9]+[0-9]*")
print (sys.argv[1]+"\n")
#rep = re.compile("M106 S255\n\g<0>")
out = open(sys.argv[1]+"_fixed", 'w')


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
      elif re.search(re.compile("G28*"), r) is not None:
        print ("Found Homing Command - Inserting initial warmup.\n")
        out.write("M140 S"+str(ibedTemp)+"\n")
        out.write("M104 S"+str(iextruderTemp)+"\n")
        out.write(r)
      else:
       out.write(r)
      i2=i2+1

#os.rename(sys.argv[1]+"_fixed", sys.argv[1])
shutil.move(sys.argv[1]+"_fixed", sys.argv[1])
