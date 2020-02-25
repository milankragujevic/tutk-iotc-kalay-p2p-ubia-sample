rm -f *.o ./logserver *.so *.a

CC=mipsel-linux-gcc
INC=-"I. -I.. -I/root/RT288x_SDK/source/user/openssl-0.9.8e/include"
LIBS="-L/root/RT288x_SDK/source/romfs/lib -lpthread -lssl -lcrypto"
APP=logserver

$CC $CFLAGS $INC -c logserver.c $LIBS
$CC -shared -o lib$APP.so *.o ${LIBS}
$CC -o -D_TEST $APP logserver.o ${LIBS}
mipsel-linux-strip lib$APP.so

$CC $CFLAGS $INC -c logupload.c $LIBS
$CC -o logup logupload.o ${LIBS}
mipsel-linux-strip logup

#CC=gcc
#INC=-"I."

$CC $INC -D_TEST -o logserver logserver.c $LIBS
mipsel-linux-strip logserver

CC=gcc
INC=-"I."
LIBS="-lpthread -lssl -lcrypto"
$CC $CFLAGS $INC -c logupload.c $LIBS
$CC -o logtest logupload.o ${LIBS}


CC=gcc
INC=-"I."
LIBS="-lpthread -lssl -lcrypto"
$CC $CFLAGS $INC -c httpssimple.c $LIBS
$CC -o httpsClient  httpssimple.o ${LIBS}