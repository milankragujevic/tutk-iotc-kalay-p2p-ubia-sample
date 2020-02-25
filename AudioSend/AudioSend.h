/*
 * AudioSend.h
 *
 *  Created on: Mar 20, 2013
 *      Author: root
 */

#ifndef AUDIOSEND_H_
#define AUDIOSEND_H_

#define HEADER_WORD         "IBBM"
#define CMD_VERSION         1	//! 0:慧眼版本 1:iBaby M3S版本


#define		AUDIO_DATA_LEN 		160
typedef struct
{
	char strHead[4];
	short int nCmd;
	char  cVersion;
	char strReserve1[8]; //!保留
	int  iDataLen;
	char strReserve2[4]; //!保留
}__attribute__ ((packed)) MsgHeadaudio;

typedef  enum
{
		AUDIO_SEND_STARTING=0,
		MSG_AUDIOSEND_P_AUDIO_SEND,
		MSG_AUDIOSEND_P_AUDIO_STOP,
		MSG_AUDIOSEND_T_AUDIO_ONE_STOP,
	//	AUDIO_SEND_ERROR

}AUDIO_SEND_THREAD_STATE;

//volatile  AUDIO_SEND_THREAD_STATE   audioSendThreadState;
//int AudioSetMsgHead(char cCmdVersion, short int nCmd, MsgHead * pHead, int iMsgLen);
//int AudioMsgBody(int iSocket, void* pDataBody, int iBodyLen, int iTimeOut);


extern int Creataudio_send_thread();


#endif /* AUDIOSEND_H_ */
