/*
 * VideoAdapter.c
 *
 *  Created on: Mar 22, 2013
 *      Author: root
 */
#include "common/utils.h"
#include "Video/VideoAdapter.h"
#include "common/common.h"
#include "common/common_func.h"
#include "common/msg_queue.h"
#include "common/logfile.h"
#include "Video/video.h"
#include "IOCtl/ioctl.h"
#include "Audio/Audio.h"

volatile int iOpenTimeTest = 0;

volatile int iReswitchTimes = 0;

/*
 * @brief 视频线程开始采集的返回消息处理
 * @author:张历
 * @param[in]:<self><接受到的消息>
 * @param[out]:<没有参数>
 * @Return <NULL>
 */
void ReceivedStartVideoCaptureMsgRepply(msg_container *self);

/*
 * @brief 视频线程停止采集的返回消息处理
 * @author:张历
 * @param[in]:<self><接受到的消息>
 * @param[out]:<没有参数>
 * @Return <NULL>
 */
void ReceivedStopVideoCaptureMsgRepply(msg_container *self);

/*
 * @brief 视频线程设置参数的返回消息处理
 * @author:张历
 * @param[in]:<self><接受到的消息>
 * @param[out]:<没有参数>
 * @Return <NULL>
 */
void ReceicedSettingVideoMsgRepply(msg_container *self);

/*
 * @brief 视频线程重新切换USB的消息处理
 * @author:张历
 * @param[in]:<self><接受到的消息>
 * @param[out]:<没有参数>
 * @Return <NULL>
 */
void ReceicedVideoReswitchUSBCMD(msg_container *self);

/*
 * @brief 视频线程重启的消息处理
 * @author:张历
 * @param[in]:<self><接受到的消息>
 * @param[out]:<没有参数>
 * @Return <NULL>
 */
void ReceicedVideoNeedRebootCMD(msg_container *self);

cmd_item VIDEO_CMD_TABLE[] = {
		{MSG_VIDEO_T_START_CAPTURE ,ReceivedStartVideoCaptureMsgRepply},
		{MSG_VIDEO_T_STOP_CAPTURE ,ReceivedStopVideoCaptureMsgRepply},
		{MSG_VIDEO_T_SETTING_PARAMS ,ReceicedSettingVideoMsgRepply},
		{MSG_VIDEO_T_RESWITCH_USB, ReceicedVideoReswitchUSBCMD},
		{MSG_VIDEO_T_NEED_REBOOT,ReceicedVideoNeedRebootCMD},
};

void videoCaptureCMDTableFun(msg_container *self){
	MsgData *msgrcv = (MsgData *)self->msgdata_rcv;
	cmd_item icmd;
	icmd.cmd = msgrcv->cmd;
	DEBUGPRINT(DEBUG_INFO,"==========cmd==========================%d",msgrcv->cmd);
	msg_fun fun = utils_cmd_bsearch(&icmd, VIDEO_CMD_TABLE, sizeof(VIDEO_CMD_TABLE)/sizeof(cmd_item));
	if(fun != NULL) {
		fun(self);
	}
}

void ReceivedStartVideoCaptureMsgRepply(msg_container *self)
{
	struct HandleVideoMsg *repMsg = (struct HandleVideoMsg *)self->msgdata_rcv;
	switch(repMsg->data[0]){
	case 0:{
		//Success !!! Tell other thread that they can work now
		int i=0;
		MsgData *sndData = (MsgData *)self->msgdata_snd;
		sndData->len = 0;
		sndData->cmd = CMD_AV_STARTED;//CMD_AV_STARTED;
		sndData->type = AUDIO_MSG_TYPE;
		msg_queue_snd(self->rmsgid, sndData, sizeof(MsgData) - sizeof(long) + sndData->len);
		for(i = CHILD_PROCESS_MSG_TYPE+1; i < LAST_TYPE; i++)
		{
	    	if (TRUE == if_not_use_thread(i))
	    	{
	    		continue;
	    	}
			sndData->type = i;
			msg_queue_snd(self->rmsgid, sndData, sizeof(MsgData) - sizeof(long) + sndData->len);
		}
		iReswitchTimes = 0;//reset when open success
//		iOpenTimeTest++;
//		DEBUGPRINT(DEBUG_INFO,"********Child Process Received Video Start Capture Success Msg======iOpenTimeTest===%d\n",iOpenTimeTest);
//		usleep(1500000);
//		MsgData *sndData = self->msgdata_snd;
//		sndData->len = 0;
//		sndData->cmd = 2;//CMD_AV_STARTED;
//		sndData->type = AUDIO_MSG_TYPE;
//		int iRet = msg_queue_snd(self->rmsgid, sndData, sizeof(MsgData) - sizeof(long) + sndData->len);
//		if(iRet<0){
//			printf("+++++++++++++++++++++++++Send Audio Stop Failed++++++++++++++++++++++++++\n");
//		}
	}
		break;
	case -1:{
		//Failed
		DEBUGPRINT(DEBUG_ERROR,"********Child Process Received Video Open Device Failed Msg\n");
	}
		break;
	case -2:{
		//Failed
		DEBUGPRINT(DEBUG_ERROR,"********Child Process Received  Video Init Device Failed Msg\n");
		}
			break;
	case -3:{
		//Failed
		DEBUGPRINT(DEBUG_ERROR,"********Child Process Received Video MMAP Failed Msg\n");
		}
			break;
	case -4:{
		//Failed
		DEBUGPRINT(DEBUG_ERROR,"********Child Process Received Video In Queue Failed Msg\n");
		}
			break;
	case -5:{
		//Failed
		DEBUGPRINT(DEBUG_ERROR,"********Child Process Received Video On Capture Failed Msg\n");
		}
			break;
	default:{
		//Unknown
		DEBUGPRINT(DEBUG_ERROR,"********Child Process Received Video Start Unknown Means=%d\n",repMsg->data[0]);
	}
		break;

	}
}
void ReceivedStopVideoCaptureMsgRepply(msg_container *self)
{
	struct HandleVideoMsg *repMsg = (struct HandleVideoMsg *)self->msgdata_rcv;
	switch(repMsg->data[0]){
		case 0:{
			//Success and Set Flag
			printf("ready to g_lAudioStopFlag $$$$$$$$$$$$$$$$$$$\n");
			g_lAudioStopFlag = 1;
//			MsgData *sndData = self->msgdata_snd;
//			sndData->len = 0;
//			sndData->cmd = 2;//CMD_AV_STARTED;
//			sndData->type = AUDIO_MSG_TYPE;
//			msg_queue_snd(self->rmsgid, sndData, sizeof(MsgData) - sizeof(long) + sndData->len);
//			DEBUGPRINT(DEBUG_INFO,"********Child Process Received Video Stop Capture Success Msg\n");
//			usleep(500000);
//			MsgData *sndData = self->msgdata_snd;
//			sndData->len = 0;
//			sndData->cmd = 1;//CMD_AV_STARTED;
//			sndData->type = AUDIO_MSG_TYPE;
//			int iRet = msg_queue_snd(self->rmsgid, sndData, sizeof(MsgData) - sizeof(long) + sndData->len);
//			if(iRet<0){
//				printf("+++++++++++++++++++++++++Send Audio Start Failed++++++++++++++++++++++++++\n");
//			}

		}
			break;
		case -1:{
			//Failed
			g_lAudioStopFlag = 2;
			DEBUGPRINT(DEBUG_ERROR,"********Child Process Received Video OFF Capture Failed Msg\n");
		}
			break;
		case -2:{
			//Failed
			g_lAudioStopFlag = 2;
			DEBUGPRINT(DEBUG_ERROR,"********Child Process Received  Video UMMAP Failed Msg\n");
			}
				break;
		case -3:{
			//Failed
			g_lAudioStopFlag = 2;
			DEBUGPRINT(DEBUG_ERROR,"********Child Process Received Video Close Device Failed Msg\n");
			}
				break;
		default:{
			//Unknown
			g_lAudioStopFlag = 2;
			DEBUGPRINT(DEBUG_ERROR,"********Child Process Received Video STOP Unknown Means=%d\n",repMsg->data[0]);
		}
			break;

	}
}
void ReceicedSettingVideoMsgRepply(msg_container *self)
{
	struct HandleVideoMsg *repMsg = (struct HandleVideoMsg *)self->msgdata_rcv;
		switch(repMsg->data[0]){
		case 0:{
			//Success
			DEBUGPRINT(DEBUG_INFO,"Child Process Received Video Setting Device Success Msg\n");
		}
			break;
		case -1:{
			//Failed
			DEBUGPRINT(DEBUG_ERROR,"Child Process Received Video Setting Device Failed Msg\n");
		}
			break;
		default:{
			//Unknown
			DEBUGPRINT(DEBUG_ERROR,"Child Process Received Video Start Unknown Means=%d\n",repMsg->data[0]);
		}
			break;

		}
}

void ReceicedVideoReswitchUSBCMD(msg_container *self){
	struct HandleVideoMsg *sndMsg=self->msgdata_snd;
	sndMsg->type = AUDIO_MSG_TYPE;
	sndMsg->len = 0;
	sndMsg->cmd =MSG_AUDIO_P_STOP_CAPTURE;
	msg_queue_snd(self->rmsgid,sndMsg,sizeof(MsgData)-sizeof(long int)+sndMsg->len);
}

void ReceicedVideoNeedRebootCMD(msg_container *self){
	struct HandleVideoMsg *sndMsg=self->msgdata_snd;
	sndMsg->type = PLAY_BACK_MSG_TYPE;
	sndMsg->len = 0;
	sndMsg->cmd =0xff;
	msg_queue_snd(self->rmsgid,sndMsg,sizeof(MsgData)-sizeof(long int)+sndMsg->len);
	DEBUGPRINT(DEBUG_INFO, "========================Video reboot========================\n");
	DEBUGPRINT(DEBUG_INFO, "========================Video reboot========================\n");
	DEBUGPRINT(DEBUG_INFO, "========================Video reboot========================\n");
	DEBUGPRINT(DEBUG_INFO, "========================Video reboot========================\n");
	DEBUGPRINT(DEBUG_INFO, "========================Video reboot========================\n");
	DEBUGPRINT(DEBUG_INFO, "========================Video reboot========================\n");
}

void VideoAdapter_cmd_table_fun(msg_container *self){
	MsgData *msgrcv = (MsgData *)self->msgdata_rcv;
	cmd_item icmd;
	icmd.cmd = msgrcv->cmd;
	DEBUGPRINT(DEBUG_INFO,"==========cmd========cmd==================%d",msgrcv->cmd);
	msg_fun fun = utils_cmd_bsearch(&icmd, VIDEO_CMD_TABLE, sizeof(VIDEO_CMD_TABLE)/sizeof(cmd_item));
	if(fun != NULL) {
		fun(self);
	}
	else {
		DEBUGPRINT(DEBUG_INFO, "%s: cannot find command\n", __FUNCTION__);
	}
}

