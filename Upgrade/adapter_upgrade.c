/*
 * adapter_httpserver.c
 *
 *  Created on: Apr 8, 2013
 *      Author: root
 */
#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include "adapter_upgrade.h"
#include "common/common.h"
#include "common/utils.h"
#include "upgrade.h"

void Upgrade_cmd_table(msg_container *self);
void UpgradeStart(msg_container self);
void UpgradeEnd(msg_container *self);
void UpgradeGetUpdateInfo(msg_container self);
void retUpgradeGetUpdateInfo(msg_container *self);
void UpgradeDownLoadBlock(msg_container self);
void retUpgradeSeverLoginCam(msg_container *self);
void UpgradeState(msg_container self);
void retUpgradeState(msg_container *self);
void retUpgradeEnd(msg_container self);
void retUpgradeStart(msg_container *self);

cmd_item Upgrade_cmd_table[] = {

		{MSG_UPGRADE_T_GET_UPDATE_INFO, UpgradeGetUpdateInfo},//Camera 验证（只当USB线连上Cam时进行）
		{MSG_UPGRADE_P_GET_UPDATE_INFO, retUpgradeGetUpdateInfo},//Camera 验证（只当USB线连上Cam时进行）

		{MSG_UPGRADE_T_DOWN_LOAD_BLOCK,UpgradeDownLoadBlock},//Camera 注册
		{MSG_UPGRADE_P_DOWN_LOAD_BLOCK,retUpgradeSeverLoginCam},//Camera 注册

		{MSG_UPGRADE_T_UPGRADE_STATE,UpgradeState},//Camera上传文件请求（报警，日志）
		{MSG_UPGRADE_P_UPGRADE_STATE,retUpgradeState},//Camera上传文件请求（报警，日志）

		{MSG_UPGRADE_T_STOP,UpgradeEnd},
		{MSG_UPGRADE_P_STOP,retUpgradeEnd},

		{MSG_UPGRADE_T_START,UpgradeStart},
		{MSG_UPGRADE_P_START,retUpgradeStart},
};

void Upgrade_cmd_table(msg_container *self) {
	MsgData *msgrcv = (MsgData *)self->msgdata_rcv;
	cmd_item icmd;
	icmd.cmd = msgrcv->cmd;
	msg_fun fun = utils_cmd_bsearch(&icmd, Upgrade_cmd_table, sizeof(Upgrade_cmd_table)/sizeof(cmd_item));
	if(fun != NULL) {
		fun(self);
	}
}

void UpgradeStart(msg_container self){
	printf("HttpServerStart\n");
	Child_Process_Msg *msgsnd = (Child_Process_Msg*)self.msgdata_snd;
	msgsnd->msg_type = UPGRADE_MSG_TYPE;
	msgsnd->Thread_cmd = MSG_UPGRADE_T_START;
	msgsnd->length = 0;
	if(msg_queue_snd(self.rmsgid, msgsnd/*self->msgdata_snd*/, sizeof(struct MsgData)-sizeof(long int)) == -1)
	{
		printf("send msg 2 httpserver fail\n");
	}else{
		printf("send msg 2 httpserver SUCCESS\n");
	}

}

void retUpgradeStart(msg_container *self){

	Child_Process_Msg *msgrev = (Child_Process_Msg*)self->msgdata_rcv;
	printf("\rmsgrev->data:%s msgrev->length:%d\n",msgrev->data,msgrev->length);

	if(0 == strncmp(msgrev->data ,"SUCCESS",msgrev->length)){
		printf("IP is complete\r\n");
	}else{
		printf("IP is Lost\r\n");
	}
	//printf("CMD:%d,data:%s \n__LINE__ %d\n",msgrev->Thread_cmd,msgrev->data,__LINE__);
}

void UpgradeGetUpdateInfo(msg_container self){
	Child_Process_Msg *msgsnd = (Child_Process_Msg*)self.msgdata_snd;
	msgsnd->msg_type = UPGRADE_MSG_TYPE;
	msgsnd->Thread_cmd = MSG_UPGRADE_T_GET_UPDATE_INFO;
	msgsnd->length = 0;
	if(msg_queue_snd(self.rmsgid, msgsnd, sizeof(struct MsgData)-sizeof(long int)) == -1)
	{

	}

}

void retUpgradeGetUpdateInfo(msg_container *self){
	Child_Process_Msg *msgsnd = (Child_Process_Msg*)self->msgdata_snd;
	Child_Process_Msg *msgrev = (Child_Process_Msg*)self->msgdata_rcv;

	if(0 == strncmp(msgrev->data,"FAIL",4)){

	}else{
		printf("12345  msgrev->data:%s\n",msgrev->data);
		memcpy(msgsnd->data,msgrev->data,msgrev->length);
		msgsnd->msg_type = HTTP_SERVER_MSG_TYPE;
		msgsnd->Thread_cmd = MSG_HTTPSSEVER_T_LOGIN_CAM;
		msgsnd->length = msgrev->length;
		if(msg_queue_snd(self->rmsgid, msgsnd, sizeof(struct MsgData)-sizeof(long int)+msgsnd->length) == -1)
		{

		}else{

		}
	}
}

void UpgradeDownLoadBlock(msg_container *self){
	Child_Process_Msg *msgsnd = (Child_Process_Msg*)self->msgdata_snd;
	msgsnd->msg_type = UPGRADE_MSG_TYPE;
	msgsnd->Thread_cmd = MSG_HTTPSSEVER_T_LOGIN_CAM;
	msgsnd->length = 0;
	if(msg_queue_snd(self->rmsgid, self->msgdata_snd, sizeof(struct MsgData)-sizeof(long int)) == -1)
	{

	}

}

void retUpgradeSeverLoginCam(msg_container *self){
	Child_Process_Msg *msgsnd = (Child_Process_Msg*)self->msgdata_snd;
	Child_Process_Msg *msgrev = (Child_Process_Msg*)self->msgdata_rcv;

	if(0 != strncmp(msgrev->data,"FAIL",4)){

	}else{
		memcpy(msgsnd->data,msgrev->data,msgrev->length);
		msgsnd->msg_type = HTTP_SERVER_MSG_TYPE;
		msgsnd->Thread_cmd = MSG_HTTPSSEVER_T_LOGIN_CAM;
		msgsnd->length = msgrev->length;
//		if(msg_queue_snd(self->rmsgid, self->msgdata_snd, sizeof(struct MsgData)+msgrev->length-sizeof(long int)) == -1)
//		{
//
//		}
	}
}

void UpgradeState(msg_container *self){
	Child_Process_Msg *msgsnd = (Child_Process_Msg*)self->msgdata_snd;
	msgsnd->msg_type = UPGRADE_MSG_TYPE;
	msgsnd->Thread_cmd = MSG_HTTPSSEVER_T_PUSH_ALARM_2_CLOUND_CAM;
	msgsnd->length = 0;
	if(msg_queue_snd(self->rmsgid, self->msgdata_snd, sizeof(struct MsgData)-sizeof(long int)) == -1)
	{

	}

}

void retUpgradeState(msg_container *self){
	Child_Process_Msg *msgsnd = (Child_Process_Msg*)self->msgdata_snd;
	Child_Process_Msg *msgrev = (Child_Process_Msg*)self->msgdata_rcv;

	if(0 != strncmp(msgrev->data,"FAIL",4)){

	}else{
		memcpy(msgsnd->data,msgrev->data,msgrev->length);
		msgsnd->msg_type = UPGRADE_MSG_TYPE;
		msgsnd->Thread_cmd = MSG_HTTPSSEVER_T_LOGIN_CAM;
		msgsnd->length = msgrev->length;
//		if(msg_queue_snd(self->rmsgid, self->msgdata_snd, sizeof(struct MsgData)+msgrev->length-sizeof(long int)) == -1)
//		{
//
//		}
	}
}

void UpgradeEnd(msg_container *self){
	Child_Process_Msg *msgsnd = (Child_Process_Msg*)self->msgdata_snd;
	msgsnd->msg_type = UPGRADE_MSG_TYPE;
	msgsnd->Thread_cmd = MSG_UPGRADE_T_STOP;
	msgsnd->length = 0;
	if(msg_queue_snd(self->rmsgid, self->msgdata_snd, sizeof(struct MsgData)-sizeof(long int)) == -1)
	{

	}

}

void retUpgradeEnd(msg_container *self){
	Child_Process_Msg *msgsnd = (Child_Process_Msg*)self->msgdata_snd;
	msgsnd->msg_type = HTTP_SERVER_MSG_TYPE;
	msgsnd->Thread_cmd = MSG_HTTPSSERVER_T_STOP;
	msgsnd->length = 0;
	if(msg_queue_snd(self->rmsgid, self->msgdata_snd, sizeof(struct MsgData)-sizeof(long int)) == -1)
	{

	}

}
