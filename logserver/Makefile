export CC=mipsel-linux-gcc
export MAKE=make -w
export INC=-I. -I.. -I/root/RT288x_SDK/source/user/openssl-0.9.8e/include
export LIBS=-L/root/RT288x_SDK/source/romfs/lib -lpthread -lssl -lcrypto
APP=logserver
export CFLAGS=
TOPTASK=$(CC) $(CFLAGS) $(INC) -c *.c $(LIBS)
APPTASK=$(CC) -shared -o lib$(APP).so *.o $(SUBOBJS) $(LIBS)
SUBOBJS=

all:
	$(SUBTASK)
	$(TOPTASK)
	$(APPTASK)
clean:
	rm -f *.o ./$(APP) *.so *.a
