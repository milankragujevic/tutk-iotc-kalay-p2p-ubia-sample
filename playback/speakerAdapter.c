/*
 * speakerAdapter.c
 *
 *  Created on: Apr 2, 2013
 *      Author: root
 */
#include "playback/speakerAdapter.h"
#include "common/common.h"
#include "common/msg_queue.h"
#include "common/logfile.h"
#include "playback/speaker.h"
#include "SerialPorts/SerialPorts.h"

#define NEED_REBOOT_MSG_LEN 128

typedef struct
{
	long int type;
	int cmd;
	char sign[4];
	int len;
	char data[NEED_REBOOT_MSG_LEN];
}NeedRebootMsg;

void speakerAdapter_cmd_table_fun (msg_container *self);

/*
 * @brief 放音线程是否活着的消息处理
 * @author:张历
 * @param[in]:<self><接受到的消息>
 * @param[out]:<没有参数>
 * @Return <NULL>
 */
void ReceivedCheckAliveMsgReply(msg_container *self);

/*
 * @brief 放音线程重启的消息处理
 * @author:张历
 * @param[in]:<self><接受到的消息>
 * @param[out]:<没有参数>
 * @Return <NULL>
 */
void ReceivedNeedRebootCMD(msg_container *self);

/*
 * @brief 放音线程向Apps发送需要重启的消息（未实现）
 * @author:张历
 * @param[in]:<self><接受到的消息>
 * @param[out]:<没有参数>
 * @Return <NULL>
 */
void SendNeedRebootRequestToApps(msg_container *self);

cmd_item SPEAKER_CMD_TABLE[] = {
		{MSG_SPEAKER_T_CHECK_ALIVE,ReceivedCheckAliveMsgReply},
		{MSG_SPEAKER_T_NEED_REBOOT,ReceivedNeedRebootCMD},
};

void speakerAdapter_cmd_table_fun (msg_container *self){
	MsgData *msgrcv = (MsgData *)self->msgdata_rcv;
	cmd_item icmd;
	icmd.cmd = msgrcv->cmd;
	msg_fun fun = utils_cmd_bsearch(&icmd, SPEAKER_CMD_TABLE, sizeof(SPEAKER_CMD_TABLE)/sizeof(cmd_item));
	if(fun != NULL) {
		fun(self);
	}
}

void ReceivedCheckAliveMsgReply(msg_container *self){

}

void ReceivedNeedRebootCMD(msg_container *self){
//	DEBUGPRINT(DEBUG_INFO,"child process come reboot!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
//	sleep(1);
	serialport_sendAuthenticationFailure();
	//system("reboot");

	//向IO线程发送消息
	//DEBUGPRINT(DEBUG_INFO,"READY TO SEND IO KILL KERNEL\n");
	//send_msg_to_io_reboot_rt5350();
	//sleep(2);
	LOG_TEST("speaker reboot time: %08X", time(NULL));
	sleep(2);
	strcpy(NULL, "ABC");
//	sleep(1);
//	sleep(1);
//	DEBUGPRINT(DEBUG_INFO,"child process after reboot!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	SendNeedRebootRequestToApps(self);
}

void SendNeedRebootRequestToApps(msg_container *self){
	NeedRebootMsg *rebootmsg = self->msgdata_snd;

	//control channel
	rebootmsg->type = NEWS_CHANNEL_MSG_TYPE;
	rebootmsg->cmd = 0;
	rebootmsg->data[0] = 0;
	rebootmsg->data[1] = 0;
	rebootmsg->data[2] = 0;
	rebootmsg->data[3] = 0;
	rebootmsg->data[4] = 0;
	rebootmsg->data[5] = 0;
	rebootmsg->len = 6;
//	if(msg_queue_snd(self->rmsgid, rebootmsg, sizeof(struct MsgData)-sizeof(long int)+rebootmsg->len) == -1)
//	{
//		DEBUGPRINT(DEBUG_INFO,"==================send need reboot msg failed=======================\n");
//		return;
//	}

	//p2p channel
	rebootmsg->type = P2P_MSG_TYPE;
	rebootmsg->cmd = 0;
//	if(msg_queue_snd(self->rmsgid, rebootmsg, sizeof(struct MsgData)-sizeof(long int)+rebootmsg->len) == -1)
//	{
//		DEBUGPRINT(DEBUG_INFO,"==================send need reboot msg failed=======================\n");
//		return;
//	}
}
