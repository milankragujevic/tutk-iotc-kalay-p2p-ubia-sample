/*
 * do_https_m3s.c
 *
 *  Created on: Mar 12, 2013
 *      Author: root
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
#include "tools.h"
#include "encryption.h"
#include "do_https_m3s.h"
#include "upgrade.h"
#include "common/thread.h"
#include "common/common.h"
#include "common/common_config.h"

typedef struct {
	int MSGID_REV_DO_HTTPS ;
	int MSGID_SEND_DO_HTTPS ;
	Child_Process_Msg * s_rev_https;
	Child_Process_Msg * s_send_https;
}stDoUpgradeMSG ;

typedef struct{
	int length_KEY;
	char pcPbKeyValue[256];
    char Rcheck[512];
    char Rcheck_URL[512];
    char strKeyN[172+4];
    char strKeyE[4+4];

}stRcheck;

volatile int R_UpgradeHost;
volatile int R_UpgradeThread;

int InitArgsHttpsThread(stDoUpgradeMSG *args);
//int GetBuildMSG(char **pSendMsg,const char* Addr,char *alarmgroup,const char*alarmtype);
int UpgradeGetBuildMSG(char **ppSendMsg,long int UpdateTime,char *UpdateMsg,char *IP,const int Port,const char* Addr,int alarmgroup,int alarmtype);
int PostBuildMSG(char **pSendMsg,char *IP,const int Port,const char* addr,const char* alarmtype,const char *filetype,
		long int filegroup,const int number,long int fileleng,char *file);
int RevMsgUpgrade(stDoUpgradeMSG *self);
int SendMsgUpgrade(stDoUpgradeMSG *self,const int CMD,char *data,int length);
int UpgradeSend2CloudMSG(char **cloud_return_value,char *https_2_cloudmsg,char *IP,int Port);
int GetIPByHost(char *pIP, const char *pHostName);
//int SetStableFlag(void);
int SetIPU_R(char *r,char *strIPUEnrID,char *strIPUEnrURLID,char * strkey_N,char *strkey_E);

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

void *DO_Upgrade_Thread(){

	stRcheck *RcheckBuffer = NULL;
	RcheckBuffer = malloc(sizeof(stRcheck));
	if(RcheckBuffer == NULL){

	}else{
		memset(RcheckBuffer,0,sizeof(stRcheck));
	}
	stDoUpgradeMSG * doUpgradeMsg = NULL;
	doUpgradeMsg = malloc(sizeof(stDoUpgradeMSG));
	if(-1 == InitArgsHttpsThread(doUpgradeMsg)){

	}

	char IP[17];
	int count = 0;
	int elsef_lag = 0;
	char *Upgrade2CloudMSG = NULL;
	char *CloudReturnValue = NULL;


	while(1){

		if(count % 100 == 0){
			count = 0;
			//getTimeAndPrint(2,__LINE__,"Hello M3s this here is audioAlarm thread");
		}
		count++;

		memset(doUpgradeMsg->s_rev_https->data,0,MAX_MSG_LEN);
		elsef_lag = 0;
		if(RevMsgUpgrade(doUpgradeMsg) ==-1){
			thread_usleep(100000);
		}else{
			printf("Message:Rev msg from https !!!CMD :%d\n",doUpgradeMsg->s_rev_https->Thread_cmd);
			switch(doUpgradeMsg->s_rev_https->Thread_cmd){

			/*
			 * thread doing start-work must get IP addrs By Host
			 *  * */
				case MSG_UPGRADE_T_START:
					if(0 == GetIPByHost(IP, "api.ibabycloud.com")){
						printf("GetIPByHost OK\n");
						SendMsgUpgrade(doUpgradeMsg,MSG_UPGRADE_P_START,"SUCCESS",strlen("SUCCESS"));
					}else{
						printf("GetIPByHost error \n");
						SendMsgUpgrade(doUpgradeMsg,MSG_UPGRADE_P_START,"FAIL",strlen("FAIL"));
					}
					break;
			/*
			 * camera VERIFYCAM by iOS
			 * Get fuction .....
			 * */
				case MSG_UPGRADE_T_UPGRADE_STATE:
					if(-1 == UpgradeGetBuildMSG(&Upgrade2CloudMSG,0,"hello",IP,CLOUD_HTTPS_SERVER_PORT,ADDR_VERIFYCAM,0,0)){
						//printf("build message is error....\n");
					}else{
						//printf("Https2CloudMSG:%s",Https2CloudMSG);
						if(-1 == UpgradeSend2CloudMSG(&CloudReturnValue,Upgrade2CloudMSG,IP,CLOUD_HTTPS_SERVER_PORT)){

						}else{
							elsef_lag = 1;
						}
					}
					if(elsef_lag == 1){
						if(8 == strlen(CloudReturnValue)){
							SendMsgUpgrade(doUpgradeMsg,MSG_HTTPSSEVER_P_VERIFYCAM,CloudReturnValue,strlen(CloudReturnValue));
						}else if(0 == strncmp(CloudReturnValue,"FAIL",4)){
							SendMsgUpgrade(doUpgradeMsg,MSG_HTTPSSEVER_P_VERIFYCAM,"FAIL",strlen("FAIL"));
						}
					}else{
						//fail
					}

					elsef_lag = 0;
					free(Upgrade2CloudMSG);
					Upgrade2CloudMSG = NULL;
					free(CloudReturnValue);
					CloudReturnValue = NULL;

					break;
				case MSG_HTTPSSEVER_T_LOGIN_CAM:

#if 0
//					get_pb_key_value(RcheckBuffer->pcPbKeyValue, &RcheckBuffer->length_KEY)
//					memcpy(RcheckBuffer->strKeyN,RcheckBuffer->pcPbKeyValue,172);
//					memcpy(RcheckBuffer->strKeyE,RcheckBuffer->pcPbKeyValue+172,4);
#endif
					memcpy(RcheckBuffer->pcPbKeyValue,"1/QDFK78fTzAembbXZO59TOYXBaMvycs10YqCd1ggeq2ieo6wbLBfRgIMJa6Id4N7+HYXVWmkWjAV3r3fP7H17PeBnRuo3zQ109fopC1jTSzl6tGTYG6ca3D+dh5u7hNjU2g2o4np3yYJioxjLiQS/PG+8/s3EWh6oGYGv9Lwqc=AQAB",176);
					printf("RcheckBuffer->pcPbKeyValue:%s\n",RcheckBuffer->pcPbKeyValue);
					memcpy(RcheckBuffer->strKeyN,RcheckBuffer->pcPbKeyValue,172);
					memcpy(RcheckBuffer->strKeyE,RcheckBuffer->pcPbKeyValue+sizeof(RcheckBuffer->strKeyN),4);
					//printf("RcheckBuffer->strKeyN:%s\n RcheckBuffer->strKeyE:%s   adddd\n",RcheckBuffer->strKeyN,RcheckBuffer->strKeyE);

					if(0 == SetIPU_R(doUpgradeMsg->s_rev_https->data,RcheckBuffer->Rcheck,RcheckBuffer->Rcheck_URL,RcheckBuffer->strKeyN,RcheckBuffer->strKeyE)){
						if(EXIT_FAILURE == set_config_value_to_ram(CONFIG_Camera_EncrytionID,RcheckBuffer->Rcheck,strlen(RcheckBuffer->Rcheck))){
							//fail
						}else{
							//success
						}
					}else{
						//fail
					}

					if(-1 == PostBuildMSG(&Upgrade2CloudMSG,IP,CLOUD_HTTPS_SERVER_PORT,ADDR_LOGIN_CAM,"","",0,0,0,"")){

					}else{
						printf(":%s\n",Upgrade2CloudMSG);
						if(-1 == UpgradeSend2CloudMSG(&CloudReturnValue,Upgrade2CloudMSG,IP,CLOUD_HTTPS_SERVER_PORT)){

						}else{
							elsef_lag = 1;
						}
					}

					if(1 == elsef_lag){
						if(0 == strncmp(CloudReturnValue,"OK",2)){
							char *Camera_EncrytionID = NULL;
							int n = 0;
							get_config_item_value(CONFIG_Camera_EncrytionID,Camera_EncrytionID, &n);
							if(n == 0){
								memcpy(Camera_EncrytionID,"NULL",4);
							}
							set_config_value_to_flash(CONFIG_Camera_EncrytionID,Camera_EncrytionID, n);
						}else if(0 == strncmp(CloudReturnValue,"FAIL",4)){

						}
					}else{
						//fail
					}
					elsef_lag = 0;
					free(Upgrade2CloudMSG);
					Upgrade2CloudMSG = NULL;
					free(CloudReturnValue);
					CloudReturnValue = NULL;

					break;
				case MSG_HTTPSSEVER_T_UPLOAD_FILE_CAM:

					if(-1 == PostBuildMSG(&Upgrade2CloudMSG,IP,CLOUD_HTTPS_SERVER_PORT,ADDR_LOGIN_CAM,"","",0,0,0,"")){

					}else{
						if(-1 == UpgradeSend2CloudMSG(&CloudReturnValue,Upgrade2CloudMSG,IP,CLOUD_HTTPS_SERVER_PORT)){

						}else{
							elsef_lag = 1;
						}
					}

					if(1 == elsef_lag){

					}else{
						//fail
					}
					elsef_lag = 0;
					free(Upgrade2CloudMSG);
					Upgrade2CloudMSG = NULL;
					free(CloudReturnValue);
					CloudReturnValue = NULL;


					break;
				case MSG_HTTPSSEVER_T_PUSH_ALARM_2_CLOUND_CAM:

					if(-1 == UpgradeGetBuildMSG(&Upgrade2CloudMSG,IP,CLOUD_HTTPS_SERVER_PORT,ADDR_VERIFYCAM,*(doUpgradeMsg->s_rev_https->data),*(doUpgradeMsg->s_rev_https->data+4))){
						printf("build message is error....\n");
					}else{
						if(-1 == UpgradeSend2CloudMSG(&CloudReturnValue,Upgrade2CloudMSG,IP,CLOUD_HTTPS_SERVER_PORT)){

						}else{
							elsef_lag = 1;
						}
					}

					if(elsef_lag == 1){
						if(0 == strncmp(CloudReturnValue,"OK",2)){

						}else if(0 == strncmp(CloudReturnValue,"FAIL",4)){
							//fail
						}
					}else{
						//fail
					}

					elsef_lag = 0;
					free(Upgrade2CloudMSG);
					Upgrade2CloudMSG = NULL;
					free(CloudReturnValue);
					CloudReturnValue = NULL;

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

int InitArgsHttpsThread(stDoUpgradeMSG *args){
	int nRet = 0;
	if(NULL == args){
		return -1;
	}

	args->MSGID_REV_DO_HTTPS = msg_queue_create((key_t)UPGRADE_SERVER_MSG_KEY );
	if (args->MSGID_REV_DO_HTTPS == -1) {
		fprintf(stderr, "create MSGID_SEND_THREAD_: %s\n",strerror(errno));
		nRet = -1;
	}

	args->MSGID_SEND_DO_HTTPS = msg_queue_create((key_t)DO_UPGRADE_SERVER_MSG_KEY );
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
	thread_synchronization(&R_UpgradeThread, &R_UpgradeHost);


	return nRet;
}

int RevMsgUpgrade(stDoUpgradeMSG *self){
	int nRet = 0;
	nRet = RevMsgFunction(self->MSGID_REV_DO_HTTPS,self->s_rev_https,UPGRADE_2_DO_MSG_TYPE);
	return nRet;
}

int SendMsgUpgrade(stDoUpgradeMSG *self,const int CMD,char *data,int length){
	int nRet = 0;
	self->s_send_https->msg_type = DO_2_UPGRADE_MSG_TYPE;
	self->s_send_https->Thread_cmd = CMD;
	memcpy(self->s_send_https->data,data,length);
	self->s_send_https->length = length;
	//printf("self->s_send_https->length:%d\r\n",self->s_send_https->length);
	//printf("\n\rtype %ld \n\r CMD:%d  data :%s __LINE__:%d\r\n",self->s_send_https->msg_type,self->s_send_https->Thread_cmd,self->s_send_https->data,__LINE__);
	nRet = SendMsgFunction(self->MSGID_SEND_DO_HTTPS,self->s_send_https);
	return nRet;
}

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

int UpgradeGetBuildMSG(char **ppSendMsg,long int UpdateTime,char *UpdateMsg,char *IP,const int Port,const char* Addr,int alarmgroup,int alarmtype)
{
	char Camera_ID[32];
	char Camera_Type[8];
	char Camera_EncrytionID[256];
	char Camera_Fwver[12];
	char Camera_Appver[12];
	char Camera_Confver[12];
//	char Camera_
//	char Camera_

	if(Addr == NULL ||ppSendMsg == NULL||IP == NULL/*||alarmgroup == NULL||alarmtype == NULL*/){
		return -1;
	}
	char *pSendMsg;
	pSendMsg = malloc(1024);
	int n = 0;
#if 0
	get_mac_addr_value(Camera_ID, &n);
	get_config_item_value(CONFIG_Camera_Type,Camera_Type, &n);
	get_config_item_value(CONFIG_Camera_EncrytionID,Camera_EncrytionID, &n);

	get_config_item_value(CONFIG_Conf_Version,Camera_Fwver, &n);
	get_config_item_value(CONFIG_Camera_EncrytionID,Camera_EncrytionID, &n);
	get_config_item_value(CONFIG_Camera_EncrytionID,Camera_EncrytionID, &n);
#endif
	if(n == 0){
//		memcpy(Camera_EncrytionID,"NULL",4);
	}
																	char Camera_Fwver[12];
																//	char Camera_Appver[12];
																//	char Camera_Confver[12];
	sprintf(pSendMsg, "GET %s?camid=%s&camenr=%s&camtype=%s"
			"&fwver=%s&appver=%s&confver=%s"
			"&updatetime=%ld&updatemsg=%s "
			"HTTP/1.1\r\nHost:%s:%d\r\n",
			Addr,"220C43305048"/*Camera_ID*/,""/*Camera_Type*/,"M3S"/*Camera_EncrytionID*/,
			Camera_Fwver,Camera_Appver,Camera_Confver,

			IP,Port);
    strcat(pSendMsg, "Content-Type:application/x-www-form-urlencoded\r\n");
    strcat(pSendMsg, "Accept:*/*\r\n");
    strcat(pSendMsg, "Connection:Keep-Alive\r\n");
    strcat(pSendMsg, "Cache-Control:no-cache\r\n");
    strcat(pSendMsg, "\r\n");

    *ppSendMsg = pSendMsg;

    return 0;
}

int PostBuildMSG(char **pSendMsg,char *IP,const int Port,const char* addr,const char* alarmtype,const char *filetype,
		long int filegroup,const int number,long int fileleng,char *file){

	if(pSendMsg == NULL||addr == NULL||alarmtype == NULL||filetype == NULL||file == NULL){
		return -1;
	}

	int nRet = 0;

	char *pSendMsg_tmp = NULL;
	pSendMsg_tmp = malloc(64*1024);
    char *strTempList = NULL;
	strTempList = malloc(63*1024);
    char strPostDataLength[32];

	char Camera_ID[32];
	memset(Camera_ID,0,sizeof(Camera_ID));
	char Camera_Type[8];
	memset(Camera_Type,0,sizeof(Camera_Type));
	char Camera_EncrytionID[512];
	memset(Camera_EncrytionID,0,sizeof(Camera_EncrytionID));
	char Camera_EncrytionID_URL[512];
	memset(Camera_EncrytionID_URL,0,sizeof(Camera_EncrytionID_URL));
	int n = 0;
//	get_mac_addr_value(Camera_ID, &n);
//	get_config_item_value(CONFIG_Camera_Type,Camera_Type, &n);
	get_config_item_value(CONFIG_Camera_EncrytionID,Camera_EncrytionID, &n);
	ChangeBase64ToURL(Camera_EncrytionID_URL, Camera_EncrytionID);
	printf("Camera_EncrytionID:%s\nCamera_EncrytionID_URL:%s\n",Camera_EncrytionID,Camera_EncrytionID_URL);

	if(n == 0){
//		memcpy(Camera_EncrytionID,"NULL",4);
	}

    sprintf(strTempList, "camid=%s&camenr=%s&camtype=%s"/*"&alarmtype=%s&filetype=%s&filegroup=%ld&number=%d&fileleng=%ld&file=%s"*/,
    		"220C43305048"/*Camera_ID*/,Camera_EncrytionID_URL,"M3S"/*Camera_Type*//*,alarmtype,filetype,filegroup,number,fileleng,file*/);

    memset(strPostDataLength, '\0', sizeof(strPostDataLength));
    sprintf(strPostDataLength, "Content-Length:%d\r\n", strlen(strTempList));

    sprintf(pSendMsg_tmp, "POST %s HTTP/1.1\r\nHost:%s:%d\r\n", addr,IP, Port);
    strcat(pSendMsg_tmp, "Content-Type:application/x-www-form-urlencoded\r\n");
    strcat(pSendMsg_tmp, strPostDataLength);
    strcat(pSendMsg_tmp, "Connection:Keep-Alive\r\n");
    strcat(pSendMsg_tmp, "Cache-Control:no-cache\r\n");
    strcat(pSendMsg_tmp, "\r\n");
    strcat(pSendMsg_tmp, strTempList);
    *pSendMsg =  pSendMsg_tmp;

	return nRet = strlen(strTempList);

}

int UpgradeSend2CloudMSG(char **cloud_return_value,char *https_2_cloudmsg,char *IP,int Port){


	if(https_2_cloudmsg == NULL||IP == NULL){
		return -1;
	}
	int nRet = 0;
	int Sockfd = 0;
	struct sockaddr_in stServer;
	SSL_CTX *pCTX = NULL;
	SSL *pSSL = NULL;
	char strBuffer[MAX_MSG_LEN+1];
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
	timeout.tv_sec = 30;
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
    if( 0 > SSL_write(pSSL,https_2_cloudmsg,strlen(https_2_cloudmsg))){
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
//    char strRecvMsg[512];
    char *strRecvMsg = NULL;
    strRecvMsg = (char *)malloc(512);
    memset(strRecvMsg,0,512);
    while(nIndex<4){
    	nLength = SSL_read(pSSL, &chTemp, 1);
		if(nLength < 0)
		{
			break;
		}
		printf("%c",chTemp);
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
	printf("%p strRecvMsg:%s\n",strRecvMsg, strRecvMsg);
	printf("%p cloud_return_value:%s\n",*cloud_return_value, *cloud_return_value);
	return nRet = retDataLength;

}


#if 0

int GetCloudResponse(char *pBuffer, char *pRepMsg, int nLength)
{
    int nRet = 0;
	int nIndex = 0;
	int nCurrent = 0;
    int nMsgBegin = 0;

    if ((NULL==pBuffer)|| (NULL==pRepMsg))
    {
        DEBUGPRINT(DEBUG_ERROR, "Pointer is NULL! @:%s @:%d\n", __FILE__, __LINE__);
        return nRet = -1;
    }

    while(nCurrent < nLength)
    {
		if (nIndex < 4) /* https头信息 */
		{
			((pBuffer[nCurrent]=='\r')||(pBuffer[nCurrent]=='\n')) ? (nIndex++) : (nIndex=0);
            nMsgBegin = nCurrent + 1;
		}

        nCurrent++;
    }

    /* https 响应信息 */
    nRet = (nLength-nMsgBegin)*sizeof(unsigned char);
    memcpy(pRepMsg, &pBuffer[nMsgBegin], nRet);
    if(0 == nRet){
    	DEBUGPRINT(DEBUG_ERROR, "Pointer is NULL! @:%s @:%d\n", __FILE__, __LINE__);
    }

    return nRet;
}
#endif

#if 0
int SetStableFlag(void)
{
	int nRet = 0;
	char strImgStable[2] = {'\0'};

	ReadStable(strImgStable);
	if (strlen(strImgStable) > 0)
	{
		if (strcmp(strImgStable, "1") != 0)
		{
			nRet = 1;
		}
	}
	else
	{
		nRet = 1;
	}

	if (nRet == 1)
	{
		system("nvram_set uboot Image1Try 0");
		if (SaveStable("1") < 0)
		{
			//DEBUGPRINT(DEBUG_ERROR, "@@@: set image1 stable fail! @:%s %d\n", __FILE__, __LINE__);
			return -1;
		}

//		system("cp /dev/mtdblock4 /dev/mtdblock5");
		sleep(1);
//		system("reboot");
	}

	return 0;
}
#endif
int SetIPU_R(char *r,char *strIPUEnrID,char *strIPUEnrURLID,char * strkey_N,char *strkey_E)
{
	if(r==NULL||strIPUEnrID== NULL||strIPUEnrURLID == NULL||strkey_N==NULL||strkey_E==NULL){
		return -1;
	}
	//printf("RcheckBuffer->strKeyN:%s\n RcheckBuffer->strKeyE:%s   adddd\n",strkey_N,strkey_E);
	char r_temp[9] ;
	memset(r_temp,'\0',sizeof(r_temp));
	memcpy(r_temp,r,sizeof(r_temp));

    if (strlen(r_temp) == 8)
    {
    	printf("r_temp:%s\n",r_temp);
		memset((char*)strIPUEnrID,'\0', sizeof(strIPUEnrID));
		CreatIPUEncryptionID((char*)strIPUEnrID, r_temp,
				"1/QDFK78fTzAembbXZO59TOYXBaMvycs10YqCd1ggeq2ieo6wbLBfRgIMJa6Id4N7+HYXVWmkWjAV3r3fP7H17PeBnRuo3zQ109fopC1jTSzl6tGTYG6ca3D+dh5u7hNjU2g2o4np3yYJioxjLiQS/PG+8/s3EWh6oGYGv9Lwqc=",
				"AQAB");
				//strkey_N,strkey_E);
		ClearCRLF(strIPUEnrID);
		memset((char*)strIPUEnrURLID,'\0', sizeof(strIPUEnrURLID));
		DecodeEncryptionID(strIPUEnrID);
		ChangeBase64ToURL((char*)strIPUEnrURLID, (char*)strIPUEnrID);
    }
    return 0;
}


