OBJS=common.o thread.o msg_queue.o logfile.o common_config.o time_func.o common_func.o
OBJS += utils.o
OBJS += pingProcess.o

all:${OBJS}

%.o : %.c
	$(CC) ${CFLAGS} ${INC} ${LIBS} -c -o $@ $^
	
clean:
	rm -f *.o
	
