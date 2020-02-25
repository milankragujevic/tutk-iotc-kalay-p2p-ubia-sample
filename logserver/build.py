#!/usr/bin/python
import os, sys

def build(cmd):
    if 0 == os.system(cmd):
        print cmd
        print "-----------<ok>"
    else:
        print cmd
        print "----------<error>"

def main(argv):
    inc = "-I. -I.. -I/root/RT288x_SDK/source/user/openssl-0.9.8e/include " 
    libs = "-L/root/RT288x_SDK/source/romfs/lib -lpthread -lssl -lcrypto "
    logservercmd = "mipsel-linux-gcc " + inc + " -shared -o liblogserver.so logserver.c " + libs
    logserverstripcmd = "mipsel-linux-strip liblogserver.so"
    build("sudo rm liblogserver.so")
    build("sudo " + logservercmd)
    build("sudo " + logserverstripcmd)
    build("sudo cp liblogserver.so /home/a/tomato/RT288x_SDK/source/romfs/lib")

if __name__ == "__main__":
    main(sys.argv)
