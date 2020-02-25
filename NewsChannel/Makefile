OBJS = NewsChannel.o NewsChannelAdapter.o NewsAdapter.o
OBJS += NewsFilter.o
OBJS += NewsUtils.o
OBJS += NewsWork.o
OBJS += BellService.o

PRE_DEFINES = -D_FAST_BLINK_SWITCH=0 #{on: 1, off, 0}

all:${OBJS}

%.o : %.c
	$(CC) ${CFLAGS} $(PRE_DEFINES) ${INC} ${LIBS} -c -o $@ $^
	
clean:
	rm -f *.o
	
