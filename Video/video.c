/*
 * video.c
 *
 *  Created on: Jan 7, 2013
 *      Author: root
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h> /* getopt_long() */
#include <fcntl.h> /* low-level i/o */
#include <unistd.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h> /* for videodev2.h */
#include <pthread.h>
#include <linux/videodev2.h>
#include <semaphore.h>
#include "video.h"
#include "common/common.h"
#include "common/logfile.h"
#include "common/thread.h"
#include "common/msg_queue.h"

#define CLEAR(x) memset (&(x), 0, sizeof (x))

#define REQUSET_BUFFER_SIZE 16

//Params List

struct VideoThreadMsg{
	int videoThreadMsgID;
	int childProcessMsgID;
	int iBufferSize;
	char videoMsgRecv[MAX_VIDEO_MSG_LEN];
	char videoMsgSend[MAX_VIDEO_MSG_LEN];
	char *settingError;
};

volatile int testIndex=0;
volatile int iVideoOpenTimes=0;
volatile int iVideoSleepTime=20000;
volatile int iVideoOpenIndex=0;
volatile int iSetVideoSize=2;
volatile int iVideoOpenFailedFlag;

extern volatile int iTestFlag;
//extern volatile sem_t AVSemaphore;
volatile int iVideoNeedRebootIndex = 0;

const char* pszDevName= "/dev/video0";

struct v4l2_buffer buf;

//Function List

/*
 * @brief 启动视频采集线程
 * @author:张历
 * @param[in]:<arg><传入线程的参数>
 * @param[out]:<无参数>
 * @Return <NULL>
 */
void *startVideoCaptureThread(void *arg);

/*
 * @brief 视频采集线程的参数初始化
 * @author:张历
 * @param[in]:<t_msg><线程参数的封装结构>
 * @param[out]:<无参数>
 * @Return <成功：0 ；失败：-1>
 */
int videoCaptureThreadInit(struct VideoThreadMsg *t_msg);

/*
 * @brief 视频采集线程的线程同步
 * @author:张历
 * @param[in]:<无参数>
 * @param[out]:<无参数>
 * @Return <无参数>
 */
void videoCaptureThreadSync();

/*
 * @brief 视频采集线程的消息处理函数
 * @author:张历
 * @param[in]:<t_msg><线程参数的封装结构>
 * @param[out]:<无参数>
 * @Return <无参数>
 */
void videoCaptureThreadHandleMsg(struct VideoThreadMsg *t_msg);

/*
 * @brief 开始打开视频文件
 * @author:张历
 * @param[in]:<t_msg><线程参数的封装结构>
 * @param[out]:<无参数>
 * @Return <成功：0 ；失败：-1>
 */
int DostartVideoCapture(struct VideoThreadMsg *msg);

/*
 * @brief 开始采集函数
 * @author:张历
 * @param[in]:<t_msg><线程参数的封装结构>
 * @param[out]:<无参数>
 * @Return <0：成功 ；-1：打开设备失败；-2:初始化设备失败;-3:内存映射失败;-4:缓存如队列失败;-5:开采集失败;>
 */
int startVideoCapture(struct VideoThreadMsg *t_msg);

/*
 * @brief 打开视频驱动
 * @author:张历
 * @param[in]:<pszDevName><设备名称>
 * @param[out]:<无参数>
 * @Return <0：成功 ；-1：失败;>
 */
int OpenDevice (const char* pszDevName);

/*
 * @brief 初始化图像采集设备
 * @author:张历
 * @param[in]:<t_msg><线程参数的封装结构>
 * @param[out]:<无参数>
 * @Return <0：成功 ；-1：失败;>
 */
int InitDevice (struct VideoThreadMsg *t_msg);

/*
 * @brief 设置图像分辨率
 * @author:张历
 * @param[in]:<iVideoSize><图像分辨率等级1：640*480 ; 2: 320*240 ; 3: 160*120>
 * @param[out]:<无参数>
 * @Return <0：成功 ；-1：失败;>
 */
int SetVideoSize(int iVideoSize);

/*
 * @brief 图像地址的内存映射
 * @author:张历
 * @param[in]:<t_msg><线程参数的封装结构>
 * @param[out]:<无参数>
 * @Return <0：成功 ；-1：失败;>
 */
int InitMmap (struct VideoThreadMsg *t_msg);

/*
 * @brief 申请的缓存入采集队列
 * @author:张历
 * @param[in]:<t_msg><线程参数的封装结构>
 * @param[out]:<无参数>
 * @Return <0：成功 ；-1：失败;>
 */
int PutBufIntoInqueue(struct VideoThreadMsg *t_msg);

/*
 * @brief 停止视频采集
 * @author:张历
 * @param[in]:<t_msg><线程参数的封装结构>
 * @param[out]:<无参数>
 * @Return <0：成功 ；-1：失败;>
 */
int stopVideoCapture(struct VideoThreadMsg *t_msg);

/*
 * @brief 设置视频参数
 * @author:张历
 * @param[in]:<type><设置参数的类型>
 * @param[in]:<iValue><设置参数的值>
 * @param[out]:<pszErrMsg><设置错误消息>
 * @Return <0：成功 ；-1：失败;>
 */
int SetVideoParam (EVideoParamType type, int iValue, char* pszErrMsg);

/*
 * @brief 设置视频参数
 * @author:张历
 * @param[in]:<type><设置参数的类型>
 * @param[in]:<iValue><设置参数的值>
 * @param[out]:<无参数>
 * @Return <0：成功 ；-1：失败;>
 */
int SetVideoParamControl(int type, int value);

/*
 * @brief 从新设置视频分辨率
 * @author:张历
 * @param[in]:<iValue><设置参数的值>
 * @param[out]:<无参数>
 * @Return <0：成功 ；-1：失败;>
 */
int ResetVideoSize(int iVideoSize);

/*
 * @brief 采集图像
 * @author:张历
 * @param[in]:<t_msg><线程参数的封装结构>
 * @param[out]:<无参数>
 * @Return <0：成功 ；-1：失败;>
 */
int CaptureOnePicture(struct VideoThreadMsg *t_msg);

/*
 * @brief 处理开始视频采集失败的函数
 * @author:张历
 * @param[in]:<t_msg><线程参数的封装结构>
 * @param[in]:<errorCode><错误类型>
 * @param[out]:<无参数>
 * @Return <0：成功 ；-1：失败;>
 */
int HandleOpenError(struct VideoThreadMsg *t_msg, int errorCode);

struct iThread VideoCaptureThread = {
		.run=startVideoCaptureThread
};

int CreateVideoCaptureThread(){
	DEBUGPRINT(DEBUG_INFO,"Come Create Video Capture Thread!\n");
//#if 0
//	startVideoCaptureThread(NULL);
//	return 0;
//#else
	return thread_start(&startVideoCaptureThread);
//#endif
}

//Tread Start
void *startVideoCaptureThread(void *arg){
	DEBUGPRINT(DEBUG_INFO,"Come Start Video Capture Thread! Main loop========================\n");
	struct VideoThreadMsg *msg =NULL;
	msg=(struct VideoThreadMsg *)malloc(sizeof(struct VideoThreadMsg));
	if(msg==NULL){
		return NULL;
		DEBUGPRINT(DEBUG_ERROR,"Video Malloc Failed!");
	}
	memset(msg, 0, sizeof(struct VideoThreadMsg));
	//Video Capture Thread Init
	if(0!=videoCaptureThreadInit(msg)){
		//Init Failed
		DEBUGPRINT(DEBUG_ERROR,"Video Init Failed!");
	}

	//Video Capture Thread Sync
	videoCaptureThreadSync();
	DEBUGPRINT(DEBUG_INFO,"After Sync!!\n");


	while(1){
		videoCaptureThreadHandleMsg(msg);
		child_setThreadStatus(VIDEO_MSG_TYPE, NULL);
		if(videoCaptureThreadState == VIDEO_OPENNING){
			DostartVideoCapture(msg);
		}
		if(videoCaptureThreadState==VIDEO_RUNNING){
//			if(sem_trywait(&AVSemaphore) == 0){
				CaptureOnePicture(msg);
//				sem_post(&AVSemaphore);
//			}
		}
		thread_usleep(iVideoSleepTime);
		iVideoSleepTime = 20000;
	}
	if(msg != NULL){
		free(msg);
		msg = NULL;
	}

	if(pFrameMutexs != NULL){
		free(pFrameMutexs);
		pFrameMutexs=NULL;
	}
	DEBUGPRINT (DEBUG_INFO, "close mutex===============3\n");
	if(pBuffers !=NULL)
	{
		DEBUGPRINT (DEBUG_INFO, "close pBuffer1===============4\n");
		free (pBuffers);
		DEBUGPRINT (DEBUG_INFO, "close pBuffer2===============4\n");
		pBuffers = NULL;
	}

	return NULL;
}

int videoCaptureThreadInit(struct VideoThreadMsg *t_msg){
	videoCaptureThreadState=VIDEO_STARTING;
	video_fd=-1;
	t_msg->childProcessMsgID=msg_queue_get(CHILD_PROCESS_MSG_KEY);
	t_msg->videoThreadMsgID=msg_queue_get(CHILD_THREAD_MSG_KEY);
	iNewestBufferIndex=-1;
	if(t_msg->childProcessMsgID==-1){
		return -1;
	}
	if(t_msg->videoThreadMsgID==-1){
			return -1;
	}

	pFrameMutexs = calloc(REQUSET_BUFFER_SIZE, sizeof(pthread_mutex_t));
	    if(!pFrameMutexs)
	{
	    DEBUGPRINT(DEBUG_ERROR, "No enough memory(pFrameMutexs:%d), in file:%s line:%d\n",
		t_msg->iBufferSize*sizeof(pthread_mutex_t), __FILE__, __LINE__);
		return -1;
	}
	 int i=0;
	 for(i=0;i<REQUSET_BUFFER_SIZE;i++){
	    pthread_mutex_init((pthread_mutex_t *)pFrameMutexs+i, NULL);
	 }
	DEBUGPRINT(DEBUG_INFO, "sizeof(*pBuffers):%d\n", sizeof (*pBuffers));
	pBuffers = calloc (REQUSET_BUFFER_SIZE, sizeof (*pBuffers));
	if (!pBuffers)
	{
		DEBUGPRINT(DEBUG_ERROR, "No enough memory(pBuffers:%d), in file:%s line:%d\n",
		t_msg->iBufferSize*sizeof(*pBuffers), __FILE__, __LINE__);
		return -1;
	}

	iVideoOpenFailedFlag = 0;
	return 0;
}

void videoCaptureThreadSync(){
	rVideo=1;
	while(rChildProcess != 1){
		thread_usleep(0);
	}
	thread_usleep(0);
}

void videoCaptureThreadHandleMsg(struct VideoThreadMsg *t_msg){
	if(msg_queue_rcv(t_msg->videoThreadMsgID,t_msg->videoMsgRecv,sizeof(struct HandleVideoMsg),VIDEO_MSG_TYPE) != -1){

		struct HandleVideoMsg *recvVideoMsg=(struct HandleVideoMsg *)t_msg->videoMsgRecv;
		struct HandleVideoMsg *sendVideoMsg=(struct HandleVideoMsg *)t_msg->videoMsgSend;
		DEBUGPRINT(DEBUG_INFO,"===Received Video Msg!!!=======%i\n",recvVideoMsg->cmd);
		sendVideoMsg->type=VIDEO_MSG_TYPE;
		switch(recvVideoMsg->cmd){
			case MSG_VIDEO_P_START_CAPTURE:
				//Start
				if((videoCaptureThreadState == VIDEO_STOP)|(videoCaptureThreadState == VIDEO_STARTING)|(videoCaptureThreadState == VIDEO_ERROR)){
					videoCaptureThreadState=VIDEO_OPENNING;
					iVideoOpenIndex = 0;
					iVideoOpenTimes = 0;
				}
				break;
			case MSG_VIDEO_P_STOP_CAPTURE:
				//Stop
				if(videoCaptureThreadState == VIDEO_RUNNING){
					printf("Before Video Stop\n");
//					sem_wait(&AVSemaphore);
					int iVideoStopRet = stopVideoCapture(t_msg);
//					sem_post(&AVSemaphore);
					if( iVideoStopRet < 0){
						//Capture Off Failed
						DEBUGPRINT(DEBUG_INFO,"Stop Video Capture Faile iVideoStopRet= %i\n",iVideoStopRet);
						videoCaptureThreadState=VIDEO_ERROR;
					}
					else{
						videoCaptureThreadState=VIDEO_STOP;
					}
					char bit0=iVideoStopRet;
					sendVideoMsg->cmd=MSG_VIDEO_P_STOP_CAPTURE;
					sendVideoMsg->data[0]=bit0;
					sendVideoMsg->len = 1;
					msg_queue_snd(t_msg->childProcessMsgID,sendVideoMsg,sizeof(MsgData)-sizeof(long int)+sendVideoMsg->len);
				}
				else if(videoCaptureThreadState == VIDEO_OPENNING){
					videoCaptureThreadState=VIDEO_STOP;
					sendVideoMsg->cmd=MSG_VIDEO_P_STOP_CAPTURE;
					sendVideoMsg->data[0]=0;
					sendVideoMsg->len = 1;
					msg_queue_snd(t_msg->childProcessMsgID,sendVideoMsg,sizeof(MsgData)-sizeof(long int)+sendVideoMsg->len);
				}
				else{
					//Can not Stop
					sendVideoMsg->cmd=MSG_VIDEO_P_STOP_CAPTURE;
					sendVideoMsg->data[0]=1;
					sendVideoMsg->len = 1;
					msg_queue_snd(t_msg->childProcessMsgID,sendVideoMsg,sizeof(MsgData)-sizeof(long int)+sendVideoMsg->len);
				}
				break;
			case MSG_VIDEO_P_SETTING_PARAMS:
				//Setting
				if(videoCaptureThreadState == VIDEO_RUNNING){
					//Setting Type
					videoCaptureThreadState = VIDEO_SETTING;
					int settingType; //= recvVideoMsg->data[0]+recvVideoMsg->data[1]*256+recvVideoMsg->data[2]*65536+recvVideoMsg->data[3]*16777216;
					int settingValue; //= recvVideoMsg->data[4]+recvVideoMsg->data[5]*256+recvVideoMsg->data[6]*65536+recvVideoMsg->data[7]*16777216;;
					int *a = (int *)(recvVideoMsg->data);
					settingType = *a;
					*a = *(a + 1);
					settingValue = *a;
					DEBUGPRINT(DEBUG_INFO,"Setting Type=====%i Setting Value===%i\n",settingType,settingValue);
					int iSetRet;
					char seting_bit0;
					switch (settingType){
						case FRAME_RATE:{
								iSetRet = SetVideoParam(FRAME_RATE, settingValue, t_msg->settingError);
								seting_bit0 = iSetRet;
								sendVideoMsg->cmd=MSG_VIDEO_P_SETTING_PARAMS;
								sendVideoMsg->data[0]=seting_bit0;
								sendVideoMsg->len = 1;
								msg_queue_snd(t_msg->childProcessMsgID,sendVideoMsg,sizeof(MsgData)-sizeof(long int)+sendVideoMsg->len);
						}
								break;
						case BRIGHT:{
									iSetRet = SetVideoParam(BRIGHT, settingValue, t_msg->settingError);
									seting_bit0 = iSetRet;
									sendVideoMsg->cmd=MSG_VIDEO_P_SETTING_PARAMS;
									sendVideoMsg->data[0]=seting_bit0;
									sendVideoMsg->len = 1;
									msg_queue_snd(t_msg->childProcessMsgID,sendVideoMsg,sizeof(MsgData)-sizeof(long int)+sendVideoMsg->len);
						}
								break;
						case CONTRAST:{
									iSetRet = SetVideoParam(CONTRAST, settingValue, t_msg->settingError);
									seting_bit0 = iSetRet;
									sendVideoMsg->cmd=MSG_VIDEO_P_SETTING_PARAMS;
									sendVideoMsg->data[0]=seting_bit0;
									sendVideoMsg->len = 1;
									msg_queue_snd(t_msg->childProcessMsgID,sendVideoMsg,sizeof(MsgData)-sizeof(long int)+sendVideoMsg->len);
						}
								break;
						case SIZE:{
									iSetRet = SetVideoParam(SIZE, settingValue, t_msg->settingError);
									seting_bit0 = iSetRet;
									sendVideoMsg->cmd=MSG_VIDEO_P_SETTING_PARAMS;
									sendVideoMsg->data[0]=seting_bit0;
									sendVideoMsg->len = 4;
									msg_queue_snd(t_msg->childProcessMsgID,sendVideoMsg,sizeof(MsgData)-sizeof(long int)+sendVideoMsg->len);
						}
								break;
						case HFLIP:{
									iSetRet = SetVideoParam(HFLIP, settingValue, t_msg->settingError);
									seting_bit0 = iSetRet;
									sendVideoMsg->cmd=MSG_VIDEO_P_SETTING_PARAMS;
									sendVideoMsg->data[0]=seting_bit0;
									sendVideoMsg->len = 1;
									msg_queue_snd(t_msg->childProcessMsgID,sendVideoMsg,sizeof(MsgData)-sizeof(long int)+sendVideoMsg->len);
						}
								break;
						case VFLIP:{
									iSetRet = SetVideoParam(VFLIP, settingValue, t_msg->settingError);
									seting_bit0 = iSetRet;
									sendVideoMsg->cmd=MSG_VIDEO_P_SETTING_PARAMS;
									sendVideoMsg->data[0]=seting_bit0;
									sendVideoMsg->len = 1;
									msg_queue_snd(t_msg->childProcessMsgID,sendVideoMsg,sizeof(MsgData)-sizeof(long int)+sendVideoMsg->len);
						}
								break;
						case SAMPLE_RATE:{
									iSetRet = SetVideoParam(SAMPLE_RATE, settingValue, t_msg->settingError);
									seting_bit0 = iSetRet;
									sendVideoMsg->cmd=MSG_VIDEO_P_SETTING_PARAMS;
									sendVideoMsg->data[0]=seting_bit0;
									sendVideoMsg->len = 1;
									msg_queue_snd(t_msg->childProcessMsgID,sendVideoMsg,sizeof(MsgData)-sizeof(long int)+sendVideoMsg->len);
						}
								break;
						case AUTO_WHITE_BALANCE:{
									iSetRet = SetVideoParam(AUTO_WHITE_BALANCE, settingValue, t_msg->settingError);
									seting_bit0 = iSetRet;
									sendVideoMsg->cmd=MSG_VIDEO_P_SETTING_PARAMS;
									sendVideoMsg->data[0]=seting_bit0;
									sendVideoMsg->len = 1;
									msg_queue_snd(t_msg->childProcessMsgID,sendVideoMsg,sizeof(MsgData)-sizeof(long int)+sendVideoMsg->len);
						}
								break;
						case RED_BALANCE:{
									iSetRet = SetVideoParam(RED_BALANCE, settingValue, t_msg->settingError);
									seting_bit0 = iSetRet;
									sendVideoMsg->cmd=MSG_VIDEO_P_SETTING_PARAMS;
									sendVideoMsg->data[0]=seting_bit0;
									sendVideoMsg->len = 1;
									msg_queue_snd(t_msg->childProcessMsgID,sendVideoMsg,sizeof(MsgData)-sizeof(long int)+sendVideoMsg->len);
						}
								break;
						case BLUE_BALANCE:{
									iSetRet = SetVideoParam(BLUE_BALANCE, settingValue, t_msg->settingError);
									seting_bit0 = iSetRet;
									sendVideoMsg->cmd=MSG_VIDEO_P_SETTING_PARAMS;
									sendVideoMsg->data[0]=seting_bit0;
									sendVideoMsg->len = 1;
									msg_queue_snd(t_msg->childProcessMsgID,sendVideoMsg,sizeof(MsgData)-sizeof(long int)+sendVideoMsg->len);
						}
								break;
						default :
							sendVideoMsg->cmd=MSG_VIDEO_P_SETTING_PARAMS;
							sendVideoMsg->data[0]=1;
							sendVideoMsg->len = 1;
							msg_queue_snd(t_msg->childProcessMsgID,sendVideoMsg,sizeof(MsgData)-sizeof(long int)+sendVideoMsg->len);
							break;
					}
					videoCaptureThreadState =VIDEO_RUNNING;
				}
				else{
					//can not set
					sendVideoMsg->cmd=2;
					sendVideoMsg->data[0]=1;
					sendVideoMsg->len = 1;
					msg_queue_snd(t_msg->childProcessMsgID,sendVideoMsg,sizeof(MsgData)-sizeof(long int)+sendVideoMsg->len);
				}

				break;
			default:
				//Unknown Command
				DEBUGPRINT(DEBUG_ERROR,"===============Unknown Command");
				break;
		}
		memset(t_msg->videoMsgRecv,0,sizeof(t_msg->videoMsgRecv));
		memset(t_msg->videoMsgSend,0,sizeof(t_msg->videoMsgSend));
		sendVideoMsg = NULL;
		recvVideoMsg = NULL;
	}

}

int DostartVideoCapture(struct VideoThreadMsg *msg){
	int iRet = 0;
	int iOpenTimeCount = iVideoOpenIndex%50;
	if(iOpenTimeCount == 0){
		iVideoOpenTimes++;
		DEBUGPRINT(DEBUG_INFO,"Before Open \n");
//		sem_wait(&AVSemaphore);
		iRet = startVideoCapture(msg);
//		sem_post(&AVSemaphore);
		if(iRet == 0){
			//Open Success
			iVideoSleepTime = 0;
			iVideoOpenIndex = 0;
			videoCaptureThreadState = VIDEO_RUNNING;
			iVideoNeedRebootIndex = 0;
		}
		else{
			DEBUGPRINT(DEBUG_INFO,"Video Open Times ====%d Failed ==iRet==%d\n",iVideoOpenTimes,iRet);
			HandleOpenError(msg, iRet);
		}
		struct HandleVideoMsg *sendMsg = msg->videoMsgSend;
		sendMsg->cmd = MSG_VIDEO_P_START_CAPTURE;
		sendMsg->len = 1;
		sendMsg->type = VIDEO_MSG_TYPE;
		sendMsg->data[0] = iRet;
		msg_queue_snd(msg->childProcessMsgID,sendMsg,sizeof(MsgData)-sizeof(long int)+sendMsg->len);
	}
	iVideoOpenIndex++;
	if(iVideoOpenIndex>99){
		iVideoOpenIndex = 0;
	}
	return 0;
}

int startVideoCapture(struct VideoThreadMsg *t_msg){
	///////////////////////////////////////////////////////////////////////////////////////
	//Open Device
	int iRet=-1;
	DEBUGPRINT(DEBUG_INFO,"111111111111111Open Video Device Fd======\n");
//	FILE *videoFd = popen("ls -l /dev/video0","r");
//	if(videoFd != NULL){
//		char string[256];
////		fscanf(videoFd,"%s",string);
//		fread(string,256,1,videoFd);
//		DEBUGPRINT(DEBUG_INFO, "/Dev info=%s\n",string);
//		fclose(videoFd);
//	}
	iRet = OpenDevice(pszDevName);
	DEBUGPRINT(DEBUG_INFO,"Open Video Device Fd======%d\n",iRet);
	if(iRet == -1){
		//Open Device Failed
		DEBUGPRINT(DEBUG_ERROR, "OpenDevice failed\n");
		return -1;
	}
	video_fd=iRet;
	/////////////////////////////////////////////////////////////////////////////////////////
	if(InitDevice(t_msg) == -1)
	{
		DEBUGPRINT(DEBUG_ERROR, "Init Device failed\n");
		return -2;
	}
	//////////////////////////////////////////////////////////////////////////////////////////
	if(InitMmap(t_msg) == -1){
		return -3;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	//Put Buffer In Queue
	if(PutBufIntoInqueue(t_msg) == -1){
		return -4;
	}
	DEBUGPRINT(DEBUG_INFO, "Put Buffer In Qeuen in file:%s line:%d.\n", __FILE__, __LINE__);

	//////////////////////////////////////////////////////////////////////////////////////////////////////
	//Begain Capture
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	usleep(100000);
	if (-1 == ioctl(video_fd, VIDIOC_STREAMON, &type))
	{
	    DEBUGPRINT(DEBUG_ERROR, "ioctlfailed(VIDIOC_STREAMON), in line:%d file:%s.\n", __LINE__, __FILE__);
	    return -5;
	}
	usleep(10000);
	DEBUGPRINT(DEBUG_INFO, "Start Video Capture in file:%s line:%d.\n", __FILE__, __LINE__);

	return 0;
}

int OpenDevice (const char* pszDevName)
{
	int iRet = 0;

//	if(!access(pszDevName,F_OK)){
//		DEBUGPRINT(DEBUG_ERROR, "%s Can't Read .Can't Access!!!\n", pszDevName);
//		return -1;
//	}

	struct stat st;
	if (-1 == stat (pszDevName, &st))
	{
		DEBUGPRINT(DEBUG_ERROR, "Cannot identify:%s error: %d, %s\n", pszDevName, errno, strerror (errno));
		return -1;
	}
	if (!S_ISCHR (st.st_mode))
	{
		DEBUGPRINT(DEBUG_ERROR, "%s is no device\n", pszDevName);
		return -1;
	}

	iRet = open (pszDevName, O_RDWR| O_NONBLOCK /* required */, 0);
	printf("iRet=======================%d========pszDevName==%s====\n",iRet,pszDevName);
	if(iRet == -1){
		if(0 == access("/dev/video1",F_OK)){
			printf("=======================Open Video1======================\n");
			iRet = open ("/dev/video1", O_RDWR| O_NONBLOCK /* required */, 0);
			printf("=======================Open Video1==========iRet=%d===========\n",iRet);
		}
		else{
			printf("=======================Can not access======================\n");
		}
	}

	if(-1 == iRet)
	{
		DEBUGPRINT(DEBUG_ERROR, "times:%d, OpenDevice failed:%s, in line:%d file:%s.\n",
					iVideoOpenTimes, strerror(errno), __LINE__, __FILE__);
	}
	return iRet;
}

int InitDevice (struct VideoThreadMsg *t_msg)
{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;

	DEBUGPRINT(DEBUG_INFO, "Come in InitDevice, in line:%d file %s\n", __LINE__, __FILE__);

	//enum format
	struct v4l2_fmtdesc fmtdesc = {0};
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	while(1)
	{
		if (-1 == ioctl(video_fd, VIDIOC_ENUM_FMT, &fmtdesc))
		{
			break;
		}
		//if( == fmtdesc.flags)
		DEBUGPRINT(DEBUG_INFO, "index:%d, flags:%d, description:%s\n", fmtdesc.index, fmtdesc.flags, fmtdesc.description);
		++fmtdesc.index;
	}

	DEBUGPRINT(DEBUG_INFO, "Before VIDIOC_QUERYCAP, in line:%d file %s\n", __LINE__, __FILE__);

	if (-1 == ioctl(video_fd, VIDIOC_QUERYCAP, &cap))
	{
		if (EINVAL == errno)
		{
			DEBUGPRINT(DEBUG_ERROR, "VIDIOC_QUERYCAP failed: %s is no V4L2 device\n", "video0");
			return -1;
		}
		else
		{
			DEBUGPRINT(DEBUG_ERROR, "VIDIOC_QUERYCAP failed: %s", strerror(errno));
			return -1;
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
	{
		DEBUGPRINT(DEBUG_ERROR, "%s is no video capture device\n", "video0");
		return -1;
	}

	if (!(cap.capabilities & V4L2_CAP_STREAMING))
	{
		DEBUGPRINT(DEBUG_ERROR, "%s does not support streaming i/o\n", "video0");
		return -1;
	}

	DEBUGPRINT(DEBUG_INFO, "Before VIDIOC_CROPCAP, in line:%d file %s\n", __LINE__, __FILE__);

	/* Select video input, video standard and tune here. */
	CLEAR (cropcap);
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (0 == ioctl(video_fd, VIDIOC_CROPCAP, &cropcap))
	{
		DEBUGPRINT(DEBUG_INFO, "Before VIDIOC_S_CROP, in line:%d file %s\n", __LINE__, __FILE__);

		struct v4l2_crop crop;
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; /* reset to default */
		if (-1 == ioctl(video_fd, VIDIOC_S_CROP, &crop))
		{
			switch (errno)
			{
			case EINVAL:
				/* Cropping not supported. */
				DEBUGPRINT(DEBUG_ERROR, "Cropping not supported, in line:%d file %s\n", __LINE__, __FILE__);
				break;
			default:
				/* Errors ignored. */
				break;
			}
		}
	}
	else
	{
		/* Errors ignored. */
	}

	DEBUGPRINT(DEBUG_INFO, "Before set format, in line:%d file %s\n", __LINE__, __FILE__);

    SetVideoSize(iSetVideoSize);

	return 0;
}

int SetVideoSize(int iVideoSize)
{
    DEBUGPRINT(DEBUG_ERROR, "Come in SetVideoSize, in file:%s line:%d\n", __FILE__, __LINE__);

	int iRet = -1;

	struct v4l2_format fmt;
	CLEAR (fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG; //V4L2_PIX_FMT_YUYV; //
	fmt.fmt.pix.field = V4L2_FIELD_NONE;//V4L2_FIELD_INTERLACED;//

	if(1 == iVideoSize)
	{
		fmt.fmt.pix.width = 640;
		fmt.fmt.pix.height = 480;
	}
	else if(2 == iVideoSize)
	{
		fmt.fmt.pix.width = 320;
		fmt.fmt.pix.height = 240;
	}
	else if(3 == iVideoSize)
	{
		fmt.fmt.pix.width = 160;
		fmt.fmt.pix.height = 120;
	}

    iRet = ioctl(video_fd, VIDIOC_S_FMT, &fmt);
    if (-1 == iRet)
    {
        DEBUGPRINT(DEBUG_ERROR, "VIDIOC_TRY_FMT failed:%s, in file:%s line:%d\n",
                strerror(errno), __FILE__, __LINE__);
    }
	return iRet;
}

int InitMmap (struct VideoThreadMsg *t_msg)
{
	struct v4l2_requestbuffers req;

	CLEAR (req);
	req.count = REQUSET_BUFFER_SIZE;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	DEBUGPRINT(DEBUG_INFO, "Come in InitMmap, in line:%d file %s==fd===%d\n", __LINE__, __FILE__,video_fd);
	if (-1 == ioctl(video_fd, VIDIOC_REQBUFS, &req))
	{
		if (EINVAL == errno)
		{
			DEBUGPRINT(DEBUG_ERROR, "%s does not support memory mapping\n", "video0");
			return -1;
		}
		else
		{
			DEBUGPRINT(DEBUG_ERROR, "VIDIOC_REQBUFS=====Error=%s\n",strerror(errno));
			return -1;
		}
	}

	t_msg->iBufferSize = req.count;

	DEBUGPRINT(DEBUG_INFO, "iBufferSize:%d, in line:%d file %s\n", t_msg->iBufferSize, __LINE__, __FILE__);
	if (t_msg->iBufferSize < 2)
	{
		DEBUGPRINT(DEBUG_ERROR, "Insufficient buffer memory on %s\n", "video0");
		return -1;
	}



	int i = 0;

    CLEAR (buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    usleep(10000);
	for (i = 0; i < t_msg->iBufferSize; ++i)
	{
		DEBUGPRINT(DEBUG_INFO, "in file:%s line:%d.\n", __FILE__, __LINE__);

		buf.index = i;
		if (-1 == ioctl(video_fd, VIDIOC_QUERYBUF, &buf))
		{
			return -1;
		}

		DEBUGPRINT(DEBUG_INFO, "buf.size:%d, buf.flags:%d.\n", buf.length, buf.flags);

		pBuffers[i].iBufferLen = buf.length;
		pBuffers[i].start = mmap (NULL/* start anywhere */, buf.length,
			PROT_READ | PROT_WRITE /* required */, MAP_SHARED /* recommended */, video_fd, buf.m.offset);
		if (MAP_FAILED == pBuffers[i].start)
		{
			DEBUGPRINT(DEBUG_ERROR, "mmap failed, in line:%d file:%s.\n", __LINE__, __FILE__);
			return -1;
		}
	}
	DEBUGPRINT(DEBUG_INFO, "in file:%s line:%d.\n", __FILE__, __LINE__);
	return 0;
}

int PutBufIntoInqueue(struct VideoThreadMsg *t_msg)
{
	int i=0;
	CLEAR (buf);
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	usleep(10000);
	for(i=0; i<t_msg->iBufferSize; i++)
	{
		buf.index = i;
		if (-1 == ioctl(video_fd, VIDIOC_QBUF, &buf))
		{
			DEBUGPRINT(DEBUG_ERROR, "ioctl(VIDIOC_QBUF) failed, buf.index:%d, in line:%d file:%s.\n", buf.index, __LINE__, __FILE__);
			return -1;
		}
	}
	return 0;
}


int stopVideoCapture(struct VideoThreadMsg *t_msg){

	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == ioctl(video_fd, VIDIOC_STREAMOFF, &type))
	{
	    DEBUGPRINT(DEBUG_ERROR, "ioctlfailed(VIDIOC_STREAMON), in line:%d file:%s.\n", __LINE__, __FILE__);
	    return -1;
	}
	DEBUGPRINT (DEBUG_INFO, "close off===============1\n");
	unsigned int i = 0;
	for (; i < t_msg->iBufferSize; ++i)
	{
		pthread_mutex_destroy((pthread_mutex_t *)(pFrameMutexs+i));
		if (-1 == munmap (pBuffers[i].start, pBuffers[i].iBufferLen))
		{
			return -2;
		}
	}

	/* add by wfb 04161125 */
	{
		struct v4l2_requestbuffers req;
	    CLEAR (req);
		req.count = 0;
		req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		req.memory = V4L2_MEMORY_MMAP;

	    if (-1 == ioctl(video_fd, VIDIOC_REQBUFS, &req))
	    {
             printf("ioctl(video_fd, VIDIOC_REQBUFS  release\n");
             return -2;
	    }
	}

	t_msg->iBufferSize = 0;
	DEBUGPRINT (DEBUG_INFO, "close munmap===============2\n");
	if ( -1 == close (video_fd))
	{
		DEBUGPRINT (DEBUG_ERROR, "close");
		return -3;
	}
	video_fd = -1;
	DEBUGPRINT (DEBUG_INFO, "close fd===============3\n");

	return 0;
}

int SetVideoParam (EVideoParamType type, int iValue, char* pszErrMsg)
{
	int iRet = 0;

	switch( type )
	{
	case FRAME_RATE:
		break;

	case SAMPLE_RATE:
		break;

	case BRIGHT:
		iRet = SetVideoParamControl(V4L2_CID_BRIGHTNESS, iValue);
		break;

	case CONTRAST:
		iRet = SetVideoParamControl(V4L2_CID_CONTRAST, iValue);
		break;

	case SIZE:
		iRet = ResetVideoSize(iValue);
		break;

	case HFLIP:
		iRet = SetVideoParamControl(V4L2_CID_HFLIP, iValue);
		break;

	case VFLIP:
		iRet = SetVideoParamControl(V4L2_CID_VFLIP, iValue);
		break;

	case AUTO_WHITE_BALANCE:
		iRet = SetVideoParamControl(V4L2_CID_AUTO_WHITE_BALANCE, iValue);
		break;

	case RED_BALANCE:
		iRet = SetVideoParamControl(V4L2_CID_RED_BALANCE, iValue);
		break;

	case BLUE_BALANCE:
		iRet = SetVideoParamControl(V4L2_CID_BLUE_BALANCE, iValue);
		break;
	}
	return iRet;
}

int SetVideoParamControl(int type, int value)
{
	struct v4l2_queryctrl queryctrl;
	memset (&queryctrl, 0, sizeof (queryctrl));
	queryctrl.id = type;

	if (-1 == ioctl(video_fd, VIDIOC_QUERYCTRL, &queryctrl))
	{
		DEBUGPRINT(DEBUG_INFO, "Query Failed=========================================\n");
		return -1;
	}
	DEBUGPRINT(DEBUG_INFO, "MaxValue===%i ===============MinValue===%i========\n",0,0);
	//?��???????此项�??
	if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
	{
		DEBUGPRINT(DEBUG_ERROR, "%d is not supported, in file:%s line:%d\n", type, __FILE__, __LINE__);
		return -1;
	}
	DEBUGPRINT(DEBUG_INFO, "Name==%s==MaxValue===%i ===============MinValue===%i========Changeable=%i\n",queryctrl.name,queryctrl.maximum,queryctrl.minimum,queryctrl.flags);
	struct v4l2_control control;
	memset (&control, 0, sizeof (control));
	control.id = type;
	control.value = value;
	return ioctl(video_fd, VIDIOC_S_CTRL, &control);
}

int ResetVideoSize(int iVideoSize)
{
    DEBUGPRINT(DEBUG_ERROR, "Come in ResetVideoSize, in file:%s line:%d\n", __FILE__, __LINE__);

	int iRet = -1;
    //usleep(10000);
    //???�?????
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(video_fd, VIDIOC_STREAMOFF, &type))
    {
        DEBUGPRINT(DEBUG_ERROR, "VIDIOC_STREAMOFF failed:%s, in line:%d file:%s.\n",
                strerror(errno), __LINE__, __FILE__);
    }

    iRet = SetVideoSize(iVideoSize);
    iSetVideoSize = iVideoSize;
    //�??�?????
    usleep(100000);
    if (-1 == ioctl(video_fd, VIDIOC_STREAMON, &type))
    {
        DEBUGPRINT(DEBUG_ERROR, "VIDIOC_STREAMON failed:%s, in line:%d file:%s.\n",
                strerror(errno), __LINE__, __FILE__);
    }

	return iRet;
}


int CaptureOnePicture(struct VideoThreadMsg *t_msg){

	if(iTestFlag == 1){
		printf("+++++++++++++++++++++++++++++++++++++Why Come Here++++++++++++++++++++++++++++++++++++++++++++");
	}
	int iLastFrameIndex = 0;
	CLEAR (buf);

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;

	/*	VIDIOC_QBUF, VIDIOC_DQBUF
	Return Value

	On success 0 is returned, on error -1 and the errno variable is set appropriately:
	EAGAIN
	Non-blocking I/O has been selected using O_NONBLOCK and no buffer was in the outgoing queue.
	EINVAL
	The buffer type is not supported, or the index is out of bounds, or no buffers have been
	allocated yet, or the userptr or length are invalid.
	ENOMEM
	Not enough physical or virtual memory was available to enqueue a user pointer buffer.
	EIO
	VIDIOC_DQBUF failed due to an internal error. Can also indicate temporary problems like signal
	loss. Note the driver might dequeue an (empty) buffer despite returning an error, or even stop
	capturing.
	*/
	int  iDQNUFRet = -1;
	while ( 0 == (iDQNUFRet = ioctl(video_fd, VIDIOC_DQBUF, &buf))){
		iVideoNeedRebootIndex = 0;
		iLastFrameIndex = iNewestBufferIndex;
		//�??�??�????????�??�???��????????????????�???��?buffer�??
		pBuffers[buf.index].byteRefs = 1;
		pBuffers[buf.index].iDataLen = buf.bytesused;
		iNewestBufferIndex = buf.index;
//		DEBUGPRINT(DEBUG_INFO, "=====Capture Success!!!!!!===%d===%d\n",buf.index,pBuffers[buf.index].iDataLen);
		pthread_mutex_lock((pthread_mutex_t *)&pFrameMutexs[iLastFrameIndex]);
		--pBuffers[iLastFrameIndex].byteRefs;
			if(0 == pBuffers[iLastFrameIndex].byteRefs)
			{
				buf.index = iLastFrameIndex;
				if( 0 != ioctl(video_fd, VIDIOC_QBUF, &buf) ){
//				DEBUGPRINT(DEBUG_INFO, "ioctl(VIDIOC_QBUF) failed:%s, errno:%d, in line:%d file:%s\n",strerror(errno), errno, __LINE__, __FILE__);
			}
			else{
//				DEBUGPRINT(DEBUG_INFO, "==================Release Success!!!!!!\n");
				}
			}
			pthread_mutex_unlock((pthread_mutex_t *)pFrameMutexs+iLastFrameIndex);
	}

	if ( -1 == iDQNUFRet)
	{
		iVideoNeedRebootIndex++;
//		DEBUGPRINT(DEBUG_INFO, "iVideoNeedRebootIndex================%d\n",iVideoNeedRebootIndex);
		if(iVideoNeedRebootIndex == 300){
			DEBUGPRINT(DEBUG_INFO, "iVideoNeedRebootIndex================%d\n",iVideoNeedRebootIndex);
			DEBUGPRINT(DEBUG_INFO, "iVideoNeedRebootIndex================%d\n",iVideoNeedRebootIndex);
			DEBUGPRINT(DEBUG_INFO, "iVideoNeedRebootIndex================%d\n",iVideoNeedRebootIndex);
			DEBUGPRINT(DEBUG_INFO, "iVideoNeedRebootIndex================%d\n",iVideoNeedRebootIndex);
			//如果是由于插入usb强制切换而导致 那么则不重启,否则重启
			if (FALSE == g_nAvErrorFlag)
			{
				struct HandleVideoMsg *sendVideoMsg=(struct HandleVideoMsg *)t_msg->videoMsgSend;
				sendVideoMsg->type = VIDEO_MSG_TYPE;
				sendVideoMsg->len = 0;
				sendVideoMsg->cmd = 0xff;//MSG_VIDEO_P_VIDEO_QBUF_NEED_STOP;
				msg_queue_snd(t_msg->childProcessMsgID,sendVideoMsg,sizeof(MsgData)-sizeof(long int)+sendVideoMsg->len);
			}
		}
		if(iVideoNeedRebootIndex>2000){
			iVideoNeedRebootIndex=299;
		}
		switch (errno)
		{
			case EAGAIN:
	//				DEBUGPRINT(DEBUG_INFO, "No buffer was in the outgoing queue, in line:%d file:%s\n", __LINE__, __FILE__);
					break;
			case EIO:
				DEBUGPRINT(DEBUG_ERROR, "EIO===%d===ioctl(VIDIOC_DQBUF) failed:%s, continue, in line:%d file:%s\n",video_fd,strerror(errno), __LINE__, __FILE__);
				break;
			default:
				DEBUGPRINT(DEBUG_ERROR, "default===%d===ioctl(VIDIOC_DQBUF) failed:%s, continue, in line:%d file:%s\n",video_fd,strerror(errno), __LINE__, __FILE__);
				int i=0;
				CLEAR (buf);
				buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf.memory = V4L2_MEMORY_MMAP;
				usleep(10000);
				for(i=0; i<t_msg->iBufferSize; i++)
				{
					buf.index = i;
					if (-1 == ioctl(video_fd, VIDIOC_QBUF, &buf))
						{
						DEBUGPRINT(DEBUG_INFO,"VIDIOC_QBUF===Failed\n");
						}
					}
				usleep(10000);
					break;
		}
	}
	return 0;
}

int GetNewestVideoFrame (char** pData, int* piLen, int* piIndex)
{
    if(VIDEO_RUNNING != videoCaptureThreadState || -1 == video_fd)
	{
		//DEBUGPRINT(DEBUG_INFO, "GetNewestVideoFrame fdVideo:%d, m_videoState:%d, in file:%s line:%d\n",
		//	fdVideo, m_videoState, __FILE__, __LINE__);
		return -1;
	}
    int iIndex = iNewestBufferIndex;
    pthread_mutex_lock((void *)&pFrameMutexs[iIndex]);
    if (0 == pBuffers[iIndex].byteRefs)
    {
        pthread_mutex_unlock((void *)&pFrameMutexs[iIndex]);
        return -1;
    }
    ++pBuffers[iIndex].byteRefs;
    pthread_mutex_unlock((void *)&pFrameMutexs[iIndex]);

	*pData = pBuffers[iIndex].start;
	*piLen = pBuffers[iIndex].iDataLen;
	*piIndex = iIndex;
	return 0;
}


int ReleaseNewestVideoFrame (int iIndex)
{
    int iRet = 0;

    if(VIDEO_RUNNING != videoCaptureThreadState || -1 == video_fd)
	{
		DEBUGPRINT(DEBUG_INFO, "--====--ReleaseNewestVideoFrame fdVideo:%d, m_videoState:%d, in file:%s line:%d\n",video_fd, videoCaptureThreadState, __FILE__, __LINE__);
		return -1;
	}

    pthread_mutex_lock((void *)&pFrameMutexs[iIndex]);
    --pBuffers[iIndex].byteRefs;
    if(0 == pBuffers[iIndex].byteRefs)
    {
        CLEAR (buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = iIndex;
        if( 0 != ioctl(video_fd, VIDIOC_QBUF, &buf) )
        {
            DEBUGPRINT(DEBUG_INFO, "ioctl(VIDIOC_QBUF) failed:%s, in line:%d file:%s\n",
                       strerror(errno), __LINE__, __FILE__);
            iRet = -1;
        }
    }
    pthread_mutex_unlock((void *)&pFrameMutexs[iIndex]);

	return iRet;
}

int HandleOpenError(struct VideoThreadMsg *t_msg, int errorCode){
	DEBUGPRINT(DEBUG_INFO,"Video Open Error Code = %d\n",errorCode);
	iVideoOpenFailedFlag = 1;
	switch (errorCode){
	case -1:{
		//Notify Re MFI
		if(iVideoOpenTimes == 3){
			videoCaptureThreadState = VIDEO_ERROR;
			iVideoOpenTimes = 0;
			struct HandleVideoMsg *sendVideoMsg=(struct HandleVideoMsg *)t_msg->videoMsgSend;
			sendVideoMsg->type=VIDEO_MSG_TYPE;
			sendVideoMsg->cmd=MSG_VIDEO_P_RESWITCH_USB;
			sendVideoMsg->len = 0;
			msg_queue_snd(t_msg->childProcessMsgID,sendVideoMsg,sizeof(MsgData)-sizeof(long int)+sendVideoMsg->len);
		}
		}
		break;
	case -2:{
		videoCaptureThreadState = VIDEO_ERROR;
		if(-1 == close(video_fd)){
			DEBUGPRINT (DEBUG_ERROR, "close Failed\n");
		}
		}
		break;
	case -3:{
		videoCaptureThreadState = VIDEO_ERROR;
		if(-1 == close(video_fd)){
			DEBUGPRINT (DEBUG_ERROR, "close Failed\n");
		}
		}
		break;
	case -4:{
		videoCaptureThreadState = VIDEO_ERROR;
		if(-1 == close(video_fd)){
			DEBUGPRINT (DEBUG_ERROR, "close Failed\n");
		}
		}
		break;
	case -5:{
		videoCaptureThreadState = VIDEO_ERROR;
		unsigned int i = 0;
		for (; i < t_msg->iBufferSize; ++i)
		{
			pthread_mutex_destroy((pthread_mutex_t *)(pFrameMutexs+i));
			if (-1 == munmap (pBuffers[i].start, pBuffers[i].iBufferLen))
			{
				return -2;
			}
		}
		struct v4l2_requestbuffers req;
		CLEAR (req);
		req.count = 0;
		req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		req.memory = V4L2_MEMORY_MMAP;
		if (-1 == ioctl(video_fd, VIDIOC_REQBUFS, &req))
		{
			printf("ioctl(video_fd, VIDIOC_REQBUFS  release\n");
		    return -2;
		}
		t_msg->iBufferSize = 0;
		DEBUGPRINT (DEBUG_INFO, "close munmap===============2\n");
		if ( -1 == close (video_fd))
		{
			DEBUGPRINT (DEBUG_ERROR, "close Faild\n");
			return -3;
		}
		}
		break;
	default:
		videoCaptureThreadState = VIDEO_ERROR;
		break;
	}
	return 0;
}
