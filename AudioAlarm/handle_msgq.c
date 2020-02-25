#include <stdio.h>

#include "common/logfile.h"
#include "audioalarm_common.h"
#include "audioalarm.h"
#include "VideoAlarm/VideoAlarm.h"
#include "common/msg_queue.h"
#include "NewsChannel/NewsChannelAdapter.h"
#include "p2p/tutk_rdt_video_server.h"

int AudioAlarmSendStartResponse(ArgsStruct *self)
{
	self->s_send_process->msg_type = AUDIO_ALARM_MSG_TYPE;
	self->s_send_process->Thread_cmd =
		MSG_AUDIOALARM_P_START_AUDIO_RESPONSE;
	self->s_send_process->length = 0;
	return SendMSG_2_Process(self);
}

int AudioAlarmSendStopResponse(ArgsStruct *self)
{
	self->s_send_process->msg_type = AUDIO_ALARM_MSG_TYPE;
	self->s_send_process->Thread_cmd = MSG_AUDIOALARM_P_STOP_AUDIO_RESPONSE;
	self->s_send_process->length = 0;
	return SendMSG_2_Process(self);
}

#define APPS_AUDIO_ALARM	2
#define AUDIO_ARARM_START	1
#define AUDIO_ARARM_STOP	2
int AudioAlarmSendAlarmResponse(ArgsStruct *self)
{
	Child_Process_Msg *msgsnd = self->s_send_process;

	msgsnd->msg_type = AUDIO_ALARM_MSG_TYPE;
	msgsnd->Thread_cmd = MSG_AUDIOALARM_P_AUDIO_ALRAM_NOW;
	msgsnd->length = 6;

	memset(msgsnd->data, 0, 4);
	msgsnd->data[4] = APPS_AUDIO_ALARM;
	msgsnd->data[5] = AUDIO_ARARM_START;
	return SendMSG_2_Process(self);
}

int AudioAlarmSendAlarmEnd(ArgsStruct *self)
{
	Child_Process_Msg *msgsnd = self->s_send_process;

	msgsnd->msg_type = AUDIO_ALARM_MSG_TYPE;
	msgsnd->Thread_cmd = MSG_AUDIOALARM_P_AUDIO_ALRAM_END;
	msgsnd->length = 6;

	memset(msgsnd->data, 0, 4);
	msgsnd->data[4] = APPS_AUDIO_ALARM;
	msgsnd->data[5] = AUDIO_ARARM_STOP;
	return SendMSG_2_Process(self);
}

int AudioAlarmSendVideoAlarm(msg_container *self, int cmd)
{
	MsgData *msgp = (MsgData *)self->msgdata_snd;
	MsgData *rcvp = (MsgData *)self->msgdata_rcv;

	msgp->type = VIDEO_ALARM_MSG_TYPE;
	msgp->cmd = MSG_VA_T_ALRAM;
	msgp->len = 4;
	memcpy(msgp+1, &cmd, 4);
	return msg_queue_snd(self->rmsgid, self->msgdata_snd, sizeof(MsgData)-sizeof(int)+4);
}

int AudioAlarmSendNewsChannel(msg_container *self)
{
	MsgData *msgp = (MsgData *)self->msgdata_snd;
	MsgData *rcvp = (MsgData *)self->msgdata_rcv;

//	printf("AudioAlarmSendNewsChannel: %d\n", rcvp->len);
//	int i;
//	char *p = rcvp+1;
//	for(i = 0; i < rcvp->len; i++){
//		//printf("0x%x ", p[i]);
//		DEBUGPRINT(DEBUG_INFO, "0x%x ", p[i]);
//	}
	DEBUGPRINT(DEBUG_INFO,"\n");
	msgp->type = NEWS_CHANNEL_MSG_TYPE;
	msgp->cmd = MSG_NEWS_CHANNEL_T_NEWS_BROADCAST;
	msgp->len = rcvp->len;
	memcpy(msgp+1, rcvp+1, rcvp->len);
	return msg_queue_snd(self->rmsgid, self->msgdata_snd, sizeof(MsgData)-sizeof(int)+rcvp->len);
}



int AudioAlarmSendP2P(msg_container *self)
{
	MsgData *msgp = (MsgData *)self->msgdata_snd;
	MsgData *rcvp = (MsgData *)self->msgdata_rcv;

//	printf("AudioAlarmSendNewsChannel: %d\n", rcvp->len);
//	int i;
//	char *p = rcvp+1;
//	for(i = 0; i < rcvp->len; i++){
//		//printf("0x%x ", p[i]);
//		DEBUGPRINT(DEBUG_INFO, "0x%x ", p[i]);
//	}
	DEBUGPRINT(DEBUG_INFO,"\n");
	msgp->type = P2P_MSG_TYPE;
	msgp->cmd = MSG_P2P_T_ALARMEVENT;
	msgp->len = rcvp->len;
	memcpy(msgp+1, rcvp+1, rcvp->len);
	return msg_queue_snd(self->rmsgid, self->msgdata_snd, sizeof(MsgData)-sizeof(int)+rcvp->len);
}

