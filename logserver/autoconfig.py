#!/usr/bin/python
# -*- coding:utf-8 -*-
import os, sys, threading, re

class R():
    def __init__(self):
        pass
    newline = "\n"
    subMakefile = '''all:
\t$(CC) $(CFLAGS) $(INC) -c *.c $(LIBS)
clean:
\trm -f *.o
'''
    mainMakefile = '''all:
\t$(SUBTASK)
\t$(TOPTASK)
\t$(APPTASK)
clean:
\trm -f *.o ./$(APP) *.so *.a
'''
    lib = False

class MakeTool(threading.Thread):
    def __init__(self, app = "hello", dir = ".", cc = "gcc"):
        threading.Thread.__init__(self)
        self.dir = dir
        self.app = app
        self.cc = cc
    def hasCFile(self, dirName):
        r = re.compile(".*.\.c$")
        for fileName in os.listdir(dirName):
            m = r.search(fileName)
            if m:
#                print fileName
                return True
        return False
    def run(self):
        head = "export CC=" + self.cc + R.newline
        head += "export MAKE=make -w" + R.newline
        if "mipsel-linux-gcc" == self.cc:
            head += "export INC=-I. -I.. -I/root/RT288x_SDK/source/user/openssl-0.9.8e/include" + R.newline
            head += "export LIBS=-L/root/RT288x_SDK/source/romfs/lib -lpthread -lssl -lcrypto" + R.newline
        elif "gcc" == self.cc:
            head += "export INC=-I. -I.." + R.newline
            head += "export LIBS=-lpthread -lssl -lcrypto" + R.newline
        head += "APP=" + self.app + R.newline
        if R.lib:
            predef = ""
        else:
            predef = "-D_TEST"
        head += "export CFLAGS=" + predef + R.newline
        subObjs = "SUBOBJS="
        subMakeAll = "SUBTASK="
        subClean = ""
        hasDir = False
        if not os.path.isdir(self.dir):
            return
        for root, dirs, files in os.walk(self.dir):
            for dir in dirs:
                fileName = root + "/" + dir
                if self.hasCFile(fileName):
#                    print "dirname", fileName
                    subObjs += fileName + "/*.o "
                    subMakeAll += "\t$(MAKE) -C " + fileName + R.newline
                    subClean += "\t$(MAKE) -C " + fileName + " clean" + R.newline
                    file = open(fileName + "/Makefile", "w")
                    file.write(R.subMakefile)
                    file.close()
                    hasDir = True
        info = "makefile create ok"
        if not hasDir:
            subMakeAll = ""
        if self.hasCFile(self.dir):
            head += "TOPTASK=$(CC) $(CFLAGS) $(INC) -c *.c $(LIBS)" + R.newline
            if R.lib:
                head += "APPTASK=$(CC) -shared -o lib$(APP).so *.o $(SUBOBJS) $(LIBS)" + R.newline
            else:
                head += "APPTASK=$(CC) -o $(APP) *.o $(SUBOBJS) $(LIBS)" + R.newline
        elif hasDir:
            if R.lib:
                head += "APPTASK=$(CC) -shared -o lib$(APP).so *.o $(SUBOBJS) $(LIBS)" + R.newline
            else:
                head += "APPTASK=$(CC) -o $(APP) *.o $(SUBOBJS) $(LIBS)" + R.newline
        else:
            info = "no application to make"
        s = head + subObjs + R.newline + subMakeAll + R.newline + R.mainMakefile + subClean
        print "o--main makefile"
        print s
        print "o--sub makefile"
        print R.subMakefile
        print "o--info"
        print info
        file = open(self.dir + "/Makefile", "w")
        file.write(s)
        file.close()

def help():
    s = '''
python autoconfig.py [mips|gcc]
'''
    print s

def main(argv):
    appName = os.path.basename(os.path.abspath("."))
    if len(argv) == 1:
        tool = MakeTool(appName, ".", "mipsel-linux-gcc")
        tool.run()
    elif len(argv) == 2:
        if "gcc" == argv[1]:
            tool = MakeTool(appName, ".", "gcc")
            tool.run()
        elif "mips" == argv[1] or "mipsel" == argv[1] or "mipsel-linux-gcc" == argv[1]:
            tool = MakeTool(appName, ".", "mipsel-linux-gcc")
            tool.run()
        elif "lib" == argv[1]:
            R.lib = True
            tool = MakeTool(appName, ".", "mipsel-linux-gcc")
            tool.run()
        else:
            help()
    else:
        help()
    os.system("kill -9 " + str(os.getpid())) #杀掉进程

if __name__ == "__main__":
    main(sys.argv)
