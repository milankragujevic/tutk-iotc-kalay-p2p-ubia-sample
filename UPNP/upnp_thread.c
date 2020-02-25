/*
 * UPNP.c
 *
 *  Created on: Jan 7, 2013
 *      Author: root
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
//#include "Child_Process/cmbtable.h"

#include "common/common.h"
#include "common/thread.h"
#include "common/msg_queue.h"

#include "upnp_thread.h"
#include "upnp_adapter.h"

enum{
	MAP_FAILED,
	MAP_SUCESS,
};

void *StartUpnpThread(void *arg);
int UpnpArgInit(upnp_msg_st* upnp_msg);
int UpnpMsgReceiveProcess(upnp_msg_st* upnp_msg, upnp_flag_st *upnp_flag);

/*UPNP入口函数*/
int CreateUpnpThread(void)
{
#if 0
	StartAudioCaptureThread(NULL);
	return 0;
#else
	//DEBUGPRINT(DEBUG_INFO,"UPNP===========111\n");
	return thread_start(&StartUpnpThread);
#endif
}

/*UPNP线程*/
void *StartUpnpThread(void *arg)
{
	upnp_msg_st *upnp_msg = malloc(sizeof(upnp_msg_st));
	if(upnp_msg == NULL){
		printf("IN StartUpnpThread: malloc  upnp_msg fail...\n");
		return NULL;
	}
	upnp_flag_st *upnp_flag = malloc(sizeof(upnp_flag_st));
	if(upnp_flag == NULL){
		free(upnp_msg);
		upnp_msg = NULL;
		printf("IN StartUpnpThread: malloc  upnp_flag fail...\n");
		return NULL;
	}else{
		upnp_flag->flag_process = UPNP_STOP_CMD;
		upnp_flag->flag_snd = 0;
		upnp_flag->count_fail = 0;
		upnp_flag->count_cmd = 0;
		memset(upnp_flag->map_port, 0, sizeof(upnp_flag->map_port));
	}

	if(UpnpArgInit(upnp_msg) != 0){
		free(upnp_msg);
		upnp_msg = NULL;
		free(upnp_flag);
		upnp_flag = NULL;
		return NULL;
	}
	thread_synchronization(&rUPNP, &rChildProcess);
	printf("UpnpThread start success\n");

	while(1){
		if(UpnpMsgReceiveProcess(upnp_msg, upnp_flag) == -1){

		}
		if(upnp_flag->flag_process == UPNP_START_CMD){
			UpnpCommandFun(upnp_msg, upnp_flag);
		}
		//计时处理模块
		if(upnp_flag->flag_success == UPNP_DELAY_START){
			upnp_flag->count_cmd++;
			if(upnp_flag->count_cmd%5000 == 0){
//				printf("count_cmd: %d\n", upnp_flag->count_cmd);
			}
			if(upnp_flag->count_cmd > T_30MIN_DELAY_COUNT){
				upnp_flag->flag_success = UPNP_DELAY_STOP;
				upnp_flag->count_cmd = 0;
				upnp_flag->flag_process = UPNP_START_CMD;
			}
		}

		usleep(UPNP_USLEEP_TIME);
	}

	return NULL;
}

int UpnpArgInit(upnp_msg_st* upnp_msg)
{
	int ret = 0;

	//rev msg from process
	upnp_msg->upnp_rev_process_id = msg_queue_get((key_t) CHILD_THREAD_MSG_KEY);
	if (upnp_msg->upnp_rev_process_id == -1) {
		printf("create CHILD_PROCESS_MSG_KEY failed\n");
		ret = -1;
	}
	//send msg to process
	upnp_msg->upnp_snd_process_id = msg_queue_get((key_t)CHILD_PROCESS_MSG_KEY);
	if (upnp_msg->upnp_snd_process_id == -1) {
		printf("create CHILD_THREAD_MSG_KEY failed\n");
		ret = -1;
	}

	upnp_msg->upnp_rev_process_msg = (Child_Process_Msg *)malloc(sizeof(Child_Process_Msg));
	if(upnp_msg->upnp_rev_process_msg == NULL){
		ret = -1;
	}
	upnp_msg->upnp_snd_process_msg = (Child_Process_Msg *)malloc(sizeof(Child_Process_Msg));
	if(upnp_msg->upnp_snd_process_msg == NULL){
		ret = -1;
	}
	//printf("upnp_arg_init:%d %d\n", upnp_msg->upnp_snd_process_id, upnp_msg->upnp_rev_process_id);

	return ret;
}

int UpnpMsgReceiveProcess(upnp_msg_st* upnp_msg, upnp_flag_st *upnp_flag)
{
	int ret = 0;
	int len;

	if(upnp_recv_msg(upnp_msg) == -1){
		ret = -1;
	}else{
		switch(upnp_msg->upnp_rev_process_msg->Thread_cmd){
			case MSG_UPNP_T_UPNPSTART:
			case MSG_UPNP_T_UPNPAGAIN:
				printf("UPNP_Thread recv start msg\n");
				if (upnp_msg->upnp_rev_process_msg->length < 6) { //端口小于65535 5byte
					strcpy(upnp_flag->map_port,
						upnp_msg->upnp_rev_process_msg->data);
					len = upnp_msg->upnp_rev_process_msg->length;
					upnp_flag->map_port[len] = '\0';
					printf("map port: %s\n", upnp_flag->map_port);
					//设置开始命令
					upnp_flag->flag_process = UPNP_START_CMD;
					//赋值为1，给子进程只发送一次命令
					upnp_flag->flag_snd = 1;
					upnp_flag->count_cmd = T_30MIN_DELAY_COUNT + 1;
				} else {
					printf("map_port fault !!!\n");
				}
				break;

			case CMD_STOP_ALL_USE_FRAME:
			case MSG_UPNP_T_UPNPSTOP:
				printf("UPNP_Thread recv stop msg\n");
				//设置停止命令
				upnp_flag->flag_process = UPNP_STOP_CMD;
				g_lUPNP = 1;
				break;
			case CMD_AV_STARTED:
			default:
				ret = -1;
				goto nothing_exit;
				break;
			}
		//收到消息，停止延迟计时
		upnp_flag->flag_success = UPNP_DELAY_STOP;
		//收到消息，清除映射错误的次数
		upnp_flag->count_fail = 0;
	}
nothing_exit:
	return ret;
}

int upnp_recv_msg(upnp_msg_st *msg)
{
	int ret = 0;

	ret = msg_queue_rcv(msg->upnp_rev_process_id, msg->upnp_rev_process_msg,
			sizeof(Child_Process_Msg), UPNP_MSG_TYPE);
	if (-1 == ret) {
		ret = -1;
	}

	return ret;
}

int upnp_send_msg(upnp_msg_st *msg)
{
	int ret = 0;
	int len;

	len = 12+msg->upnp_snd_process_msg->length;
	ret = msg_queue_snd(msg->upnp_snd_process_id, msg->upnp_snd_process_msg, len);
	if (-1 == ret) {
		ret = -1;
	}
	return ret;
}




