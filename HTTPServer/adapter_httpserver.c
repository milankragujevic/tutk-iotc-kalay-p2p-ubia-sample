/**   @file []
   *  @brief adapter_httpserver的实现
   *  @note
   *  @author:   gy
   *  @date 2013-9-2
   *  @remarks  增加注释
*/
#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include "adapter_httpserver.h"
#include "common/common.h"
#include "common/thread.h"
#include "common/utils.h"
#include "common/common_func.h"
#include "httpsserver.h"
#include "NetWorkSet/networkset.h"
#include "MFI/MFI.h"
#include "p2p/tutk_rdt_video_server.h"
#include "common/common_config.h"

#define Rlength 8
int winR = 0;
volatile int rGetIP = 0;
volatile int rGetRSUCCESS = 0;
volatile int rGetRlSUCCESS = 0;
/**uid的各种情况的处理*/
void reHttpsSeverGetUid(msg_container *self);
/**接口功能暂时保留  */
void HttpsSeverGetUid(msg_container self);
/**接口功能暂时保留  */
void HttpsServerUploadCamInfo(msg_container self);
/**serveruploadcaminfo各种情况的处理  */
void reHttpsServerUploadCamInfo(msg_container *self);
/** 发送验证测试接口  */
void HttpsSeverVerifycamTest(msg_container self);
/** adapter_httpserver遍历功能 */
void HttpServer_cmd_table_fun(msg_container *self);
/** https开始接口*/
void HttpServerStart(msg_container self);
/** https结束接口 */
void HttpServerEnd(msg_container self);
/** 发送验证接口 */
void HttpsSeverVerifycam(msg_container *self);
/** 验证的各种情况的处理  */
void retHttpsSeverVerifycam(msg_container *self);
/** 发送登录接口  */
void HttpsSeverLoginCam(msg_container *self);
/** 暂时保留接口 */
void HttpsSeverLoginCamTest(msg_container self);
/** 登录的各种情况的处理  */
void retHttpsSeverLoginCam(msg_container *self);
/** 暂时保留接口  */
void HttpsSeverPushAlarm(msg_container self);
/** 推送的各种情况处理  */
void retHttpsSeverPushAlarm(msg_container *self);
//void HttpsSeverUploadFileCam(msg_container self);
/** 暂时保留接口  */
void HttpsSeverUploadFileCam(msg_container self);
/** 暂时保留接口  */
void HttpsUpgradeState(msg_container self);
/** 暂时保留接口  */
void reHttpsUpgradeState(msg_container *self);
/** 上传文件的各种情况处理  */
void retHttpsSeverUploadFileCam(msg_container *self);
/** 结束处理 */
void retHttpServerEnd(msg_container *self);
/** 开始的各种处理 */
void retHttpServerStart(msg_container *self);
/**  暂时保留接口  */
void  HttpsAuthenticationPower(msg_container self);
/**  权限的各种情况处理 */
void  reHttpsAuthenticationPower(msg_container *self);
/** 暂时保留接口 */
void HttpsPowerState(msg_container self);
/** 电量报警的各种情况处理  */
void reHttpsPowerState(msg_container *self);

cmd_item HttpServer_cmd_table[] = {

		{MSG_HTTPSSEVER_T_VERIFYCAM, HttpsSeverVerifycam},//Camera 验证（只当USB线连上Cam时进行）
		{MSG_HTTPSSEVER_P_VERIFYCAM, retHttpsSeverVerifycam},//Camera 验证（只当USB线连上Cam时进行）

		{MSG_HTTPSSEVER_T_LOGIN_CAM,HttpsSeverLoginCam},//Camera 注册
		{MSG_HTTPSSEVER_P_LOGIN_CAM,retHttpsSeverLoginCam},//Camera 注册

		{MSG_HTTPSSEVER_T_UPLOAD_FILE_CAM,HttpsSeverUploadFileCam},//Camera上传文件请求（报警，日志）
		{MSG_HTTPSSEVER_P_UPLOAD_FILE_CAM,retHttpsSeverUploadFileCam},//Camera上传文件请求（报警，日志）

		{MSG_HTTPSSEVER_T_PUSH_ALARM_2_CLOUND_CAM,HttpsSeverPushAlarm},//Camera向云服务器推送报警信息
		{MSG_HTTPSSEVER_p_PUSH_ALARM_2_CLOUND_CAM,retHttpsSeverPushAlarm},//Camera向云服务器推送报警信息

		{MSG_HTTPSSEVER_T_GET_UID,HttpsSeverGetUid},
		{MSG_HTTPSSEVER_P_GET_UID,reHttpsSeverGetUid},

		{MSG_HTTPSSERVER_T_STOP,HttpServerEnd},
		{MSG_HTTPSSERVER_P_STOP,retHttpServerEnd},

		{MSG_HTTPSSERVER_T_START,HttpServerStart},
		{MSG_HTTPSSERVER_P_START,retHttpServerStart},

		{MSG_HTTPSSERVER_T_UPLOADING_CAM_INFO,HttpsServerUploadCamInfo},
		{MSG_HTTPSSERVER_P_UPLOADING_CAM_INFO,reHttpsServerUploadCamInfo},

		{MSG_HTTPSSERVER_T_UPDATE_STATE,HttpsUpgradeState},
		{MSG_HTTPSSERVER_P_UPDATE_STATE,reHttpsUpgradeState},

		{MSG_HTTPSSERVER_T_AUTHENTICATION_POWER,HttpsAuthenticationPower},
		{MSG_HTTPSSERVER_P_AUTHENTICATION_POWER,reHttpsAuthenticationPower},

		{MSG_HTTPSPOWER_T_STATE,HttpsPowerState},
		{MSG_HTTPSPOWER_P_STATE,reHttpsPowerState},


};
/**
  *  @brief  权限认证
  *  @author  <gy> 
  *  @param[in] <适配器基本运行环境描述结构体>
  *  @param[out] <无>
  *  @return  <无>
  *  @note
*/
void  HttpsAuthenticationPower(msg_container self)
{


}
int PowerNUM = 0;
/**
  *  @brief  权限的各种情况处理
  *  @author  <gy> 
  *  @param[in] <适配器基本运行环境描述结构体>
  *  @param[out] <无>
  *  @return  <无>
  *  @note
*/
void  reHttpsAuthenticationPower(msg_container *self)
{
	Child_Process_Msg *msgsnd = (Child_Process_Msg*)self->msgdata_snd;
	Child_Process_Msg *msgrev = (Child_Process_Msg*)self->msgdata_rcv;
	msgrev->data[msgrev->length] = 0;
	if(0 == strncmp(msgrev->data,"FAIL",4)){
		//				    		new
		msgsnd->msg_type = HTTP_SERVER_MSG_TYPE;
		msgsnd->Thread_cmd = MSG_HTTPSSERVER_T_AUTHENTICATION_POWER;
		msgsnd->length = 0;
		memset(msgsnd->data,0,strlen(msgsnd->data));
		if(msg_queue_snd(self->rmsgid, msgsnd, sizeof(struct MsgData)-sizeof(long int)) == -1)
		{

		}else{
			printf("Get POWER fail please try again\n");
		}
	}else if(0 == strncmp(msgrev->data,"OK",2)){
		if(DEVICE_TYPE_M3S == g_nDeviceType){
			tutk_and_http_set_led(NET_STATE_CONTROL_HTTP);
			//tutk_login_ok_set_led(LED_M3S_TYPE_NET, LED_CTL_TYPE_ON);
		}
		else{
			tutk_and_http_set_led(NET_STATE_CONTROL_HTTP);
			//tutk_login_ok_set_led(LED_M2_TYPE_MIXED, LED_CTL_TYPE_ON);
		}
	}else if(0 == strncmp(msgrev->data,"223",3)){
		//				    		new
		msgsnd->msg_type = HTTP_SERVER_MSG_TYPE;
		msgsnd->Thread_cmd = MSG_HTTPSSERVER_T_AUTHENTICATION_POWER;
		msgsnd->length = 0;
		memset(msgsnd->data,0,strlen(msgsnd->data));
		if(msg_queue_snd(self->rmsgid, msgsnd, sizeof(struct MsgData)-sizeof(long int)) == -1)
		{

		}else{
			printf("Get POWER fail please try again\n");
		}
	}else if(0 == strncmp(msgrev->data,"SendMSG2CloudERR",strlen("SendMSG2CloudERR"))){
		//				    		new
		msgsnd->msg_type = HTTP_SERVER_MSG_TYPE;
		msgsnd->Thread_cmd = MSG_HTTPSSERVER_T_AUTHENTICATION_POWER;
		msgsnd->length = 0;
		memset(msgsnd->data,0,strlen(msgsnd->data));
		if(msg_queue_snd(self->rmsgid, msgsnd, sizeof(struct MsgData)-sizeof(long int)) == -1)
		{

		}else{
			printf("Get POWER fail please try again\n");
		}
	}else if(0 == strncmp(msgrev->data,"NORL",strlen("NORL"))){
		set_config_value_to_flash(CONFIG_CAMERA_ENCRYTIONID,NULL,0);
		set_config_value_to_flash(CONFIG_P2P_UID,NULL, 0);
		msgsnd->msg_type = HTTP_SERVER_MSG_TYPE;
		msgsnd->Thread_cmd = MSG_HTTPSSEVER_T_VERIFYCAM;
		msgsnd->length = 0;
		if(msg_queue_snd(self->rmsgid, msgsnd, sizeof(struct MsgData)-sizeof(long int)) == -1)
		{

		}else{
			printf("Get POWER fail please try again to get Rl\n");
		}
	}else{
		DEBUGPRINT(DEBUG_INFO, "unknow protocal  wrong wrong wrong..................\n");

	}

}
/**
  *  @brief  上传文件
  *  @author  <gy> 
  *  @param[in] <适配器基本运行环境描述结构体>
  *  @param[out] <无>
  *  @return  <无>
  *  @note
*/
void HttpsSeverUploadFileCam(msg_container self){


}
/**
  *  @brief  推送上传
  *  @author  <gy> 
  *  @param[in] <适配器基本运行环境描述结构体>
  *  @param[out] <无>
  *  @return  <无>
  *  @note
*/
void HttpsSeverPushAlarm(msg_container self){

}
/**
  *  @brief  电量报警
  *  @author  <gy> 
  *  @param[in] <适配器基本运行环境描述结构体>
  *  @param[out] <无>
  *  @return  <0代表成功>   <-1代表失败>
  *  @note
*/
void HttpsPowerState(msg_container self){

}
/**
  *  @brief  电量报警的各种情况处理  
  *  @author  <gy> 
  *  @param[in] <适配器基本运行环境描述结构体>
  *  @param[out] <无>
  *  @return  <0代表成功>   <-1代表失败>
  *  @note
*/
void reHttpsPowerState(msg_container *self){
	Child_Process_Msg *msgsnd = (Child_Process_Msg*)self->msgdata_snd;
	Child_Process_Msg *msgrev = (Child_Process_Msg*)self->msgdata_rcv;
	msgrev->data[msgrev->length] = 0;
	if(0 == strncmp(msgrev->data,"FAIL",4)){
			printf("up powerstate  fail!!\n");
	}else if(0 == strncmp(msgrev->data,"OK",2)){
		    printf("up powerstate  ok!!\n");
	}else if(0 == strncmp(msgrev->data,"SendMSG2CloudERR",strlen("SendMSG2CloudERR"))){
		    printf("up powerstate  SendMSG2CloudERR!!\n");
	}else{

	}

}
/**
  *  @brief   
  *  @author  <gy> 
  *  @param[in] <无>
  *  @param[out] <无>
  *  @return  <0代表成功>   <-1代表失败>
  *  @note
*/
void UID_is_Err(){
	printf("====================================================================\n");
	{
		int lRet = set_config_value_to_flash(CONFIG_P2P_UID,NULL,0 );
		printf("hello zhang shu pan I have UID:zero ");
		DEBUGPRINT(DEBUG_INFO,"lRet = %d\n",lRet);
	}

	msg_container self;
	self.rmsgid =  msg_queue_get((key_t) CHILD_THREAD_MSG_KEY);
	Child_Process_Msg *msgsnd = (Child_Process_Msg*)self.msgdata_snd;
	msgsnd->msg_type = HTTP_SERVER_MSG_TYPE;
	msgsnd->Thread_cmd = MSG_HTTPSSEVER_T_GET_UID;
	msgsnd->length = 0;
	if(msg_queue_snd(self.rmsgid, msgsnd, sizeof(struct MsgData)-sizeof(long int)) == -1)
	{
		printf("send msg err\n");
	}else{
		printf("send msg OK\n");
	}
	printf("====================================================================\n");
}
/**
  *  @brief  adapter_httpserver遍历功能 
  *  @author  <gy> 
  *  @param[in] <适配器基本运行环境描述结构体>
  *  @param[out] <无>
  *  @return  <无>
  *  @note
*/
void HttpServer_cmd_table_fun(msg_container *self) {
	MsgData *msgrcv = (MsgData *)self->msgdata_rcv;
	cmd_item icmd;
	icmd.cmd = msgrcv->cmd;
	msg_fun fun = utils_cmd_bsearch(&icmd, HttpServer_cmd_table, sizeof(HttpServer_cmd_table)/sizeof(cmd_item));
	if(fun != NULL) {
		fun(self);
	}
}
/**
  *  @brief  https开始接口 
  *  @author  <gy> 
  *  @param[in] <适配器基本运行环境描述结构体>
  *  @param[out] <无>
  *  @return  <无>
  *  @note
*/
void HttpServerStart(msg_container self){
	printf("HttpServerStart\n");
	Child_Process_Msg *msgsnd = (Child_Process_Msg*)self.msgdata_snd;
	msgsnd->msg_type = HTTP_SERVER_MSG_TYPE;
	msgsnd->Thread_cmd = MSG_HTTPSSERVER_T_START;
	msgsnd->length = 0;
	if(msg_queue_snd(self.rmsgid, msgsnd/*self->msgdata_snd*/, sizeof(struct MsgData)-sizeof(long int)) == -1)
	{
		printf("send msg 2 httpserver fail\n");
	}else{
		printf("send msg 2 httpserver SUCCESS\n");
	}

}
/**
  *  @brief  开始的各种处理  
  *  @author  <gy> 
  *  @param[in] <适配器基本运行环境描述结构体>
  *  @param[out] <无>
  *  @return  <无>
  *  @note
*/
void retHttpServerStart(msg_container *self){
	Child_Process_Msg *msgsnd = (Child_Process_Msg*)self->msgdata_snd;
	Child_Process_Msg *msgrev = (Child_Process_Msg*)self->msgdata_rcv;
	msgrev->data[msgrev->length] = 0;
	int Lose = 1;
	int Ok = 0;
	if(0 == strncmp(msgrev->data ,"OK",msgrev->length)){
		//没有Rl，api.*****转换成功，网络OK	
		rGetIP = 1;
	}else if(0 == strncmp(msgrev->data ,"network_ok",msgrev->length)){
		//网络设置成功
		memset(msgrev->data,'\0',MAX_MSG_LEN);
		msgsnd->msg_type = NETWORK_SET_MSG_TYPE;
		msgsnd->Thread_cmd = MSG_NETWORKSET_T_NET_CONNECT_STATUS;
		memcpy(msgsnd->data,&Ok,sizeof(int));
		msgsnd->length = sizeof(int);
		if(msg_queue_snd(self->rmsgid, msgsnd, sizeof(struct MsgData)-sizeof(long int)+msgsnd->length) == -1)
		{

		}else{
			printf("network is ok ,sendmessage to Networkset thread\n");
		}
	}else if(0 == strncmp(msgrev->data ,"network_fail",msgrev->length)){
		//网络设置失败
		msgsnd->msg_type = NETWORK_SET_MSG_TYPE;
		msgsnd->Thread_cmd = MSG_NETWORKSET_T_NET_CONNECT_STATUS;
		memcpy(msgsnd->data,&Lose,sizeof(int));
		msgsnd->length = sizeof(int);
		if(msg_queue_snd(self->rmsgid, msgsnd, sizeof(struct MsgData)-sizeof(long int) + msgsnd->length) == -1)
		{

		}else{

		}
	}

#if M3S_New == 1

	else if(0 == strncmp(msgrev->data ,"HAVE",msgrev->length)){
		//有网有Rl，直接申请UID
		msgsnd->msg_type = HTTP_SERVER_MSG_TYPE;
		msgsnd->Thread_cmd = MSG_HTTPSSEVER_T_GET_UID;
		msgsnd->length = 0;
		if(msg_queue_snd(self->rmsgid, msgsnd, sizeof(struct MsgData)-sizeof(long int)) == -1)
		{

		}else{

		}
	}else{

	}
#endif

}
/**
  *  @brief  发送验证接口 
  *  @author  <gy> 
  *  @param[in] <适配器基本运行环境描述结构体>
  *  @param[out] <无>
  *  @return  <无>
  *  @note
*/
void HttpsSeverVerifycam(msg_container* self){
	Child_Process_Msg *msgsnd = (Child_Process_Msg*)self->msgdata_snd;
	msgsnd->msg_type = HTTP_SERVER_MSG_TYPE;
	msgsnd->Thread_cmd = MSG_HTTPSSEVER_T_VERIFYCAM;
	msgsnd->length = 0;
	if(msg_queue_snd(self->rmsgid, msgsnd, sizeof(struct MsgData)-sizeof(long int)) == -1){

	}else{
		printf("start Verifycam please wait \n");
	}
}
/**
  *  @brief  发送验证测试接口 
  *  @author  <gy> 
  *  @param[in] <适配器基本运行环境描述结构体>
  *  @param[out] <无>
  *  @return  <无>
  *  @note
*/
void HttpsSeverVerifycamTest(msg_container self){

	Child_Process_Msg *msgsnd = (Child_Process_Msg*)self.msgdata_snd;
	msgsnd->msg_type = HTTP_SERVER_MSG_TYPE;
	msgsnd->Thread_cmd = MSG_HTTPSSEVER_T_VERIFYCAM;
	msgsnd->length = 0;
	if(msg_queue_snd(self.rmsgid, msgsnd, sizeof(struct MsgData)-sizeof(long int)) == -1){

	}else{
		printf("start Verifycam please wait \n");
	}
}
/**
  *  @brief  验证的各种情况的处理 
  *  @author  <gy> 
  *  @param[in] <适配器基本运行环境描述结构体>
  *  @param[out] <无>
  *  @return  <无>
  *  @note
*/
void retHttpsSeverVerifycam(msg_container *self){
	Child_Process_Msg *msgsnd = (Child_Process_Msg*)self->msgdata_snd;
	Child_Process_Msg *msgrev = (Child_Process_Msg*)self->msgdata_rcv;
	msgrev->data[msgrev->length] = 0;
	if(0 == strncmp(msgrev->data,"FAIL",4)){

	}else if(0 == strncmp(msgrev->data,"OK",2)){
		rGetRSUCCESS = 1;
		//send msg to LOGIN_CAM
		memcpy(msgsnd->data,msgrev->data,msgrev->length);
		msgsnd->msg_type = HTTP_SERVER_MSG_TYPE;
		msgsnd->Thread_cmd = MSG_HTTPSSEVER_T_LOGIN_CAM;
		msgsnd->length = msgrev->length;
		if(msg_queue_snd(self->rmsgid, msgsnd, sizeof(struct MsgData)-sizeof(long int)+msgsnd->length) == -1)
		{

		}else{

		}
	}else if(0 == strncmp(msgrev->data,"SendMSG2CloudERR",strlen("SendMSG2CloudERR"))){
		HttpsSeverVerifycam(self);
		//##########################################################################//

	}else if(0 == strncmp(msgrev->data,"FAIL",strlen("FAIL"))){
		HttpsSeverVerifycam(self);
	}
}
#if 1
//no use
/**
  *  @brief  发送登录接口
  *  @author  <gy> 
  *  @param[in] <适配器基本运行环境描述结构体>
  *  @param[out] <无>
  *  @return  <无>
  *  @note
*/
void HttpsSeverLoginCam(msg_container* self){
	Child_Process_Msg *msgsnd = (Child_Process_Msg*)self->msgdata_snd;
	msgsnd->msg_type = HTTP_SERVER_MSG_TYPE;
	msgsnd->Thread_cmd = MSG_HTTPSSEVER_T_LOGIN_CAM;
	msgsnd->length = 0;
	if(msg_queue_snd(self->rmsgid, msgsnd, sizeof(struct MsgData)-sizeof(long int)) == -1)
	{

	}else{

	}
}
#endif
#if M3S_New == 1
//no use
void HttpsSeverLoginCamTest(msg_container self){
	Child_Process_Msg *msgsnd = (Child_Process_Msg*)self.msgdata_snd;
	msgsnd->msg_type = HTTP_SERVER_MSG_TYPE;
	msgsnd->Thread_cmd = MSG_HTTPSSEVER_T_LOGIN_CAM;
	msgsnd->length = 0;
	if(msg_queue_snd(self.rmsgid, msgsnd, sizeof(struct MsgData)-sizeof(long int)) == -1)
	{

	}else{

	}
}
#endif
/**
  *  @brief  登录的各种情况的处理
  *  @author  <gy> 
  *  @param[in] <适配器基本运行环境描述结构体>
  *  @param[out] <无>
  *  @return  <无>
  *  @note
*/
void retHttpsSeverLoginCam(msg_container *self){
	Child_Process_Msg *msgsnd = (Child_Process_Msg*)self->msgdata_snd;
	Child_Process_Msg *msgrev = (Child_Process_Msg*)self->msgdata_rcv;
	msgrev->data[msgrev->length] = 0;
	printf("ret Https Sever Login Cam \n");
	if(0 == strncmp(msgrev->data,"FAIL",4)){
		HttpsSeverVerifycam(self);
	}else if(0 == strncmp(msgrev->data,"OK",2)){

#if M3S_New == 1

		msgsnd->msg_type = HTTP_SERVER_MSG_TYPE;
		msgsnd->Thread_cmd = MSG_HTTPSSEVER_T_GET_UID;
		msgsnd->length = 0;
		if(msg_queue_snd(self->rmsgid, msgsnd, sizeof(struct MsgData)-sizeof(long int)) == -1)
		{

		}else{

		}
		
#endif
	}else if(0 == strncmp(msgrev->data,"SetIPU_R_FAIL",strlen("SetIPU_R_FAIL"))){
		HttpsSeverVerifycam(self);
		/***********************************************************************/
	}else if(0 == strncmp(msgrev->data,"SendMSG2CloudERR",strlen("SendMSG2CloudERR"))){
		/***********************************************************************/
	}else{
	
	}
}

/**
  *  @brief  接口功能暂时保留
  *  @author  <gy> 
  *  @param[in] <适配器基本运行环境描述结构体>
  *  @param[out] <无>
  *  @return  <无>
  *  @note
*/
void HttpsSeverGetUid(msg_container self){

}
//int Get_UID_times = 0;
/**
  *  @brief  uid的各种情况的处理
  *  @author  <gy> 
  *  @param[in] <适配器基本运行环境描述结构体>
  *  @param[out] <无>
  *  @return  <无>
  *  @note
*/
void reHttpsSeverGetUid(msg_container *self){
	Child_Process_Msg *msgsnd = (Child_Process_Msg*)self->msgdata_snd;
	Child_Process_Msg *msgrev = (Child_Process_Msg*)self->msgdata_rcv;
	msgrev->data[msgrev->length] = 0;
	int n = 0;
	char TempBuffer[255];
	memset(TempBuffer,0,sizeof(TempBuffer));
	if(0 == strncmp(msgrev->data,"FAIL",4)){
//		if(Get_UID_times > 3){
	    /************************************************************************************************/
			get_config_item_value(CONFIG_CAMERA_ENCRYTIONID,TempBuffer, &n);
			if(n>0){
				set_config_value_to_flash(CONFIG_CAMERA_ENCRYTIONID,NULL,0);
				set_config_value_to_flash(CONFIG_P2P_UID,NULL, 0);
				msgsnd->msg_type = HTTP_SERVER_MSG_TYPE;
				msgsnd->Thread_cmd = MSG_HTTPSSEVER_T_VERIFYCAM;
				msgsnd->length = 0;
				if(msg_queue_snd(self->rmsgid, msgsnd, sizeof(struct MsgData)-sizeof(long int)) == -1)
				{

				}else{
					printf("Get UID fail please try again\n");
				}
			}else{

			}
//			Get_UID_times ++;
//		}
	}else if(0 == strncmp(msgrev->data,"HAVEUID",strlen("HAVEUID"))){
		//has uid
	}else if(0 == strncmp(msgrev->data,"NORL",strlen("NORL"))){
		set_config_value_to_flash(CONFIG_CAMERA_ENCRYTIONID,NULL,0);
		set_config_value_to_flash(CONFIG_P2P_UID,NULL, 0);
		msgsnd->msg_type = HTTP_SERVER_MSG_TYPE;
		msgsnd->Thread_cmd = MSG_HTTPSSEVER_T_VERIFYCAM;
		msgsnd->length = 0;
		if(msg_queue_snd(self->rmsgid, msgsnd, sizeof(struct MsgData)-sizeof(long int)) == -1)
		{

		}else{
			printf("Get UID fail please try again\n");
			printf("Get UID fail please try again\n");
			printf("Get UID fail please try again\n");
			printf("Get UID fail please try again\n");
			printf("Get UID fail please try again\n");
			printf("Get UID fail please try again\n");
			printf("Get UID fail please try again\n");
			printf("Get UID fail please try again\n");
		}
	}else if(0 == strncmp(msgrev->data,"SendMSG2CloudERR",strlen("SendMSG2CloudERR"))){
		/************************************************************************************************/
		
	
	}else{
//		Get_UID_times = 0;
		msgsnd->msg_type = P2P_MSG_TYPE;
		msgsnd->Thread_cmd = MSG_P2P_T_TUTKID;
		memcpy(msgsnd->data,msgrev->data,msgrev->length);
		msgsnd->length = msgrev->length;
		if(msg_queue_snd(self->rmsgid, msgsnd, sizeof(struct MsgData)-sizeof(long int) + msgsnd->length) == -1)
		{

		}else{

		}

		//uid success
		android_usb_send_msg(self->rmsgid, MFI_MSG_TYPE, 12);
		msgsnd->msg_type = HTTP_SERVER_MSG_TYPE;
		msgsnd->Thread_cmd = MSG_HTTPSSERVER_T_AUTHENTICATION_POWER;
		msgsnd->length = 0;
		memset(msgsnd->data,0,strlen(msgsnd->data));
		if(msg_queue_snd(self->rmsgid, msgsnd, sizeof(struct MsgData)-sizeof(long int)) == -1)
		{

		}else{
			//printf("Get UID fail please try again\n");
		}

	}
}
/**
  *  @brief  接口功能暂时保留
  *  @author  <gy> 
  *  @param[in] <适配器基本运行环境描述结构体>
  *  @param[out] <无>
  *  @return  <无>
  *  @note
*/
void HttpsServerUploadCamInfo(msg_container self){

}
/**
  *  @brief  serveruploadcaminfo各种情况的处理
  *  @author  <gy> 
  *  @param[in] <适配器基本运行环境描述结构体>
  *  @param[out] <无>
  *  @return  <无>
  *  @note
*/
void reHttpsServerUploadCamInfo(msg_container *self){
	Child_Process_Msg *msgsnd = (Child_Process_Msg*)self->msgdata_snd;
	Child_Process_Msg *msgrev = (Child_Process_Msg*)self->msgdata_rcv;
	msgrev->data[msgrev->length] = 0;
	printf("ret Https Sever reHttpsServerUploadCamInfo \n");
	if(0 == strncmp(msgrev->data,"FAIL",4)){

	}else if(0 == strncmp(msgrev->data,"OK",2)){

		printf("############################################################## \n"
				"##################### "
				"reHttpsServerUploadCamInfo"
				"######################\n"
				"#############################################################\n");
		winR++;
		printf("send msg 2 MFI is ok.%d...\n",winR);
	}
}
/**
  *  @brief  暂时保留接口
  *  @author  <gy> 
  *  @param[in] <适配器基本运行环境描述结构体>
  *  @param[out] <无>
  *  @return  <无>
  *  @note
*/
void HttpsUpgradeState(msg_container self){

}
/**
  *  @brief  暂时保留接口
  *  @author  <gy> 
  *  @param[in] <适配器基本运行环境描述结构体>
  *  @param[out] <无>
  *  @return  <无>
  *  @note
*/
void reHttpsUpgradeState(msg_container *self){

}
/**
  *  @brief   推送的各种情况处理
  *  @author  <gy> 
  *  @param[in] <适配器基本运行环境描述结构体>
  *  @param[out] <无>
  *  @return  <无>
  *  @note
*/
void retHttpsSeverPushAlarm(msg_container *self){
	Child_Process_Msg *msgsnd = (Child_Process_Msg*)self->msgdata_snd;
	Child_Process_Msg *msgrev = (Child_Process_Msg*)self->msgdata_rcv;
	printf("retHttpsSeverPushAlarm is here\n");
	msgrev->data[msgrev->length] = 0;
	if(0 == strncmp(msgrev->data,"OK",2)){

	}else if(0 == strncmp(msgrev->data,"FAIL",4)){
		
	}else if(0 == strncmp(msgrev->data,"SendMSG2CloudERR",strlen("SendMSG2CloudERR"))){
		
	}else{
	
	}
}
/**
  *  @brief   上传文件的各种情况处理
  *  @author  <gy> 
  *  @param[in] <适配器基本运行环境描述结构体>
  *  @param[out] <无>
  *  @return  <无>
  *  @note
*/
void retHttpsSeverUploadFileCam(msg_container *self){
	Child_Process_Msg *msgsnd = (Child_Process_Msg*)self->msgdata_snd;
	Child_Process_Msg *msgrev = (Child_Process_Msg*)self->msgdata_rcv;
	msgrev->data[msgrev->length] = 0;
	if(0 == strncmp(msgrev->data,"OK",2)){

	}else if(0 == strncmp(msgrev->data,"FAIL",4)){
		
	}else if(0 == strncmp(msgrev->data,"SendMSG2CloudERR",strlen("SendMSG2CloudERR"))){
		
	}else{
	
	}
}
/**
  *  @brief   https结束接口
  *  @author  <gy> 
  *  @param[in] <适配器基本运行环境描述结构体>
  *  @param[out] <无>
  *  @return  <无>
  *  @note
*/
void HttpServerEnd(msg_container self){
	Child_Process_Msg *msgsnd = (Child_Process_Msg*)self.msgdata_snd;
	msgsnd->msg_type = HTTP_SERVER_MSG_TYPE;
	msgsnd->Thread_cmd = MSG_HTTPSSERVER_T_STOP;
	msgsnd->length = 0;
	if(msg_queue_snd(self.rmsgid,msgsnd, sizeof(struct MsgData)-sizeof(long int)) == -1)
	{

	}

}
/**
  *  @brief   结束处理
  *  @author  <gy> 
  *  @param[in] <适配器基本运行环境描述结构体>
  *  @param[out] <无>
  *  @return  <无>
  *  @note
*/
void retHttpServerEnd(msg_container *self){
	Child_Process_Msg *msgsnd = (Child_Process_Msg*)self->msgdata_snd;
	Child_Process_Msg *msgrev = (Child_Process_Msg*)self->msgdata_rcv;
	msgrev->data[msgrev->length] = 0;
	msgsnd->msg_type = HTTP_SERVER_MSG_TYPE;
	msgsnd->Thread_cmd = MSG_HTTPSSERVER_T_STOP;
	msgsnd->length = 0;
	if(msg_queue_snd(self->rmsgid, self->msgdata_snd, sizeof(struct MsgData)-sizeof(long int)) == -1)
	{

	}

}
