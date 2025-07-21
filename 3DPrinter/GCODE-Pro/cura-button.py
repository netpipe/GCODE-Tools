from gi.repository import Gtk

import re
import math
import os
import sys

#from itertools import tee, izip

#check for tmp file location

#sea = re.compile("Z"+str(height)+"*")

file2=sys.argv[1] #""
#write it to os.getenv("HOME") + '/.cura/plugins
#out = open(sys.argv[1]+"_fixed", 'w')
print (file2)
print ("opening filename to store things in\n")



#with open(filename, "r") as f:
#	lines = f.readlines()
out = open( os.getenv("HOME") + "/.cura/plugins/filename", 'w')
#os.stat("file").st_size == 0
print("write file2" + file2 )
#out.write( file2+"\n" )



name=os.getenv("HOME") + '/.cura/plugins/filename'
print ( name )
with open( name ) as f:
        print("what")
        for r in f:
            print("what2")
            if r == "":
                print("no gcode file")
                #out.write(sys.argv[1])
            else:
                print('setting file')
                file=r
                out.write( file2 )
        #regexd=re.search(sea, r)
        #if regexd is not None:
        print("what3")
        out.write(sys.argv[1])
out.close()

import fcntl, sys
pid_file = os.getenv("HOME") + '/.cura/plugins/program.pid'
fp = open(pid_file, 'w')
try:
    fcntl.lockf(fp, fcntl.LOCK_EX | fcntl.LOCK_NB)
except IOError:
    # another instance is running
    sys.exit(0)

class MyDemoApp():

    def __init__(self):
        window = Gtk.Window()
        window.set_title("Process")
        window.set_default_size(500, 300)
        window.set_position(Gtk.WindowPosition.CENTER)
        window.connect('destroy', self.destroy)

        button = Gtk.Button("Ready To Process GCODE!")
        button.connect("clicked", self.button_clicked)
        window.add(button)

        window.show_all()

    def button_clicked(self, window):
        print 'Launching pproc!'
        print file2
        os.system("xterm -e "+ os.getenv("HOME") + '/.cura/plugins/GCODE-Tools/GCODE-Pro/pproc.sh ' + file2)
        #Gtk.main_quit()

    def destroy(self, window):
        Gtk.main_quit()

def main():
                #out.write(sys.argv[1])
    app = MyDemoApp()
    Gtk.main()


if __name__ == '__main__':
    main()
