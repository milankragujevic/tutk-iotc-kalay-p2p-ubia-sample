/*
 * httpsserver.c
 *
 *  Created on: Mar 12, 2013
 *      Author: root
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <string.h>
#include <time.h>
#include "Upgrade/upgrade.h"
#include "common/thread.h"
#include "upgrade.h"
#include "do_upgrade_m3s.h"
#include "common/common.h"
#include "AudioAlarm/testcommon.h"
void* UpgradeMain(void * arg);

static void getTimeAndPrint(int Time,int tag,char *text){
	time_t mytime;
	time(&mytime);
	if(Time == 1){
		printf("Time :%s o(∩_∩)o --> Message:%s\n",ctime(&mytime),text);
	}else{
		printf(
				"File:%s Line:%d Time :%s \n "
				"o(∩_∩)o --> Message:%s\n",__FILE__,tag,ctime(&mytime),text);
	}
}
int InitUpgrade(UpgradeStruct *args);
//int upgrade_RevMsgFunction(int msgID,Child_Process_Msg*rev_msg,const int msgtype);
//int upgrade_SendMsgFunction(int msgID,Child_Process_Msg*snd_msg);
int upgrade_RevMsg4Process(UpgradeStruct *self);
int upgrade_SendMsg2Process(UpgradeStruct *self);
int upgrade_RevMsg4DoHttps(UpgradeStruct *self);
int upgrade_SendMsg2DoHttps(UpgradeStruct *self);

volatile int R_UpgradeHost = 0;
volatile int R_UpgradeThread = 0;
/*入口
 * */
int createUpgradeThread(void) {
	int createUpgradeThread = 0;
	createUpgradeThread = thread_start(UpgradeMain);
	return createUpgradeThread; //记得判断哦！！！！
}

void* UpgradeMain(void * arg){
	UpgradeStruct *self;
	self = malloc(sizeof(UpgradeStruct));
	if(NULL == self){
		getTimeAndPrint(2,__LINE__,"Init args self error!!!");
		return NULL;
	}
	if(-1 == InitUpgrade(self)){
		getTimeAndPrint(2,__LINE__,"Init args self error!!!");
		return NULL;
	}

	thread_synchronization(&rUpgrade, &rChildProcess);

	int count = 0;
//	long int file_group = 0;
//	int filenumber = 0;
//	int alarmtype = 0;
//	int push_info_flag_and_count = 0;
	while(1){
//count number 1s print one word
		if(count % 100 == 0){
			count = 0;
			//getTimeAndPrint(2,__LINE__,"Hello M3s this here is audioAlarm thread");
		}
		count++;
//revice msg from child_progess
		if(-1 == upgrade_RevMsg4Process(self)){

		}else{

			printf("Rev msg from process !!!CMD:%d",self->s_rev_process->Thread_cmd);
#if 0
			if(self->s_rev_process->Thread_cmd == MSG_HTTPSSEVER_T_PUSH_ALARM_2_CLOUND_CAM){
				push_info_flag_and_count = 5;
				memcpy(&file_group,self->s_rev_process->data,sizeof(long int ));
				memcpy(&alarmtype,(self->s_rev_process->data+4),sizeof(int));
				filenumber = 1;
			}
#endif
//send msg to do_https_thread
			if(-1 == upgrade_SendMsg2DoHttps(self)){

			}else{
				getTimeAndPrint(2,__LINE__,"send msg do httpsserver !!!");
			}

		}

//if revice msg from do_https_thread
		if(-1 == upgrade_RevMsg4DoHttps(self)){

		}else{
			printf("msg rev from do https CMD %d,data %s\n",self->s_rev_do_https->Thread_cmd,self->s_rev_do_https->data);
			if(-1 == upgrade_SendMsg2Process(self)){

			}else{

			}
#if 0

//if msg cmd is  MSG_HTTPSSEVER_P_UPLOAD_FILE_CAM
			if(self->s_rev_do_https->Thread_cmd == MSG_HTTPSSEVER_P_UPLOAD_FILE_CAM){
				if(strncmp(self->s_rev_do_https->data,"OK",2) == 0){ 
					push_info_flag_and_count --;
				}else{
					push_info_flag_and_count --;
				}
			}else{
				if(-1 == SendMsg2Process(self)){

				}else{

				}
			}int upgrade_RevMsgFunction(int msgID,Child_Process_Msg*rev_msg,const int msgtype){
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

			int upgrade_SendMsgFunction(int msgID,Child_Process_Msg*snd_msg){
				int nRet = 0;
				int result = 0;
				result = msg_queue_snd(msgID,snd_msg,sizeof(int)+sizeof(int)+sizeof(snd_msg->E_xin)+snd_msg->length);
				if(-1 == result){
					nRet = -1;
				}
				else{

				}
				return nRet;
			}

			if(push_info_flag_and_count == 0){
				self->s_rev_do_https->Thread_cmd = MSG_HTTPSSEVER_P_UPLOAD_FILE_CAM;
				memset(self->s_rev_do_https,0,MAX_MSG_LEN);
				if(-1 == SendMsg2Process(self)){

				}else{
					file_group = 0;
					filenumber = 0;
					alarmtype = 0;
					push_info_flag_and_count = -1;
				}

			}else if(push_info_flag_and_count >= 1){
				self->s_send_do_https->Thread_cmd = MSG_HTTPSSEVER_P_UPLOAD_FILE_CAM;
				memcpy(self->s_send_do_https->data,&file_group,sizeof(long int));
				memcpy(self->s_send_do_https->data+4,&alarmtype,sizeof(int));
				memcpy(self->s_send_do_https->data+8,&filenumber,sizeof(int));
				if(-1 == SendMsg2DoHttps(self)){

				}else{
					filenumber ++;
				}
			}
#endif
		}

		thread_usleep(100000);
	}

	return NULL;
}

int InitUpgrade(UpgradeStruct *args){

	int nRet = 0;
	if(args == NULL){
		nRet = FAIL;
		return nRet;
	}
//get msg from guo
	args->MSGID_REV_PROCESS = msg_queue_get((key_t) CHILD_THREAD_MSG_KEY);
	if (args->MSGID_REV_PROCESS == -1) {
		nRet = FAIL;
	}

//send msg to guo
	args->MSGID_SEND_PROCESS = msg_queue_get((key_t) CHILD_PROCESS_MSG_KEY);
	if (args->MSGID_SEND_PROCESS == -1) {
		nRet = FAIL;
	}
//revice msg from do https
	args->MSGID_REV_DO_HTTPS = msg_queue_get((key_t)DO_UPGRADE_SERVER_MSG_KEY);
	if (args->MSGID_REV_DO_HTTPS == -1) {
		nRet = FAIL;
	}
//send msg to do https
	args->MSGID_SEND_DO_HTTPS = msg_queue_get((key_t)UPGRADE_SERVER_MSG_KEY);
	if (args->MSGID_SEND_DO_HTTPS == -1) {
		nRet = FAIL;
	}

	args->s_rev_process = (Child_Process_Msg *)malloc(sizeof(Child_Process_Msg));
	if(NULL==args->s_rev_process){
		nRet = FAIL;
	}
	args->s_send_process = (Child_Process_Msg *)malloc(sizeof(Child_Process_Msg));
	if(NULL==args->s_send_process){
		nRet = FAIL;
	}

	args->s_rev_do_https = (Child_Process_Msg *)malloc(sizeof(Child_Process_Msg));
	if(NULL==args->s_rev_do_https){
		nRet = FAIL;
	}
	args->s_send_do_https = (Child_Process_Msg *)malloc(sizeof(Child_Process_Msg));
	if(NULL==args->s_send_do_https){
		nRet = FAIL;
	}

	if (thread_start(DO_Upgrade_Thread) != 0) {
		fprintf(stderr, "create thread_start: %s\n",strerror(errno));
	}

	//同步1
	host_thread_synchronization(&R_UpgradeThread, &R_UpgradeHost);

	return nRet;

}

#if 0
int upgrade_RevMsgFunction(int msgID,Child_Process_Msg*rev_msg,const int msgtype){
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

int upgrade_SendMsgFunction(int msgID,Child_Process_Msg*snd_msg){
	int nRet = 0;
	int result = 0;
	result = msg_queue_snd(msgID,snd_msg,sizeof(int)+sizeof(int)+sizeof(snd_msg->E_xin)+snd_msg->length);
	if(-1 == result){
		nRet = -1;
	}
	else{

	}
	return nRet;
}
#endif

int upgrade_RevMsg4Process(UpgradeStruct *self){
	int nRet = 0;
	nRet = RevMsgFunction(self->MSGID_REV_PROCESS,self->s_rev_process,UPGRADE_MSG_TYPE);
	return nRet;
}

int upgrade_SendMsg2Process(UpgradeStruct *self){
	int nRet = 0;
	self->s_send_process = self->s_rev_do_https;
	self->s_send_process->msg_type = UPGRADE_MSG_TYPE;
	nRet = SendMsgFunction(self->MSGID_SEND_PROCESS,self->s_send_process);
	return nRet;
}

int upgrade_RevMsg4DoHttps(UpgradeStruct *self){
	int nRet = 0;
	nRet = RevMsgFunction(self->MSGID_REV_DO_HTTPS,self->s_rev_do_https,DO_2_UPGRADE_MSG_TYPE);
	return nRet;
}

int upgrade_SendMsg2DoHttps(UpgradeStruct *self){
	int nRet = 0;
	self->s_send_do_https = self->s_rev_process;
	self->s_send_do_https->msg_type = UPGRADE_2_DO_MSG_TYPE;
	nRet = SendMsgFunction(self->MSGID_SEND_DO_HTTPS,self->s_send_do_https);
	return nRet;
}






