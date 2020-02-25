/*
 * testcommon.c
 *
 *  Created on: Mar 26, 2013
 *      Author: root
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include "common/common.h"
#include "common/thread.h"
#include "common/msg_queue.h"
#include "testcommon.h"

int InitArgs(ArgsStruct *args){

	int nRet = 0;
	if(args == NULL){
		nRet = FAIL;
		return nRet;
	}
//get msg from guo
	//args = malloc(sizeof(ArgsStruct));
	args->MSGID_REV_PROCESS = msg_queue_get((key_t) CHILD_THREAD_MSG_KEY);
	if (args->MSGID_REV_PROCESS == -1) {
		nRet = FAIL;
	}

//send msg to guo
	args->MSGID_SEND_PROCESS = msg_queue_get((key_t) CHILD_PROCESS_MSG_KEY);
	if (args->MSGID_SEND_PROCESS == -1) {
		nRet = FAIL;
	}

	args->s_rev_process = (Child_Process_Msg *)malloc(sizeof(Child_Process_Msg));
	if(NULL==args->s_rev_process){
		nRet = FAIL;
	}
	args->s_send_process = (Child_Process_Msg *)malloc(sizeof(Child_Process_Msg));
	if(NULL==args->s_send_process){
//		free(args->s_rev_process);
		nRet = FAIL;
	}
	return nRet;
}

int RevMSGProcess(ArgsStruct * msg,const int msgtype){
	int nRet = 0;
	int result = 0;
	msg->s_rev_process->Thread_cmd = 0;
	memset(msg->s_rev_process->data,'\0',MAX_MSG_LEN);
	result =msg_queue_rcv(msg->MSGID_REV_PROCESS, msg->s_rev_process, sizeof(Child_Process_Msg), msgtype);
	if(-1 == result){
		nRet = -1;
	}else{
		nRet = 0;
	}

	return nRet;
}

int SendMSG_2_Process(ArgsStruct * msg){
	int nRet = 0;
	int result = 0;
	result = msg_queue_snd(msg->MSGID_SEND_PROCESS,msg->s_send_process,sizeof(int)+sizeof(int)+
			 msg->s_send_process->length+sizeof(msg->s_send_process->E_xin));
	if(-1 == result){
		nRet = -1;
	}else{

		nRet = 0;
	}
	return nRet;
}


int RevMsgFunction(int msgID,Child_Process_Msg*rev_msg,const int msgtype){
	int nRet = 0;
	int result = 0;
	rev_msg->Thread_cmd = 0;
	memset(rev_msg->data,0,MAX_MSG_LEN);
	result = msg_queue_rcv(msgID,rev_msg,sizeof(Child_Process_Msg),msgtype);
	if(-1 == result){
		nRet = -1;
	}else{
		//printf("rev msg from process CMD:%d data:%s  __LINE__:%d\n",rev_msg->Thread_cmd,rev_msg->data,__LINE__);
	}
	return nRet;
}

int SendMsgFunction(int msgID,Child_Process_Msg*snd_msg){
	int nRet = 0;
	int result = 0;
//	snd_msg->Thread_cmd = 0;
//	memset(snd_msg->data,0,MAX_MSG_LEN);
	result = msg_queue_snd(msgID,snd_msg,sizeof(int)+sizeof(int)+sizeof(snd_msg->E_xin)+snd_msg->length);
	if(-1 == result){
		nRet = -1;
	}
	else{
		//printf("###$$$$$$snd_msg.length=%d",snd_msg->length);
		//printf("\n\rtype %ld \n\r CMD:%d  data :%s __LINE__:%d\r\n",snd_msg->msg_type,snd_msg->Thread_cmd,snd_msg->data,__LINE__);
	}
	return nRet;
}
