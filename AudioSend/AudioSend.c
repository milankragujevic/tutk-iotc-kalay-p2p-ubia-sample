#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/time.h>
#include "common/common.h"
#include "common/logfile.h"
#include "common/thread.h"
#include "common/pdatatype.h"
#include "common/msg_queue.h"
#include "Audio/Audio.h"
#include "AudioSend/AudioSend.h"
#include "NewsChannel/NewsUtils.h"
#include "NewsChannel/NewsChannel.h"

/************************************************************************/
/*************************结   构   体  定   义 ******************************/
/***********************************************************************/
#define MAX_AUDIO_MSG_LEN   512
#define SOCKETFD_MXA        5
//#define AUDIO_BUFFER_SIZE   30
#define AUDIO_DATA_SEND		304
#define DEFAULT_TIMEOUT 	0
#define MAX_SOCK_NUM 12
#define BUFSIZE 160
#define NULL_FD 0
typedef unsigned char byte;
//extern volatile char szAudioBuf[AUDIO_BUFFER_SIZE][40+160];
//extern volatile NewsChannel_t *NewsChannelSelf;

//volatile FILE *adpcmFileFDabc = NULL;

struct AudioSendThreadMsg {
	int audioSendThreadMsgID;
	int childProcessMsgID;
	char AudioMsgRecv[MAX_AUDIO_MSG_LEN];
	char AudioMsgSend[MAX_AUDIO_MSG_LEN];
};

typedef struct {
	MsgHeadaudio head; /*protocol  head*/
	int iCheckNum;
	int iDatetime;
	int iSerialNum;
	byte byteFormat;
	int iDataLen;
	char buf[BUFSIZE];
}__attribute__ ((packed)) AudioData;

struct HandleAudioSendMsg {
	long int type;
	int cmd;
	char unUsed[4];
	int len;
	int audiosocketfd;
};

typedef struct _tag_socketfd_info_ {
	int lFd[MAX_SOCK_NUM];
	int lFdNum;
} socketfd_info_t;

/*******************************************************************************/
/************************ 全  局  变    量 ****************************************/
/******************************************************************************/
volatile int AUDIOSOCKETNUM=0;
volatile int AUDIO_SEND_OR_STOP_FLAG = -1;
//typedef unsigned char byte;
static socketfd_info_t g_stSocketFdInfo;
AudioData  audiodata;

/************************************************************************/
/*************************函  数  声   明*** *****************************/
/***********************************************************************/
/*audiosend线程初始化*/
int AudioSendThreadInit(struct AudioSendThreadMsg *s_msg);
/*audiosend创建*/
void *startaudio_send_thread();
/*audiosend线程同步*/
void AudioSendThreadSync();
/*audiosend线程消息处理*/
int AudioSend_msgHandler(struct AudioSendThreadMsg *t_audiosendthreaddata);
/*保存socket*/
int AudioSend_addSockFd(int *list, int len, int fd);
/*删除socket*/
int AudioSend_rmSockFd(int *list, int len, int fd);
/*audiosend协议设置*/
int AudioSetMsgHead(char cCmdVersion, short int nCmd, MsgHeadaudio *pHead,
		int iMsgLen);
/*audiosend发送*/
int AudioMsgBody(int iSocket, void* pDataBody, int iBodyLen, int iTimeOut);		

/**********************************************************************************/
/****************************函  数  接  口*****************************************/
/********************************************************************************/
/*  \fn        int Creataudio_send_thread()
 *  \brief     创建aduiosend线程
 *
 *  \return
 *  \b  历史记录
 *   <author>    gaoyuan
 */
int Creataudio_send_thread() {
	int num;
	num = thread_start(&startaudio_send_thread);
	if (num < 0) {
		DEBUGPRINT(DEBUG_ERROR,"create  audio send thread Fail\n");
		return -1;
	} else {
//		DEBUGPRINT(DEBUG_INFO,"create  audio send thread  Start  success\n");

	}
	return 0;
}

/*  \fn        int AudioSetMsgHead(char cCmdVersion, short int nCmd, MsgHeadaudio *pHead,int iMsgLen)
 *  \brief    aduiosend协议头设置
 *
 *  \return
 *  \b  历史记录
 *   <author>    gaoyuan
 */
int AudioSetMsgHead(char cCmdVersion, short int nCmd, MsgHeadaudio *pHead,
		int iMsgLen) {
	if (NULL == pHead) {
		return -1;
	}
	memset(pHead, 0, sizeof(MsgHeadaudio));
	memcpy(pHead->strHead, HEADER_WORD, sizeof(pHead->strHead));
	pHead->cVersion = cCmdVersion;
	pHead->nCmd = nCmd;

	return 0;
}

/*  \fn        int AudioMsgBody(int iSocket, void* pDataBody, int iBodyLen, int iTimeOut)
 *  \brief    aduiosend发送
 *
 *  \return
 *  \b  历史记录
 *   <author>    gaoyuan
 */
int AudioMsgBody(int iSocket, void* pDataBody, int iBodyLen, int iTimeOut) {
	int iBytesLeft;
	char *pTemp;

	if (NULL == pDataBody) {
		DEBUGPRINT(DEBUG_INFO, "WriteMsgBody with NULL body\n");
		return -1;
	}

	pTemp = pDataBody;
	iBytesLeft = iBodyLen;

	struct sockaddr addr;
	socklen_t addrlength;
//	DEBUGPRINT(DEBUG_INFO, "yuan\n");
#if 0
	if(-1 == getpeername(iSocket, &addr, &addrlength)) {
		DEBUGPRINT(DEBUG_INFO, "error fd %s\n", strerror(errno));
		return -1;
	}
#endif
//	return send(iSocket, pDataBody, iBodyLen, 0);
	return write(iSocket, pDataBody, iBodyLen);
}

/*************************************************************/
/******************** audiosend发送函数**********************/
/************************************************************/
void *startaudio_send_thread() {
	/*init  thread  id struct*/
	DEBUGPRINT(DEBUG_INFO,"Come Start Video send thread !\n");

	int lastIndex = -1;
	char *data = NULL;

	int iFrameIndex = -1;
	int iRet = -1;
	int i;

	struct AudioSendThreadMsg *audiosendthreaddata;
	audiosendthreaddata = (struct AudioSendThreadMsg *) malloc(
			sizeof(struct AudioSendThreadMsg));
	if (audiosendthreaddata == NULL ) {
		return NULL ;DEBUGPRINT(DEBUG_INFO,"AudioSend Malloc Failed!");
	}
	memset(audiosendthreaddata, 0, sizeof(struct AudioSendThreadMsg));

	//Audio Send Thread Init
	if (AudioSendThreadInit(audiosendthreaddata) != 0) {
		//Init Failed
		DEBUGPRINT(DEBUG_INFO,"Audio Send Init Failed!");
	}

//	adpcmFileFDabc = fopen("/mnt/SendadPcm.adpcm","w");
//	if(adpcmFileFDabc == NULL){
//		DEBUGPRINT(DEBUG_INFO,"Aduio Create File Failed2222222222222222222\n");
//		return NULL;
//	}


//	DEBUGPRINT(DEBUG_INFO,"AudioSend init success\n");
//Audio Send Thread sync
	AudioSendThreadSync();
	usleep(2000000);
	
	//struct timeval tm0, tm1;
	//gettimeofday(&tm0, NULL);
	while (1) {
		//gettimeofday(&tm1, NULL);
		//audiosend消息处理
//		DEBUGPRINT(DEBUG_INFO, "++++++++keep if you want keep (audio)++++++\n");
		AudioSend_msgHandler(audiosendthreaddata);
		child_setThreadStatus(AUDIO_SEND_MSG_TYPE, NULL);

		//发送标志
		if (AUDIO_SEND_OR_STOP_FLAG == 1)
		{
			if(AUDIOSOCKETNUM > 0)
			{
				 //取音频帧
				iRet = GetAudioFrameWithWantedIndex(&data, iFrameIndex);
				
				if (iRet == -3) {
						iFrameIndex = 0;
						usleep(30000);
						continue;
				} else if (iRet == -2) {
						usleep(30000);
					    continue;
				} else if (iRet == -1) {
						usleep(30000);
						continue;
				} else if ((0 <= iRet)  && (iRet < AUDIO_BUFFER_SIZE-1)) {
						iFrameIndex++;
				} else if ((AUDIO_BUFFER_SIZE-1 <= iRet) && (iRet < AUDIO_MAX_INDEX)) {
						iFrameIndex += iRet;
						usleep(30000);
						continue;
				}
				if(data == NULL)
				{
					 continue;
				}
				//DEBUGPRINT(DEBUG_INFO,"iFrameIndex=%d\n",iFrameIndex);
				if(data == NULL)
				{
					continue;
				}
#if 0
				if (iFrameIndex == lastIndex)
				{
						continue;
				}
#endif
//				if (iFrameIndex != lastIndex)
//				{
#if 0
					char *text = *szAudioBuf + lastIndex * 200;
					*(int *) (text + 23 + 4 + 4 + 4 + 1) = 160;
					*(int *) (text + 23 + 4 +4)=iFrameIndex;
#endif
					AudioSetMsgHead(CMD_VERSION, AUDIO_DATA_SEND, &audiodata.head,
									sizeof(AudioData) - sizeof(MsgHeadaudio) + AUDIO_DATA_LEN);
					audiodata.head.iDataLen = AUDIO_DATA_LEN + 40 - 23;
					audiodata.iDataLen = AUDIO_DATA_LEN;
					audiodata.iSerialNum=iFrameIndex;

					memcpy(audiodata.buf, data+40, AUDIO_DATA_LEN);

//					if(adpcmFileFDabc != NULL){
//						int iRet = fwrite(data+40,160,1,adpcmFileFDabc);
//						if(iRet<0){
//							DEBUGPRINT(DEBUG_INFO,"write adpcm Failed\n");
//						}
//					}

					//循环向socket发送数据
					for(i = 0; i < MAX_SOCK_NUM; i++)
					{
						if(NewsChannelSelf->usrChannel[i].fd != -1 &&
								NewsChannelSelf->usrChannel[i].type == AUDIO_CHANNEL &&
								NewsChannelSelf->usrChannel[i].direction == SOCK_DIRECTION_OUT) {
							//if(tm1.tv_sec * 1000000 + tm1.tv_usec - tm0.tv_sec * 1000000 - tm0.tv_usec > 6000000) {
								//gettimeofday(&tm0, NULL);
//								system("date > /tmp/audioInfo");
								//DEBUGPRINT(DEBUG_INFO, "print time to file\n");
							//}
//							DEBUGPRINT(DEBUG_INFO, "++++++tick a red head++++++ before send (sleep)\n");
							int retval = AudioMsgBody(NewsChannelSelf->usrChannel[i].fd,&audiodata,23 + 17 + AUDIO_DATA_LEN,DEFAULT_TIMEOUT);
#if 0
							if(-1 == NewsChannel_sockError(retval)) {
								NewsChannelSelf->usrChannel[i].direction = SOCK_DIRECTION_NULL;
								AUDIOSOCKETNUM--;
//								continue;
							}
#else
							//if(0 > retval) {
							if(-1 == NewsChannel_sockError(retval)){
								NewsChannelSelf->usrChannel[i].direction = SOCK_DIRECTION_NULL;
								AUDIOSOCKETNUM--;
//								continue;
							}
#endif
//							if (-1== AudioMsgBody(NewsChannelSelf->usrChannel[i].fd,&audiodata,23 + 17 + AUDIO_DATA_LEN,DEFAULT_TIMEOUT))
//							{
////								g_stSocketFdInfo.lFd[i] = NULL_FD;
////								AUDIOSOCKETNUM--;
//								NewsChannelSelf->usrChannel[i].direction = SOCK_DIRECTION_NULL;
//								continue;
//							}
//							DEBUGPRINT(DEBUG_INFO, "++++++++kissed by a green girl+++++ after send (blue)\n");
						}
//						if(g_stSocketFdInfo.lFd[i] != NULL_FD){
//							if (-1== AudioMsgBody(g_stSocketFdInfo.lFd[i],&audiodata,23 + 17 + AUDIO_DATA_LEN,DEFAULT_TIMEOUT))
//							{
//												g_stSocketFdInfo.lFd[i] = NULL_FD;
//												AUDIOSOCKETNUM--;
//												continue;
//							}
//						}
					}
//					lastIndex = iFrameIndex;
//				}
                usleep(20000);
                continue;
			}
		}
		//停止标志
		if (AUDIO_SEND_OR_STOP_FLAG == 0)
		{

		}

		usleep(200000);
	}
	return NULL ;

}

/*  \fn       int AudioSendThreadInit(struct AudioSendThreadMsg *t_audiosendthreaddata)
 *  \brief   audiosend线程初始化
 *
 *  \return
 *  \b  历史记录
 *   <author>    gaoyuan
 */
int AudioSendThreadInit(struct AudioSendThreadMsg *t_audiosendthreaddata) {
//	audioSendThreadState = AUDIO_SEND_STARTING;
//	int i;
//	int AUDIO_SEND_FLAG=-1;
//	int AUDIO_STOP_FLAG=-1;
	memset(&g_stSocketFdInfo, 0x0, sizeof(g_stSocketFdInfo));
#if 0
	for (i = 0; i < AUDIO_BUFFER_SIZE; i++) {
		AudioData* pAudioData = (AudioData*) szAudioBuf[i];

		AudioSetMsgHead(CMD_VERSION, AUDIO_DATA_SEND, &(pAudioData->head),
				sizeof(AudioData) - sizeof(MsgHeadaudio) + AUDIO_DATA_LEN);
		pAudioData->head.iDataLen = AUDIO_DATA_LEN + 40 - 23;
		pAudioData->iDataLen = AUDIO_DATA_LEN;
		//		pAudioData->byteFormat=1;

//			pAudioData->

		printf("i=%d\n", i);
	}
#endif
	t_audiosendthreaddata->childProcessMsgID = msg_queue_get(
			CHILD_PROCESS_MSG_KEY);
	t_audiosendthreaddata->audioSendThreadMsgID = msg_queue_get(
			CHILD_THREAD_MSG_KEY);
//	printf("%d\n",t_audiosendthreaddata->childProcessMsgID);
//	printf("%d\n",t_audiosendthreaddata->audioSendThreadMsgID);
	if (t_audiosendthreaddata->childProcessMsgID == -1) {
		return -1;
	}
	if (t_audiosendthreaddata->audioSendThreadMsgID == -1) {
		return -1;
	}
	return 0;
}

/*  \fn      void AudioSendThreadSync() 
 *  \brief   audiosend线程同步
 *
 *  \return
 *  \b  历史记录
 *   <author>    gaoyuan
 */
void AudioSendThreadSync() {
	rAudioSend = 1;
	while (rChildProcess != 1) {
		thread_usleep(0);
	}
	thread_usleep(0);
}

/*  \fn     int AudioSend_addSockFd(int *list, int len, int fd)
 *  \brief  保存socket
 *
 *  \return
 *  \b  历史记录
 *   <author>    gaoyuan
 */
int AudioSend_addSockFd(int *list, int len, int fd) {
	int *head = list;
	int *tail = list + len;
	int i = 0;
	for (; head != tail; ++head, ++i) {
		if (*head == NULL_FD) {
			*head = fd;
			return i;
		}
	}
	return -1;
}

/*  \fn       AudioSend_rmSockFd(int *list, int len, int fd)
 *  \brief    删除socket
 *
 *  \return
 *  \b  历史记录
 *   <author>    gaoyuan
 */
int AudioSend_rmSockFd(int *list, int len, int fd) {
	int *head = list;
	int *tail = list + len;
	int i = 0;
	for (; head != tail; ++head, ++i) {
		if (*head == fd) {
			*head = NULL_FD;
			return i;
		}
	}
	return -1;
}

/*  \fn      int AudioSend_msgHandler(struct AudioSendThreadMsg *t_audiosendthreaddata) 
 *  \brief   audiosend消息处理
 *
 *  \return
 *  \b  历史记录
 *   <author>    gaoyuan
 */
int AudioSend_msgHandler(struct AudioSendThreadMsg *t_audiosendthreaddata) {
	int i;
	int lFlag = 0;

	struct HandleAudioSendMsg *handaudiorecv =
			(struct HandleAudioSendMsg *) t_audiosendthreaddata->AudioMsgRecv;
	struct HandleAudioSendMsg *handaudiosend =
			(struct HandleAudioSendMsg *) t_audiosendthreaddata->AudioMsgSend;
	if (msg_queue_rcv(t_audiosendthreaddata->audioSendThreadMsgID,
			handaudiorecv, MAX_AUDIO_MSG_LEN, AUDIO_SEND_MSG_TYPE) == -1) {

	} else {
		switch (handaudiorecv->cmd) {
		case MSG_AUDIOSEND_P_AUDIO_SEND:
//			DEBUGPRINT(DEBUG_INFO,"==========cmd  start send======\n");
//			DEBUGPRINT(DEBUG_INFO, "handaudiorecv->audiosocketfd: %d\n", handaudiorecv->audiosocketfd);

			if (-1
					== NewsUtils_addSockFd(g_stSocketFdInfo.lFd, MAX_SOCK_NUM,
							handaudiorecv->audiosocketfd)) {
				DEBUGPRINT(DEBUG_ERROR, "cannot add socket to audio send module\n");
			}else{
				AUDIOSOCKETNUM++;
			}
#if 0
			for(i=0;i<g_stSocketFdInfo.lFdNum;i++)
			{
				if(g_stSocketFdInfo.lFd[i]==handaudiorecv->audiosocketfd)
				{
					lFlag = 1;
					break;
				}
			}
			if(0 == lFlag)
			{
				g_stSocketFdInfo.lFd[g_stSocketFdInfo.lFdNum]=handaudiorecv->audiosocketfd;
				g_stSocketFdInfo.lFdNum++;
			}
			//										DEBUGPRINT(DEBUG_INFO,"==========cmd  start send======\n");
#endif

			break;


		case MSG_AUDIOSEND_T_AUDIO_ONE_STOP:
			if (-1
					== NewsUtils_rmSockFd(g_stSocketFdInfo.lFd, MAX_SOCK_NUM,
							handaudiorecv->audiosocketfd)) {
				DEBUGPRINT(DEBUG_INFO, "audio send remove socket fail\n");
			}else{
				AUDIOSOCKETNUM--;
			}
  //          DEBUGPRINT(DEBUG_INFO, "handaudiorecv->audiosocketfd: %d\n", handaudiorecv->audiosocketfd);
			break;
		case CMD_AV_STARTED:

			AUDIO_SEND_OR_STOP_FLAG=1;

			break;
		case CMD_STOP_ALL_USE_FRAME:
			memset(g_stSocketFdInfo.lFd, 0, 12);
			AUDIO_SEND_OR_STOP_FLAG= 0;
			AUDIOSOCKETNUM=0;
			g_lAudioSend = TRUE;


			break;
		default:
//			DEBUGPRINT(DEBUG_INFO,"==========cmd  error======\n");
			break;
		}

	}


	return 0;

}

