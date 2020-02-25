#OBJS=httpsserver.o do_https_m3s.o adapter_httpserver.o encryption.o tools.o 
OBJS=$(patsubst %.c, %.o, $(wildcard *.c))
all:${OBJS}

%.o : %.c
	$(CC) ${CFLAGS} ${INC} ${LIBS} -c -o $@ $^
	
clean:
	rm -f *.o
	
