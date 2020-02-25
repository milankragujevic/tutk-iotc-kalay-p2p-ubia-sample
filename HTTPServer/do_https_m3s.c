/**   @file []
   *  @brief do_https的实现
   *  @note
   *  @author:   gy
   *  @date 2013-9-2
   *  @remarks 增加注释
*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <time.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include "AudioAlarm/testcommon.h"
#include "common/logfile.h"
#include "tools.h"
#include "encryption.h"
#include "do_https_m3s.h"
#include "httpsserver.h"
#include "common/thread.h"
#include "common/common.h"
#include "common/common_config.h"
#include "common/jansson.h"
#include "common/logserver.h"


/**照片文件名的长度*/
#define PHOTONAME_LENGTH 128

/**dohttps线程结构体*/
typedef struct {
	int MSGID_REV_DO_HTTPS ;             ///<dohttps线程接收的id
	int MSGID_SEND_DO_HTTPS ;            ///<dohttps线程发送的id
	Child_Process_Msg * s_rev_https;     ///<接收消息的结构体
	Child_Process_Msg * s_send_https;    ///<发送消息的结构体
}stDoHttpsMSG ;

/**key R‘的结构体*/
typedef struct{
	int length_KEY;                     ///<key长度
	char pcPbKeyValue[256]; 			///<公钥
    char Rcheck[512];					///<R'
    char Rcheck_URL[512];				///<R'_URL
    char strKeyN[172+4];				///<
    char strKeyE[4+4];					///<模数

}stRcheck;

/**上传文件的结构体*/
typedef struct{
	int filegroup ;						///<文件组数
	int alarmtype ;						///<报警类型
	int DataLength;						///<数据长度
	int number;							///<图片的顺序
	char pFileName[PHOTONAME_LENGTH];   ///<文件的名字
	char *pPhotodata;					///<文件值的指针
}stPhoto;

/**与dohttps同步标志*/
volatile int R_HttpsHost;
/**与子进程同步标志*/
volatile int R_HttpsThread;
/**设置网络标志*/
volatile int flag_setnetwork;
/**uid是否存在标志*/
volatile int my_uid;
/**权限认证保存数据*/
volatile  conncheck   user_pm[3];


/**dohttps线程初始化*/
int InitArgsHttpsThread(stDoHttpsMSG *args);
/**get方法报警推送拼协议函数jansson*/
int GetBuildMSG(char **ppSendMsg,char *IP,char *Camera_EncrytionID,const int Port,const char* Addr,int alarmgroup,int alarmtype,char *SV);
/**post方法拼协议头函数jansson*/
int PostBuildMSG(char **pSendMsg,char *IP,char *Camera_EncrytionID,const int Port,const char* addr,char *SV);
/**post方法拼文件上传协议函数jansson*/
int POSTUploadFileMSG(char **pSendMsg,char *IP,char *Camera_EncrytionID,const int Port,const char* addr,int alarmtype,const char *filetype,
		long int filegroup,const int number,long int fileleng,char *file,char *SV);
/**get方法拼协议头函数 */
int GetCameraMSG(char **ppSendMsg,char *IP,char *Camera_EncrytionID,const int Port,const char* Addr,char *SV);
/**得到uid从云端 */
int getUID4Cloud(char *CloudReturnValue,char *UID);
/**接收消息函数 */
int RevMsgHttps(stDoHttpsMSG *self);
/**post方法拼权限认证协议函数jansson */
int PostBuildMsg_AuthenticationPower(char **ppSendMsg,char *IP,char *Camera_EncrytionID,const int Port,const char* Addr,char *SV);
/**发送消息函数 */
int SendMsgHttps(stDoHttpsMSG *self,const int CMD,char *data,int length);
/**发送消息给云端函数 */
int Send2CloudMSG(char **cloud_return_value,char *https_2_cloudmsg,char *IP,int Port,int msglength);
/**根据主机名获得ip */
int GetIPByHost(char *pIP, const char *pHostName);
/**得到IPU_R */
int SetIPU_R(char *r,char *strIPUEnrID,char *strIPUEnrURLID,char * strkey_N,char *strkey_E);
/** 发送升级状态给云端函数*/
int Send2CloudMSG4UpgradeState(char *https_2_cloudmsg,char *IP,int Port,int msglength);
/*暂不再使用 */
int GetCameraMSG_info(char **ppSendMsg,char *IP,char *Camera_EncrytionID,const int Port,const char* Addr,char *SV);
/**post方法拼推送协议的函数 */
int POSTBuildMSG_PushMsg(char **ppSendMsg,char *IP,char *Camera_EncrytionID,const int Port,const char* Addr,int filegroup,int alarmtype,char *SV);
/**post方法拼电量状态协议函数*/
int PostPower_State(char **ppSendMsg,char *IP,char *Camera_EncrytionID,const int Port,const char* Addr,char *EnergyS,char *ChargeS,char *sv);

int write_file(char * data);
int write_file1(char * data,const char *ret);
int write_log(long date,char *ret_json);
#define DOT_OR_DOTDOT(s) ((s)[0] == '.' && (!(s)[1] || ((s)[1] == '.' && !(s)[2])))

char* c_new_path(char *mypath,char *path,char *d_name){

	memset(mypath,0,sizeof(mypath));
	if (d_name && DOT_OR_DOTDOT(d_name))
		return NULL;
	sprintf(mypath,"%s/%s",path,d_name);
	return mypath;
}
/**
  *  @brief  根据path删除文件
  *  @author  <gy> 
  *  @param[in] <path为删除目录>
  *  @param[out] <无>
  *  @return  <0代表成功>   <-1代表失败>
  *  @note
*/
int my_remove_file(char * path){
	char mypath[140];
	printf("__LINE__:%d\n",__LINE__);
	struct stat path_stat;
	printf("path:%s\n",path);
	if(lstat(path,&path_stat)){
		return -1;
	}
	printf("path_stat:%d\n",path_stat.st_mode);
	if(S_ISDIR(path_stat.st_mode)){
		printf("S_ISDIR\n");
		DIR *dp;
		struct dirent *d;
		dp = opendir(path);
		if(dp == NULL){
			return -1;
		}

		char *new_path;
		while((d = readdir(dp))!=NULL){
			new_path = c_new_path(mypath,path,d->d_name);
			printf("__LINE__:%d",__LINE__);
			my_remove_file(new_path);
			printf("__LINE__:%d",__LINE__);
		}
		printf("\n");

		if (closedir(dp) < 0) {
		//	bb_perror_msg("can't close '%s'", path);
			return -1;
		}
		if (rmdir(path) < 0) {
			//bb_perror_msg("can't remove '%s'", path);
			return -1;
		}
	}else{
		if(0!=remove(path)){
			perror("remove error\n");
			return -1;
		}else{
			printf("success remove :%s\n",path);
		}
	}

	return 0;
}

int makeheadinfo(char *Rl){


	return 0;
}

/**
  *  @brief  dohttps线程处理过程函数
  *  @author  <gy> 
  *  @param[in] <无>
  *  @param[out] <无>
  *  @return     <-1代表失败>
  *  @note
*/
void *DO_Https_Thread(){

	stPhoto *PhotoData = NULL;
	PhotoData = malloc(sizeof(stPhoto));
	if(PhotoData == NULL){

	}else{
		memset(PhotoData,0,sizeof(stPhoto));
	}
	stRcheck *RcheckBuffer = NULL;
	RcheckBuffer = malloc(sizeof(stRcheck));
	if(RcheckBuffer == NULL){

	}else{
		memset(RcheckBuffer,0,sizeof(stRcheck));
	}
	stDoHttpsMSG * doHttpsMsg = NULL;
	doHttpsMsg = malloc(sizeof(stDoHttpsMSG));
	if(-1 == InitArgsHttpsThread(doHttpsMsg)){

	}
	char UID[30];
	char IP[17];
	char R[8+1] ;
	char CMD[100];
	char R_16[16+1];
	char PICfilename[100];
	char ServerAddr[200];
	int ServerPort = 8443;
	char TempRl[512];
	memset(TempRl,'\0',512);
	memset(R,0,sizeof(R));
	int resFlag = 0;
	int elsef_lag = 0;
	int update_server_flag = 0;
	int n = 0;
	int msglength = 0;
	char *Https2CloudMSG = NULL;
	char *CloudReturnValue = NULL;
	typedef struct Sometmp_t {
		char cameraSSID[256];
		char cameraIp[256];
		char buf[256];
		int len;
		FILE *file;
	}Sometmp_t;

	Sometmp_t sometmp_t = {0};
#if 0
	set_config_value_to_flash(CONFIG_CAMERA_TYPE,"M3S", strlen("M3S"));
#endif
	memset(UID,'\0',sizeof(UID));
	int cmpSSIDAndIp() {
		if(EXIT_SUCCESS == get_config_item_value(CONFIG_WIFI_SSID, sometmp_t.buf, &sometmp_t.len)) {

		} else {
			LOG_TEST("cmpSSIDAndIp: get_config_item_value\n");
		}
		if(0 == strcmp(sometmp_t.cameraSSID, sometmp_t.buf)) {
			get_cur_ip_addr(sometmp_t.buf);
			if(0 == strcmp(sometmp_t.cameraIp, sometmp_t.buf)) {

			} else {
				strcpy(sometmp_t.cameraIp, sometmp_t.buf);
				return 1;
			}
		} else {
			strcpy(sometmp_t.cameraSSID, sometmp_t.buf);
			get_cur_ip_addr(sometmp_t.buf);
			strcpy(sometmp_t.cameraIp, sometmp_t.buf);
			return 2;
		}
		return 0;
	}

	int postSSIDAndIp() {
		if(-1 == POSTBuildMSG_SSIDUpload(&Https2CloudMSG,IP,R_16, ServerPort,
				ADDR_PUSH_SSID_IP_2_CLOUND_CAM,sometmp_t.cameraSSID,
				sometmp_t.cameraIp, "06fae42e7bc148af9ce235044ca4de67")){

		}else{
			printf("****************************************************************\n");
			printf("**********************POSTBuildMSG_SSIDUpload*******************\n");
			printf("****************************************************************\n");
			if(-1 == Send2CloudMSG(&CloudReturnValue,Https2CloudMSG,IP,ServerPort,strlen(Https2CloudMSG))){

			}else{
				sometmp_t.file = fopen("/usr/share/ssidupload", "w");
				if(NULL != sometmp_t.file) {
					fprintf(sometmp_t.file, "%s\n%s\n", Https2CloudMSG, CloudReturnValue);
					fclose(sometmp_t.file);
					sometmp_t.file = NULL;
				}
				LOG_TEST("%s\n", CloudReturnValue);
			}
		}
		return 0;
	}
	while(1){
		memset(doHttpsMsg->s_rev_https->data,0,MAX_MSG_LEN);
		memset(doHttpsMsg->s_send_https->data,0,MAX_MSG_LEN);
		elsef_lag = 0;
		n = 0;
		child_setThreadStatus(HTTPS_SERVER_DO_TYPE, NULL);
		if(RevMsgHttps(doHttpsMsg) ==-1){
			thread_usleep(300000);
		}else{
			switch(doHttpsMsg->s_rev_https->Thread_cmd){
/***********************************************************************************************************************/
				case MSG_HTTPSSERVER_T_START:
					flag_setnetwork = 1;
//					my_uid = 1;
					get_config_item_value(CONFIG_SERVER_URL,ServerAddr, &n);
					if(n>0){
						n=0;
					}else{
						n=0;
					}
					memset(IP,0,sizeof(IP));
					//解析api.icamreaXXXX.com成功
					while(0 != GetIPByHost(IP, ServerAddr)){
						sleep(1);
					}

					log_s("Guozhixin GetIP OK",strlen("Guozhixin GetIP OK"));

					//if(0 == GetIPByHost(IP, ServerAddr)){
						//解析api.icamreaXXXX.com成功后，检测flash中是否有Rl
						flag_setnetwork = 0;
						SendMsgHttps(doHttpsMsg,MSG_HTTPSSERVER_P_START,"network_ok",strlen("network_ok"));
						get_config_item_value(CONFIG_CAMERA_ENCRYTIONID,RcheckBuffer->Rcheck, &n);
						if(n > 0){
							//有返回HAVE
							setRl(RcheckBuffer->Rcheck, strlen(RcheckBuffer->Rcheck));
							SendMsgHttps(doHttpsMsg,MSG_HTTPSSERVER_P_START,"HAVE",strlen("HAVE"));
						}else{
							//没有返回OK
							SendMsgHttps(doHttpsMsg,MSG_HTTPSSERVER_P_START,"OK",strlen("OK"));
						}
					//}else{
						//域名解析api.icamreaXXXX.com失败，网站出错，或网络出错
					//	SendMsgHttps(doHttpsMsg,MSG_HTTPSSERVER_P_START,"network_fail",strlen("network_fail"));
					//}
					break;
/***********************************************************************************************************************/
				case MSG_HTTPSSEVER_T_VERIFYCAM:
					printf("www is %s IP %s\n",ServerAddr,IP);
					android_usb_send_msg(doHttpsMsg->MSGID_REV_DO_HTTPS, MFI_MSG_TYPE, 8);
					//NO Return......
					//验证是否有Rl
					get_config_item_value(CONFIG_CAMERA_ENCRYTIONID,RcheckBuffer->Rcheck, &n);
					if(n > 0){
						//如果有发送HAD
						log_s("Guozhixin Have R",strlen("Guozhixin Have R"));
						SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_VERIFYCAM,"HAD",strlen("HAD"));
						//初始化标志位
						elsef_lag = 0;
						break;
					}else{
						printf("==============================================================================================\n");
						printf("==============================================================================================\n");
						//如果没有品字符串
						if(update_server_flag == 0){
							if(-1 == GetCameraMSG(&Https2CloudMSG,IP,"",ServerPort,"","")){

							}else{
								//向云端发送请求
								if(-1 == Send2CloudMSG4UpgradeState(Https2CloudMSG,IP,ServerPort,strlen(Https2CloudMSG))){

								}else{
									//有返回值，或者无超时，指标值位
									elsef_lag = 1;
								}
							}
							printf("==============================================================================================\n");
							//如果消息标志为1
							if(1 == elsef_lag){
								printf("<><>------------------------MSG_HTTPSSERVER_T_UPLOADING_CAM_INFO---------------------------<><>\n");
								DEBUGPRINT(DEBUG_INFO,"<><>------------------------MSG_HTTPSSERVER_T_UPLOADING_CAM_INFO---------------------------<><>\n");
							}else{
								printf("<><>------------------------MSG_HTTPSSERVER_T_UPLOADING_CAM_INFO-------FailFailFailFailFailFailFail--------------------<><>\n");
								DEBUGPRINT(DEBUG_INFO,"<><>------------------------MSG_HTTPSSERVER_T_UPLOADING_CAM_INFO-------FailFailFailFailFailFailFail--------------------<><>\n");
							}
							elsef_lag = 0;
							update_server_flag = 1;
							free(Https2CloudMSG);
							Https2CloudMSG = NULL;
						}
					}

					memset(R_16,'\0',sizeof(R_16));
					memcpy(R_16,RcheckBuffer->Rcheck,16);
					printf("R_16 = %s\n",R_16);

					if(-1 == PostBuildMSG(&Https2CloudMSG,IP,R_16,ServerPort,ADDR_VERIFYCAM,SV_ADDR_VERIFYCAM)){

					}else{
						printf("****************************************************************\n");
						printf("**********************VERIFYCAM string finish*******************\n");
						printf("****************************************************************\n");
						if(-1 == Send2CloudMSG(&CloudReturnValue,Https2CloudMSG,IP,ServerPort,strlen(Https2CloudMSG))){
						
						}else{
							elsef_lag = 1;
						}
					}
					
					json_t *revobject1;
					json_error_t error1;
					char *str1 = NULL,*str2 = NULL;
					revobject1=json_object();

					if(elsef_lag == 1){
						revobject1=json_loads(CloudReturnValue,JSON_COMPACT,&error1);
						printf("__LINE__:%d\n",__LINE__);
						if(0!=json_unpack(revobject1,"{s:s}","ResultMessage",&str1)){
							printf("__LINE__:%d\n",__LINE__);
							SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_VERIFYCAM,"FAIL",strlen("FAIL"));
						}else{
							printf("__LINE__:%d\n",__LINE__);
							if(strcmp(str1,"100")==0){
								printf("__LINE__:%d\n",__LINE__);
								json_unpack(revobject1,"{s:s}","ReturnValue",&str2);
								if(strlen(str2) == 8){
									memset(R,'\0',sizeof(R));
									memcpy(R,str2,8);
									SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_VERIFYCAM,"OK",strlen("OK"));
								}else{
									printf("__LINE__:%d\n",__LINE__);
								}
								printf("__LINE__:%d\n",__LINE__);
							}else if(strcmp(str1,"211")==0){
								SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_VERIFYCAM,"FAIL",strlen("FAIL"));
							}else if(strcmp(str1,"500")==0){
								SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_VERIFYCAM,"FAIL" ,strlen("FAIL"));
							}else{
								SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_VERIFYCAM,"FAIL" ,strlen("FAIL"));
							}
						}
					}else{
						SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_VERIFYCAM,"SendMSG2CloudERR",strlen("SendMSG2CloudERR"));
					}

					elsef_lag = 0;
					free(Https2CloudMSG);
					Https2CloudMSG = NULL;
					free(CloudReturnValue);
					CloudReturnValue = NULL;
					json_decref(revobject1);
					//json_decref(revnextobject);
					break;
/***********************************************************************************************************************/
				case MSG_HTTPSSEVER_T_LOGIN_CAM:
					DEBUGPRINT(DEBUG_INFO,"ServerAddr is %s IP %s\n",www,IP);
					//NO Return......
					get_pb_key_value(RcheckBuffer->pcPbKeyValue, &RcheckBuffer->length_KEY);
					memcpy(RcheckBuffer->strKeyN,RcheckBuffer->pcPbKeyValue,172);
					memcpy(RcheckBuffer->strKeyE,RcheckBuffer->pcPbKeyValue+172,4);
					if(0 == SetIPU_R(R,RcheckBuffer->Rcheck,RcheckBuffer->Rcheck_URL,RcheckBuffer->strKeyN,RcheckBuffer->strKeyE)){

					}else{
						SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_LOGIN_CAM,"SetIPU_R_FAIL",strlen("SetIPU_R_FAIL"));
						break;
					}

					if(-1 == PostBuildMSG(&Https2CloudMSG,IP,RcheckBuffer->Rcheck,ServerPort,ADDR_LOGIN_CAM,SV_ADDR_LOGIN_CAM)){

					}else{
						if(-1 == Send2CloudMSG(&CloudReturnValue,Https2CloudMSG,IP,ServerPort,strlen(Https2CloudMSG))){

						}else{
							elsef_lag = 1;
						}
					}

					json_t *revobject2;
					json_error_t error2;
					char *str3 = NULL;
					revobject2=json_object();

					if(1 == elsef_lag){
						revobject2=json_loads(CloudReturnValue,JSON_COMPACT,&error2);
						if(0!=json_unpack(revobject2,"{s:s}","ResultMessage",&str3)){
							SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_LOGIN_CAM,"FAIL",strlen("FAIL"));
						}else{
							DEBUGPRINT(DEBUG_INFO," revmessag form clond login_cam:ResultMessage=%s  \n",str3);
							if(strcmp(str3,"100")==0)
							{
								{
									int lRet = set_config_value_to_flash(CONFIG_CAMERA_ENCRYTIONID,RcheckBuffer->Rcheck, strlen(RcheckBuffer->Rcheck));
									DEBUGPRINT(DEBUG_INFO,"lRet = %d\n",lRet);
									setRl(RcheckBuffer->Rcheck, strlen(RcheckBuffer->Rcheck));
								}
								SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_LOGIN_CAM,"OK",strlen("OK"));
							}else if(strcmp(str3,"211")==0){
								SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_LOGIN_CAM,"NORL",strlen("NORL"));
							}else if(strcmp(str3,"500")==0){
								SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_LOGIN_CAM,"FAIL",strlen("FAIL"));
							}else{
								SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_LOGIN_CAM,"FAIL",strlen("FAIL"));
							}
						}

					}
					else{
						SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_VERIFYCAM,"SendMSG2CloudERR",strlen("SendMSG2CloudERR"));
					}
					elsef_lag = 0;
					free(Https2CloudMSG);
					Https2CloudMSG = NULL;
					free(CloudReturnValue);
					CloudReturnValue = NULL;
					json_decref(revobject2);

					break;
/***********************************************************************************************************************/
				case MSG_HTTPSSEVER_T_GET_UID:
					DEBUGPRINT(DEBUG_INFO,"!!!!!!!!!!!!MSG_HTTPSSEVER_T_GET_UID!!!!!!!!!!!!!!!!!! \n");
					//GetIPByHost(IP, www);
					android_usb_send_msg(doHttpsMsg->MSGID_REV_DO_HTTPS, MFI_MSG_TYPE, 9);
					DEBUGPRINT(DEBUG_INFO,"<><>--------------------------------get UID www is %s IP %s-------------------<><>\n",ServerAddr,IP);
					//NO Return......
					//如果有UID就不要了
//					if(strlen(UID)>0){
//						SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_GET_UID,"HAVEUID",strlen("HAVEUID"));
//						DEBUGPRINT(DEBUG_INFO,"<><>--------------------------------I have UID -------------------<><>\n");
//						printf("<><>--------------------------------I have UID -------------------<><>\n");
//						flag_setnetwork = 0;
//						my_uid = 0;
//						break;
//					}else{
//						printf("__LINE__:%d\n",__LINE__);
//
//					}

					my_uid = 1;
					memset(UID,'\0',sizeof(UID));
					get_config_item_value(CONFIG_P2P_UID,UID, &n);
					printf("__LINE__:%d\n",__LINE__);
					if(strlen(UID)>0){
						printf("__LINE__:%d\n",__LINE__);
						log_s("Guozhixin Have UID",strlen("Guozhixin Have UID"));
						my_uid = 0;
						SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_GET_UID,UID,strlen(UID));
						break;
					}else{
						printf("__LINE__:%d\n",__LINE__);
						printf("Now flash don't have UID ,I must get it now ......\n");
					}

					memset(RcheckBuffer->Rcheck,'\0',sizeof(RcheckBuffer->Rcheck));
					get_config_item_value(CONFIG_CAMERA_ENCRYTIONID,RcheckBuffer->Rcheck, &n);
					if(strlen(RcheckBuffer->Rcheck)==0){
						SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_GET_UID,"NORL",strlen("NORL"));
						break;
					}
					memset(UID,'\0',sizeof(UID));
					memset(R_16,'\0',sizeof(R_16));
					memcpy(R_16,RcheckBuffer->Rcheck,16);
					printf("__LINE__:%d\n",__LINE__);
					if(-1 == PostBuildMSG(&Https2CloudMSG,IP,R_16,ServerPort,ADDR_GETP2PID,SV_ADDR_GETP2PID)){

					}else{
						printf("****************************************************************\n");
						if(-1 == Send2CloudMSG(&CloudReturnValue,Https2CloudMSG,IP,ServerPort,strlen(Https2CloudMSG))){
						//	SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_GET_UID,"TIMEOUT",strlen("TIMEOUT"));
						}else{
							elsef_lag = 1;
						}
					}
					json_t *revobject5,*revnextobject1;
					json_error_t error5;
					char *str6 = NULL,*str7 = NULL,*str8 = NULL;
					revobject5=json_object();
					revnextobject1=json_object();
					printf("__LINE__:%d\n",__LINE__);
					if(elsef_lag == 1){
						printf("__LINE__:%d\n",__LINE__);
//						flag_setnetwork = 0;
						revobject5=json_loads(CloudReturnValue,JSON_COMPACT,&error5);

						if(0!=json_unpack(revobject5,"{s:s,s:o}","ResultMessage",&str6,"ReturnValue",&revnextobject1)){
							printf("__LINE__:%d\n",__LINE__);
							fprintf(stderr,"__LINE__:%d,%s\n",error5.line,error5.text);
							SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_GET_UID,"FAIL",strlen("FAIL"));
						}else{
							printf("__LINE__:%d\n",__LINE__);
							if(strcmp(str6,"100")==0)
							{
								json_unpack(revnextobject1,"{s:s,s:s}","coninfo",&str8,"contype",&str7);

								if(strlen(str8)==20){

									memcpy(UID,str8,strlen(str8));
									{
										int lRet = set_config_value_to_flash(CONFIG_P2P_UID,UID, strlen(UID));
										printf("hello zhang shu pan I have UID:%s ",UID);
										DEBUGPRINT(DEBUG_INFO,"lRet = %d\n",lRet);
									}

									my_uid = 0;
									DEBUGPRINT(DEBUG_INFO,"=================================================\n"
														"=========UID:%s=========\n"
														"=================================================\n",UID);
									SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_GET_UID,UID,strlen(UID));

								}else{
									SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_GET_UID,"FAIL",strlen("FAIL"));
								}

							}else if(strcmp(str6,"211")==0){
								SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_GET_UID,"NORL",strlen("NORL"));
							}else if(strcmp(str6,"500")==0){
								SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_GET_UID,"FAIL",strlen("FAIL"));
							}else if(strcmp(str6,"220")==0){
								SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_GET_UID,"NORL",strlen("NORL"));
							}else{
								SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_GET_UID,"FAIL",strlen("FAIL"));
							}
						}
					}else{
						SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_VERIFYCAM,"SendMSG2CloudERR",strlen("SendMSG2CloudERR"));
					}

					elsef_lag = 0;
					free(Https2CloudMSG);
					Https2CloudMSG = NULL;
					free(CloudReturnValue);
					CloudReturnValue = NULL;
					json_decref(revobject5);
					json_decref(revnextobject1);
					DEBUGPRINT(DEBUG_INFO,"<><>--------------------------------get UID www is %s IP %s-------------------<><>\n",ServerAddr,IP);
					break;
/***********************************************************************************************************************/
				case MSG_HTTPSSERVER_T_AUTHENTICATION_POWER:   //权限认证
					//不加验证RcheckBuffer->Rcheck是否存在，在UID处验证过

					memset(RcheckBuffer->Rcheck,'\0',sizeof(RcheckBuffer->Rcheck));
					get_config_item_value(CONFIG_CAMERA_ENCRYTIONID,RcheckBuffer->Rcheck, &n);
					if(strlen(RcheckBuffer->Rcheck)==0){
						SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_GET_UID,"NORL",strlen("NORL"));
						break;
					}
					memset(R_16,'\0',sizeof(R_16));
					memcpy(R_16,RcheckBuffer->Rcheck,16);

					if(-1 == PostBuildMSG(&Https2CloudMSG,IP,R_16,ServerPort,ADDR_CONNCHECK,SV_ADDR_CONNCHECK)){

				    }else{
				    	if(-1 == Send2CloudMSG(&CloudReturnValue,Https2CloudMSG,IP,ServerPort,strlen(Https2CloudMSG))){

				    	}else{
				    		elsef_lag = 1;
				    	}
				    }

				    json_t *revconncheck,*revconncheck1;
				    json_error_t  conncheckerror;
				    revconncheck=json_object();
				    revconncheck1=json_object();
				    char *checkstr=NULL;
				    char *num[3];
				    int n1,req_stat;
				    char *checkkey[3];
				    char *checktype[3];
				    if(elsef_lag == 1)
				    {
				    	printf("****************pfiodsafpodis dfa****************************************************\n");
						revconncheck=json_loads(CloudReturnValue,JSON_COMPACT,&conncheckerror);
						printf("__LINE__:%d\n",__LINE__);
						if(0!=json_unpack(revconncheck,"{s:s,s:o}","ResultMessage",&checkstr,"ReturnValue",&revconncheck1)){
							SendMsgHttps(doHttpsMsg,MSG_HTTPSSERVER_P_AUTHENTICATION_POWER,"FAIL",strlen("FAIL"));
						}else{
							if(strcmp(checkstr,"100")==0)
							{
								printf("__LINE__:%d\n",__LINE__);
								memset((void *)user_pm,0,sizeof(user_pm));
								write_file(CloudReturnValue);
								if(0!=json_unpack(revconncheck1,"[{s:s,s:s,s:s},{s:s,s:s,s:s},{s:s,s:s,s:s}]",
										"HasAuth",&num[0],"SecretK",&checkkey[0],"RqterType",&checktype[0],
						    		"HasAuth",&num[1],"SecretK",&checkkey[1],"RqterType",&checktype[1],
						    		"HasAuth",&num[2],"SecretK",&checkkey[2],"RqterType",&checktype[2])){
									SendMsgHttps(doHttpsMsg,MSG_HTTPSSERVER_P_AUTHENTICATION_POWER,"FAIL",strlen("FAIL"));
									printf("__LINE__:%d\n",__LINE__);
								}else{
									printf("__LINE__:%d\n",__LINE__);
									printf("HasAuth:%s,SecretK:%s,RqterType:%s==HasAuth:%s,SecretK:%s,RqterType:%s==HasAuth:%s,SecretK:%s,RqterType:%s\n",
											num[0],checkkey[0],checktype[0],num[1],checkkey[1],checktype[1],num[2],checkkey[2],checktype[2]);

									for(n1 = 0;n1 < 3;n1++){
//										user_pm[n1].Cam_mode = num[n1];
										memcpy(&user_pm[n1].Cam_mode,num[n1],strlen(num[n1]));
										memcpy(user_pm[n1].Cam_key,checkkey[n1],strlen(checkkey[n1]));
										memcpy(user_pm[n1].Cam_type,checktype[n1],strlen(checktype[n1]));
										printf("111111111111111111%c %s %s\n",user_pm[n1].Cam_mode,user_pm[n1].Cam_key,user_pm[n1].Cam_type);
									}
									req_stat = 1;
									SendMsgHttps(doHttpsMsg,MSG_HTTPSSERVER_P_AUTHENTICATION_POWER,"OK",strlen("OK"));
								}
							}else if(strcmp(checkstr,"211")==0){
								SendMsgHttps(doHttpsMsg,MSG_HTTPSSERVER_P_AUTHENTICATION_POWER,"NORL",strlen("NORL"));
							}else if(strcmp(checkstr,"500")==0){
								SendMsgHttps(doHttpsMsg,MSG_HTTPSSERVER_P_AUTHENTICATION_POWER,"FAIL",strlen("FAIL"));
							}else if(strcmp(checkstr,"223")==0){
								SendMsgHttps(doHttpsMsg,MSG_HTTPSSERVER_P_AUTHENTICATION_POWER,"223",strlen("223"));
							}else if(strcmp(checkstr,"220")==0){
								SendMsgHttps(doHttpsMsg,MSG_HTTPSSERVER_P_AUTHENTICATION_POWER,"FAIL",strlen("FAIL"));
							}else{
								SendMsgHttps(doHttpsMsg,MSG_HTTPSSERVER_P_AUTHENTICATION_POWER,"FAIL",strlen("FAIL"));
							}
						}

					}else{
						SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_VERIFYCAM,"SendMSG2CloudERR",strlen("SendMSG2CloudERR"));
					}

				    elsef_lag = 0;
				    free(Https2CloudMSG);
				    Https2CloudMSG = NULL;
				    free(CloudReturnValue);
				    CloudReturnValue = NULL;
				    json_decref(revconncheck);
				    json_decref(revconncheck1);

//				    if(strnlen(R_16, 16) < 16) {
//						get_config_item_value(CONFIG_CAMERA_ENCRYTIONID,RcheckBuffer->Rcheck, &n);
//						memset(R_16,'\0',sizeof(R_16));
//						memcpy(R_16,RcheckBuffer->Rcheck,16);
//				    }
				    if(req_stat == 1){
					    if(0 != cmpSSIDAndIp()) {
					    	LOG_TEST("IP is changed\n");
					    	postSSIDAndIp();
					    } else {
					    	LOG_TEST("IP is not change\n");
					    }
					    LOG_TEST("return %s\n", CloudReturnValue);
					    free(Https2CloudMSG);
						Https2CloudMSG = NULL;
						free(CloudReturnValue);
						CloudReturnValue = NULL;
				    }

				    break;

/***********************************************************************************************************************/
				case MSG_HTTPSSEVER_T_PUSH_ALARM_2_CLOUND_CAM:
					printf("****************************************************************\n");
					printf("***************PUSH_ALARM_2_CLOUND_CAM   start!!!***************\n");
					printf("****************************************************************\n");
					DEBUGPRINT(DEBUG_INFO,"MSG_HTTPSSEVER_T_PUSH_ALARM_2_CLOUND_CAM /n");

					memset(PhotoData,'\0',sizeof(stPhoto));
					PhotoData->number = 1;
					PhotoData->filegroup = 0;
					memcpy(&PhotoData->filegroup,doHttpsMsg->s_rev_https->data,4);
					memcpy(&PhotoData->alarmtype,doHttpsMsg->s_rev_https->data+sizeof(int),4);
					memset(RcheckBuffer->Rcheck,'\0',sizeof(RcheckBuffer->Rcheck));
					get_config_item_value(CONFIG_CAMERA_ENCRYTIONID,RcheckBuffer->Rcheck, &n);

					if(strlen(RcheckBuffer->Rcheck)==0||strlen(UID)==0){
						resFlag = -1;
						goto DELFILE;
					}else{
						DEBUGPRINT(DEBUG_INFO,"Have UID and  Rl /n");
					}
					//DEBUGPRINT(DEBUG_INFO,"__LINE__ :%d/n",__LINE__);
					memset(R_16,'\0',sizeof(R_16));
					memcpy(R_16,RcheckBuffer->Rcheck,16);
					if(-1 == POSTBuildMSG_PushMsg(&Https2CloudMSG,IP,R_16, ServerPort,
							ADDR_PUSH_ALARM_2_CLOUND_CAM,PhotoData->filegroup,
							PhotoData->alarmtype,SV_ADDR_PUSH_ALARM_2_CLOUND_CAM)){
						resFlag = -1;
						goto DELFILE;
					}else{
						printf("****************************************************************\n");
					    printf("***********PUSH_ALARM_2_CLOUND_CAM  string finish***************\n");
				    	printf("****************************************************************\n");
						if(-1 == Send2CloudMSG(&CloudReturnValue,Https2CloudMSG,IP,ServerPort,strlen(Https2CloudMSG))){
							resFlag = -1;
							goto DELFILE;
						}else{
							elsef_lag = 1;
#ifdef Http_LOG_ON
							write_log(PhotoData->filegroup,CloudReturnValue);
#endif
						}
					}
					DEBUGPRINT(DEBUG_INFO,"this push info alram __LINE__ :%d/n",__LINE__);
					json_t *revobject3;
					json_error_t error3;
					char *str4;
					int jsR = 0;
					revobject3=json_object();
					revobject3=json_loads(CloudReturnValue,JSON_COMPACT,&error3);
					printf("****************************************************************\n");
					printf("****************************************************************\n");
					printf("revobject3:%s\n",json_dumps(revobject3,JSON_COMPACT));
					printf("****************************************************************\n");
					printf("****************************************************************\n");
					if(elsef_lag == 1){
						revobject3=json_loads(CloudReturnValue,JSON_COMPACT,&error3);
						jsR = json_unpack(revobject3,"{s:s}","ResultMessage",&str4);
						if(0 != jsR){
							printf("__LINE__:%d\n",__LINE__);
							printf("__LINE__:%d\n",__LINE__);
							SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_p_PUSH_ALARM_2_CLOUND_CAM,"FAIL",strlen("FAIL"));
							json_decref(revobject3);
							printf("__LINE__:%d\n",__LINE__);
							printf("__LINE__:%d\n",__LINE__);
						}else{
							printf("__LINE__:%d str4:%s\n",__LINE__,str4);
							if(strcmp(str4,"100")==0){
								printf("__LINE__:%d\n",__LINE__);
								printf("__LINE__:%d\n",__LINE__);
								printf("__LINE__:%d\n",__LINE__);
								SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_p_PUSH_ALARM_2_CLOUND_CAM,"OK",strlen("OK"));
							}else if(strcmp(str4,"211")==0){
								resFlag = -1;
								SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_p_PUSH_ALARM_2_CLOUND_CAM,"NORL",strlen("NORL"));
								goto DELFILE;
							}else if(strcmp(str4,"500")==0){
								resFlag = -1;
								SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_p_PUSH_ALARM_2_CLOUND_CAM,"FAIL",strlen("FAIL"));
								goto DELFILE;
							}else{
								printf("__LINE__:%d",__LINE__);
								resFlag = -1;
								SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_p_PUSH_ALARM_2_CLOUND_CAM,"FAIL",strlen("FAIL"));
								goto DELFILE;
							}
							//json_decref(revobject3);
						}
						json_decref(revobject3);
					}else{
						printf("__LINE__:%d",__LINE__);
						SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_p_PUSH_ALARM_2_CLOUND_CAM,"SendMSG2CloudERR",strlen("SendMSG2CloudERR"));
						resFlag = -1;
						goto DELFILE;
					}
					DELFILE:

					if(resFlag == -1){
						memset(CMD,'\0',sizeof(CMD));
						memset(PICfilename,'\0',sizeof(PICfilename));
						sprintf(PICfilename,"/usr/share/alarmpic/%d",PhotoData->filegroup);
						sprintf(CMD,"rm -rf %s",PICfilename);
						DEBUGPRINT(DEBUG_INFO,"<><>---********************---CMD:%s---********************---<><>\n",CMD);
						if(my_remove_file(PICfilename) != 0){
						//	fprintf(stderr,"PICfilename cant rm %s",strerror(errno));
						}else{
							printf("==============================================\n"
								"=============  Delete File  ==================\n"
								"==============================================\n");
						}
						printf("==============================================\n"
								"=============  Delete File  ==================\n"
								"==============================================\n");
						SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_p_PUSH_ALARM_2_CLOUND_CAM,"FAIL",strlen("FAIL"));

					}

					resFlag = 0;
					elsef_lag = 0;
					free(Https2CloudMSG);
					Https2CloudMSG = NULL;
					free(CloudReturnValue);
					CloudReturnValue = NULL;

					break;

/***********************************************************************************************************************/

				case MSG_HTTPSSEVER_T_UPLOAD_FILE_CAM:
					DEBUGPRINT(DEBUG_INFO,"MSG_HTTPSSEVER_T_UPLOAD_FILE_CAM /n");
					printf("****************************************************************\n");
					printf("********************UPLOAD_FILE_CAM   start!!!******************\n");
					printf("****************************************************************\n");

					memcpy(PhotoData->pFileName,doHttpsMsg->s_rev_https->data,doHttpsMsg->s_rev_https->length);
					sprintf(PhotoData->pFileName+doHttpsMsg->s_rev_https->length,"%d",PhotoData->number);
					strncat(PhotoData->pFileName,".jpg",strlen(".jpg"));
					get_config_item_value(CONFIG_CAMERA_ENCRYTIONID,RcheckBuffer->Rcheck, &n);
					if(strlen(RcheckBuffer->Rcheck)==0||strlen(UID)==0){
						resFlag = -1;
						goto FAILEXIT;
					}else{

					}
					DEBUGPRINT(DEBUG_INFO,"::>_<::---------------> do Https __LINE__:%d\n",__LINE__);
					PhotoData->DataLength = ReadFile(&(PhotoData->pPhotodata), PhotoData->pFileName);
					if(PhotoData->DataLength <= 0){
						resFlag = -1;
						goto FAILEXIT;
					}

					memset(R_16,'\0',sizeof(R_16));
					memcpy(R_16,RcheckBuffer->Rcheck,16);
					msglength = POSTUploadFileMSG(&Https2CloudMSG,IP,R_16,ServerPort,ADDR_UPLOAD_FILE_CAM,
												PhotoData->alarmtype,"1",PhotoData->filegroup,PhotoData->number,PhotoData->DataLength,
												PhotoData->pPhotodata,SV_ADDR_UPLOAD_FILE_CAM);
					printf("__LINE__:%d  msglength:%d\n",__LINE__,msglength);
					if(-1 == msglength){
						goto FAILEXIT;
						resFlag = -1;
					}else{

						printf("****************************************************************\n");
						printf("*******************UPLOAD_FILE_CAM  string finish***************\n");
						printf("****************************************************************\n");

						if(-1 == Send2CloudMSG(&CloudReturnValue,Https2CloudMSG,IP,ServerPort,msglength)){
							resFlag = -1;
							goto FAILEXIT;
						}else{
							DEBUGPRINT(DEBUG_INFO,"(----------------<><><><><><><><><>------------------)\n");
							elsef_lag = 1;
						}
					}
					json_t *revobject4;
					json_error_t error4;
					char *str5;
					revobject4=json_object();

					if(elsef_lag == 1){
						revobject4=json_loads(CloudReturnValue,JSON_COMPACT,&error4);
						if(0!=json_unpack(revobject4,"{s:s}","ResultMessage",&str5)){
							SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_VERIFYCAM,"FAIL",strlen("FAIL"));
							json_decref(revobject4);
						}else{
							if(strcmp(str5,"100")==0){
								SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_UPLOAD_FILE_CAM,"OK",strlen("OK"));
							}else if(strcmp(str5,"211")==0){
								resFlag=-1;
								SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_VERIFYCAM,"NORL",strlen("NORL"));
								json_decref(revobject4);
								goto FAILEXIT;
							}else if(strcmp(str5,"500")==0){
								resFlag=-1;
								SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_VERIFYCAM,"FAIL",strlen("FAIL"));
								json_decref(revobject4);
								goto FAILEXIT;
							}else{
								resFlag=-1;
								SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_VERIFYCAM,"FAIL",strlen("FAIL"));
								json_decref(revobject4);
								goto FAILEXIT;
							}
							json_decref(revobject4);
						}
					}else{
						resFlag = -1;
						SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_VERIFYCAM,"SendMSG2CloudERR",strlen("SendMSG2CloudERR"));
						goto FAILEXIT;
					}
					DEBUGPRINT(DEBUG_INFO,"::>_<::---------------> do Https __LINE__:%d",__LINE__);
					if(PhotoData->number == 3){
						resFlag = -1;
						goto FAILEXIT;
					}else{
						PhotoData->number++;
					}

					FAILEXIT:

					if(resFlag == -1){
						memset(CMD,'\0',sizeof(CMD));
						memset(PICfilename,'\0',sizeof(PICfilename));
						memcpy(PICfilename,PhotoData->pFileName,strlen(PhotoData->pFileName)-5);
						sprintf(CMD,"rm -rf %s",PICfilename);
						printf("<><>---********************---CMD:%s---********************---<><>\n",CMD);
						if(my_remove_file(PICfilename) != 0){
						//	fprintf(stderr,"PICfilename cant rm %s",strerror(errno));
						}else{
							printf("==============================================\n"
								"=============  Delete File  ==================\n"
								"==============================================\n");
						}
						printf("==============================================\n"
								"=============  Delete File  ==================\n"
								"==============================================\n");
						SendMsgHttps(doHttpsMsg,MSG_HTTPSSEVER_P_UPLOAD_FILE_CAM,"DELFAIL",strlen("DELFAIL"));
						PhotoData->number = 0;
					}

					resFlag = 0;
					elsef_lag = 0;
					free(Https2CloudMSG);
					Https2CloudMSG = NULL;
					free(CloudReturnValue);
					CloudReturnValue = NULL;
					free(PhotoData->pPhotodata);
					PhotoData->pPhotodata = NULL;
					PhotoData->DataLength = 0;

					break;

				case MSG_HTTPSPOWER_T_STATE:
//					printf("82828737373777777777777777777777777777777777777777777777777777777777\n");
//					printf("82828737373777777777777777777777777777777777777777777777777777777777\n");
//					printf("82828737373777777777777777777777777777777777777777777777777777777777\n");
					memset(RcheckBuffer->Rcheck,'\0',sizeof(RcheckBuffer->Rcheck));
					get_config_item_value(CONFIG_CAMERA_ENCRYTIONID,RcheckBuffer->Rcheck, &n);
					if(strlen(RcheckBuffer->Rcheck)==0){
						SendMsgHttps(doHttpsMsg,MSG_HTTPSPOWER_P_STATE,"NORL",strlen("NORL"));
						break;
					}
//					char one[2];
//					one[0]=33;
//					one[1]=14;

				    memset(R_16,'\0',sizeof(R_16));
					memcpy(R_16,RcheckBuffer->Rcheck,16);
//					memcpy(R_16,"abcdefgddddddddd",16);
					printf("R_16 = %s\n",R_16);
//					if(-1 == PostPower_State(&Https2CloudMSG,IP,R_16,ServerPort,ADDR_CAMSTATE,one[0],one[1])){


					if(-1 == PostPower_State(&Https2CloudMSG,IP,R_16,ServerPort,ADDR_CAMSTATE,doHttpsMsg->s_rev_https->E_xin[0],doHttpsMsg->s_rev_https->E_xin[1],SV_ADDR_CAMSTATE)){
						printf("__LINE__:%d",__LINE__);
				    }else{
				    	if(-1 == Send2CloudMSG(&CloudReturnValue,Https2CloudMSG,IP,ServerPort,strlen(Https2CloudMSG))){
				    		printf("__LINE__:%d",__LINE__);
				    	}else{
					    		elsef_lag = 1;
					    	 }
				    }
					write_file1(CloudReturnValue,"a+");
					json_t *revobject7;
					char *str_state = NULL;
					json_error_t error7;
					revobject1=json_object();
					printf("__LINE__:%d",__LINE__);
					if(elsef_lag == 1){
						revobject1=json_loads(CloudReturnValue,JSON_COMPACT,&error7);
						if(0!=json_unpack(revobject1,"{s:s}","ResultMessage",&str_state)){
							SendMsgHttps(doHttpsMsg,MSG_HTTPSPOWER_P_STATE,"FAIL",strlen("FAIL"));
						}else{
							printf("__LINE__:%d",__LINE__);
							if(strcmp(str_state,"100")==0){
									SendMsgHttps(doHttpsMsg,MSG_HTTPSPOWER_P_STATE,"OK",strlen("OK"));
							}else if(strcmp(str_state,"211")==0){
								SendMsgHttps(doHttpsMsg,MSG_HTTPSPOWER_P_STATE,"NORL",strlen("NORL"));
							}else if(strcmp(str_state,"500")==0){
								SendMsgHttps(doHttpsMsg,MSG_HTTPSPOWER_P_STATE,"FAIL" ,strlen("FAIL"));
							}else{
								SendMsgHttps(doHttpsMsg,MSG_HTTPSPOWER_P_STATE,"FAIL" ,strlen("FAIL"));
							}
						}
					}else{
						SendMsgHttps(doHttpsMsg,MSG_HTTPSPOWER_P_STATE,"SendMSG2CloudERR",strlen("SendMSG2CloudERR"));
					}

					elsef_lag = 0;
					free(Https2CloudMSG);
					Https2CloudMSG = NULL;
					free(CloudReturnValue);
					CloudReturnValue = NULL;
					json_decref(revobject7);

					break;
				case MSG_HTTPSSERVER_T_STOP:
					break;
				default:
					break;
			}
		}
	}

	return NULL;
}

/**
  *  @brief  dohttps线程初始化
  *  @author  <gy> 
  *  @param[in] <dohttps线程结构体>
  *  @param[out] <无>
  *  @return   <-1代表失败> <0代表成功>
  *  @note
*/
int InitArgsHttpsThread(stDoHttpsMSG *args){
	int nRet = 0;
	if(NULL == args){
		return -1;
	}

	args->MSGID_REV_DO_HTTPS = msg_queue_create((key_t)DO_HTTP_SERVER_MSG_KEY );
	if (args->MSGID_REV_DO_HTTPS == -1) {
		fprintf(stderr, "create MSGID_SEND_THREAD_: %s\n",strerror(errno));
		nRet = -1;
	}

	args->MSGID_SEND_DO_HTTPS = msg_queue_create((key_t)HTTP_SERVER_MSG_KEY );
	if (args->MSGID_SEND_DO_HTTPS == -1) {
		fprintf(stderr, "create MSGID_REV_THREAD_: %s\n",strerror(errno));
		nRet = -1;
	}

	args->s_rev_https = malloc(sizeof(Child_Process_Msg));
	if(args->s_rev_https == NULL){
		nRet = -1;
	}

	args->s_send_https = malloc(sizeof(Child_Process_Msg));
	if(args->s_send_https == NULL){
		nRet = -1;
	}
//同步1
	thread_synchronization(&R_HttpsThread, &R_HttpsHost);

	return nRet;
}
/**
  *  @brief  接收消息函数
  *  @author  <gy> 
  *  @param[in] <dohttps线程结构体>
  *  @param[out] <无>
  *  @return   <-1代表失败> <0代表成功>
  *  @note
*/
int RevMsgHttps(stDoHttpsMSG *self){
	int nRet = 0;
	nRet = RevMsgFunction(self->MSGID_REV_DO_HTTPS,self->s_rev_https,HTTP_2_DO_MSG_TYPE);
	return nRet;
}
/**
  *  @brief  发送消息函数
  *  @author  <gy> 
  *  @param[in] <dohttps线程结构体>
  *  @param[in] <CMD协议号>
  *  @param[in] <data发送的数据>
  *  @param[in] <data长度>
  *  @param[out] <无>
  *  @return   <-1代表失败> <0代表成功>
  *  @note
*/
int SendMsgHttps(stDoHttpsMSG *self,const int CMD,char *data,int length){
	int nRet = 0;
	memset(self->s_send_https->data,0,MAX_MSG_LEN);
	self->s_send_https->msg_type = DO_2_HTTP_MSG_TYPE;
	self->s_send_https->Thread_cmd = CMD;
	memcpy(self->s_send_https->data,data,length);
	self->s_send_https->length = length;
	printf("self->s_send_https->length:%d\r\n",self->s_send_https->length);
	printf("\n\rtype %ld \n\r CMD:%d  data :%s __LINE__:%d\r\n",self->s_send_https->msg_type,self->s_send_https->Thread_cmd,self->s_send_https->data,__LINE__);
	nRet = SendMsgFunction(self->MSGID_SEND_DO_HTTPS,self->s_send_https);
	return nRet;
}
/**
  *  @brief  根据主机名获得ip
  *  @author  <gy> 
  *  @param[in] <pIP得到的ip>
  *  @param[in] <pHostName主机名>
  *  @param[out] <无>
  *  @return   <小于0代表失败> <大于0代表成功>
  *  @note
*/
int GetIPByHost(char *pIP, const char *pHostName)
{

    int nRet = 0;
    char **ppList;
    struct hostent *pHostent;
    char strIP[40+1];

    if ((NULL==pIP) || (NULL==pHostName))
    {
        //DEBUGPRINT(DEBUG_ERROR, "Pointer is NULL! @:%s @:%d\n", __FILE__, __LINE__);
        return -2;
    }

    if((pHostent = gethostbyname(pHostName)) == NULL)
    {
        //DEBUGPRINT(DEBUG_ERROR, "Get host IP by name error! @:%s @:%d\n", __FILE__, __LINE__);
        return -1;
    }

    switch(pHostent->h_addrtype)
    {
       case AF_INET:
       case AF_INET6:
            ppList = pHostent->h_addr_list;
            for (; *ppList!=NULL; ppList++)
            {
                sprintf(pIP, "%s", inet_ntop(pHostent->h_addrtype, *ppList, strIP, sizeof(strIP)));
                if (strlen(pIP) > 0)
                {
                    break;
                }
            }
            break;
       default:
            //DEBUGPRINT(DEBUG_ERROR, "Unknown address type! @:%s @:%d\n", __FILE__, __LINE__);
            nRet = -3;
            break;
    }

    if (strlen(pIP)>0)
    {
        if (pIP[strlen(pIP)-1] == '\n')
        {
            pIP[strlen(pIP)-1] = '\0';
        }
    }

    return nRet;
}
/**
  *  @brief  get方法拼协议头函数
  *  @author  <gy> 
  *  @param[in] <ppSendMsg指针>
  *  @param[in] <IP地址>
  *  @param[in] <Camera_EncrytionID摄像头地址>
  *  @param[in] <Port端口>
  *  @param[in] <sv的值>
  *  @param[in] <Port端口>
  *  @param[out] <无>
  *  @return   <-1代表失败> <0代表成功>
  *  @note
*/
int GetCameraMSG(char **ppSendMsg,char *IP,char *Camera_EncrytionID,const int Port,const char* Addr,char *SV)
{
	char Camera_ID[32];
	char ItIsMAC[13];
	char Camera_Version[20];
	char Camera_EncrytionID_URL[512];
	memset(Camera_EncrytionID_URL,0,sizeof(Camera_EncrytionID_URL));
	if(Addr == NULL ||ppSendMsg == NULL||IP == NULL ||Camera_EncrytionID == NULL||SV == NULL/*||alarmgroup == NULL||alarmtype == NULL*/){
		return -1;
	}
	char *pSendMsg;
	pSendMsg = malloc(1024);
	int n = 0;
	get_mac_addr_value(Camera_ID, &n);
	memset(ItIsMAC,'\0',sizeof(ItIsMAC));
	memcpy(ItIsMAC,Camera_ID+12,12);
	memset(Camera_Version,'\0',sizeof(Camera_Version));
#if 1
	get_config_item_value(CONFIG_CONF_VERSION,Camera_Version, &n);
#endif
	ChangeBase64ToURL(Camera_EncrytionID_URL, Camera_EncrytionID);
	printf("Camera_EncrytionID:%s  Camera_EncrytionID_URL:%s\n",Camera_EncrytionID,Camera_EncrytionID_URL);
	sprintf(pSendMsg, "GET /camapi/updatestate.ashx?camid=%s&camenr=%s&updatemsg=%s HTTP/1.1\r\nHost:%s:%d\r\n",
			ItIsMAC,Camera_EncrytionID_URL,Camera_Version,IP,Port);
    strcat(pSendMsg, "Content-Type:application/x-www-form-urlencoded\r\n");
    strcat(pSendMsg, "Accept:*/*\r\n");
    strcat(pSendMsg, "Connection:Keep-Alive\r\n");
    strcat(pSendMsg, "Cache-Control:no-cache\r\n");
    strcat(pSendMsg, "\r\n");
    log_s(pSendMsg,strlen(pSendMsg));
    *ppSendMsg = pSendMsg;
	printf("==============================================================================================\n");
	printf("==============================================================================================\n");
	printf("==============================================================================================\n");
    return 0;
}


#if 0
int GetBuildMSG(char **ppSendMsg,char *IP,char *Camera_EncrytionID,const int Port,const char* Addr,int alarmgroup,int alarmtype,char *SV)
{
	char Camera_ID[32];
	char ItIsMAC[13];
	memset(ItIsMAC,'\0',sizeof(ItIsMAC));
	char Camera_Type[8];
	char Camera_EncrytionID_URL[512];
	memset(Camera_EncrytionID_URL,0,sizeof(Camera_EncrytionID_URL));
	if(Addr == NULL ||ppSendMsg == NULL||IP == NULL ||Camera_EncrytionID == NULL||SV == NULL/*||alarmgroup == NULL||alarmtype == NULL*/){
		return -1;
	}
	char *pSendMsg;
	pSendMsg = (char*)malloc(1024);
	int n = 0;

	get_mac_addr_value(Camera_ID, &n);
	memcpy(ItIsMAC,Camera_ID+12,12);

#if 1
	get_config_item_value(CONFIG_CAMERA_TYPE,Camera_Type, &n);

#endif

	printf("ItIsMAC:%s __FILE__ %s __LINE__:%d\n",ItIsMAC,__FILE__,__LINE__);
	ChangeBase64ToURL(Camera_EncrytionID_URL, Camera_EncrytionID);
	printf("Camera_EncrytionID:%s  Camera_EncrytionID_URL:%s\n",Camera_EncrytionID,Camera_EncrytionID_URL);
	sprintf(pSendMsg, "GET %s?camid=%s&camtype=%s&alarmgroup=%d&alarmtype=%d&camenr=%s&sn=%s&sv=%s HTTP/1.1\r\nHost:%s:%d\r\n",
			Addr,/*"A8B94E007D02"*/ItIsMAC,/*"M3S"*/Camera_Type,alarmgroup,alarmtype,Camera_EncrytionID_URL,SC,SV
			,IP,Port);
    strcat(pSendMsg, "Content-Type:application/x-www-form-urlencoded\r\n");
    strcat(pSendMsg, "Accept:*/*\r\n");
    strcat(pSendMsg, "Connection:Keep-Alive\r\n");
    strcat(pSendMsg, "Cache-Control:no-cache\r\n");
    strcat(pSendMsg, "\r\n");

    *ppSendMsg = pSendMsg;
    return 0;
}

int GetCameraMSG_info(char **ppSendMsg,char *IP,char *Camera_EncrytionID,const int Port,const char* Addr,char *SV)
{
	char Camera_ID[32];
	char ItIsMAC[13];
	char Camera_Type[8];
	char Camera_EncrytionID_URL[512];
	memset(Camera_EncrytionID_URL,0,sizeof(Camera_EncrytionID_URL));
	if(Addr == NULL ||ppSendMsg == NULL||IP == NULL ||Camera_EncrytionID == NULL||SV == NULL/*||alarmgroup == NULL||alarmtype == NULL*/){
		return -1;
	}
	char *pSendMsg;
	pSendMsg = malloc(1024);
	int n = 0;
	get_mac_addr_value(Camera_ID, &n);
	memset(ItIsMAC,'\0',sizeof(ItIsMAC));
	memcpy(ItIsMAC,Camera_ID+12,12);
#if 1
	get_config_item_value(CONFIG_CAMERA_TYPE,Camera_Type, &n);

#endif
	ChangeBase64ToURL(Camera_EncrytionID_URL, Camera_EncrytionID);
	printf("Camera_EncrytionID:%s  Camera_EncrytionID_URL:%s\n",Camera_EncrytionID,Camera_EncrytionID_URL);
	sprintf(pSendMsg, "GET /camapi/uploadingcaminfo.htm?camid=%s&camenr=%s&hardversion=%s&producttype=%s&productno=%s&hardwareversion=%s&sn=%s&sv=%s HTTP/1.1\r\nHost:%s:%d\r\n",
			/*Addr,*//*"A8B94E007D02"*/ItIsMAC,Camera_EncrytionID_URL,"1.0.0.1",Camera_Type,Camera_Type,"V2.0",SC,SV
			,IP,Port);
    strcat(pSendMsg, "Content-Type:application/x-www-form-urlencoded\r\n");
    strcat(pSendMsg, "Accept:*/*\r\n");
    strcat(pSendMsg, "Connection:Keep-Alive\r\n");
    strcat(pSendMsg, "Cache-Control:no-cache\r\n");
    strcat(pSendMsg, "\r\n");
    *ppSendMsg = pSendMsg;

    return 0;
}

#endif
/**
  *  @brief  post方法拼推送协议的函数
  *  @author  <gy> 
  *  @param[in] <ppSendMsg指针>
  *  @param[in] <IP地址>
  *  @param[in] <Camera_EncrytionID摄像头地址>
  *  @param[in] <Port端口>
  *  @param[in] <sv的值>
  *  @param[in] <Port端口>
  *  @param[in] <filegroup报警文件组>
  *  @param[in] <alarmtype报警类型>
  *  @param[out] <无>
  *  @return   <-1代表失败> <0代表成功>
  *  @note
*/
int POSTBuildMSG_PushMsg(char **ppSendMsg,char *IP,char *Camera_EncrytionID,const int Port,const char* Addr,int  filegroup,int alarmtype,char *SV)
{

	if(ppSendMsg == NULL||Addr == NULL||SV==NULL||IP==NULL||Camera_EncrytionID==NULL){
		return -1;
	}

	char ItIsMAC[13];
	memset(ItIsMAC,'\0',sizeof(ItIsMAC));

	char strPostDataLength[32];
	char *version;
	version = Cam_firmware_HTTP;
	char *bversion;
	bversion = Cam_hardware_HTTP;
	char Camera_ID[32];
	memset(Camera_ID,0,sizeof(Camera_ID));
	char Camera_Type[8];
	memset(Camera_Type,0,sizeof(Camera_Type));
	char Camera_EncrytionID_URL[512];
	memset(Camera_EncrytionID_URL,0,sizeof(Camera_EncrytionID_URL));
	int n = 0;
	get_mac_addr_value(Camera_ID, &n);
	memcpy(ItIsMAC,Camera_ID+12,12);
	get_config_item_value(CONFIG_CAMERA_TYPE,Camera_Type, &n);
	ChangeBase64ToURL(Camera_EncrytionID_URL, Camera_EncrytionID);
	printf("Camera_EncrytionID:%s  Camera_EncrytionID_URL:%s\n",Camera_EncrytionID,Camera_EncrytionID_URL);
	char FileGroup[20];
	char AlarmType[20];
	sprintf(FileGroup,"%d",filegroup);
	sprintf(AlarmType,"%d",alarmtype);

	printf("ItIsMAC:%s __FILE__ %s __LINE__:%d\n",ItIsMAC,__FILE__,__LINE__);

	json_t *object,*object1;
	object = json_pack("{ssssssssssssss}","camid",ItIsMAC,"camenr",Camera_EncrytionID_URL,"cmpt",Camera_Type,"chwv",version,"hwv",bversion,"sc",SC,"sv",SV);
	if(!object)
	{
	     printf("creat object failed!\n");
	     return -1;
    }
	object1 = json_pack("{ssss}","alarmtype",AlarmType,"filegroup",FileGroup);

	if(!object1)
    {
	       json_decref(object);
	       return -1;
	}

	char *pSendMsg_tmp = NULL;
	pSendMsg_tmp = malloc(1*1024);
	char *strTempList = NULL;
	strTempList = malloc(1*1024);

    memset(strTempList,0,1024);
	sprintf(strTempList,"headinfo=%s&content=%s",json_dumps(object,JSON_COMPACT),json_dumps(object1,JSON_COMPACT));
	memset(strPostDataLength, '\0', sizeof(strPostDataLength));
	sprintf(strPostDataLength, "Content-Length:%d\r\n", strlen(strTempList));

	sprintf(pSendMsg_tmp, "POST %s"
				" HTTP/1.1\r\nHost:%s:%d\r\n", Addr,
				IP, Port);
	strcat(pSendMsg_tmp, "Content-Type:application/x-www-form-urlencoded\r\n");//"Accept:text/xml,application/xhtml+xml,*/*\r\n");
	strcat(pSendMsg_tmp, strPostDataLength);
	strcat(pSendMsg_tmp, "Connection:Keep-Alive\r\n");
	strcat(pSendMsg_tmp, "Cache-Control:no-cache\r\n");
	strcat(pSendMsg_tmp, "\r\n");
	strcat(pSendMsg_tmp, strTempList);
	log_s(pSendMsg_tmp,strlen(pSendMsg_tmp));
	DEBUGPRINT(DEBUG_INFO,"************************POSTBuildMSG_PushMsg=%s****************************\n",strTempList);
	*ppSendMsg = pSendMsg_tmp;
	free(strTempList);
	strTempList = NULL;
	json_decref(object);
	json_decref(object1);

    return 0;
}

/**
  *  @brief  post方法拼推送协议的函数
  *  @author  <gy> 
  *  @param[in] <ppSendMsg指针>
  *  @param[in] <IP地址>
  *  @param[in] <Camera_EncrytionID摄像头地址>
  *  @param[in] <Port端口>
  *  @param[in] <sv的值>
  *  @param[in] <Port端口>
  *  @param[in] <filegroup报警文件组>
  *  @param[in] <alarmtype报警类型>
  *  @param[out] <无>
  *  @return   <-1代表失败> <0代表成功>
  *  @note
*/
int POSTBuildMSG_SSIDUpload(char **ppSendMsg,char *IP,char *Camera_EncrytionID,const int Port,const char* Addr,const char *SSID, const char *ip, char *SV)
{

	if(ppSendMsg == NULL||Addr == NULL||SV==NULL||IP==NULL||Camera_EncrytionID==NULL){
		return -1;
	}

	char ItIsMAC[13];
	memset(ItIsMAC,'\0',sizeof(ItIsMAC));

	char strPostDataLength[32];
	char *version;
	version = Cam_firmware_HTTP;
	char *bversion;
	bversion = Cam_hardware_HTTP;
	char Camera_ID[32];
	memset(Camera_ID,0,sizeof(Camera_ID));
	char Camera_Type[8];
	memset(Camera_Type,0,sizeof(Camera_Type));
	char Camera_EncrytionID_URL[512];
	memset(Camera_EncrytionID_URL,0,sizeof(Camera_EncrytionID_URL));
	int n = 0;
	get_mac_addr_value(Camera_ID, &n);
	memcpy(ItIsMAC,Camera_ID+12,12);
	get_config_item_value(CONFIG_CAMERA_TYPE,Camera_Type, &n);
	ChangeBase64ToURL(Camera_EncrytionID_URL, Camera_EncrytionID);
	printf("Camera_EncrytionID:%s  Camera_EncrytionID_URL:%s\n",Camera_EncrytionID,Camera_EncrytionID_URL);
//	char FileGroup[20];
//	char AlarmType[20];
//	sprintf(FileGroup,"%d",filegroup);
//	sprintf(AlarmType,"%d",alarmtype);

	printf("ItIsMAC:%s __FILE__ %s __LINE__:%d\n",ItIsMAC,__FILE__,__LINE__);

	json_t *object,*object1;
	object = json_pack("{ssssssssssssss}","camid",ItIsMAC,"camenr",Camera_EncrytionID_URL,"cmpt",Camera_Type,"chwv",version,"hwv",bversion,"sc",SC,"sv",SV);
	if(!object)
	{
	     printf("creat object failed!\n");
	     return -1;
    }
	object1 = json_pack("{sssssi}", "SSID", SSID, "IP", ip, "TS", time(NULL));

	if(!object1)
    {
	       json_decref(object);
	       return -1;
	}

	char *pSendMsg_tmp = NULL;
	pSendMsg_tmp = malloc(1*1024);
	char *strTempList = NULL;
	strTempList = malloc(1*1024);

    memset(strTempList,0,1024);
	sprintf(strTempList,"headinfo=%s&content=%s",json_dumps(object,JSON_COMPACT),json_dumps(object1,JSON_COMPACT));
	memset(strPostDataLength, '\0', sizeof(strPostDataLength));
	sprintf(strPostDataLength, "Content-Length:%d\r\n", strlen(strTempList));

	sprintf(pSendMsg_tmp, "POST %s"
				" HTTP/1.1\r\nHost:%s:%d\r\n", Addr,
				IP, Port);
	strcat(pSendMsg_tmp, "Content-Type:application/x-www-form-urlencoded\r\n");//"Accept:text/xml,application/xhtml+xml,*/*\r\n");
	strcat(pSendMsg_tmp, strPostDataLength);
	strcat(pSendMsg_tmp, "Connection:Keep-Alive\r\n");
	strcat(pSendMsg_tmp, "Cache-Control:no-cache\r\n");
	strcat(pSendMsg_tmp, "\r\n");
	strcat(pSendMsg_tmp, strTempList);
	DEBUGPRINT(DEBUG_INFO,"************************POSTBuildMSG_PushMsg=%s****************************\n",strTempList);
	LOG_TEST("%s\n", pSendMsg_tmp);
	log_s(pSendMsg_tmp, strlen(pSendMsg_tmp));
	*ppSendMsg = pSendMsg_tmp;
	free(strTempList);
	strTempList = NULL;
	json_decref(object);
	json_decref(object1);

    return 0;
}
/**
  *  @brief  post方法拼推送协议的函数
  *  @author  <gy> 
  *  @param[in] <ppSendMsg指针>
  *  @param[in] <IP地址>
  *  @param[in] <Camera_EncrytionID摄像头地址>
  *  @param[in] <Port端口>
  *  @param[in] <sv的值>
  *  @param[in] <Port端口>
  *  @param[in] <filegroup报警文件组>
  *  @param[in] <alarmtype报警类型>
  *  @param[in] <number图片序号>
  *  @param[in] <fileleng文件的长度>
  *  @param[in] <file要上传的文件>
  *  @param[out] <拼好的协议>
  *  @return   <-1代表失败> <0代表成功>
  *  @note
*/
int POSTUploadFileMSG(char **pSendMsg,char *IP,char *Camera_EncrytionID,const int Port,const char* addr,int alarmtype,const char *filetype,
		long int filegroup,const int number,long int fileleng,char *file,char *SV){

	if(pSendMsg == NULL||addr == NULL||filetype == NULL||file == NULL||SV==NULL||IP==NULL||Camera_EncrytionID==NULL){
		return -1;
	}

	char ItIsMAC[13];
	memset(ItIsMAC,'\0',sizeof(ItIsMAC));

    char strPostDataLength[32];

	char Camera_ID[32];
	memset(Camera_ID,0,sizeof(Camera_ID));
	char Camera_Type[8];
	memset(Camera_Type,0,sizeof(Camera_Type));
	char Camera_EncrytionID_URL[512];
	memset(Camera_EncrytionID_URL,0,sizeof(Camera_EncrytionID_URL));
	int n = 0;
	get_mac_addr_value(Camera_ID, &n);
	memcpy(ItIsMAC,Camera_ID+12,12);
	get_config_item_value(CONFIG_CAMERA_TYPE,Camera_Type, &n);
	ChangeBase64ToURL(Camera_EncrytionID_URL, Camera_EncrytionID);
	printf("Camera_EncrytionID:%s  Camera_EncrytionID_URL:%s\n",Camera_EncrytionID,Camera_EncrytionID_URL);
	char *version;
	version = Cam_firmware_HTTP;
	char *bversion;
	bversion = Cam_hardware_HTTP;

	char AlarmType[20];
	char FileGroup[20];
	char Number[20];
	sprintf(AlarmType,"%d",alarmtype);
	sprintf(FileGroup,"%ld",filegroup);
	sprintf(Number,"%d",number);

    memset(strPostDataLength, '\0', sizeof(strPostDataLength));

    json_t *object,*object1;
//    object=json_object();

    object = json_pack("{ssssssssssssss}","camid",ItIsMAC,"camenr",Camera_EncrytionID_URL,
    		"cmpt",Camera_Type,"chwv",version,"hwv",bversion,"sc",SC,"sv",SV);
    if(!object)
    {
      	printf("creat object failed!\n");
      	return -1;
    }
    object1 = json_pack("{ssssssssss}","alarmtype",AlarmType,"filetype",filetype,
    		"filegroup",FileGroup,"number",Number);
    if(!object1)
     {
        	printf("creat object failed!\n");
        	json_decref(object);
        	return -1;
     }

	char *pSendMsg_tmp = NULL;
	pSendMsg_tmp = malloc(1*1024+fileleng);
	char *strTempList = NULL;
	strTempList = malloc(1*1024+fileleng);
	memset(strTempList,0,1024);

    sprintf(strTempList,"headinfo=%s&content=%s&file=%s",json_dumps(object,JSON_COMPACT),json_dumps(object1,JSON_COMPACT),file);
    sprintf(strPostDataLength, "Content-Length:%d\r\n",strlen(strTempList));
    sprintf(pSendMsg_tmp, "POST %s"
    			" HTTP/1.1\r\nHost:%s:%d\r\n", addr,
    			IP, Port);
   strcat(pSendMsg_tmp, "Content-Type:application/x-www-form-urlencoded\r\n");
   strcat(pSendMsg_tmp, strPostDataLength);
   strcat(pSendMsg_tmp, "Connection:Keep-Alive\r\n");
   strcat(pSendMsg_tmp, "Cache-Control:no-cache\r\n");
   strcat(pSendMsg_tmp, "\r\n");
   strcat(pSendMsg_tmp, strTempList);
   log_s(pSendMsg_tmp,strlen(pSendMsg_tmp));
   *pSendMsg =  pSendMsg_tmp;
   printf("__LINE__:%d\n",__LINE__);
   DEBUGPRINT(DEBUG_INFO,"************************POSTBuildMSG_PushMsg=%s****************************\n",strTempList);
   printf("__LINE__:%d\n",__LINE__);
   free(strTempList);
   strTempList = NULL;
   json_decref(object);
   json_decref(object1);
   printf("__LINE__:%d\n",__LINE__);
	return strlen(pSendMsg_tmp);

}
/**
  *  @brief  post方法拼权限认证协议函数
  *  @author  <gy> 
  *  @param[in] <ppSendMsg指针>
  *  @param[in] <IP地址>
  *  @param[in] <Camera_EncrytionID摄像头地址>
  *  @param[in] <Port端口>
  *  @param[in] <sv的值>
  *  @param[in] <Port端口>
  *  @param[out] <拼好的协议>
  *  @return   <-1代表失败> <0代表成功>
  *  @note
*/
int PostBuildMsg_AuthenticationPower(char **ppSendMsg,char *IP,char *Camera_EncrytionID,const int Port,const char* Addr,char *SV)
{
	if(ppSendMsg == NULL||Addr == NULL||SV == NULL){
			return -1;
		}
		char *pTemp = NULL;
		int nRet = 0;
		char ItIsMAC[13];
		memset(ItIsMAC,'\0',sizeof(ItIsMAC));

		char Camera_ID[32];
		memset(Camera_ID,0,sizeof(Camera_ID));
		char Camera_Type[8];
		memset(Camera_Type,0,sizeof(Camera_Type));
		char Camera_EncrytionID_URL[512];
		memset(Camera_EncrytionID_URL,0,sizeof(Camera_EncrytionID_URL));
		int n = 0;
		get_mac_addr_value(Camera_ID, &n);
		memcpy(ItIsMAC,Camera_ID+12,12);
		get_config_item_value(CONFIG_CAMERA_TYPE,Camera_Type, &n);
		ChangeBase64ToURL(Camera_EncrytionID_URL, Camera_EncrytionID);

		printf("Camera_EncrytionID:%s  Camera_EncrytionID_URL:%s\n",Camera_EncrytionID,Camera_EncrytionID_URL);
		char *version;
		version = Cam_firmware_HTTP;
		char *bversion;
		bversion = Cam_hardware_HTTP;
		if(n == 0){
	//		memcpy(Camera_EncrytionID,"NULL",4);
		}

	    json_t *object;
		object = json_pack("{ssssssssssssss}","camid",ItIsMAC,"camenr",Camera_EncrytionID_URL,"cmpt",Camera_Type,"chwv",version,"hwv",bversion,"sc",SC,"sv",SV);
		if(!object)
		{
			 printf("creat object failed!\n");
			 return -1;
		}
		
		char *pSendMsg_tmp = NULL;
		pSendMsg_tmp = malloc(1*1024);
	    char *strTempList = NULL;
		strTempList = malloc(1*1024);
	    char strPostDataLength[32];

		sprintf(strTempList,"headinfo=%s",json_dumps(object,JSON_COMPACT));
		memset(strPostDataLength, '\0', sizeof(strPostDataLength));
		sprintf(strPostDataLength, "Content-Length:%d\r\n", strlen(strTempList));

		sprintf(pSendMsg_tmp, "POST %s"
					" HTTP/1.1\r\nHost:%s:%d\r\n", Addr,
					IP, Port);
	   strcat(pSendMsg_tmp, "Accept:text/xml,application/xhtml+xml,*/*\r\n");
	   strcat(pSendMsg_tmp, strPostDataLength);
	   strcat(pSendMsg_tmp, "Connection:Keep-Alive\r\n");
	   strcat(pSendMsg_tmp, "Cache-Control:no-cache\r\n");
	   strcat(pSendMsg_tmp, "\r\n");
	   strcat(pSendMsg_tmp, strTempList);
	   pTemp = pSendMsg_tmp;
	   DEBUGPRINT(DEBUG_INFO, "************************POSTBuildMSG_PushMsg=%s****************************\n",strTempList);
	   *ppSendMsg =  pSendMsg_tmp;
	   free(strTempList);
	   strTempList = NULL;
	   json_decref(object);

		return 0;

}

/**
  *  @brief  post方法拼协议头函数
  *  @author  <gy> 
  *  @param[in] <ppSendMsg指针>
  *  @param[in] <IP地址>
  *  @param[in] <Camera_EncrytionID摄像头地址>
  *  @param[in] <Port端口>
  *  @param[in] <sv的值>
  *  @param[in] <Port端口>
  *  @param[out] <拼好的协议>
  *  @return   <-1代表失败> <0代表成功>
  *  @note
*/
int PostBuildMSG(char **pSendMsg,char *IP,char *Camera_EncrytionID,const int Port,const char* addr,char *SV){

	if(pSendMsg == NULL||addr == NULL||SV == NULL){
		return -1;
	}

	int nRet = 0;
	char ItIsMAC[13];
	memset(ItIsMAC,'\0',sizeof(ItIsMAC));
	char Camera_ID[32];
	memset(Camera_ID,0,sizeof(Camera_ID));
	char Camera_Type[8];
	memset(Camera_Type,0,sizeof(Camera_Type));
	char Camera_EncrytionID_URL[512];
	memset(Camera_EncrytionID_URL,0,sizeof(Camera_EncrytionID_URL));
	int n = 0;
	get_mac_addr_value(Camera_ID, &n);
	memcpy(ItIsMAC,Camera_ID+12,12);
	get_config_item_value(CONFIG_CAMERA_TYPE,Camera_Type, &n);
	ChangeBase64ToURL(Camera_EncrytionID_URL, Camera_EncrytionID);

	printf("Camera_EncrytionID:%s  Camera_EncrytionID_URL:%s\n",Camera_EncrytionID,Camera_EncrytionID_URL);
	char *version;
	version = Cam_firmware_HTTP;
	char *bversion;
	bversion = Cam_hardware_HTTP;
	if(n == 0){
//		memcpy(Camera_EncrytionID,"NULL",4);
	}

	json_t *object;
	object = json_pack("{ssssssssssssss}","camid",ItIsMAC,"camenr",Camera_EncrytionID_URL,"cmpt",Camera_Type,"chwv",version,"hwv",bversion,"sc",SC,"sv",SV);
	if(!object){
		return -1;
	}
	char *pSendMsg_tmp = NULL;
	pSendMsg_tmp = malloc(1*1024);
    char *strTempList = NULL;
	strTempList = malloc(1*1024);
    char strPostDataLength[32];

	sprintf(strTempList,"headinfo=%s",json_dumps(object,JSON_COMPACT));
	memset(strPostDataLength, '\0', sizeof(strPostDataLength));
	sprintf(strPostDataLength, "Content-Length:%d\r\n", strlen(strTempList));

	sprintf(pSendMsg_tmp, "POST %s"
			" HTTP/1.1\r\nHost:%s:%d\r\n", addr,
			IP, Port);
	strcat(pSendMsg_tmp, "Content-Type:application/x-www-form-urlencoded\r\n");
	strcat(pSendMsg_tmp, strPostDataLength);
	strcat(pSendMsg_tmp, "Connection:Keep-Alive\r\n");
	strcat(pSendMsg_tmp, "Cache-Control:no-cache\r\n");
	strcat(pSendMsg_tmp, "\r\n");
	strcat(pSendMsg_tmp, strTempList);
	log_s(addr,strlen(addr));
	log_s(pSendMsg_tmp,strlen(pSendMsg_tmp));
	*pSendMsg =  pSendMsg_tmp;
	nRet = strlen(strTempList);
	DEBUGPRINT(DEBUG_INFO,"----------------- Message =%s\n-------------",strTempList);
	free(strTempList);
	strTempList = NULL;
    json_decref(object);

    return  nRet;

}

/**
  *  @brief  post方法拼电量状态协议函数
  *  @author  <gy> 
  *  @param[in] <ppSendMsg指针>
  *  @param[in] <IP地址>
  *  @param[in] <Camera_EncrytionID摄像头地址>
  *  @param[in] <Port端口>
  *  @param[in] <sv的值>
  *  @param[in] <Port端口>
  *  @param[in] <EnergyS电量状态>
  *  @param[in] <ChargeS充电状态>
  *  @param[out] <拼好的协议>
  *  @return   <-1代表失败> <0代表成功>
  *  @note
*/
int PostPower_State(char **ppSendMsg,char *IP,char *Camera_EncrytionID,const int Port,const char* Addr,char *EnergyS,char *ChargeS,char *sv)
{

	if(ppSendMsg == NULL||Addr == NULL||IP==NULL||Camera_EncrytionID==NULL){
		return -1;
	}

	char ItIsMAC[13];
	memset(ItIsMAC,'\0',sizeof(ItIsMAC));
	char *pSendMsg_tmp = NULL;
	pSendMsg_tmp = malloc(1*1024);
	char *strTempList = NULL;
	strTempList = malloc(1*1024);

	char strPostDataLength[32];
	char *version;
	version = Cam_firmware_HTTP;
	char *bversion;
	bversion = Cam_hardware_HTTP;
	char Camera_ID[32];
	memset(Camera_ID,0,sizeof(Camera_ID));
	char Camera_Type[8];
	memset(Camera_Type,0,sizeof(Camera_Type));
	char Camera_EncrytionID_URL[512];
	memset(Camera_EncrytionID_URL,0,sizeof(Camera_EncrytionID_URL));
	int n = 0;
	get_mac_addr_value(Camera_ID, &n);
	memcpy(ItIsMAC,Camera_ID+12,12);
	get_config_item_value(CONFIG_CAMERA_TYPE,Camera_Type, &n);
	ChangeBase64ToURL(Camera_EncrytionID_URL, Camera_EncrytionID);
	printf("Camera_EncrytionID:%s  Camera_EncrytionID_URL:%s\n",Camera_EncrytionID,Camera_EncrytionID_URL);
//	char FileGroup[20];
//	char AlarmType[20];
//	sprintf(FileGroup,"%d",filegroup);
//	sprintf(AlarmType,"%d",alarmtype);
	int EnergyStatus=(int)EnergyS;
	int ChargeStatus=(int)ChargeS;
	printf("ItIsMAC:%s __FILE__ %s __LINE__:%d\n",ItIsMAC,__FILE__,__LINE__);

	json_t *object,*object1;
	object = json_pack("{ssssssssssssss}","camid",ItIsMAC,"camenr",Camera_EncrytionID_URL,"cmpt",Camera_Type,"chwv",version,"hwv",bversion,"sc",SC,"sv",sv);
	if(!object)
	{
		 free(strTempList);
		 strTempList = NULL;
		 free(pSendMsg_tmp);
		 pSendMsg_tmp = NULL;
	     printf("creat object failed!\n");
	     return -1;
    }
	object1 = json_pack("{sisi}","energyStatus",EnergyStatus,"chargeStatus",ChargeStatus);
	if(!object1)
    {
		free(strTempList);
		strTempList = NULL;
		free(pSendMsg_tmp);
		pSendMsg_tmp = NULL;
	    json_decref(object);
	    return -1;
	}

	memset(strTempList,0,1024);
	sprintf(strTempList,"%s",json_dumps(object1,JSON_COMPACT));

	write_file1(strTempList,"w");

    memset(strTempList,0,1024);
	sprintf(strTempList,"headinfo=%s&content=%s",json_dumps(object,JSON_COMPACT),json_dumps(object1,JSON_COMPACT));
	memset(strPostDataLength, '\0', sizeof(strPostDataLength));
	sprintf(strPostDataLength, "Content-Length:%d\r\n", strlen(strTempList));

	sprintf(pSendMsg_tmp, "POST %s"
				" HTTP/1.1\r\nHost:%s:%d\r\n", Addr,
				IP, Port);
	strcat(pSendMsg_tmp, "Content-Type:application/x-www-form-urlencoded\r\n");//"Accept:text/xml,application/xhtml+xml,*/*\r\n");
	strcat(pSendMsg_tmp, strPostDataLength);
	strcat(pSendMsg_tmp, "Connection:Keep-Alive\r\n");
	strcat(pSendMsg_tmp, "Cache-Control:no-cache\r\n");
	strcat(pSendMsg_tmp, "\r\n");
	strcat(pSendMsg_tmp, strTempList);
	log_s(pSendMsg_tmp,strlen(pSendMsg_tmp));
	DEBUGPRINT(DEBUG_INFO,"************************POSTBuildMSG_PushMsg=%s****************************\n",strTempList);
	*ppSendMsg = pSendMsg_tmp;
	free(strTempList);
	strTempList = NULL;
	json_decref(object);
	json_decref(object1);

    return 0;
}


/**
  *  @brief  得到uid从云端
  *  @author  <gy> 
  *  @param[in] <CloudReturnValue指针>
  *  @param[in] <UID获得的uid>
  *  @param[out] <得到uid>
  *  @return   <-1代表失败> <0代表成功>
  *  @note
*/
int getUID4Cloud(char *CloudReturnValue,char *UID){
	int nRet = 0;
	int ConnectType = 0;
	char *pConnectType = strstr(CloudReturnValue,"ConnectType");
	if(pConnectType == NULL){
		return -1;
	}else{
		sscanf(pConnectType,"ConnectType=%d",&ConnectType);
	}

	char *pConnectInfo = strstr(CloudReturnValue,"ConnectInfo");
	if(pConnectInfo == NULL){
		free(pConnectInfo);
		pConnectInfo = NULL;
		return -1;
	}else{
		sscanf(pConnectInfo,"ConnectInfo=%s",UID);
	}

	DEBUGPRINT(DEBUG_INFO,"<<-------------M3S get UID from Cloud is %s------------>>\n",UID);
	//*UID = ConnectInfo;
	return nRet;
}

/**
  *  @brief  发送消息给云端函数
  *  @author  <gy> 
  *  @param[in] <cloud_return_value返回值指针>
  *  @param[in] <IP地址>
  *  @param[in] <https_2_cloudmsg要发送的数据指针>
  *  @param[in] <Port端口>
  *  @param[in] <msglength消息长度>
  *  @param[out] <无>
  *  @return   <-1代表失败> <0代表成功>
  *  @note
*/
int Send2CloudMSG(char **cloud_return_value,char *https_2_cloudmsg,char *IP,int Port,int msglength){

	if(https_2_cloudmsg == NULL||IP == NULL){
		printf("__LINE__:%d \n",__LINE__);
		return -1;
	}
	int nRet = 0;
	int Sockfd = 0;
	struct sockaddr_in stServer;
	SSL_CTX *pCTX = NULL;
	SSL *pSSL = NULL;
	//char strBuffer[MAX_MSG_LEN+1];
	/*create ssl socket*/
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	pCTX = SSL_CTX_new(SSLv23_client_method());;
	if(NULL == pCTX){
		log_s("SSL_CTX_new error",strlen("SSL_CTX_new error"));
		return -1;
	}
	//create socket
	if((Sockfd = socket(AF_INET,SOCK_STREAM,0))<0){
		log_s("socket error",strlen("socket error"));
		SSL_CTX_free(pCTX);
		return -1;
	}
    /* set timeout */
	struct timeval timeout;
	timeout.tv_sec = 60;
	timeout.tv_usec = 0;
	nRet = setsockopt(Sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval));
	if (0 > nRet)
	{
		close(Sockfd);
		Sockfd = 0;
        SSL_CTX_free(pCTX);
		return -1;
	}
    /* init the server add and port */
    bzero(&stServer, sizeof(stServer));
    stServer.sin_family = AF_INET;
	stServer.sin_addr.s_addr = inet_addr(IP);
    stServer.sin_port = htons(Port);
    /*socket connect Cloud....*/
    if(0 != (connect(Sockfd, (struct sockaddr *) &stServer, sizeof(stServer)))){
    	log_s("connect error",strlen("connect error"));
    	SSL_CTX_free(pCTX);
    	close(Sockfd);
		Sockfd = 0;
		return -1;
    }
    /*create a pSSl base pCTX*/
    pSSL = SSL_new(pCTX);
    SSL_set_fd(pSSL,Sockfd);
    /*SSL connect */
    if(-1 == SSL_connect(pSSL)){
    	log_s("SSL_connect error",strlen("SSL_connect error"));
    	SSL_shutdown(pSSL);
    	SSL_free(pSSL);
    	SSL_CTX_free(pCTX);
    	close(Sockfd);
    	Sockfd = 0;
    	return -1;
    }
    printf("https_2_cloudmsg:%s",https_2_cloudmsg);
    if( 0 > SSL_write(pSSL,https_2_cloudmsg,msglength)){
    	log_s("SSL_write error",strlen("SSL_write error"));
    	SSL_shutdown(pSSL);
		SSL_free(pSSL);
		SSL_CTX_free(pCTX);
		close(Sockfd);
		Sockfd = 0;
		return -1;
    }
    //read from Cloud
    int nIndex = 0;
    int nLength = 0;
    int nTemp = 0;
    char chTemp;
    char *strRecvMsg = NULL;
    strRecvMsg = malloc(512);
    memset(strRecvMsg,0,512);
    while(nIndex<4){
    	nLength = SSL_read(pSSL, &chTemp, 1);
		if(nLength < 0)
		{
			log_s("SSL_read Head Connect error",strlen("SSL_read Head Connect error"));
			break;
		}
		if(nTemp>500){
			break;
		}else{
			strRecvMsg[nTemp++] = chTemp;
		}
		((chTemp=='\r')||(chTemp=='\n')) ? (nIndex++) : (nIndex=0);
		//printf("chTemp:%c",chTemp);
    }
    printf("\n");
    if(0 == strlen(strRecvMsg)){
    	log_s("SSL_read Head read 0",strlen("SSL_read Head read 0"));
    	SSL_shutdown(pSSL);
		SSL_free(pSSL);
		SSL_CTX_free(pCTX);
		close(Sockfd);
		Sockfd = 0;
		free(strRecvMsg);
		strRecvMsg = NULL;
		return -1;
    }else{
    	printf("============================================\n"
    			"__LINE__:%d strRecvMsg:%s\n"
    			"===========================================\n",__LINE__,strRecvMsg);

    }

    long int retDataLength = 0;
    char *pTmp = strstr(strRecvMsg,"Content-Length:");
	if(NULL == pTmp){
		SSL_shutdown(pSSL);
		SSL_free(pSSL);
		SSL_CTX_free(pCTX);
		close(Sockfd);
		Sockfd = 0;
		free(strRecvMsg);
		strRecvMsg = NULL;
		return -1;
	}else{
		sscanf(pTmp,"Content-Length: %ld",&retDataLength);
	}

	if(retDataLength >= 512){
		goto TOOLONG;
	}
	printf("__LINE__:%d retDataLength:%ld\n ",__LINE__,retDataLength);

	memset(strRecvMsg,0,512);
	log_s("SSL_read data read",strlen("SSL_read data read"));
	if(SSL_read(pSSL,strRecvMsg,retDataLength)<0){
		log_s("SSL_read data read error",strlen("SSL_read data read error"));
		SSL_shutdown(pSSL);
		SSL_free(pSSL);
		SSL_CTX_free(pCTX);
		close(Sockfd);
		Sockfd = 0;
		free(strRecvMsg);
		strRecvMsg = NULL;
		return -1;
	}else{

	}

	TOOLONG:
	SSL_shutdown(pSSL);
	SSL_free(pSSL);
	SSL_CTX_free(pCTX);
	close(Sockfd);
	Sockfd = 0;
	*cloud_return_value = strRecvMsg;
	DEBUGPRINT(DEBUG_INFO,"=========================strRecvMsg:%s==============================\n", strRecvMsg);
	log_s(strRecvMsg,strlen(strRecvMsg));
	return nRet = retDataLength;

}
/**
  *  @brief  发送升级状态给云端函数
  *  @author  <gy> 
  *  @param[in] <IP地址>
  *  @param[in] <https_2_cloudmsg要发送的数据指针>
  *  @param[in] <Port端口>
  *  @param[in] <msglength消息长度>
  *  @param[out] <无>
  *  @return   <-1代表失败> <0代表成功>
  *  @note
*/
int Send2CloudMSG4UpgradeState(char *https_2_cloudmsg,char *IP,int Port,int msglength){

	if(https_2_cloudmsg == NULL||IP == NULL){
		printf("__LINE__:%d \n",__LINE__);
		return -1;
	}
	int nRet = 0;
	int Sockfd = 0;
	struct sockaddr_in stServer;
	SSL_CTX *pCTX = NULL;
	SSL *pSSL = NULL;
	//char strBuffer[MAX_MSG_LEN+1];
	/*create ssl socket*/
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	pCTX = SSL_CTX_new(SSLv23_client_method());;
	if(NULL == pCTX){
		return -1;
	}
	//create socket
	if((Sockfd = socket(AF_INET,SOCK_STREAM,0))<0){
		SSL_CTX_free(pCTX);
		return -1;
	}
    /* set timeout */
	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	nRet = setsockopt(Sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval));
	nRet = setsockopt(Sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(struct timeval));
	if (0 > nRet)
	{
		close(Sockfd);
		Sockfd = 0;
        SSL_CTX_free(pCTX);
		return -1;
	}
    /* init the server add and port */
    bzero(&stServer, sizeof(stServer));
    stServer.sin_family = AF_INET;
	stServer.sin_addr.s_addr = inet_addr(IP);
    stServer.sin_port = htons(Port);
    /*socket connect Cloud....*/
    if(0 != (connect(Sockfd, (struct sockaddr *) &stServer, sizeof(stServer)))){
    	SSL_CTX_free(pCTX);
    	close(Sockfd);
		Sockfd = 0;
		return -1;
    }
    /*create a pSSl base pCTX*/
    pSSL = SSL_new(pCTX);
    SSL_set_fd(pSSL,Sockfd);
    /*SSL connect */
    if(-1 == SSL_connect(pSSL)){
    	SSL_shutdown(pSSL);
    	SSL_free(pSSL);
    	SSL_CTX_free(pCTX);
    	close(Sockfd);
    	Sockfd = 0;
    	return -1;
    }
    printf("https_2_cloudmsg:%s",https_2_cloudmsg);
    if( 0 > SSL_write(pSSL,https_2_cloudmsg,msglength)){
    	SSL_shutdown(pSSL);
		SSL_free(pSSL);
		SSL_CTX_free(pCTX);
		close(Sockfd);
		Sockfd = 0;
		return -1;
    }

	SSL_shutdown(pSSL);
	SSL_free(pSSL);
	SSL_CTX_free(pCTX);
	close(Sockfd);
	Sockfd = 0;
	return 0;
#if 0
    //read from Cloud
    int nIndex = 0;
    int nLength = 0;
    int nTemp = 0;
    char chTemp;
    char *strRecvMsg = NULL;
    strRecvMsg = malloc(512);
    memset(strRecvMsg,0,512);
    while(nIndex<4){
    	nLength = SSL_read(pSSL, &chTemp, 1);
		if(nLength < 0)
		{
			break;
		}
		strRecvMsg[nTemp++] = chTemp;
		((chTemp=='\r')||(chTemp=='\n')) ? (nIndex++) : (nIndex=0);
    }

    if(0 == strlen(strRecvMsg)){
    	SSL_shutdown(pSSL);
		SSL_free(pSSL);
		SSL_CTX_free(pCTX);
		close(Sockfd);
		Sockfd = 0;
		free(strRecvMsg);
		strRecvMsg = NULL;
		return -1;
    }else{
    	printf("============================================\n"
    			"__LINE__:%d strRecvMsg:%s\n"
    			"===========================================\n",__LINE__,strRecvMsg);
    }
//    printf("__LINE__:%d \n",__LINE__);
    long int retDataLength = 0;
    char *pTmp = strstr(strRecvMsg,"Content-Length:");
	if(NULL == pTmp){
		SSL_shutdown(pSSL);
		SSL_free(pSSL);
		SSL_CTX_free(pCTX);
		close(Sockfd);
		Sockfd = 0;
		free(strRecvMsg);
		strRecvMsg = NULL;
		return -1;
	}else{
		sscanf(pTmp,"Content-Length: %ld",&retDataLength);
	}

	printf("__LINE__:%d retDataLength:%ld\n ",__LINE__,retDataLength);

	memset(strRecvMsg,0,512);
	if(SSL_read(pSSL,strRecvMsg,retDataLength)<0){
		SSL_shutdown(pSSL);
		SSL_free(pSSL);
		SSL_CTX_free(pCTX);
		close(Sockfd);
		Sockfd = 0;
		free(strRecvMsg);
		strRecvMsg = NULL;
		return -1;
	}else{

	}
	SSL_shutdown(pSSL);
	SSL_free(pSSL);
	SSL_CTX_free(pCTX);
	close(Sockfd);
	Sockfd = 0;
	*cloud_return_value = strRecvMsg;
	printf("%p =========================strRecvMsg:%s==============================\n",strRecvMsg, strRecvMsg);
	return nRet = retDataLength;
#endif

}

/* get the enr radon number */
int CreatCamEncryptionID(unsigned char *pCamEncryptionID, const unsigned char *pRandom)
{
    unsigned char strRSACipher[512];

    if((NULL==pCamEncryptionID) || (NULL==pRandom))
    {
        return -1;
    }

    memset(strRSACipher, 0, sizeof(strRSACipher));
    RSAPublicEncryption(strRSACipher, pRandom, 1);
    Base64Encode/**
  *  @brief  发送升级状态给云端函数
  *  @author  <gy> 
  *  @param[in] <IP地址>
  *  @param[in] <https_2_cloudmsg要发送的数据指针>
  *  @param[in] <Port端口>
  *  @param[in] <msglength消息长度>
  *  @param[out] <无>
  *  @return   <-1代表失败> <0代表成功>
  *  @note
*/(pCamEncryptionID, strRSACipher, 128);

    return 0;
}

/**
  *  @brief  得到IPU_R
  *  @author  <gy> 
  *  @param[in] <strIPUEnrID值>
  *  @param[in] <strIPUEnrURLID值>
  *  @param[in] <strkey_N值>
  *  @param[in] <strkey_E值>
  *  @param[out] <无>
  *  @return   <-1代表失败> <0代表成功>
  *  @note
*/
int SetIPU_R(char *r,char *strIPUEnrID,char *strIPUEnrURLID,char * strkey_N,char *strkey_E)
{
	if(r==NULL||strIPUEnrID== NULL||strIPUEnrURLID == NULL||strkey_N==NULL||strkey_E==NULL){
		return -1;
	}
	//printf("RcheckBuffer->strKeyN:%s\n RcheckBuffer->strKeyE:%s   adddd\n",strkey_N,strkey_E);
	char r_temp[9] ;
	memset(r_temp,'\0',sizeof(r_temp));
	memcpy(r_temp,r,8);

    if (strlen(r_temp) == 8)
    {
    	printf("r_temp:%s\n",r_temp);
		memset((char*)strIPUEnrID,'\0', 512);
		CreatIPUEncryptionID((char*)strIPUEnrID, r_temp,
//				"1/QDFK78fTzAembbXZO59TOYXBaMvycs10YqCd1ggeq2ieo6wbLBfRgIMJa6Id4N7+HYXVWmkWjAV3r3fP7H17PeBnRuo3zQ109fopC1jTSzl6tGTYG6ca3D+dh5u7hNjU2g2o4np3yYJioxjLiQS/PG+8/s3EWh6oGYGv9Lwqc=",
//				"AQAB");
				strkey_N,strkey_E);
		ClearCRLF(strIPUEnrID);
		printf("Rl is %s\n",strIPUEnrID);
		memset((char*)strIPUEnrURLID,'\0', 512);
		//DecodeEncryptionID(strIPUEnrID);
		ChangeBase64ToURL((char*)strIPUEnrURLID, (char*)strIPUEnrID);
		printf("Rl is %s\n",strIPUEnrURLID);
    }
    return 0;
}


int write_log(long date,char *ret_json){
	if(ret_json == NULL){

		FILE *fp = fopen("/usr/share/power", "w");
		if(fp == NULL){
			printf("open failed\n");
			return -1;
		}
		char data[512];
		memset(data,'\0',sizeof(data));
		sprintf(data,"%ld %s\n",date,"Server Fail");
		fwrite(data, strlen(data), 1, fp);
		fclose(fp);
		return -1;
	}
    FILE *fp = fopen("/usr/share/power", "w");
    if(fp == NULL){
		printf("open failed\n");
		return -1;
    }
    char data[512];
    memset(data,'\0',sizeof(data));
    sprintf(data,"%ld ",date);
    fwrite(data, strlen(data), 1, fp);
    fwrite(ret_json,strlen(ret_json),1,fp);
	
    fclose(fp);
    return 0;

}

int write_file(char * data){

	if(data != NULL){
		FILE *fp = fopen("/usr/share/log", "w");
		if(fp == NULL){
			printf("open failed\n");
			return -1;
		}

		fwrite(data, strlen(data), 1, fp);

		fclose(fp);
	}
    return 0;

}

int write_file1(char * data,const char *ret){

	if(data != NULL){
		FILE *fp = fopen("/usr/share/powerstate", ret);
		if(fp == NULL){
			printf("open failed\n");
			return -1;
		}

		fwrite(data, strlen(data), 1, fp);

		fclose(fp);
	}
    return 0;

}

