/**   @file []
   *  @brief httpsserver的实现
   *  @note
   *  @author:   gy
   *  @date 2013-9-2
   *  @remarks 增加注释
*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <string.h>
#include <time.h>
#include "common/thread.h"
#include "httpsserver.h"
#include "do_https_m3s.h"
#include "common/common.h"
#include "AudioAlarm/testcommon.h"


int push_info_flag_and_count = 0;
/**保存要删除的文件*/
char rmdirname[20];

/**https线程的初始化 */
int InitTwoArgs(HttpsStruct *args);
/**接收子进程消息 */
int RevMsg4Process(HttpsStruct *self);
/**向子进程发送消息 */
int SendMsg2Process(HttpsStruct *self);
/**从dohttps接收消息 */
int RevMsg4DoHttps(HttpsStruct *self);
/**向dohttps发送消息*/
int SendMsg2DoHttps(HttpsStruct *self);
/**https主函数*/
void* HttpsServerMain(void * arg);
int delay_send(HttpsStruct *self,int times);
/**https于dohttps同步标志 */
volatile int R_HttpsHost = 0;
/**https与子进程同步标志 */
volatile int R_HttpsThread = 0;


/**
  *  @brief  创建httpsserver线程 
  *  @author  <gy> 
  *  @param[in] <无>
  *  @param[out] <无>
  *  @return   <无>
  *  @note
*/
int createHttpsServerThread(void) {
	int createHttpsServer = 0;
	createHttpsServer = thread_start(HttpsServerMain);
	return createHttpsServer; //记得判断哦！！！！
}

/**
  *  @brief  httpsserver处理过程函数
  *  @author  <gy> 
  *  @param[in] <无>
  *  @param[out] <无>
  *  @return   <无>
  *  @note
*/
void* HttpsServerMain(void * arg){
	HttpsStruct *self;
	self = malloc(sizeof(HttpsStruct));
	if(NULL == self){
		return NULL;
	}
	if(-1 == InitTwoArgs(self)){
		return NULL;
	}

	thread_synchronization(&rHTTPServer, &rChildProcess);
	int myRltime = 0;
	int n = 0;
	int power_number = 0;
	int delay_time = 0;
	while(1){

		if(my_uid != 0){
			myRltime++;
			if(myRltime > 500){

				self->s_send_do_https = self->s_rev_process;
				self->s_send_do_https->msg_type = HTTP_2_DO_MSG_TYPE;
				self->s_send_do_https->Thread_cmd = MSG_HTTPSSEVER_T_VERIFYCAM;
				SendMsgFunction(self->MSGID_SEND_DO_HTTPS,self->s_send_do_https);

				myRltime = 0;

				printf("Fuck Cloud  Fuck Cloud Fuck Cloud Fuck Cloud  Fuck Cloud  Fuck Cloud \n");
				printf("Fuck Cloud  Fuck Cloud Fuck Cloud Fuck Cloud  Fuck Cloud  Fuck Cloud \n");
				printf("Fuck Cloud  Fuck Cloud Fuck Cloud Fuck Cloud  Fuck Cloud  Fuck Cloud \n");
			}
			//printf("network is not ok!!! I NO have UID\n");
		}else{
			myRltime = 0;
		}
		child_setThreadStatus(HTTP_SERVER_MSG_TYPE, NULL);

		if(-1 == RevMsg4Process(self)){

		}else{

			if(self->s_rev_process->Thread_cmd == MSG_HTTPSPOWER_T_STATE){
				printf("wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwqqqwwwwwqqqqqqqqqqqqqqqqqqqqqq\n");
				if(-1 == SendMsg2DoHttps(self)){

				}else{

				}
			}else if(flag_setnetwork != 0 && self->s_rev_process->Thread_cmd == MSG_HTTPSSEVER_T_UPLOAD_FILE_CAM){
				memset(rmdirname,0,sizeof(rmdirname));
				memcpy(rmdirname,self->s_rev_process->data,self->s_rev_process->length);
				if(my_remove_file(rmdirname) != 0){

				}else{
					printf("==============================================\n"
							"=============  Delete File  AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA  ==================\n"
							"==============================================\n");
				}
			}else if(flag_setnetwork == 0 && self->s_rev_process->Thread_cmd == MSG_HTTPSSEVER_T_UPLOAD_FILE_CAM){
				for(n  = 0;n < 3;n++){
					if(-1 == SendMsg2DoHttps(self)){

					}else{

					}
				}
			}else if(flag_setnetwork != 0 && self->s_rev_process->Thread_cmd == MSG_HTTPSSEVER_T_PUSH_ALARM_2_CLOUND_CAM){

			}/*else if(power_number !=0 && self->s_rev_process->Thread_cmd == MSG_HTTPSSERVER_T_AUTHENTICATION_POWER){
				delay_send(self,delay_time);
			}else if(power_number == 0 && self->s_rev_process->Thread_cmd == MSG_HTTPSSERVER_T_AUTHENTICATION_POWER){

				if(-1 == SendMsg2DoHttps(self)){

				}else{
					power_number = power_number +1;
				}

			}*/else{
				if(-1 == SendMsg2DoHttps(self)){

				}else{

				}
			}

		}

//if revice msg from do_https_thread
		if(-1 == RevMsg4DoHttps(self)){

		}else{
			if(-1 == SendMsg2Process(self)){

			}else{

			}
		}

		thread_usleep(300000);
	}

	return NULL;
}


/**
  *  @brief  httpsserver初始化处理 
  *  @author  <gy> 
  *  @param[in] <HttpsStruct结构体>
  *  @param[out] <无>
  *  @return   <0代表成功>  <-1代表失败>
  *  @note
*/
int InitTwoArgs(HttpsStruct *args){

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

	args->MSGID_REV_DO_HTTPS = msg_queue_get((key_t)HTTP_SERVER_MSG_KEY);
	if (args->MSGID_REV_DO_HTTPS == -1) {
		nRet = FAIL;
	}

	args->MSGID_SEND_DO_HTTPS = msg_queue_get((key_t)DO_HTTP_SERVER_MSG_KEY);
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

	thread_start(DO_Https_Thread);

	//同步1
	host_thread_synchronization(&R_HttpsThread, &R_HttpsHost);

	return nRet;

}

/**
  *  @brief  接收子进程消息 
  *  @author  <gy> 
  *  @param[in] <HttpsStruct结构体>
  *  @param[out] <无>
  *  @return   <0代表成功>  <-1代表失败>
  *  @note
*/
int RevMsg4Process(HttpsStruct *self){
	int nRet = 0;
	nRet = RevMsgFunction(self->MSGID_REV_PROCESS,self->s_rev_process,HTTP_SERVER_MSG_TYPE);
	return nRet;
}

/**
  *  @brief  向子进程发送消息 
  *  @author  <gy> 
  *  @param[in] <HttpsStruct结构体>
  *  @param[out] <无>
  *  @return   <0代表成功>  <-1代表失败>
  *  @note
*/
int SendMsg2Process(HttpsStruct *self){
	int nRet = 0;
	self->s_send_process = self->s_rev_do_https;
	self->s_send_process->msg_type = HTTP_SERVER_MSG_TYPE;
	nRet = SendMsgFunction(self->MSGID_SEND_PROCESS,self->s_send_process);
	return nRet;
}

/**
  *  @brief  从dohttps接收消息 
  *  @author  <gy> 
  *  @param[in] <HttpsStruct结构体>
  *  @param[out] <无>
  *  @return   <0代表成功>  <-1代表失败>
  *  @note
*/
int RevMsg4DoHttps(HttpsStruct *self){
	int nRet = 0;
	nRet = RevMsgFunction(self->MSGID_REV_DO_HTTPS,self->s_rev_do_https,DO_2_HTTP_MSG_TYPE);
	return nRet;
}

/**
  *  @brief  向dohttps发送消息 
  *  @author  <gy> 
  *  @param[in] <HttpsStruct结构体>
  *  @param[out] <无>
  *  @return   <0代表成功>  <-1代表失败>
  *  @note
*/
int SendMsg2DoHttps(HttpsStruct *self){
	int nRet = 0;
	self->s_send_do_https = self->s_rev_process;
	self->s_send_do_https->msg_type = HTTP_2_DO_MSG_TYPE;
	nRet = SendMsgFunction(self->MSGID_SEND_DO_HTTPS,self->s_send_do_https);
	return nRet;
}

int delay_send(HttpsStruct *self,int times){
	int nRet = 0;
	if (times > 50){
		nRet = SendMsg2DoHttps(self);
		if(-1 == nRet){
			return nRet ;
		}else{
			nRet  = 1;
		}
	}else{
		nRet  = -1;
	}

	return nRet ;
}




