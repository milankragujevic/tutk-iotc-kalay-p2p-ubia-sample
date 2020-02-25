/*
 * BellService.c
 *
 *  Created on: Nov 27, 2013
 *      Author: root
 */
#include "common/logfile.h"
#include "common/common.h"
#include "common/thread.h"
#include "common/msg_queue.h"
#include "VideoAlarm/VideoAlarm.h"
#include "Udpserver/udpServer.h"
#include <fcntl.h>

#define ON  1
#define OFF 0
#define RD  0
#define SET 1
#define SLEEP_TIME 50
#define BELL_DEV_NAME  "/proc/Bell"
enum {
	MODE,
	BUTTON,
	INFRARED,
	LED,
};

enum {
	NULL_ACT,
	INFRARED_ACT,
};

typedef struct BellService_t {
	int rMsgId;
	int lMsgId;
	char IOBuf[4];
	int bellDevFd;
	time_t tm;
	int timerAction;
}BellService_t;

BellService_t *bellService_t;

volatile char bellAlarmInfo[4] = {0, 0, 0, 0};

int BellServiceThreadStart();
void* BellServiceThreadRun(void* arg);
int BellServiceThreadInit(BellService_t *self);
int BellServiceThreadExit(BellService_t *self);

void BellServiceIOHandler(BellService_t *self);
void BellServiceButtonDownAction(BellService_t *self);
void BellServiceInfraredAction(BellService_t *self);
void BellServiceTimerHandler(BellService_t *self);


int BellServiceThreadStart() {
	thread_start(BellServiceThreadRun);
	return 0;
}

void* BellServiceThreadRun(void* arg) {
	bellService_t = malloc(sizeof(BellService_t));
	if(NULL == bellService_t) {
		DEBUGPRINT(DEBUG_INFO, errstr);
		return 0;
	}
	BellService_t *self = bellService_t;
	if(-1 == BellServiceThreadInit(self)) {
		DEBUGPRINT(DEBUG_INFO, errstr);
		return 0;
	}
	while(1) {
		BellServiceIOHandler(self);
//		BellServiceTimerHandler(self);
		thread_usleep(SLEEP_TIME);
	}
	return 0;
}

int BellServiceThreadInit(BellService_t *self) {
	bzero(self, sizeof(BellService_t));

	if(NewsChannel_msgQueueFdInit(&self->lMsgId, CHILD_THREAD_MSG_KEY) == -1) {
			return -1;
	}
	if(NewsChannel_msgQueueFdInit(&self->rMsgId, CHILD_PROCESS_MSG_KEY) == -1) {
		return -1;
	}
	self->bellDevFd = open(BELL_DEV_NAME, O_RDONLY);
	if(-1 == self->bellDevFd) {
		DEBUGPRINT(DEBUG_INFO, "open error\n");
		return -1;
	}
	self->IOBuf[0] = SET;
	self->IOBuf[3] = OFF;
	read(self->bellDevFd, self->IOBuf, 4);
	self->IOBuf[0] = RD;
	return 0;
}

int BellServiceThreadExit(BellService_t *self) {
	return 0;
}


void BellServiceIOHandler(BellService_t *self) {
	static char preInfraRedStatus = OFF;
	static char preButtonStatus = OFF;

	read(self->bellDevFd, self->IOBuf, 4);
//	DEBUGPRINT(DEBUG_INFO, "Button OFF\n");
	if(preButtonStatus != self->IOBuf[1]) {
		preButtonStatus = self->IOBuf[1];
		if(ON == self->IOBuf[1]) {
			BellServiceButtonDownAction(self);
			DEBUGPRINT(DEBUG_INFO, "Button ON\n");
		}
	}
#if 0
	if(preInfraRedStatus != self->IOBuf[2]) {
		preInfraRedStatus = self->IOBuf[2];
		if(ON == self->IOBuf[2]) {
			BellServiceInfraredAction(self);
			DEBUGPRINT(DEBUG_INFO, "InfraRed ON\n");
		}
	}
#endif
}

void BellServiceButtonDownAction(BellService_t *self){
	DEBUGPRINT(DEBUG_INFO, "BellServiceButtonDownAction\n");
	FILE *file = fopen("/usr/share/btnAlarm", "a+");
	if(NULL != file) {
		fprintf(file, "start time: %d\n", time(NULL));
		fclose(file);
	}
//	static i = 0;
//	if(i > 250) {
//		i = 0;
//		FILE *file = fopen("/usr/share/btnAlarm", "w");
//		if(NULL != file) {
//			fprintf(file, "start time: %d", time(NULL));
//			fclose(file);
//		}
//	} else {
//		FILE *file = fopen("/usr/share/btnAlarm", "a+");
//		if(NULL != file) {
//			fprintf(file, "start time: %d", time(NULL));
//			fclose(file);
//		}
//		++i;
//	}
	char msgBuf[256];
	MsgData *msg = (MsgData *)msgBuf;

	msg->type = VIDEO_ALARM_MSG_TYPE;
	msg->cmd = MSG_VA_T_ALRAM;
	msg->len = 4;
//	msgBuf[sizeof(MsgData) + 1] = 3;
	*(int *)(msg + 1) = 3;
//	*(int*)(msgBuf[sizeof(MsgData)]) = 2;
	bellAlarmInfo[0] = 1;
	msg_queue_snd(self->lMsgId, msg, sizeof(MsgData) - sizeof(long) + msg->len);

#if 0
	msg->type = UDP_SERVER_MSG_TYPE;
	msg->cmd = MSG_UDPSERVER_T_BELL_ALARM_ACTION;
	msg->len = 2;
	if(ON == bellInfo_t.bellSoundSwitch) {
		msgBuf[sizeof(MsgData) + 1] = 1; //yes
	}
	else {
		msgBuf[sizeof(MsgData) + 1] = 2; //no
	}
	msgBuf[sizeof(MsgData)] = BUTTON;         //button alarm
	msg_queue_snd(self->lMsgId, msg, sizeof(MsgData) - sizeof(long) + msg->len);
#endif
	return;
#if 0
	if(ON == bellInfo_t.bellSoundSwitch) {
		//send message to udp, https
		char msgBuf[128];
		MsgData *msg = (MsgData *)msgBuf;

		msg->type = VIDEO_ALARM_MSG_TYPE;
		msg->cmd = MSG_VA_T_ALRAM;
		msg->len = 4;
		msgBuf[sizeof(MsgData) + 1] = 3;
		msg_queue_snd(self->lMsgId, msg, sizeof(MsgData) - sizeof(long) + msg->len);


		msg->type = UDP_SERVER_MSG_TYPE;
		msg->cmd = MSG_UDPSERVER_T_BELL_ALARM_ACTION;
		msg->len = 2;
		msgBuf[sizeof(MsgData) + 1] = 1; //yes
		msgBuf[sizeof(MsgData)] = BUTTON;     //infrared alarm
		msg_queue_snd(self->lMsgId, msg, sizeof(MsgData) - sizeof(long) + msg->len);
	}
	else {
		//send message to udp, http
	}
#endif
}

#if 0
void BellServiceInfraredAction(BellService_t *self) {
	if(ON == bellInfo_t.infraredSwitch) {
		//send message to udp, http
		char msgBuf[128];
		MsgData *msg = (MsgData *)msgBuf;

		msg->type = VIDEO_ALARM_MSG_TYPE;
		msg->cmd = MSG_VA_T_ALRAM;
		msg->len = 4;
		msgBuf[sizeof(MsgData) + 1] = 4;
		bellAlarmInfo[0] = 2;
		msg_queue_snd(self->lMsgId, msg, sizeof(MsgData) - sizeof(long) + msg->len);

		msg->type = UDP_SERVER_MSG_TYPE;
		msg->cmd = MSG_UDPSERVER_T_BELL_ALARM_ACTION;
		msg->len = 2;
		if(ON == bellInfo_t.bellSoundSwitch) {
			msgBuf[sizeof(MsgData) + 1] = 1; //yes
		}
		else {
			msgBuf[sizeof(MsgData) + 1] = 2; //no
		}
		msgBuf[sizeof(MsgData)] = INFRARED;         //infrared alarm
		msg_queue_snd(self->lMsgId, msg, sizeof(MsgData) - sizeof(long) + msg->len);

		self->timerAction = INFRARED_ACT;
	}
	else {
		//do nothing
	}
}
#endif

#if 0
void BellServiceTimerHandler(BellService_t *self) {
	static time_t mTime = 0;
	static int flag = 0;
	time(&self->tm);
	if(INFRARED_ACT == self->timerAction) {
		if(0 == flag) {
			time(&mTime);
			flag = 1;
			self->IOBuf[0] = SET;
			self->IOBuf[3] = ON;
			read(self->bellDevFd, self->IOBuf, 4);
			self->IOBuf[0] = RD;
		}
		else {
			if(self->tm - mTime > 1) {
				self->IOBuf[0] = SET;
				self->IOBuf[3] = OFF;
				read(self->bellDevFd, self->IOBuf, 4);
				self->IOBuf[0] = RD;
				self->timerAction = NULL_ACT;
				flag = 0;
			}
		}
	}
}
#endif
