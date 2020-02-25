/*
 * AudioAdapter.c
 *
 *  Created on: Mar 22, 2013
 *      Author: root
 */
#include "common/utils.h"
#include "Audio/AudioAdapter.h"
#include "Audio/Audio.h"
#include "common/common.h"
#include "common/msg_queue.h"
#include "common/logfile.h"
#include "Video/video.h"
#include "IOCtl/ioctl.h"
#include "Video/VideoAdapter.h"
#include "common/common_config.h"

#define RESWITCH_TIME 3

extern volatile int iVideoOpenFailedFlag;

/*
 * @brief 音频线程开始采集的返回消息处理
 * @author:张历
 * @param[in]:<self><接受到的消息>
 * @param[out]:<没有参数>
 * @Return <NULL>
 */
void ReceivedAudioStartCaptureMsgReply(msg_container *self);
/*
 * @brief 音频线程停止采集的返回消息处理
 * @author:张历
 * @param[in]:<self><接受到的消息>
 * @param[out]:<没有参数>
 * @Return <NULL>
 */
void ReceivedAudioStopCaptureMsgReply(msg_container *self);
/*
 * @brief 音频线程开始压缩的返回消息处理
 * @author:张历
 * @param[in]:<self><接受到的消息>
 * @param[out]:<没有参数>
 * @Return <NULL>
 */
void ReceivedAudioStartEncodeMsgReply(msg_container *self);
/*
 * @brief 音频线程停止压缩的返回消息处理
 * @author:张历
 * @param[in]:<self><接受到的消息>
 * @param[out]:<没有参数>
 * @Return <NULL>
 */
void ReceivedAudioStopEncodeMsgReply(msg_container *self);
/*
 * @brief 音频线程重新切换USB消息处理
 * @author:张历
 * @param[in]:<self><接受到的消息>
 * @param[out]:<没有参数>
 * @Return <NULL>
 */
void ReceivedAudioReswitchUSBCMD(msg_container *self);

/*
 * @brief 音频线程重启消息处理
 * @author:张历
 * @param[in]:<self><接受到的消息>
 * @param[out]:<没有参数>
 * @Return <NULL>
 */
void ReceivedAudioNeedRebootCMDD(msg_container *self);

cmd_item AUDIO_CMD_TABLE[] = {
		{MSG_AUDIO_T_START_CAPTURE,ReceivedAudioStartCaptureMsgReply},
		{MSG_AUDIO_T_STOP_CAPTURE,ReceivedAudioStopCaptureMsgReply},
		{MSG_AUDIO_T_START_ENCODE,ReceivedAudioStartEncodeMsgReply},
		{MSG_AUDIO_T_STOP_ENCODE,ReceivedAudioStopEncodeMsgReply},
		{MSG_AUDIO_T_RESWITCH_USB,ReceivedAudioReswitchUSBCMD},
		{MSG_AUDIO_T_AUDIO_NEED_REBOOT,ReceivedAudioNeedRebootCMDD},
};

void AudioAdapter_cmd_table_fun(msg_container *self){
	printf("============child process received msg=======\n");
	MsgData *msgrcv = (MsgData *)self->msgdata_rcv;
	cmd_item icmd;
	icmd.cmd = msgrcv->cmd;
	msg_fun fun = utils_cmd_bsearch(&icmd, AUDIO_CMD_TABLE, sizeof(AUDIO_CMD_TABLE)/sizeof(cmd_item));
	if(fun != NULL) {
		fun(self);
	}
}

void ReceivedAudioStartCaptureMsgReply(msg_container *self){
	struct HandleAudioMsg *recvMsg = (struct HandleAudioMsg *)self->msgdata_rcv;
	DEBUGPRINT(DEBUG_INFO,"********Child Process Received Audio Start repply===============================\n");
	switch(recvMsg->data[0]){
	case 0:{
		//Success and Start Video Capture
		MsgData *sndMsg=self->msgdata_snd;
		sndMsg->type = VIDEO_MSG_TYPE;
		sndMsg->len = 0;
		sndMsg->cmd =MSG_VIDEO_P_START_CAPTURE;
		int iRet = -1;
		iRet = msg_queue_snd(self->rmsgid,sndMsg,sizeof(MsgData)-sizeof(long int)+sndMsg->len);
		if(iRet<0){
			printf("+++++++++++++++++++++++++Send Video Start Failed++++++++++++++++++++++++++\n");
		}
		DEBUGPRINT(DEBUG_INFO,"********Child Process Received Audio Start repply Success\n");
	}
	break;
	case -1:{
		//Failed
		DEBUGPRINT(DEBUG_INFO,"********Child Process Received Audio Start repply Failed pParams is NULL\n");
	}
		break;
	case -2:{
		//Failed
		DEBUGPRINT(DEBUG_INFO,"********Child Process Received Audio Start repply Init Device Failed\n");
		}
		break;
	default :{
		//Success
			DEBUGPRINT(DEBUG_INFO,"********Child Process Received Audio Start repply Unknown Means\n");
		}
		break;
	}
}
void ReceivedAudioStopCaptureMsgReply(msg_container *self){
	struct HandleAudioMsg *recvMsg = (struct HandleAudioMsg *)self->msgdata_rcv;
		switch(recvMsg->data[0]){
		case 0:{
			//Success
			if(iVideoOpenFailedFlag == 0){
				MsgData *sndMsg=self->msgdata_snd;
				sndMsg->type = VIDEO_MSG_TYPE;
				sndMsg->len = 0;
				sndMsg->cmd =MSG_VIDEO_P_STOP_CAPTURE;
				int iRet=msg_queue_snd(self->rmsgid,sndMsg,sizeof(MsgData)-sizeof(long int)+sndMsg->len);
				if(iRet<0){
					printf("+++++++++++++++++++++++++Send Video Stop Failed++++++++++++++++++++++++++\n");
				}
				DEBUGPRINT(DEBUG_INFO,"********Child Process Received Audio Stop reply Success\n");
			}
			else{
				iVideoOpenFailedFlag = 0;
				struct HandleAudioMsg *sndMsg=self->msgdata_snd;
				sndMsg->type = IO_CTL_MSG_TYPE;
				sndMsg->len = 1;
				sndMsg->cmd =MSG_IOCTL_T_USB_SWITCH;
				sndMsg->data[0] = USB_SWITCH_TO_MFI;
				int iRet=msg_queue_snd(self->rmsgid,sndMsg,sizeof(MsgData)-sizeof(long int)+sndMsg->len);
			}
		}
		break;
		case -1:{
			//Failed
			g_lAudioStopFlag = 2;
			DEBUGPRINT(DEBUG_INFO,"********Child Process Received Audio Stop reply Failed===-1\n");
		}
			break;
		case -2:{
			//Failed
			g_lAudioStopFlag = 2;
			DEBUGPRINT(DEBUG_INFO,"********Child Process Received Audio Stop reply Failed===-2\n");
		}
			break;
		case -3:{
			//Failed
			g_lAudioStopFlag = 2;
			DEBUGPRINT(DEBUG_INFO,"********Child Process Received Audio Stop reply Failed===-3\n");
		}
			break;
		case -4:{
			//Failed
			g_lAudioStopFlag = 2;
			DEBUGPRINT(DEBUG_INFO,"********Child Process Received Audio Stop reply Failed===-4\n");
		}
			break;
		default :{
			//Success
			g_lAudioStopFlag = 2;
				DEBUGPRINT(DEBUG_INFO,"********Child Process Received Audio Stop reply Unknown Means\n");
			}
			break;
	}
}
void ReceivedAudioStartEncodeMsgReply(msg_container *self){
	struct HandleAudioMsg *recvMsg = (struct HandleAudioMsg *)self->msgdata_rcv;
		switch(recvMsg->data[0]){
		case 0:{
			//Success
			DEBUGPRINT(DEBUG_INFO,"********Child Process Received Audio Start Encode repply Success\n");
		}
		break;
		case -1:{
			//Failed
			DEBUGPRINT(DEBUG_INFO,"********Child Process Received Audio Start Encode repply Failed pParams is NULL\n");
		}
			break;
		default :{
			//Success
				DEBUGPRINT(DEBUG_INFO,"********Child Process Received Audio Start Encode repply Unknown Means\n");
			}
			break;
		}
}
void ReceivedAudioStopEncodeMsgReply(msg_container *self){
	struct HandleAudioMsg *recvMsg = (struct HandleAudioMsg *)self->msgdata_rcv;
		switch(recvMsg->data[0]){
		case 0:{
			//Success
			DEBUGPRINT(DEBUG_INFO,"********Child Process Received Audio Stop Encode repply Success\n");
		}
		break;
		case -1:{
			//Failed
			DEBUGPRINT(DEBUG_INFO,"********Child Process Received Audio Stop Encode repply Failed pParams is NULL\n");
		}
			break;
		default :{
			//Success
				DEBUGPRINT(DEBUG_INFO,"********Child Process Received Audio Stop Encode repply Unknown Means\n");
			}
			break;
		}
}

void ReceivedAudioReswitchUSBCMD(msg_container *self){
	DEBUGPRINT(DEBUG_INFO,"======================================ReceivedAudioReswitchUSBCMD==============================================\n");
	struct HandleAudioMsg *sndMsg=self->msgdata_snd;
	char acBuf[256] = {0};  //add by wfb
	int nRLen = 0;
	sndMsg->type = IO_CTL_MSG_TYPE;
	sndMsg->len = 1;
	sndMsg->cmd =MSG_IOCTL_T_USB_SWITCH;
	sndMsg->data[0] = USB_SWITCH_TO_MFI;
	int iRet=msg_queue_snd(self->rmsgid,sndMsg,sizeof(MsgData)-sizeof(long int)+sndMsg->len);
	//Add by zhang li 2013.8.24 10:14
	//function: when reswitch usb 3 times failed,reboot
	iReswitchTimes++;
	printf("==============================iReswitchTimes times ==========================%d\n",iReswitchTimes);
	printf("==============================iReswitchTimes times ==========================%d\n",iReswitchTimes);
	printf("==============================iReswitchTimes times ==========================%d\n",iReswitchTimes);
	if(iReswitchTimes == RESWITCH_TIME){
		get_config_item_value(CONFIG_CAMERA_ENCRYTIONID, acBuf, &nRLen);
		//如果有r‘则重启，无R‘不重启 add by wfb
		if (0 != nRLen)
		{
			MsgData *sndMsg=self->msgdata_snd;
			sndMsg->type = PLAY_BACK_MSG_TYPE;
			sndMsg->len = 0;
			sndMsg->cmd =0xff;
			int iRet = -1;
			iReswitchTimes = 0;
			iRet = msg_queue_snd(self->rmsgid,sndMsg,sizeof(MsgData)-sizeof(long int)+sndMsg->len);
		}

	}
}

void ReceivedAudioNeedRebootCMDD(msg_container *self){
	MsgData *sndMsg=self->msgdata_snd;
	sndMsg->type = PLAY_BACK_MSG_TYPE;
	sndMsg->len = 0;
	sndMsg->cmd =0xff;
	int iRet = -1;
	iRet = msg_queue_snd(self->rmsgid,sndMsg,sizeof(MsgData)-sizeof(long int)+sndMsg->len);
	if(iRet<0){
		printf("+++++++++++++++++++++++++Send Audio Need Reboot Failed++++++++++++++++++++++++++\n");
	}
	DEBUGPRINT(DEBUG_INFO,"========================Child Process Received Audio Need Reboot CMD\n");
}
