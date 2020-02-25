/*
 * Audio.c
 *
 *  Created on: Jan 7, 2013
 *      Author: root zhang
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <semaphore.h>
#include "alsa/asoundlib.h"
#include "common/common.h"
#include "common/thread.h"
#include "common/logfile.h"
#include "common/msg_queue.h"
#include "Audio.h"

extern inline void AdpcmEncode(short nPcm, int iIndex, unsigned char* encoded, int* pre_sample, int* piIndex);
extern volatile int iVideoOpenningFlag;

volatile unsigned int iWriteCount;

struct AudioThreadData{
	int audioThreadMsgID;
	int childProcessMsgID;
	char charAudioMsgRecv[AUDIO_CAPTURE_MSG_LEN];
	char charAudioMsgSend[AUDIO_CAPTURE_MSG_LEN];
	int iFrames;
};

typedef enum
{
	AUDIO_STOP,
	AUDIO_STARTING,
	AUDIO_OPENNING,
	AUDIO_RUNNING,
	AUDIO_ERROR
}E_AUDIO_STATE;

typedef enum
{
	false_Audio_Encode=0,
	ture_Audio_Encode=1,
}E_Audio_Encode_bool;

volatile E_AUDIO_STATE m_audioState;
volatile char szAudioBuf[AUDIO_BUFFER_SIZE][40+160];
volatile int byteNewestFrame;
volatile int byteAlarmBufIndex;
volatile short nAlarmAudioBuf[AUDIO_ALARM_BUF_SIZE][AUDIO_ALARM_DATA_LEN];
volatile int audioiPreSample;
volatile int audioindexl;
volatile snd_pcm_t *m_pHandle = NULL;
volatile E_Audio_Encode_bool isEncode;
char* pszDataBuffer = NULL;
volatile int byteWrite = 0;
volatile int iAlarmBufCount;

volatile int iAudioOpenIndex = 0;
volatile int iAudioOpenTimes = 0;
volatile int iAudioSleepTime = 20000;

volatile int iTestFlag = 0;

volatile int iTestPrintFlag = 0;

//volatile sem_t AVSemaphore;
#define AUDIO_REBOOT_NUM 100;
volatile int iAudioNeedRebootIndex = 0;

//volatile int iAudioStoreFlag = 0;
//volatile FILE *pcmFileFD = NULL;
//volatile FILE *adpcmFileFD = NULL;
volatile int nLoops = 0;


/*
 * @brief 开始音频采集线程
 * @author:张历
 * @param[in]:<arg><线程启动参数>
 * @param[out]:<无参数>
 * @Return <-1：失败；其他：线程ID ； >
 */
void *StartAudioCaptureThread(void* arg);

/*
 * @brief 音频采集线程的参数初始化
 * @author:张历
 * @param[in]:<t_audioThreadData><线程参数的封装结构>
 * @param[out]:<无参数>
 * @Return <成功：0 ；失败：-1>
 */
int AudioCaptureThreadInit(struct AudioThreadData *t_audioThreadData);

/*
 * @brief 音频采集线程的线程同步
 * @author:张历
 * @param[in]:<无参数>
 * @param[out]:<无参数>
 * @Return <无参数>
 */
void AudioCaptureThreadSync();

/*
 * @brief 音频采集线程的消息处理函数
 * @author:张历
 * @param[in]:<t_audioThreadData><线程参数的封装结构>
 * @param[in]:<pParams><音频采集参数>
 * @param[out]:<无参数>
 * @Return <无参数>
 */
void AudioCaptureThreadHandleMsg(struct AudioThreadData *t_audioThreadData, snd_pcm_hw_params_t *pParams);

/*
 * @brief 开始打开音频文件
 * @author:张历
 * @param[in]:<t_audioThreadData><线程参数的封装结构>
 * @param[in]:<pParams><音频采集参数>
 * @param[out]:<无参数>
 * @Return <成功：0 ；失败：-1>
 */
int DoStartAudioCapture(struct AudioThreadData *t_audioThreadData, snd_pcm_hw_params_t *pParams);

/*
 * @brief 停止视频采集
 * @author:张历
 * @param[in]:<t_audioThreadData><线程参数的封装结构>
 * @param[in]:<pParams><音频采集参数>
 * @param[out]:<无参数>
 * @Return <0：成功 ；-1：失败;>
 */
int StopAudioCapture(struct AudioThreadData *t_audioThreadData, snd_pcm_hw_params_t *pParams);

/*
 * @brief 开始采集函数
 * @author:张历
 * @param[in]:<t_msg><线程参数的封装结构>
 * @param[in]:<pParams><音频采集参数>
 * @param[out]:<无参数>
 * @Return <0：成功 ；-1：音频参数为空；-2：初始化设备失败；其他：各种参数的设置失败>
 */
int StartAudioCapture(struct AudioThreadData *t_audioThreadData, snd_pcm_hw_params_t *pParams);

/*
 * @brief 初始化图像采集设备
 * @author:张历
 * @param[in]:<t_msg><线程参数的封装结构>
 * @param[in]:<pParams><音频采集参数>
 * @param[in]:<ppHandle><音频采集句柄>
 * @Return <0：成功 ；-2：初始化设备失败；其他：各种参数的设置失败>
 */
int InitAudioDevice(struct AudioThreadData *t_audioThreadData, snd_pcm_t** ppHandle, snd_pcm_hw_params_t** ppParams);

/*
 * @brief 采集一包音频数据
 * @author:张历
 * @param[in]:<t_audioThreadData><线程参数的封装结构>
 * @param[in]:<pParams><音频采集参数>
 * @param[out]:<无参数>
 * @Return <0：成功 ；-1：失败;>
 */
int CaptureOnePakageAudioData(struct AudioThreadData *t_audioThreadData, snd_pcm_hw_params_t *pParams);

/*
 * @brief 处理开始音频采集失败的函数
 * @author:张历
 * @param[in]:<t_audioThreadData><线程参数的封装结构>
 * @param[in]:<pParams><音频采集参数>
 * @param[in]:<errorCode><错误类型>
 * @param[out]:<无参数>
 * @Return <0：成功 ；-1：失败;>
 */
int HandleAudioOpenError(struct AudioThreadData *t_audioThreadData, snd_pcm_hw_params_t *pParams, int errorCode);

int CreateAudioCaptureThread()
{
#if 0
	StartAudioCaptureThread(NULL);
	return 0;
#else
	return thread_start(&StartAudioCaptureThread);
#endif
}


void *StartAudioCaptureThread(void* arg)
{
	DEBUGPRINT(DEBUG_INFO,"Start Audio Capture Thread=====================\n");
	struct AudioThreadData *audioThreadData = NULL;
	audioThreadData=malloc(sizeof(struct AudioThreadData));
	if(audioThreadData == NULL){
		//Alloc AudioData Failed
		m_audioState=AUDIO_ERROR;
		return NULL;
	}
	memset(audioThreadData,0,sizeof(struct AudioThreadData));
	//Thread Init
	DEBUGPRINT(DEBUG_INFO,"Before Init=====================\n");
	if(AudioCaptureThreadInit(audioThreadData) != 0){
		return NULL;
	}
	//Thread Sync
	DEBUGPRINT(DEBUG_INFO,"Start Audio Capture Sync=====================\n");
	AudioCaptureThreadSync();
	DEBUGPRINT(DEBUG_INFO,"After Audio Capture Sync=====================\n");

	snd_pcm_hw_params_t *pParams = NULL;
	snd_pcm_hw_params_alloca(&pParams);

//	pcmFileFD = fopen("/mnt/testzhangli","w");
//	if(pcmFileFD == NULL){
//		DEBUGPRINT(DEBUG_INFO,"Aduio Create File Failed111111111111111\n");
//		return NULL;
//	}
//	adpcmFileFD = fopen("/mnt/ZhangLiadPcm.adpcm","w");
//	if(adpcmFileFD == NULL){
//		DEBUGPRINT(DEBUG_INFO,"Aduio Create File Failed2222222222222222222\n");
//		return NULL;
//	}

	while(1){
		//Handle Message

//		iTestPrintFlag++;
//		if(iTestPrintFlag == 100){
//			DEBUGPRINT(DEBUG_INFO,"Audio Alive===================================================\n");
//			iTestPrintFlag=0;
//		}
		//printf("1111111111111111111111111111111111111111111111111111111111111111111111111111\n");
		g_nAudioFlag1 = 0;
		AudioCaptureThreadHandleMsg(audioThreadData,pParams);
		child_setThreadStatus(AUDIO_MSG_TYPE, NULL);
		g_nAudioFlag1 = 1;
		if(m_audioState == AUDIO_OPENNING)
		{
			g_nAudioFlag1 = 3;
			DoStartAudioCapture(audioThreadData,pParams);
			g_nAudioFlag1 = 4;
		}
		if(m_audioState == AUDIO_RUNNING)
		{
//			if(sem_trywait(&AVSemaphore) == 0){
			g_nAudioFlag1 = 5;
			CaptureOnePakageAudioData(audioThreadData,pParams);
			g_nAudioFlag1 = 6;
//				sem_post(&AVSemaphore);
//			}
		}
		usleep(iAudioSleepTime);
		iAudioSleepTime = 20000;
	}
	snd_pcm_hw_params_free(pParams);
	if(audioThreadData != NULL){
		free(audioThreadData);
	}
	audioThreadData = NULL;

	if(pszDataBuffer != NULL){
		free(pszDataBuffer);
	}
	pszDataBuffer = NULL;
	return NULL;
}

int AudioCaptureThreadInit(struct AudioThreadData *t_audioThreadData)
{
	t_audioThreadData->childProcessMsgID=msg_queue_get(CHILD_PROCESS_MSG_KEY);
	t_audioThreadData->audioThreadMsgID=msg_queue_get(CHILD_THREAD_MSG_KEY);
	m_audioState=AUDIO_STARTING;
	m_pHandle = NULL;
	memset(szAudioBuf,0,sizeof(szAudioBuf));
	byteNewestFrame = 0;
	byteAlarmBufIndex = 0;
	iAlarmBufCount=0;
	byteWrite=0;
	memset(nAlarmAudioBuf,0,sizeof(nAlarmAudioBuf));
	audioiPreSample = 0;
	audioindexl = 0;
	isEncode = ture_Audio_Encode;//false_Audio_Encode;//

	pszDataBuffer = (char*)malloc(2); //modified by zhang do not Dynamic Malloc;
//	DEBUGPRINT(DEBUG_INFO,"before sem init=====================\n");
////	int iRett = sem_init(&AVSemaphore,0,1);
//	DEBUGPRINT(DEBUG_INFO,"sem init Ret=====================%d\n",iRett);
//	if(-1 == iRett){
//		return -1;
//	}
	DEBUGPRINT(DEBUG_INFO,"after sem init=====================\n");
	if(t_audioThreadData->childProcessMsgID == -1){
		return -1;
	}

	if(t_audioThreadData->audioThreadMsgID == -1){
			return -1;
	}
	return 0;
}

void AudioCaptureThreadSync(){
	rAudio=1;
	while(rChildProcess != 1){
		thread_usleep(0);
	}
	thread_usleep(0);
}

void AudioCaptureThreadHandleMsg(struct AudioThreadData *t_audioThreadData, snd_pcm_hw_params_t *pParams){
	if(msg_queue_rcv(t_audioThreadData->audioThreadMsgID, t_audioThreadData->charAudioMsgRecv, sizeof(struct HandleAudioMsg), AUDIO_MSG_TYPE) != -1){

		struct HandleAudioMsg *audioMsgRecv=(struct HandleAudioMsg *)t_audioThreadData->charAudioMsgRecv;
		struct HandleAudioMsg *audioMsgSend=(struct HandleAudioMsg *)t_audioThreadData->charAudioMsgSend;
		audioMsgSend->type = AUDIO_MSG_TYPE;
		char bit0;
		DEBUGPRINT(DEBUG_INFO,"====Get Msg Audio Thread!!===%d\n",audioMsgRecv->cmd);
		switch (audioMsgRecv->cmd){
			case MSG_AUDIO_P_START_CAPTURE:{
				//Start Capture
				DEBUGPRINT(DEBUG_INFO,"====Get Msg Audio Thread!! CMD: Start\n");

				if(m_audioState == AUDIO_STOP || m_audioState == AUDIO_ERROR || m_audioState == AUDIO_STARTING){
					m_audioState = AUDIO_OPENNING;
					iAudioOpenIndex = 0;
					iAudioOpenTimes = 0;
				}
			}
				break;
			case MSG_AUDIO_P_STOP_CAPTURE:{
				//Stop Capture
				DEBUGPRINT(DEBUG_INFO,"====Get Msg Audio Thread!! CMD: Stop\n");
				if(m_audioState == AUDIO_RUNNING){
					DEBUGPRINT(DEBUG_INFO,"Stop Audio Sem Before Wait====================\n");
//					sem_wait(&AVSemaphore);
					DEBUGPRINT(DEBUG_INFO,"Stop Audio Sem After Wait====================\n");
					iTestFlag = 1;
					int iAudioStopRet = StopAudioCapture(t_audioThreadData, pParams);
					iTestFlag = 0;
					DEBUGPRINT(DEBUG_INFO,"Stop Audio Sem Before post====================\n");
//					sem_post(&AVSemaphore);
					DEBUGPRINT(DEBUG_INFO,"Stop Audio Sem After post====================\n");
					bit0 = iAudioStopRet;
					audioMsgSend->cmd = MSG_AUDIO_P_STOP_CAPTURE;
					audioMsgSend->data[0] = bit0;
					audioMsgSend->len = 1;
					msg_queue_snd(t_audioThreadData->childProcessMsgID,audioMsgSend,sizeof(MsgData)-sizeof(long int)+audioMsgSend->len);
				}
				else if(m_audioState == AUDIO_OPENNING){
					m_audioState = AUDIO_STOP;
					audioMsgSend->cmd = MSG_AUDIO_P_STOP_CAPTURE;
					audioMsgSend->data[0] = 0;
					audioMsgSend->len = 1;
					msg_queue_snd(t_audioThreadData->childProcessMsgID,audioMsgSend,sizeof(MsgData)-sizeof(long int)+audioMsgSend->len);
				}
			}
				break;
			case MSG_AUDIO_P_START_ENCODE:{
				//Start Encode
				DEBUGPRINT(DEBUG_INFO,"====Audio Thread Start Encode CMD===\n");
				if(m_audioState == AUDIO_RUNNING){
					isEncode=ture_Audio_Encode;
					audioMsgSend->cmd = MSG_AUDIO_P_START_ENCODE;
					audioMsgSend->data[0] = 0;
					audioMsgSend->len = 1;
					msg_queue_snd(t_audioThreadData->childProcessMsgID,audioMsgSend,sizeof(MsgData)-sizeof(long int)+audioMsgSend->len);
				}
				}
				break;
			case MSG_AUDIO_P_STOP_ENCODE:{
				//Stop Encode
				DEBUGPRINT(DEBUG_INFO,"====Audio Thread Stop Encode CMD===\n");
				if(m_audioState == AUDIO_RUNNING){
					isEncode=false_Audio_Encode;
					byteNewestFrame = 0;
					byteWrite = 0;
					memset(szAudioBuf,0,sizeof(szAudioBuf));
					audioMsgSend->cmd = MSG_AUDIO_P_STOP_ENCODE;
					audioMsgSend->data[0] = 0;
					audioMsgSend->len = 1;
					msg_queue_snd(t_audioThreadData->childProcessMsgID,audioMsgSend,sizeof(MsgData)-sizeof(long int)+audioMsgSend->len);
				}
				}
				break;
			default:
				//Unknown Command
				DEBUGPRINT(DEBUG_ERROR,"Audio===============Unknown Command\n");
				break;
		}
		memset(t_audioThreadData->charAudioMsgRecv,0,sizeof(t_audioThreadData->charAudioMsgRecv));
		memset(t_audioThreadData->charAudioMsgSend,0,sizeof(t_audioThreadData->charAudioMsgSend));
		audioMsgRecv = NULL;
		audioMsgSend = NULL;
	}
}

int CaptureOnePakageAudioData(struct AudioThreadData *t_audioThreadData, snd_pcm_hw_params_t *pParams){

	int iRet;

	if(NULL == pszDataBuffer)
	{
		return -1;
	}

	int i;
#ifdef _ADT_AUDIO
	short *adt_audiodata = nAlarmAudioBuf[byteAlarmBufIndex] + iAlarmBufCount;
#endif
	for(i=0; i<320; i++)  //320
	{
		g_nAudioFlag2 = (++nLoops) & 0xffff;
		iRet = snd_pcm_readi((snd_pcm_t *)m_pHandle, pszDataBuffer, t_audioThreadData->iFrames);
		g_nAudioFlag2 = (nLoops & 0xffff) | 0x10000;
//		DEBUGPRINT(DEBUG_INFO, "===============read:%d, =========iFrames:%d\n", iRet, (int)t_audioThreadData->iFrames);
		if (iRet == -EPIPE)
		{
			// EPIPE means overrun
			DEBUGPRINT(DEBUG_INFO, "overrun occurred\n");
			usleep(10000);
			g_nAudioFlag3 = 1;
			if(0 > snd_pcm_prepare((snd_pcm_t *)m_pHandle)){
				DEBUGPRINT(DEBUG_INFO,"snd_pcm_prepare Failed\n");
			}
			g_nAudioFlag3 = 2;
			continue;
		}
		else if (iRet < 0)
		{
			DEBUGPRINT(DEBUG_ERROR, "error from read===iRet=%d==%s\n",iRet,snd_strerror(iRet));
			iAudioNeedRebootIndex++;
			if(iAudioNeedRebootIndex == 1000){
				iAudioNeedRebootIndex = 0;
				struct HandleAudioMsg *audioMsgSend=(struct HandleAudioMsg *)t_audioThreadData->charAudioMsgSend;
				audioMsgSend->type = AUDIO_MSG_TYPE;
				audioMsgSend->cmd = 0xff;
				audioMsgSend->len = 0;
				msg_queue_snd(t_audioThreadData->childProcessMsgID,audioMsgSend,sizeof(MsgData)-sizeof(long int)+audioMsgSend->len);
			}
			if(iAudioNeedRebootIndex>100000){
				iAudioNeedRebootIndex=1000;
			}
			usleep(40000);
			g_nAudioFlag3 = 3;
			if(0 > snd_pcm_prepare((snd_pcm_t *)m_pHandle)){
					DEBUGPRINT(DEBUG_INFO,"snd_pcm_prepare Failed\n");
			}
			g_nAudioFlag3 = 4;
				break;
		}
		else if (iRet != (int)t_audioThreadData->iFrames)
		{
			DEBUGPRINT(DEBUG_ERROR, "short read:%d, required:%d\n", iRet, (int)t_audioThreadData->iFrames);//, snd_strerror(iRet)
			usleep(10000);
				continue;
		}

		//Test Reboot Code
//		iAudioNeedRebootIndex++;
////		DEBUGPRINT(DEBUG_INFO,"iAudioNeedRebootIndex==============================================%d\n",iAudioNeedRebootIndex);
//		if(iAudioNeedRebootIndex == 160000){
//			printf("Send Reboot\n");
//			struct HandleAudioMsg *audioMsgSend=(struct HandleAudioMsg *)t_audioThreadData->charAudioMsgSend;
//			audioMsgSend->type = AUDIO_MSG_TYPE;
//			audioMsgSend->cmd = MSG_AUDIO_P_REBOOT;
//			audioMsgSend->len = 0;
//			int iRet = 0;
//			iRet = msg_queue_snd(t_audioThreadData->childProcessMsgID,audioMsgSend,sizeof(MsgData)-sizeof(long int)+audioMsgSend->len);
//			printf("Send Reboot=========%d\n",iRet);
//		}
		/////////


		nAlarmAudioBuf[byteAlarmBufIndex][iAlarmBufCount] = *(short*)pszDataBuffer;
		++iAlarmBufCount;
		if(AUDIO_ALARM_DATA_LEN <= iAlarmBufCount)
		{
#ifdef _RECOARD_TEST_
			FILE *file = fopen("/mnt/ref.pcm","a+");
			fwrite((char*)nAlarmAudioBuf[byteAlarmBufIndex],2048,1,file);
			fclose(file);
#endif
			byteAlarmBufIndex++;
			if(byteAlarmBufIndex == 5){
				byteAlarmBufIndex=0;
			}
			iAlarmBufCount = 0;
		}
#ifndef _ADT_AUDIO
		if(isEncode == ture_Audio_Encode){
			AdpcmEncode(*(short*)pszDataBuffer, i, szAudioBuf[byteWrite]+40+(i>>1), &audioiPreSample, &audioindexl);
		}
#endif
//		if(iAudioStoreFlag == 1){
//		if(pcmFileFD != NULL){
//			int iRet = fwrite(pszDataBuffer,2,1,pcmFileFD);
//			if(iRet<0){
//				DEBUGPRINT(DEBUG_INFO,"write pcm Failed\n");
//			}
//		}
//		}
	}
	//消回声
#ifdef _ADT_AUDIO
	short audio_tmp[320];
	memcpy(adt_audiodata, audio_tmp, sizeof(audio_tmp));
	audio_filter(audio_tmp, adt_audiodata);
	int adt_index = 0;
	for(;adt_index < 320; ++adt_index) {
		if(isEncode == ture_Audio_Encode){
			AdpcmEncode(*(short*)(adt_audiodata + adt_index), adt_index, szAudioBuf[byteWrite]+40+(adt_index>>1), &audioiPreSample, &audioindexl);
		}
	}
#endif
	iAudioNeedRebootIndex = 0;
	if(isEncode == ture_Audio_Encode){
		byteNewestFrame = byteWrite;
//		DEBUGPRINT(DEBUG_INFO,"Capture One byteWrite==%i=====byteNewestFrame===%i=====\n",byteWrite,byteNewestFrame);
		++byteWrite;
		if(AUDIO_BUFFER_SIZE <= byteWrite)
		{
			byteWrite = 0;
		}

//		if (adpcmFileFD != NULL){
//			int iRet = fwrite(szAudioBuf[byteNewestFrame]+40,160,1,adpcmFileFD);
//			if(iRet<0){
//				DEBUGPRINT(DEBUG_INFO,"write adpcm Failed\n");
//			}
//		}

		//Add by zhangli 2013.5.25
		iWriteCount++;
		if(iWriteCount>=AUDIO_MAX_INDEX){
			DEBUGPRINT(DEBUG_INFO,"++++++++++++++++++++++++++++++++++++++++++Audio is running+++++++++++++++++++++++++++++++++\n");
			iWriteCount = 0;
		}
//		DEBUGPRINT(DEBUG_INFO,"byteWrite==%d byteNewestFrame==%d iWriteCount==%d\n",byteWrite,byteNewestFrame,iWriteCount);
	}

//	if(iAudioStoreFlag == 1){

//	}

	return 0;
}

int DoStartAudioCapture(struct AudioThreadData *t_audioThreadData, snd_pcm_hw_params_t *pParams){
	int iRet = 0;
	int iOpenTimeCount = iAudioOpenIndex%50;
	if(iOpenTimeCount == 0){
		iAudioOpenTimes++;
		DEBUGPRINT(DEBUG_INFO,"Audio Sem Before Wait====================\n");
//		sem_wait(&AVSemaphore);
		DEBUGPRINT(DEBUG_INFO,"Audio Sem After Wait====================\n");
		iRet = StartAudioCapture(t_audioThreadData,pParams);
		DEBUGPRINT(DEBUG_INFO,"Audio Sem Before post====================\n");
//		sem_post(&AVSemaphore);
		DEBUGPRINT(DEBUG_INFO,"Audio Sem After Wait====================\n");
		if(iRet == 0){
			//Open Success
			iAudioOpenIndex = 0;
			m_audioState = AUDIO_RUNNING;
		}
		else{
			HandleAudioOpenError(t_audioThreadData, pParams, iRet);
			DEBUGPRINT(DEBUG_INFO,"Audio Open return ====%d\n",iRet);
		}
		DEBUGPRINT(DEBUG_INFO,"Audio Open Times ====%d\n",iAudioOpenTimes);
		struct HandleAudioMsg *sendMsg =(struct HandleAudioMsg *)t_audioThreadData->charAudioMsgSend;
		sendMsg->cmd = MSG_AUDIO_P_START_CAPTURE;
		sendMsg->data[0] = iRet;
		sendMsg->len = 1;
		sendMsg->type = AUDIO_MSG_TYPE;
		msg_queue_snd(t_audioThreadData->childProcessMsgID,sendMsg,sizeof(MsgData)-sizeof(long int)+sendMsg->len);
	}
	iAudioOpenIndex++;
	if(iAudioOpenIndex>99){
		iAudioOpenIndex = 0;
	}
	return 0;
}

int StartAudioCapture(struct AudioThreadData *t_audioThreadData, snd_pcm_hw_params_t *pParams)
{
	int iRet = 0;
	if(pParams == NULL)
	{
		DEBUGPRINT(DEBUG_ERROR,"pParams alloc Failed!\n");
		return -1;
	}
	iRet = InitAudioDevice(t_audioThreadData,&m_pHandle,&pParams);
	if(0 > iRet)
	{
		DEBUGPRINT(DEBUG_ERROR,"Audio Init Device Failed!\n");
		return iRet;
	}
	m_audioState=AUDIO_RUNNING;
	iAudioNeedRebootIndex=0;
	return 0;
}

int StopAudioCapture(struct AudioThreadData *t_audioThreadData, snd_pcm_hw_params_t *pParams){
	m_audioState=AUDIO_STOP;
	if(t_audioThreadData == NULL || m_pHandle == NULL ||pParams == NULL){
		return -1;
	}
	DEBUGPRINT(DEBUG_INFO,"Before Stop Audio==================\n");
	if(0 > snd_pcm_prepare((snd_pcm_t *)m_pHandle)){
		return -2;
		DEBUGPRINT(DEBUG_INFO,"snd_pcm_prepare Failed\n");
	}
	if(0 > snd_pcm_drain((snd_pcm_t *)m_pHandle)){
		DEBUGPRINT(DEBUG_INFO,"snd_pcm_drain Failed\n");
		return -3;
	}
	if(0 > snd_pcm_close((snd_pcm_t *)m_pHandle)){
		DEBUGPRINT(DEBUG_INFO,"snd_pcm_close Failed\n");
		return -4;
	}
	m_pHandle = NULL;
	memset(szAudioBuf,0,sizeof(szAudioBuf));
	memset(nAlarmAudioBuf,0,sizeof(nAlarmAudioBuf));
	byteWrite = 0;
	iAlarmBufCount = 0;
	byteAlarmBufIndex=0;
	byteNewestFrame = 0;
	iWriteCount = 0;
	DEBUGPRINT(DEBUG_INFO,"Audio Stopped Success===============================\n");
	return 0;
}

int GetNewestAudioFrame(char** pData, int* piIndex)
{
    if(NULL == m_pHandle)
    {
    	DEBUGPRINT(DEBUG_INFO,"=======fd===NULL\n");
        return -1;
    }
    if(isEncode == false_Audio_Encode ){
    	DEBUGPRINT(DEBUG_INFO,"=======isEncode===false\n");
    	return -1;
    }
    if(m_audioState != AUDIO_RUNNING){
    	DEBUGPRINT(DEBUG_INFO,"========m_audioState !=Running\n");
    	return -1;
    }
//    DEBUGPRINT(DEBUG_INFO,"========byteNewestFrame =%i\n",byteNewestFrame);
	*piIndex = byteNewestFrame;
	*pData = szAudioBuf[byteNewestFrame];//+40;

    return 0;
}

int GetAudioFrameWithWantedIndex(char** pData, int iWantIndex)
{
	if(NULL == m_pHandle)
	{
		DEBUGPRINT(DEBUG_INFO,"=======fd===NULL\n");
		*pData = NULL;
		return -1;
	}
	if(isEncode == false_Audio_Encode ){
		DEBUGPRINT(DEBUG_INFO,"=======isEncode===false\n");
		*pData = NULL;
		return -1;
	}
	if(m_audioState != AUDIO_RUNNING){
		DEBUGPRINT(DEBUG_INFO,"========m_audioState !=Running\n");
		*pData = NULL;
		return -1;
	}
	if((iWantIndex>=iWriteCount)){
		if((iWantIndex -iWriteCount) >= 2){
			DEBUGPRINT(DEBUG_INFO,"========we go back!!!===please go back !!!\n");
			*pData = NULL;
			return -3;
		}
		else{
//			DEBUGPRINT(DEBUG_INFO,"========you are faster than me please wait !\n");
			*pData = NULL;
			return -2;
		}
	}
	if(iWantIndex<iWriteCount){
		if((iWriteCount-iWantIndex)>=AUDIO_BUFFER_SIZE){
			DEBUGPRINT(DEBUG_INFO,"========you wanted data distance is more than I can reach !\n");
			*pData = NULL;
			return iWriteCount-iWantIndex;
		}
		else{
			int retIndex = iWantIndex%AUDIO_BUFFER_SIZE;
//			DEBUGPRINT(DEBUG_INFO,"retIndex=%d========byteNewestFrame=%d==========iWantIndex=%d=====iWriteCount=%d\n",retIndex,byteNewestFrame,iWantIndex,iWriteCount);
			*pData = szAudioBuf[retIndex];//+40;
			return iWriteCount-iWantIndex;
		}
	}
	DEBUGPRINT(DEBUG_INFO,"========Get audio frame can not come here !\n");
	return 0;
}

int GetAlarmAudioFrame(char** pData, int* piIndex)
{
    if(NULL == m_pHandle)
    {
        return -1;
    }
    if(m_audioState != AUDIO_RUNNING){
    	return -1;
    }
	*piIndex = byteAlarmBufIndex-1;
	if(*piIndex < 0){
		*piIndex += AUDIO_ALARM_BUF_SIZE;
	}
	*pData = (char*)nAlarmAudioBuf[*piIndex];
    return 0;
}

int InitAudioDevice(struct AudioThreadData *t_audioThreadData, snd_pcm_t** ppHandle, snd_pcm_hw_params_t** ppParams)
{
	DEBUGPRINT(DEBUG_INFO, "Come in InitAudioDevice111111111111\n");

    unsigned int iChannels = 1;
    unsigned int iSampleRate = 8000;
    int iDir = 0;

    snd_pcm_uframes_t iFrames;

    if( NULL == ppHandle || NULL == ppParams || NULL == t_audioThreadData)
    {
    	DEBUGPRINT(DEBUG_INFO, "InitAudioDevice NULL Params\n");
		return -2;
    }

    //open audio device
    DEBUGPRINT(DEBUG_INFO, "Come in InitAudioDevice222222222222222\n");
//    FILE *audioFileFd = popen("ls -l /dev/default:0","r");
//    if(audioFileFd != NULL){
//    	char string[256];
//		fscanf(audioFileFd,"%s",string);
//		DEBUGPRINT(DEBUG_INFO, "/Dev info=%s\n",string);
//		fclose(audioFileFd);
//    }

    int iRet = snd_pcm_open(ppHandle, "default:0", SND_PCM_STREAM_CAPTURE, 0);//"plughw:0,0"  "default"
    DEBUGPRINT(DEBUG_INFO, "Open Result ===%d\n",iRet);
    if (iRet < 0)
	{
        DEBUGPRINT(DEBUG_ERROR, "Unable to open pcm device\n");
        return -3;
    }
    DEBUGPRINT(DEBUG_INFO, "Come in InitAudioDevice333333333333333\n");
    /* Fill it in with default values. */
    iRet = snd_pcm_hw_params_any(*ppHandle, *ppParams);
    if (iRet < 0)
    {
       DEBUGPRINT(DEBUG_ERROR, "Unable to ppParams init\n");
       return -4;
    }
    DEBUGPRINT(DEBUG_INFO, "Come in InitAudioDevice444444444444444\n");
    /* Set the desired hardware parameters. */

    /* Interleaved mode */
    iRet = snd_pcm_hw_params_set_access(*ppHandle, *ppParams, SND_PCM_ACCESS_RW_INTERLEAVED); //SND_PCM_ACCESS_RW_NONINTERLEAVED

    if(iRet < 0){
    	DEBUGPRINT(DEBUG_ERROR, "Unable to snd params set access\n");
    	return -5;
    }
    DEBUGPRINT(DEBUG_INFO, "Come in InitAudioDevice5555555555555555555\n");
	//snd_pcm_hw_params_set_avail_min(handle, params, 4096);

    /* Signed 16-bit little-endian format */
    iRet = snd_pcm_hw_params_set_format(*ppHandle, *ppParams, SND_PCM_FORMAT_S16_LE); //SND_PCM_FORMAT_IMA_ADPCM
    if(iRet < 0){
        DEBUGPRINT(DEBUG_ERROR, "Unable to snd params set format\n");
        return -6;
    }
    DEBUGPRINT(DEBUG_INFO, "Come in InitAudioDevice66666666666666\n");
    /* Two channels (stereo) */
    iRet = snd_pcm_hw_params_set_channels(*ppHandle, *ppParams, iChannels);
    if(iRet < 0){
    	DEBUGPRINT(DEBUG_ERROR, "Unable to snd params set channels\n");
    	return -7;
    }

    DEBUGPRINT(DEBUG_INFO, "Come in InitAudioDevice77777777777777\n");
    /* rate bits/second sampling rate (CD quality) */
    iRet = snd_pcm_hw_params_set_rate_near(*ppHandle, *ppParams, &iSampleRate, &iDir);
    if(iRet < 0){
    	DEBUGPRINT(DEBUG_ERROR, "Unable to snd params set rate\n");
    	return -8;
    }
    DEBUGPRINT(DEBUG_INFO, "Come in InitAudioDevice8888888888888888\n");
    /* Set period size to 32 frames. */
    //modified by zhangpeng 20130307
    //iFrames = 32;
    iFrames = 1;

	DEBUGPRINT(DEBUG_INFO, "1 set_period_size_near iFrames:%d\n", (int)iFrames);
	iRet = snd_pcm_hw_params_set_period_size_near(*ppHandle, *ppParams, &iFrames, &iDir);
	if(iRet < 0){
		DEBUGPRINT(DEBUG_ERROR, "Unable to snd params set period size\n");
		return -9;
	}
	DEBUGPRINT(DEBUG_INFO, "2 set_period_size_near iFrames:%d, dir:%d\n", (int)iFrames, iDir);

    /* Write the parameters to the driver */
    iRet = snd_pcm_hw_params(*ppHandle, *ppParams);
    if (iRet < 0)
	{
        DEBUGPRINT(DEBUG_ERROR, "unable to set hw parameters, in file:%s line:%d\n", __FILE__, __LINE__);
        return -10;
    }
    //modified by zhangpeng 30140307
	/*iRet = snd_pcm_hw_params_get_period_size(*ppParams, &iFrames, &iDir);
	if(iRet < 0){
		DEBUGPRINT(DEBUG_ERROR, "Unable to snd params get period size\n");
		return -11;
	}
	DEBUGPRINT(DEBUG_INFO, "get_period_size frames:%d\n", (int)iFrames);
	if(1 > iFrames)
	{
		DEBUGPRINT(DEBUG_ERROR, "Frames:%d is too small, in file:%s line:%d\n",(int)iFrames, __FILE__, __LINE__);
		return -12;
	}
	t_audioThreadData->iFrames=iFrames;*/
    t_audioThreadData->iFrames = 1;

    return 0;
}

int CloseFDBecauseOfError()
{
	if(0 > snd_pcm_prepare((snd_pcm_t *)m_pHandle)){
		DEBUGPRINT(DEBUG_INFO,"snd_pcm_prepare Failed\n");
		return -1;
	}
	if(0 > snd_pcm_drain((snd_pcm_t *)m_pHandle)){
		DEBUGPRINT(DEBUG_INFO,"snd_pcm_drain Failed\n");
		return -2;
	}
	if(0 > snd_pcm_close((snd_pcm_t *)m_pHandle)){
		DEBUGPRINT(DEBUG_INFO,"snd_pcm_close Failed\n");
		return -3;
	}
	m_pHandle = NULL;
	memset(szAudioBuf,0,sizeof(szAudioBuf));
	memset(nAlarmAudioBuf,0,sizeof(nAlarmAudioBuf));
	byteWrite = 0;
	iAlarmBufCount = 0;
	byteAlarmBufIndex=0;
	byteNewestFrame = 0;
	return 0;
}

int HandleAudioOpenError(struct AudioThreadData *t_audioThreadData, snd_pcm_hw_params_t *pParams, int errorCode){
	DEBUGPRINT(DEBUG_INFO,"Audio Open Error Code = %d",errorCode);
	switch (errorCode){
	case -1:{
		m_audioState = AUDIO_ERROR;
		}
		break;
	case -2:{
		m_audioState = AUDIO_ERROR;
		}
		break;
	case -3:{
			if(iAudioOpenTimes == 4){
				m_audioState = AUDIO_ERROR;
				iAudioOpenTimes = 0;
				//Notify MFI ReDo
				struct HandleAudioMsg *audioMsgSend=(struct HandleAudioMsg *)t_audioThreadData->charAudioMsgSend;
				audioMsgSend->type = AUDIO_MSG_TYPE;
				audioMsgSend->cmd = MSG_AUDIO_P_RESWITCH_USB;
				audioMsgSend->len = 0;
				msg_queue_snd(t_audioThreadData->childProcessMsgID,audioMsgSend,sizeof(MsgData)-sizeof(long int)+audioMsgSend->len);
			}
		}
		break;
	case -4:{
		m_audioState = AUDIO_ERROR;
		CloseFDBecauseOfError();
		}
		break;
	case -5:{
		m_audioState = AUDIO_ERROR;
		CloseFDBecauseOfError();
		}
		break;
	case -6:{
		m_audioState = AUDIO_ERROR;
		CloseFDBecauseOfError();
		}
		break;

	case -7:{
		m_audioState = AUDIO_ERROR;
		CloseFDBecauseOfError();
		}
		break;
	case -8:{
		m_audioState = AUDIO_ERROR;
		CloseFDBecauseOfError();
		}
		break;
	case -9:{
		m_audioState = AUDIO_ERROR;
		CloseFDBecauseOfError();
		}
		break;
	case -10:{
		CloseFDBecauseOfError();
		}
		break;
	case -11:{
		m_audioState = AUDIO_ERROR;
		CloseFDBecauseOfError();
		}
		break;
	case -12:{
		m_audioState = AUDIO_ERROR;
		CloseFDBecauseOfError();
		}
		break;
	default:
		m_audioState = AUDIO_ERROR;
		break;
	}
	return 0;
}

