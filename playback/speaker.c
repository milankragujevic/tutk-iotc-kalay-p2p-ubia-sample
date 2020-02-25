/*
 * speaker.c
 *
 *  Created on: Mar 18, 2013
 *      Author: root
 */
#include <stdlib.h>
#include <string.h>
#include "common/thread.h"
#include "speaker.h"
#include "Audio/Audio.h"
#include "common/logfile.h"
#include "common/common.h"
#include "driver/I2S/i2s_ctrl.h"
#include "common/msg_queue.h"
#include "SerialPorts/SerialPorts.h"

#include <fcntl.h>
#include <stdio.h>

#define DELAYTIME 4

#define SPEAKER_DATA_CACHE_SIZE 1

#if 0
#define TEST_PRINT(format, arg...)\
	DEBUGPRINT(DEBUG_INFO, format, ##arg)
#else
#define TEST_PRINT(format, arg...)
#endif

struct SpeakerMsgContainer{
	int speakerThreadMsgID;
	int childProcessMsgID;
	char msgRecv[1024];
	char msgSend[1024];
};

volatile int m_fdI2S = -1;
unsigned char m_szAudioBuffer[SPEAKER_BUF_SIZE*4096];
volatile volatile int m_iPlayIndex = 0;


volatile int m_iAudioBufIndex = 0;
volatile int m_iDataCount = 0;
volatile int m_iBufUsedCount = 0;
volatile int m_iRecvDataIndex = 0;
volatile int m_iPreSample = 0;
volatile int m_iDecodeIndex = 0;
volatile int m_iEmptyCount = 0;

volatile int iRebootFlag = 0;

volatile FILE *openSound = NULL;
volatile char openSoundBuffer[4096];
volatile int PlayOpenSoundFlag = 0;


/*
 * @brief 开始声音播放线程
 * @author:张历
 * @param[in]:<arg><线程启动参数>
 * @param[out]:<无参数>
 * @Return <-1：失败；其他：线程ID ； >
 */
void *StartSpeakerThread(void *arg);

/*
 * @brief 打开喇叭设备
 * @author:张历
 * @param[in]:<无参数>
 * @param[out]:<无参数>
 * @Return <0：成功；-1：失败；>
 */
int OpenSpeaker();

/*
 * @brief 放音线程的参数初始化
 * @author:张历
 * @param[in]:<msgContainer><线程参数的封装结构>
 * @param[out]:<无参数>
 * @Return <成功：0 ；失败：-1>
 */
int SpeakerThreadInit(struct SpeakerMsgContainer *msgContainer);

/*
 * @brief 放音线程的线程同步
 * @author:张历
 * @param[in]:<无参数>
 * @param[out]:<无参数>
 * @Return <无参数>
 */
void SpeakerThreadSync();

/*
 * @brief 放音线程的消息处理函数
 * @author:张历
 * @param[in]:<msgContainer><线程参数的封装结构>
 * @param[out]:<无参数>
 * @Return <无参数>
 */
void HandleSpeakerMsg(struct SpeakerMsgContainer *msgContainer);

/*
 * @brief 关闭喇叭设备
 * @author:张历
 * @param[in]:<无参数>
 * @param[out]:<无参数>
 * @Return <无参数>
 */
int CloseSpeaker();

/*
 * @brief 播放开机音乐
 * @author:张历
 * @param[in]:<无参数>
 * @param[out]:<无参数>
 * @Return <无参数>
 */
void PlayOpenSound();

extern inline void AdpcmDecode(unsigned char* raw, int len, unsigned char* decoded, int* pre_sample, int* index, int* iBufIndex, int iMaxIndex);

int CreateSpeakerThread(){
	return thread_start(StartSpeakerThread);
}

void *StartSpeakerThread(void *arg){

	unsigned char szEmptyAudioBuf[4096];
	memset(szEmptyAudioBuf, 0, sizeof(szEmptyAudioBuf));
	struct SpeakerMsgContainer *msgContainer=malloc(sizeof(struct SpeakerMsgContainer));
	if(msgContainer == NULL){
		return NULL;
	}
	SpeakerThreadInit(msgContainer);
	printf("11111111111111111111111111111111111111111111111111111111111111111111111\n");

	SpeakerThreadSync();
	DEBUGPRINT(DEBUG_INFO,"====After Sync Play Empty \n");

	//Add Open Sound
	PlayOpenSoundFlag = 1;
	PlayOpenSound();
	PlayOpenSoundFlag = 0;
	//

	m_iPlayIndex = 0;
	m_iAudioBufIndex = 0;
	m_iDataCount = 0;
	m_iBufUsedCount = 0;
	m_iRecvDataIndex = 0;
	m_iPreSample = 0;
	m_iDecodeIndex = 0;
	m_iEmptyCount = 0;
	iRebootFlag = 0;
	unsigned int unLoops = 0;

	while(1){
		unLoops++;
		g_nSpeakerFlag3++;
		if (999 == g_nSpeakerFlag3)
		{
			g_nSpeakerFlag3 = 0;
		}
		HandleSpeakerMsg(msgContainer);
		child_setThreadStatus(PLAY_BACK_MSG_TYPE, NULL);
		if(iRebootFlag == 0){
			if(m_iAudioBufIndex == m_iPlayIndex){
				m_iEmptyCount++;
				if(m_iEmptyCount == 2){
					g_nSpeakerFlag1 = unLoops & 0xffff;
//					printf("||||||||||||||||||||||||||||||||||||||||||||||||||||||------\n");
					ioctl(m_fdI2S, I2S_PUT_AUDIO, szEmptyAudioBuf);
//					printf("|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||\n");
					g_nSpeakerFlag1 = (unLoops & 0xffff) | 0x10000;
					m_iEmptyCount = 0;
				}
				continue;
			}
			else{
				int i = 0;
				for(i=0;i<SPEAKER_DATA_CACHE_SIZE;i++){
					g_nSpeakerFlag2 = unLoops & 0xffff;;
					ioctl(m_fdI2S, I2S_PUT_AUDIO, m_szAudioBuffer + m_iPlayIndex*4096*SPEAKER_DATA_CACHE_SIZE + i*4096);
					g_nSpeakerFlag2 = (unLoops & 0xffff) | 0x10000;;
				}
				++m_iPlayIndex;
				if((SPEAKER_BUF_SIZE/SPEAKER_DATA_CACHE_SIZE) <= m_iPlayIndex)
				{
					m_iPlayIndex = 0;
				}
				if((m_iAudioBufIndex - m_iPlayIndex*SPEAKER_DATA_CACHE_SIZE) > 40){
					m_iPlayIndex = m_iAudioBufIndex/SPEAKER_DATA_CACHE_SIZE;
					//printf("===================m_iAudioBufIndex=%d======m_iPlayIndex=%d==========================================Jump jump\n",m_iAudioBufIndex,m_iPlayIndex);
				}
			}
		}
		usleep(20000);
		/*
		else if((m_iAudioBufIndex - m_iPlayIndex)<=DELAYTIME || (m_iAudioBufIndex - m_iPlayIndex)<=DELAYTIME-SPEAKER_BUF_SIZE){
//			TEST_PRINT("====Play Audio Play  =======================again\n");
			m_iEmptyCount = 0;
			//yuan ma
//			ioctl(m_fdI2S, I2S_PUT_AUDIO, m_szAudioBuffer + m_iPlayIndex*4096);

			//////////////////////////
			/// xin ma
			int i = 0;
			printf("PlaySound=====================\n");
			for(i=0;i<10;i++){
				ioctl(m_fdI2S, I2S_PUT_AUDIO, m_szAudioBuffer + m_iPlayIndex*40960 + i*4096);
			}
			//////////////////////////////////////
			++m_iPlayIndex;
			if(10 <= m_iPlayIndex)
			{
				m_iPlayIndex = 0;
			}
		}
		else{
			printf("Else else =======================\n");
			if((m_iAudioBufIndex-m_iPlayIndex)>DELAYTIME){
				m_iPlayIndex = m_iAudioBufIndex-DELAYTIME;
				TEST_PRINT("m_iAudioBufIndex>DELAYTIME==========================================\n");
			}
			else if(((m_iAudioBufIndex - m_iPlayIndex)<=DELAYTIME-SPEAKER_BUF_SIZE)){
//				m_iPlayIndex = m_iAudioBufIndex-DELAYTIME;
				TEST_PRINT("else else===========================================\n");
			}

		}
	*/
	}

	CloseSpeaker();

	return NULL;
}

int OpenSpeaker(){
	if(m_fdI2S != -1){
		return -1;
	}
	m_fdI2S = open("/dev/i2s0", O_RDWR|O_SYNC);
	if( m_fdI2S < 0 )
	{
		return -1;
	}
	ioctl(m_fdI2S, I2S_SRATE, 8000);
	ioctl(m_fdI2S, I2S_TX_VOL, 1);
	ioctl(m_fdI2S, I2S_TX_ENABLE, 0);
	return 0;
}

int SpeakerThreadInit(struct SpeakerMsgContainer *msgContainer){
	if(OpenSpeaker() != 0){
		DEBUGPRINT(DEBUG_ERROR,"======Open Speaker Failed====\n");
		return -1;
	}
	msgContainer->childProcessMsgID=msg_queue_get(CHILD_PROCESS_MSG_KEY);
	msgContainer->speakerThreadMsgID=msg_queue_get(CHILD_THREAD_MSG_KEY);
	return 0;
}

void SpeakerThreadSync(){
	rplayback = 1;
	while(rChildProcess != 1){
		thread_usleep(0);
	}
	thread_usleep(0);
}

void HandleSpeakerMsg(struct SpeakerMsgContainer *msgContainer){
//	printf("===========================================================speaker===================alive\n");
	if(msg_queue_rcv(msgContainer->speakerThreadMsgID, msgContainer->msgRecv, 1024, PLAY_BACK_MSG_TYPE) != -1){
		MsgData *msgR=(MsgData *)msgContainer->msgRecv;
		MsgData *msgS=(MsgData *)msgContainer->msgSend;
		DEBUGPRINT(DEBUG_INFO,"=================================Speaker================msgR->cmd=============%d=========================\n",msgR->cmd);
		switch(msgR->cmd){
			case MSG_SPEAKER_P_REBOOT:{
				DEBUGPRINT(DEBUG_INFO,"   ********    *********   *********         ****        ****      ***************\n");
				DEBUGPRINT(DEBUG_INFO,"   *       *   *           *         *     *      *    *     *            *       \n");
				DEBUGPRINT(DEBUG_INFO,"   *       *   *           *         *    *        *  *        *          *       \n");
				DEBUGPRINT(DEBUG_INFO,"   ********    *********   *********      *        *  *        *          *       \n");
				DEBUGPRINT(DEBUG_INFO,"   *     *     *           *         *    *        *  *        *          *       \n");
				DEBUGPRINT(DEBUG_INFO,"   *      *    *           *         *     *      *     *    *            *       \n");
				DEBUGPRINT(DEBUG_INFO,"   *       *   *********   *********         ****        ****             *       \n");
				iRebootFlag = 1;
				DEBUGPRINT(DEBUG_INFO,"before close speaker!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
				CloseSpeaker();
				DEBUGPRINT(DEBUG_INFO,"after close speaker!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
				msgS->cmd=MSG_SPEAKER_P_REBOOT;
				msgS->type=PLAY_BACK_MSG_TYPE;
				msgS->len=0;
				msg_queue_snd(msgContainer->childProcessMsgID,msgContainer->msgSend,sizeof(MsgData)-sizeof(long int));
//				DEBUGPRINT(DEBUG_INFO,"come reboot!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
				sleep(1);
				//serialport_sendAuthenticationFailure();
				//system("reboot");
//				sleep(1);
//				DEBUGPRINT(DEBUG_INFO,"after reboot!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
//				DEBUGPRINT(DEBUG_INFO,"After reboot!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
			}
				break;
			case MSG_SPEAKER_P_CHECK_ALIVE:{
				DEBUGPRINT(DEBUG_INFO,"====Speaker Thread Recv Check Msg");
				msgS->cmd=MSG_SPEAKER_P_CHECK_ALIVE;
				msgS->type=PLAY_BACK_MSG_TYPE;
				msgS->len=0;
				msg_queue_snd(msgContainer->childProcessMsgID,msgContainer->msgSend,sizeof(MsgData)-sizeof(long int));
			}
				break;
			default:
				DEBUGPRINT(DEBUG_INFO,"Audio Unknow command=========================================\n");
				break;
		}
	}
}

int CloseSpeaker(){
	ioctl(m_fdI2S, I2S_TX_DISABLE, 0);
	if(m_fdI2S != -1){
		close(m_fdI2S);
	}
	m_fdI2S = -1;
	return 0;
}

void PlayOpenSound(){
	openSound = fopen("/home/111littile.wav","r");
	if(openSound == NULL){
		printf("Open File Failed\n");
		return;
	}
	else{
		printf("Open File Success\n");
	}
	while(fread(openSoundBuffer,4096,1,openSound)>0){
		printf("******************************************bo fang sheng ying!!!*************************\n");
		PlayMusic(openSoundBuffer);
	}
	printf("22222222222222222222222222222222222222222222222222222222222222222222222\n");
	printf("==============m_iAudioBufIndex===%d==================m_iPlayIndex===%d==\n",m_iAudioBufIndex,m_iPlayIndex);
	m_iPlayIndex = 0;
	while(m_iAudioBufIndex != m_iPlayIndex){
		int i = 0;
		for(i=0;i<SPEAKER_DATA_CACHE_SIZE;i++){
			ioctl(m_fdI2S, I2S_PUT_AUDIO, m_szAudioBuffer + m_iPlayIndex*4096*SPEAKER_DATA_CACHE_SIZE + i*4096);
			printf("gei qudong ====================================================\n");
		}
		++m_iPlayIndex;
		if((SPEAKER_BUF_SIZE/SPEAKER_DATA_CACHE_SIZE) <= m_iPlayIndex)
		{
			m_iPlayIndex = 0;
		}
	}
	if(fclose(openSound) < 0){
		printf("guan bi wen jian shi bai\n");
	}
	openSound = NULL;
	printf("333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333\n");
}

int PlaySpeaker(char* pszAudioDecode){
	if(PlayOpenSoundFlag == 0){
		if(m_fdI2S == -1){
			DEBUGPRINT(DEBUG_INFO,"===Did not Open Device===");
			return -1;
		}

		AdpcmDecode(pszAudioDecode, AUDIO_DATA_LEN, m_szAudioBuffer, &m_iPreSample, &m_iDecodeIndex, &m_iBufUsedCount, SPEAKER_BUF_SIZE*4096/2);

		m_iDataCount += 1280;//640;
		if(4096*SPEAKER_DATA_CACHE_SIZE <= m_iDataCount)
		{
			m_iDataCount -= 4096*SPEAKER_DATA_CACHE_SIZE;
			m_iAudioBufIndex = m_iRecvDataIndex;
	//		printf("m_iRecvDataIndex=======%d\n",m_iRecvDataIndex);
			++m_iRecvDataIndex;
			if((SPEAKER_BUF_SIZE/SPEAKER_DATA_CACHE_SIZE) <= m_iRecvDataIndex)
			{
				m_iRecvDataIndex = 0;
			}
		}

	//	DEBUGPRINT(DEBUG_INFO,"m_iAudioBufIndex============%i=========\n",m_iAudioBufIndex);
		return 0;
	}
	else{
		return -1;
	}

}
volatile int iMusicIndex=0;
int PlayMusic(char* buffer){
	if(m_fdI2S == -1){
		DEBUGPRINT(DEBUG_INFO,"===Did not Open Device===\n");
		return -1;
	}
	if(iMusicIndex >= 409600){
		iMusicIndex = 0;
		m_iAudioBufIndex = 0;
	}
	if(buffer == NULL){
		DEBUGPRINT(DEBUG_INFO,"===music buffer is null===\n");
		return -1;
	}
	memcpy(m_szAudioBuffer+iMusicIndex,buffer,4096);
	m_iAudioBufIndex++;
	iMusicIndex += 4096;
	return 0;
}

volatile char buffer[4096];

void *startAudioPlay(void *arg);

int CreateTestThread(){

	return thread_start(startAudioPlay);
}

void *startAudioPlay(void *arg){

	rAudioSend = 1;
	while(rChildProcess != 1){
		thread_usleep(0);
	}

	DEBUGPRINT(DEBUG_INFO,"=====================get and play Thread\n");
	thread_usleep(0);
	int myindex=0;
	char *pData =NULL;
	int iIndex = -1;
	int iRet = -1;
	DEBUGPRINT(DEBUG_INFO,"=====================get and play Thread Variable\n");

//	FILE *file=fopen("/mnt/record01","a+");
//	if(file == NULL){
//		printf("Da kai Lu ying wen jian shi bai \n");
//		return NULL;
//	}
	while(1){
		iRet = GetNewestAudioFrame(&pData,&iIndex);
		if(iRet == 0){
//			DEBUGPRINT(DEBUG_INFO,"=====come Index ==*iIndex=%i===index=%i\n",iIndex,myindex);
			if(myindex != iIndex){
//				DEBUGPRINT(DEBUG_INFO,"=====================get Index ===%i\n",iIndex);
				if(pData != NULL){
//					DEBUGPRINT(DEBUG_INFO,"Play Sound============\n");
					PlaySpeaker(pData);
//					int size;
//					size = fwrite(pData,160,1,file);
//					printf("writed size===%d",size);
				}
				myindex=iIndex;
				pData = NULL;
			}
			else{
				thread_usleep(20000);
			}
		}
		else{
			thread_usleep(20000);
		}
	}

	/* Add Open Music */
//	int fd = open("/home/music.wav",O_RDWR| O_NONBLOCK,0);
//	int iRet;
//	if(fd < 0){
//		printf("Da kai shi bai!!!\n");
//		return NULL;
//	}
//	while(read(fd,buffer,4096)>0){
//		printf("bo fang sheng ying!!!\n");
//		PlaySpeaker(&buffer);
//		usleep(300000);
//	}
/*
	FILE *file=fopen("/home/music.wav","r");
	int iRet;
	if(file == NULL){
		printf("Da kai shi bai!!!\n");
		return NULL;
	}
	printf("before bo fang!!!");
	while(fread(buffer,4096,1,file)>0){
		printf("bo fang sheng ying!!!\n");
//		PlaySpeaker(buffer);
	 	 PlayMusic(buffer);
		usleep(30000);
	}
	fclose(file);
*/
	return NULL;
}

