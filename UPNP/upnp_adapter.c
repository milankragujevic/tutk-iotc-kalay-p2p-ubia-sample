/*
 * UPNPAdapter.c
 *
 *  Created on: Mar 21, 2013
 *      Author: root
 */

#include <stdio.h>
#include <string.h>

#include "common/utils.h"

#include "upnp_thread.h"
#include "upnp_adapter.h"

void upnp_map_success(upnp_msg_st *arg, upnp_flag_st *upnp_flag)
{
	Child_Process_Msg *tx = arg->upnp_snd_process_msg;

	tx->msg_type = UPNP_MSG_TYPE;
	tx->Thread_cmd = MSG_UPNP_P_UPNPSUCESS;
	tx->length = strlen(upnp_flag->map_port);
	strncpy(tx->data, upnp_flag->map_port, tx->length);

	upnp_send_msg(arg);
}

void upnp_map_failed(upnp_msg_st *arg, upnp_flag_st *upnp_flag)
{
	Child_Process_Msg *tx = arg->upnp_snd_process_msg;

	tx->msg_type = UPNP_MSG_TYPE;
	tx->Thread_cmd = MSG_UPNP_P_UPNPFAILED;
	tx->length = strlen(upnp_flag->map_port);
	strncpy(tx->data, upnp_flag->map_port, tx->length);

	upnp_send_msg(arg);
}

void UpnpCommandFun(upnp_msg_st *msg, upnp_flag_st *upnp_flag)
{
	int ret = 0;

	if(SetUpnp(upnp_flag->map_port) == 0){
		//map success
		if(upnp_flag->flag_snd == 1){
			upnp_flag->flag_snd = 0;
			upnp_map_success(msg, upnp_flag);
		}

		upnp_flag->flag_process = UPNP_STOP_CMD;
		//开始计时
		upnp_flag->flag_success = UPNP_DELAY_START;
		upnp_flag->count_cmd = 0;

	}else{
		upnp_flag->count_fail++;
		if(upnp_flag->count_fail > 3){
			printf("map fail, more than 3 times\n");
			upnp_flag->flag_process = UPNP_STOP_CMD;
			//清零
			upnp_flag->count_fail = 0;
			upnp_map_failed(msg, upnp_flag);
			//停止计数模块
			upnp_flag->flag_success = UPNP_DELAY_STOP;
		}
	}
}

void upnp_cmd_table_fun(msg_container *msg)
{
	int ret;
#if 0
	int len = msg->msgdata_snd[12];

	ret = msg_queue_snd(msg->rmsgid, msg->msgdata_snd, 16+len);
	if(ret != 0){
		printf("In upnp_cmd_table_fun: send msg failed\n");
	}
#else
	Child_Process_Msg *msgself = msg->msgdata_rcv;
	int cmd = msgself->Thread_cmd;
	int len = msgself->length;

	switch(cmd){
	case MSG_UPNP_T_UPNPSTART:
	case MSG_UPNP_T_UPNPSTOP:
	case MSG_UPNP_T_UPNPAGAIN:
		ret = msg_queue_snd(msg->rmsgid, msg->msgdata_rcv, 16 + len);
		if (ret != 0) {
			printf("In upnp_cmd_table_fun: send msg failed\n");
		}
		break;
	case MSG_UPNP_P_UPNPSUCESS:
		//send message to other thread
		break;
	case MSG_UPNP_P_UPNPFAILED:
		//send message to other thread
		break;
	default:
		break;
	}
#endif
}


