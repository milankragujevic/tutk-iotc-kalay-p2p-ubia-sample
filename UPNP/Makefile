#OBJS= upnp_thread.o upnp_adapter.o upnp_protocol.o 
OBJS=$(patsubst %.c, %.o, $(wildcard *.c))

all:${OBJS}

%.o : %.c
	$(CC) ${CFLAGS} ${INC} ${LIBS} -c -o $@ $^
	
clean:
	rm -f *.o
	
