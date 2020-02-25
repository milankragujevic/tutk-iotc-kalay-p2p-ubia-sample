/*
 * VideoSend.h
 *
 *  Created on: Mar 20, 2013
 *      Author: root
 */

#ifndef VIDEOSEND_H_
#define VIDEOSEND_H_

#define HEADER_WORD         "IBBM"
#define CMD_VERSION         1	//! 0:慧眼版本 1:iBaby M3S版本


typedef struct
{
	char strHead[4];
	short int nCmd;
	char  cVersion;
	char strReserve1[8]; // bao liu
	int  iDataLen;
	char strReserve2[4]; //!bao liu
}__attribute__ ((packed)) MsgHead;


typedef  enum
{
		VIDEO_SEND_STARTING=0,
		MSG_VIDEOSEND_P_VIDEO_SEND,
		MSG_VIDEOSEND_P_VIDEO_STOP
	//	AUDIO_SEND_ERROR

}VIDEO_SEND_THREAD_STATE;
//volatile  VIDEO_SEND_THREAD_STATE   videoSendThreadState;

extern int Creatvideo_send_thread();

#endif /* VIDEOSEND_H_ */
