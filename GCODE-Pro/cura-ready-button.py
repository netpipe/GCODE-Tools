from gi.repository import Gtk
import os


#check for tmp file location

#sea = re.compile("Z"+str(height)+"*")
file=""
#write it to os.getenv("HOME") + '/.cura/plugins

        #regexd=re.search(sea, r)
        #if regexd is not None:

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
        os.system("xterm -e "+ os.getenv("HOME") + '/.cura/plugins/GCODE-Tools/GCODE-Pro/pproc.sh ' + file)

    def destroy(self, window):
        Gtk.main_quit()

def main():
    with open(os.getenv("HOME") + '/.cura/plugins/test') as f:
        for r in f:
            if r == "":
                print("no gcode file string")
                r.write(sys.argv[1])
            else:
                print('setting file')
                file=r
    app = MyDemoApp()
    Gtk.main()


if __name__ == '__main__':
    main()
