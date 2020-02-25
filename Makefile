MAKE_FILE_VERSION=2
export CC=mipsel-linux-gcc
MAKE=make -w
CP=cp -rf

DFLAGS=
#DFLAGS += -D_DISABLE_VIDEO_SEND
#DFLAGS += -D_DISABLE_AUDIO_SEND
#DFLAGS += -D_DISABLE_SPEAKER
#DFLAGS += -D_DISABLE_AUDIO_ALARM
#DFLAGS += -D_DISABLE_VIDEO_ALARM
#DFLAGS += -D_DISABLE_NETSET
#DFLAGS += -D_DISABLE_SERIALPORT
DFLAGS += -D_DISABLE_OLD_ALG
DFLAGS += -D_DISABLE_UPNP
#DFLAGS += -DFOR_APPLE_TEST
#DFLAGS += -DHttp_LOG_ON # if defined http log will switch on
#DFLAGS += -D_DEBUG_TEST
#DFLAGS += -D_RECOARD_TEST_

export CFLAGS=-O2 -Wall
CFLAGS += -D_DEBUG_LEVER=0
CFLAGS += -D_DEBUG_OPT=0 #{0: none, 1: printf, 2: udp, 3: printf and udp}
CFLAGS += -D_UDP_VERSION=2 #{1: without upgrade,2: with upgrade}
CFLAGS += -D_ITHREAD=1
CFLAGS += -D_TCP_VERSION=2 #{1: old, 2: changed}
CFLAGS += -D_TUTK_VERSION=1 #{0: disable,1: enable}
CFLAGS += -D_MSG_QUEUE_VERSION=2
CFLAGS += -DM3S_New=1 #{0: USA, 1: JAPAN}
CFLAGS += -D_JOIN_VERIFY=1 #{0: Off, 1: On}
CFLAGS += -D_GUARD_MODE=1 #{0: Off, 1: On}
CFLAGS += -D__FACTORY_TEST=1 #{0: Off, 1: On}
CFLAGS += -D_POPEN_REDEF=1 #{0: Off, 1: On}
#CFLAGS += -D_ADT_AUDIO
CFLAGS += $(DFLAGS)

sdk_source=/root/m3s_tj_sdk/source

export INC=-I${sdk_source}/linux-2.6.21.x/include
INC += -I${sdk_source}/lib/include
INC += -I.
INC += -I..
INC += -I${sdk_source}/user/openssl-0.9.8e/include
INC += -I../alsa/include
INC += -I../jpeg-6b

export LIBS=-lpthread
LIBS += -L./lib
LIBS += -lm -lcrypto -lssl -lnvram -ljpeg
LIBS += -lasound -lstdc++ -lIOTCAPIs -lRDTAPIs
LIBS += -lminiupnpc -lasound
LIBS += -lfftw3
LIBS += -ljansson
LIBS += -llogserver
LIBS += -lusb
LIBS += -ladtaudio

common=common
Audio=Audio
AudioAlarm=AudioAlarm
AudioSend=AudioSend
FlashRW=FlashRW
HTTPServer=HTTPServer
MFI=MFI
NetWorkSet=NetWorkSet
NewsChannel=NewsChannel
NTP=NTP
playback=playback
SerialPorts=SerialPorts
Tooling=Tooling
Udpserver=Udpserver
Upgrade=Upgrade
UPNP=UPNP
Video=Video
VideoAlarm=VideoAlarm
VideoSend=VideoSend
IOCtl=IOCtl
kernel_code=kernel_code
Child_Process=Child_Process
Hardware=Hardware
p2p=p2p

OBJS=iCamera.o
TARGET=iCamera
SUBOBJS=${common}/*.o
SUBOBJS += ${Audio}/*.o
SUBOBJS += ${AudioAlarm}/*.o
SUBOBJS += ${AudioSend}/*.o
SUBOBJS += ${FlashRW}/*.o
SUBOBJS += ${HTTPServer}/*.o
SUBOBJS += ${MFI}/*.o
SUBOBJS += ${NetWorkSet}/*.o
SUBOBJS += ${NewsChannel}/*.o
SUBOBJS += ${NTP}/*.o
SUBOBJS += ${playback}/*.o
SUBOBJS += ${SerialPorts}/*.o
SUBOBJS += ${Tooling}/*.o
SUBOBJS += ${Udpserver}/*.o
SUBOBJS += #${Upgrade}/*.o
SUBOBJS += ${UPNP}/*.o
SUBOBJS += ${Video}/*.o
SUBOBJS += ${VideoAlarm}/*.o
SUBOBJS += ${VideoSend}/*.o
SUBOBJS += ${IOCtl}/*.o
SUBOBJS += ${kernel_code}/*.o
SUBOBJS += ${Child_Process}/*.o
SUBOBJS += ${p2p}/*.o
SUBOBJS += ${Hardware}/*.o

all:$(OBJS)
	$(MAKE) -C ${common}
	$(MAKE) -C ${Audio}
	$(MAKE) -C ${AudioAlarm}
	$(MAKE) -C ${AudioSend}
	$(MAKE) -C ${FlashRW}
	$(MAKE) -C ${HTTPServer}
	$(MAKE) -C ${MFI}
	$(MAKE) -C ${NetWorkSet}
	$(MAKE) -C ${NewsChannel}
	$(MAKE) -C ${NTP}
	$(MAKE) -C ${playback}
	$(MAKE) -C ${SerialPorts}
	$(MAKE) -C ${Tooling}
	$(MAKE) -C ${Udpserver}
	$(MAKE) -C ${Upgrade}
	$(MAKE) -C ${UPNP}
	$(MAKE) -C ${Video}
	$(MAKE) -C ${VideoAlarm}
	$(MAKE) -C ${VideoSend}
	$(MAKE) -C ${IOCtl}	
	$(MAKE) -C ${kernel_code}
	$(MAKE) -C ${Child_Process}
	$(MAKE) -C ${p2p}
	$(MAKE) -C ${Hardware}
	$(CC) -o ${TARGET} $^ ${SUBOBJS} ${LIBS}
	cp ${TARGET} /root/sharednfs/
	rm -f ${sdk_source}/romfs/sbin/${TARGET}
	cp ${TARGET} ${sdk_source}/vendors/Ralink/RT5350/

%.o:%.c
	$(CC) -c ${CFLAGS} ${INC} -o $@ $^

clean:
	rm -f ${TARGET} *.o
	$(MAKE) -C ${common} clean
	$(MAKE) -C ${Audio} clean
	$(MAKE) -C ${AudioAlarm} clean
	$(MAKE) -C ${AudioSend} clean
	$(MAKE) -C ${FlashRW} clean
	$(MAKE) -C ${HTTPServer} clean
	$(MAKE) -C ${MFI} clean
	$(MAKE) -C ${NetWorkSet} clean
	$(MAKE) -C ${NewsChannel} clean
	$(MAKE) -C ${NTP} clean
	$(MAKE) -C ${playback} clean
	$(MAKE) -C ${SerialPorts} clean
	$(MAKE) -C ${Tooling} clean
	$(MAKE) -C ${Udpserver} clean
	$(MAKE) -C ${Upgrade} clean
	$(MAKE) -C ${UPNP} clean
	$(MAKE) -C ${Video} clean
	$(MAKE) -C ${VideoAlarm} clean
	$(MAKE) -C ${VideoSend} clean
	$(MAKE) -C ${IOCtl} clean	
	$(MAKE) -C ${kernel_code} clean
	$(MAKE) -C ${Child_Process} clean
	$(MAKE) -C ${p2p} clean
	$(MAKE) -C ${Hardware} clean
