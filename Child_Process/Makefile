OBJS=child_process.o 
#OBJS += childProcess.o cmbtable.o TimeFunc.o child_process.o

all:${OBJS}

%.o : %.c
	$(CC) ${CFLAGS} ${INC} ${LIBS} -c -o $@ $^
	
clean:
	rm -f *.o
	
