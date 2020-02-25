
OBJS=tutk_rdt_video_server.o control_channel.o video_channel.o audio_channel.o control_channel_msg_handler.o tutk_session.o


all:${OBJS}

%.o : %.c
	$(CC) ${CFLAGS} ${INC} ${LIBS} -c -o $@ $^
	
clean:
	rm -f *.o
	
