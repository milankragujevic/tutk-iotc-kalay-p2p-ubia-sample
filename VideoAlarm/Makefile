OBJS = VideoAlarm.o
OBJS += VideoAlarmAdapter.o
OBJS += jpegdecode.o


all:${OBJS}

%.o : %.c
	$(CC) ${CFLAGS} ${INC} ${LIBS} -c -o $@ $^
	
clean:
	rm -f *.o
	
