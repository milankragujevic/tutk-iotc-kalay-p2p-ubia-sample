#OBJS=audioalarm.o handle_audio.o handle_msgq.o save_audio_file.o testcommon.o
OBJS=$(patsubst %.c, %.o, $(wildcard *.c))

all:${OBJS}

%.o : %.c
	$(CC) ${CFLAGS} ${INC} ${LIBS} -c -o $@ $^
	
clean:
	rm -f *.o
	
