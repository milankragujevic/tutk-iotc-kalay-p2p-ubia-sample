#OBJS=udpServer.o 
OBJS=$(patsubst %.c, %.o, $(wildcard *.c))

all:${OBJS}

%.o : %.c
	$(CC) ${CFLAGS} ${INC} ${LIBS} -c -o $@ $^
	
clean:
	rm -f *.o
	
