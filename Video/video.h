/*@file [视频采集线程]
 * @brief 按协议进行工作，主要工作为打开设备采集图像
 * @note
 * Author:张历
 * Date:2013-9-24
 * Created on: Jan 7, 2013
 */

#ifndef VIDEO_H_
#define VIDEO_H_


#include <stdio.h>
#include <errno.h>

#define VIDEO_CAPTURE_MSG_LEN 128
#define MAX_VIDEO_MSG_LEN 512

enum {
	MSG_VIDEO_P_START_CAPTURE = 1,
	MSG_VIDEO_P_STOP_CAPTURE,
	MSG_VIDEO_P_SETTING_PARAMS,
	MSG_VIDEO_P_RESWITCH_USB,
	MSG_VIDEO_P_CHECK_ALIVE
};

typedef struct
{
	void * start;
	size_t iBufferLen;
	int	   iDataLen;
    int   byteRefs;
}VideoBuffer;

typedef enum
{
	FRAME_RATE = 0,
	BRIGHT,
	CONTRAST,
	SIZE,
	HFLIP,
	VFLIP,
	SAMPLE_RATE,
	AUTO_WHITE_BALANCE,
	RED_BALANCE,
	BLUE_BALANCE
}EVideoParamType;


typedef enum{
	VIDEO_STARTING,
	VIDEO_OPENNING,
	VIDEO_RUNNING,
	VIDEO_SETTING,
	VIDEO_STOP,
	VIDEO_ERROR
}E_VIDEO_CAPTURE_THREAD_STATE;

struct HandleVideoMsg{
	long int type;
	int cmd;
	char unUsed[4];
	int len;
	char data[VIDEO_CAPTURE_MSG_LEN-16];
};

//Global Variable
volatile int video_fd;
volatile E_VIDEO_CAPTURE_THREAD_STATE videoCaptureThreadState;
volatile VideoBuffer *pBuffers;
volatile pthread_mutex_t* pFrameMutexs;
volatile int iNewestBufferIndex;

//Function

/*
 * @brief 创建视频采集线程
 * @author:张历
 * @param[in]:<没有参数>
 * @param[out]:<没有参数>
 * @Return <成功：线程ID ；失败：-1>
 */
int CreateVideoCaptureThread();


/*
 * @brief 获取当前最新的视频帧
 * @author:张历
 * @param[in]:<没有参数>
 * @param[out]:<pData><指向图像地址的指针>
 * @param[out]:<piLen><图像的大小>
 * @param[out]:<piIndex><图像的索引号>
 * @Return <成功：0 ；失败：-1>
 */
int GetNewestVideoFrame (char** pData, int* piLen, int* piIndex);


/*
 * @brief 释放占用的图像
 * @author:张历
 * @param[in]:<iIndex><要释放的图像索引>
 * @param[out]:<无参数>
 * @Return <成功：0 ；失败：-1>
 */
int ReleaseNewestVideoFrame (int iIndex);

#endif /* VIDEO_H_ */
