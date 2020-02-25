#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>
//#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
//#include <sys/mman.h>
//#include <sys/ioctl.h>
#include "common/common.h"
#include "common/logfile.h"
#include "common/thread.h"
#include "common/msg_queue.h"
#include "Video/video.h"
#include "common/pdatatype.h"
#include "VideoSend/VideoSend.h"
#include "NewsChannel/NewsUtils.h"
#include "NewsChannel/NewsChannel.h"

/************************************************************************/
/*************************结  构  体  定   义 ******************************/
/***********************************************************************/
#define MAX_VIDEO_MSG_LEN  512
#define SOCKETFD_MXA  5
#define INVALID_SOCKET_ID  0
//extern volatile char szAudioBuf[AUDIO_BUFFER_SIZE][40+160];
#define CHECKNUM_FIELD 			int iCheckNum;//common
typedef unsigned char byte;     //common
#define VIDEO_DATA_SEND			400     //common
#define DEFAULT_TIMEOUT 	0  //common
#define MAX_SOCK_NUM 12
#define NULL_FD 0

struct VideoSendThreadMsg {
	int videoSendThreadMsgID;
	int childProcessMsgID;
//	int iBufferSize;
	char VideoMsgRecv[MAX_VIDEO_MSG_LEN];
	char VideoMsgSend[MAX_VIDEO_MSG_LEN];
//char *settingError;
};

/**视频数据包协议结构体**/
typedef struct {
	MsgHead head;
	int iDatetime;
	byte byteFormat;
	int iDataLen;
	char buf[100 * 1024];
}__attribute__ ((packed)) VideoData;

struct HandleVideoSendMsg {
	long int type;
	int cmd;
	char unUsed[4];
	int len;
	int videosocketfd;
//	unsigned char data[AUDIO_CAPTURE_MSG_LEN-16];
};

/**socket保存结构体**/
typedef struct _tag_socketfd_info_ {
	int lFd[MAX_SOCK_NUM];
	int lFdNum;
} socketfd_info_t;

/*******************************************************************************/
/************************ 全  局  变   量  **************************************/
/******************************************************************************/
volatile int VIDEOSOCKETNUM = 0;
volatile int VIDEO_SEND_OR_STOP_FLAG = -1;
//volatile int VIDEO_STOP_FLAG = -1;
//volatile int VIDEO_OTHER_STOP_FLAG = -1;
VideoData videoData;
static socketfd_info_t g_stSocketFdInfo;
volatile int videosocketfd[SOCKETFD_MXA] = { INVALID_SOCKET_ID };

typedef struct WifiInfo_t {
    char info[9];
    int time;
}WifiInfo_t;

WifiInfo_t mWifiInfo;

/************************************************************************/
/*************************函  数  声  明**********************************/
/***********************************************************************/
/*videosend线程初始化*/
int VideoSendThreadInit(struct VideoSendThreadMsg *s_msg);
/*启动videosend线程*/
void *startvideo_send_thread();
/*videosend线程同步*/
void VideoSendThreadSync();
/*videosend消息处理*/
int VideoSend_msgHandler(struct VideoSendThreadMsg *t_videosendthreaddata);
/*videosend协议头设置*/
int VideoSetMsgHead(char cCmdVersion, short int nCmd, MsgHead * pHead,
		int iMsgLen);
/*videosend数据发送*/
int VideoMsgBody(int iSocket, void* pDataBody, int iBodyLen, int iTimeOut);
/*创建videosend线程*/
int Creatvideo_send_thread();

/**********************************************************************************/
/****************************函  数  接  口*****************************************/
/********************************************************************************/
/*  \fn        int Creatvideo_send_thread()
 *  \brief    创建videosend线程函数
 *
 *  \return
 *  \b  历史记录：
 *   <author>    gaoyuan
 */
int Creatvideo_send_thread() {
	int num;

	num = thread_start(&startvideo_send_thread);
	if (num < 0) {
		DEBUGPRINT(DEBUG_ERROR,"creat  video send Faile\n");
		return -1;
	} else {
		DEBUGPRINT(DEBUG_INFO,"creat  video send thread  Start  success\n");

	}
	return 0;
}

/*  \fn       int VideoSetMsgHead(char cCmdVersion, short int nCmd, MsgHead *pHead,int iMsgLen)
 *  \brief    videosend数据协议头设置
 *
 *  \return
 *  \b  历史记录：
 *   <author>    gaoyuan
 */
int VideoSetMsgHead(char cCmdVersion, short int nCmd, MsgHead *pHead,
		int iMsgLen) {
	if (NULL == pHead) {
		return -1;
	}
	memset(pHead, 0, sizeof(MsgHead));
	memcpy(pHead->strHead, HEADER_WORD, sizeof(HEADER_WORD));
//	printf("file:%s,line:%d\n",__FILE__,__LINE__);
	pHead->cVersion = cCmdVersion;
	pHead->nCmd = nCmd;

	return 0;
}

/*  \fn       int VideoMsgBody(int iSocket, void* pDataBody, int iBodyLen, int iTimeOut)
 *  \brief    videosend数据发送
 *
 *  \return
 *  \b  历史记录：
 *   <author>    gaoyuan
 */
int VideoMsgBody(int iSocket, void* pDataBody, int iBodyLen, int iTimeOut) {

//	DEBUGPRINT(DEBUG_INFO, "%X\n", *(int *)pDataBody);
	int iBytesLeft;
	char *pTemp;

	if (NULL == pDataBody) {
		DEBUGPRINT(DEBUG_INFO, "WriteMsgBody with NULL body\n");
		return -1;
	}
	struct sockaddr addr;
	socklen_t addrlength;
	pTemp = pDataBody;
	iBytesLeft = iBodyLen;
//	DEBUGPRINT(DEBUG_INFO, "yuan\n");
#if 0
	if(-1 == getpeername(iSocket, &addr, &addrlength)) {
		DEBUGPRINT(DEBUG_INFO, "error fd %s\n", strerror(errno));
		return -1;
	}
#endif
//	DEBUGPRINT(DEBUG_INFO, "+++++++where can i find my peer++++++++\n");
//	return send(iSocket, pDataBody, iBodyLen, 0);
	return write(iSocket, pDataBody, iBodyLen);
	//return 0;
}

/*************************************************************/
/******************** videosend发送函数**********************/
/************************************************************/
void *startvideo_send_thread() {
	/*init  thread  id struct*/
	DEBUGPRINT(DEBUG_INFO,"Come Start Video send thread !\n");

	int iRet = 0;
	int iCurVideoIndex = 0;
	int iDataLen = 0;
	int iLastVideoIndex = 0;
	char* pVideoData = NULL;
//	time_t tmBefore, tmAfter;
//		int count=0;
	time_t tm = time(NULL);
	int i;

	struct VideoSendThreadMsg *videosendthreaddata;
	videosendthreaddata = (struct videoSendThreadMsg *) malloc(
			sizeof(struct VideoSendThreadMsg));
	if (videosendthreaddata == NULL ) {
		return NULL ; DEBUGPRINT(DEBUG_INFO,"VideoSend Malloc Failed!");
	}
	memset(videosendthreaddata, 0, sizeof(struct VideoSendThreadMsg));

	//Audio Send Thread Init
	if (VideoSendThreadInit(videosendthreaddata) != 0) {
		//Init Failed
		DEBUGPRINT(DEBUG_INFO,"Video Send Init Failed!");
	}
//	DEBUGPRINT(DEBUG_INFO,"VideoSend init success\n");
//Video Send Thread sync
	VideoSendThreadSync();
	usleep(200000);

    //    int usleepTime = 200000;
	//struct timeval tm0, tm1;
	//gettimeofday(&tm0, NULL);
    mWifiInfo.time = 20000;
    int videoNum = 0;
	while (1) {
		//gettimeofday(&tm1, NULL);
		//videosend消息处理
		VideoSend_msgHandler(videosendthreaddata);
		child_setThreadStatus(VIDEO_SEND_MSG_TYPE, NULL);
//		DEBUGPRINT(DEBUG_INFO, "+++++keep keep and keep+++++\n");
		//发送标志
		if (VIDEO_SEND_OR_STOP_FLAG == 1)
		{
			if(VIDEOSOCKETNUM > 0)
			{
				//取视频帧
				if (-1== GetNewestVideoFrame(&pVideoData, &iDataLen,&iCurVideoIndex))
				{
							DEBUGPRINT(DEBUG_INFO, "GetNewestVideoFrame failed, in file:%s line:%d\n", __FILE__, __LINE__);
							continue;
				}
#ifdef _MUIT_FRAME_SPEED
                get_wifi_signal_level(mWifiInfo.info);
                
                if(*(mWifiInfo.info + 7) > -65) {
                    mWifiInfo.time = 20000;
                }
                else if(*(mWifiInfo.info + 7) > -75) {
                    mWifiInfo.time = 200000;
                }
                else {
                    mWifiInfo.time = 2000000;
                }
#endif
				//设置协议
				iLastVideoIndex = iCurVideoIndex;
				VideoSetMsgHead(CMD_VERSION, VIDEO_DATA_SEND, &videoData.head, 0);
				videoData.head.iDataLen = iDataLen + 9;
				videoData.iDataLen = iDataLen;
				videoData.byteFormat = 1;
				if (videoData.head.iDataLen < 0)
				{
						DEBUGPRINT(DEBUG_INFO, "videoData.head.iDataLen < 0\n");
						ReleaseNewestVideoFrame(iCurVideoIndex);
						continue;
					} else if (videoData.head.iDataLen + 23 >= 100 * 1024){
//										DEBUGPRINT(DEBUG_INFO, "videoData.head.iDataLen + 23 >= 70 * 1024\n");
						ReleaseNewestVideoFrame(iCurVideoIndex);
						continue;
					}
				memcpy(videoData.buf, pVideoData, iDataLen);
				//循环向socket发送数据
				for(i = 0;i < MAX_SOCK_NUM;i++)
				{
#if 0
					if(g_stSocketFdInfo.lFd[i] != NULL_FD){
//						DEBUGPRINT(DEBUG_INFO, "+++++++++hit dog's leg++++++ before send %d\n", i);
						//if(tm1.tv_sec * 1000000 + tm1.tv_usec - tm0.tv_sec * 1000000 - tm0.tv_usec > 6000000) {
						//	gettimeofday(&tm0, NULL);
//							system("date > /tmp/videoInfo");
						//	DEBUGPRINT(DEBUG_INFO, "print time to file\n");
						//}
						iRet = VideoMsgBody(g_stSocketFdInfo.lFd[i], &videoData,videoData.iDataLen + 23 + 9, DEFAULT_TIMEOUT);
//						DEBUGPRINT(DEBUG_INFO, "++++++++pick my mind+++++++ after send %d\n", i);
						//if ((videoData.iDataLen + 23 + 9) > iRet)
						//{
							if(-1 == NewsChannel_sockError(iRet)) {
								DEBUGPRINT(DEBUG_ERROR, "WriteMsgBody pVideoData Faile:%d, error:%s, in line:%d file:%s\n",
																	iRet, strerror(errno), __LINE__, __FILE__);
	//							ReleaseNewestVideoFrame(iCurVideoIndex);
								g_stSocketFdInfo.lFd[i] = NULL_FD;
								VIDEOSOCKETNUM--;
							}
//							continue;
//						}
					}
#else
					if(NewsChannelSelf->usrChannel[i].fd != -1 && NewsChannelSelf->usrChannel[i].type == VIDEO_CHANNEL && NewsChannelSelf->usrChannel[i].direction == SOCK_DIRECTION_OUT) {
//						if(tm1.tv_sec * 1000000 + tm1.tv_usec - tm0.tv_sec * 1000000 - tm0.tv_usec > 6000000) {
//							gettimeofday(&tm0, NULL);
//							system("date > /tmp/videoInfo");
//							DEBUGPRINT(DEBUG_INFO, "print time to file\n");
//						}
						iRet = VideoMsgBody(NewsChannelSelf->usrChannel[i].fd, &videoData,videoData.iDataLen + 23 + 9, DEFAULT_TIMEOUT);
						if (-1== iRet)
						{
	//								g_stSocketFdInfo.lFd[i] = NULL_FD;
//							VIDEOSOCKETNUM--;
							continue;
						}
						++videoNum;
						if(time(NULL) - 30 >= tm) {
							time(&tm);
							LOG_TEST("startvideo_send_thread: VideoMsgBody %d\n", videoNum);
							videoNum = 0;
						}
					}
#endif
				}
				if (-1 == ReleaseNewestVideoFrame(iCurVideoIndex))
				{
										//break;
				}
                usleep(mWifiInfo.time);
                continue;
			}
            usleep(200000);
            continue;
		}
		//停止标志
		if (VIDEO_SEND_OR_STOP_FLAG == 0) {

		}

		usleep(200000);
	}
	return NULL ;

}

/*  \fn        int VideoSendThreadInit(struct VideoSendThreadMsg *t_videosendthreaddata)
 *  \brief    videosend线程初始化
 *
 *  \return
 *  \b  历史记录：
 *   <author>    gaoyuan
 */
int VideoSendThreadInit(struct VideoSendThreadMsg *t_videosendthreaddata) {
	//videoSendThreadState = VIDEO_SEND_STARTING;

//	int AUDIO_SEND_FLAG=-1;
//	int AUDIO_STOP_FLAG=-1;
	memset(&g_stSocketFdInfo, 0x0, sizeof(g_stSocketFdInfo));

	t_videosendthreaddata->childProcessMsgID = msg_queue_get(
			CHILD_PROCESS_MSG_KEY);
	t_videosendthreaddata->videoSendThreadMsgID = msg_queue_get(
			CHILD_THREAD_MSG_KEY);
//	printf("%d\n", t_videosendthreaddata->childProcessMsgID);
//	printf("%d\n", t_videosendthreaddata->videoSendThreadMsgID);
	if (t_videosendthreaddata->childProcessMsgID == -1) {
		return -1;
	}
	if (t_videosendthreaddata->videoSendThreadMsgID == -1) {
		return -1;
	}
	return 0;
}

/*  \fn      void VideoSendThreadSync()
 *  \brief    videosend线程同步
 *
 *  \return
 *  \b  历史记录：
 *   <author>    gaoyuan
 */
void VideoSendThreadSync() {
	rVideoSend = 1;
	while (rChildProcess != 1) {
		thread_usleep(0);
	}
	thread_usleep(0);
}

/*  \fn       int VideoSend_msgHandler(struct VideoSendThreadMsg *t_videosendthreaddata)
 *  \brief    videosend线程消息处理
 *
 *  \return
 *  \b  历史记录：
 *   <author>    gaoyuan
 */
int VideoSend_msgHandler(struct VideoSendThreadMsg *t_videosendthreaddata) {
//	int i;
//	int lFlag = 0;
	struct HandleVideoSendMsg *handvideorecv =
			(struct HandleVideoSendMsg *) t_videosendthreaddata->VideoMsgRecv;
//	struct HandleVideoSendMsg *handvideosend=(struct HandleVideoSendMsg *)t_videosendthreaddata->VideoMsgSend;

	if (msg_queue_rcv(t_videosendthreaddata->videoSendThreadMsgID,
			handvideorecv, MAX_VIDEO_MSG_LEN, VIDEO_SEND_MSG_TYPE) == -1) {

	} else {

		switch (handvideorecv->cmd) {
		case MSG_VIDEOSEND_P_VIDEO_SEND:
//			DEBUGPRINT(DEBUG_INFO, "handvideorecv->videosocketfd: %d\n", handvideorecv->videosocketfd);
	//		VIDEO_SEND_FLAG = 1;
			if (-1
					== NewsUtils_addSockFd(g_stSocketFdInfo.lFd, MAX_SOCK_NUM,
							handvideorecv->videosocketfd)) {
				DEBUGPRINT(DEBUG_ERROR, "cannot add socket to video send module\n");
			}else
			{
				VIDEOSOCKETNUM++;
			}
#if 0
			/*save socket*/
			for(i=0;i<g_stSocketFdInfo.lFdNum;i++)
			{
				if(g_stSocketFdInfo.lFd[i]==handvideorecv->videosocketfd)
				{
					lFlag = 1;
					break;
				}
			}
			if(0 == lFlag)
			{
				g_stSocketFdInfo.lFd[g_stSocketFdInfo.lFdNum]=handvideorecv->videosocketfd;
				g_stSocketFdInfo.lFdNum++;
			}

			DEBUGPRINT(DEBUG_INFO,"==========cmd  start send======\n");
//											handvideosend->type=VIDEO_SEND_MSG_TYPE;
			//										handvideosend->cmd=MSG_VIDEOSEND_P_VIDEO_SEND;
			//										msgSnd->len = ;
			//									msg_queue_snd(t_videosendthreaddata->childProcessMsgID, handvideosend, sizeof(struct VideoSendThreadMsg *) - sizeof(long));
#endif
			break;

		case CMD_AV_STARTED:
			VIDEO_SEND_OR_STOP_FLAG = 1;

			break;
		case CMD_STOP_ALL_USE_FRAME:
			memset(g_stSocketFdInfo.lFd, 0, 12);

			VIDEO_SEND_OR_STOP_FLAG =0;
			VIDEOSOCKETNUM=0;
			g_lVideoSend = TRUE;

	//		DEBUGPRINT(DEBUG_INFO,"==========cmd  other stop send======\n");

			break;
		default:
	//		DEBUGPRINT(DEBUG_INFO,"==========cmd  error======\n");
			break;
		}

	}


	return 0;
}

