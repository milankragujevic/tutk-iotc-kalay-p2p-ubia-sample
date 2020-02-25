OBJS=MFI.o 
OBJS += MFIAdapter.o
OBJS += android_usb.o
all:${OBJS}

%.o : %.c
	$(CC) ${CFLAGS} -D_ANDROID_USB_IMP ${INC} ${LIBS} -c -o $@ $^
	
clean:
	rm -f *.o
	
