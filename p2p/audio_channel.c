#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "tutk_rdt_video_server.h"

//extern volatile int iAudioStoreFlag;
/**
 * @brief 初始化音频协议头的数据
 * @author <王志强>
 * ＠param[in]<nCmd><发送音频数据的命令>
 * ＠param[in]<pHead><音频数据的首地址>
 * @return <0: 成功>
 *
 * */
int set_amsg_head(short int nCmd, RequestHead *pHead)
{
	if (NULL == pHead) {
		return -1;
	}
	memset(pHead, 0, sizeof(RequestHead));
	memcpy(pHead->head, ICAMERA_NETWORK_HEAD, sizeof(ICAMERA_NETWORK_HEAD));
	pHead->version = ICAMERA_NETWORK_VERSION;
	pHead->cmd = nCmd;
	pHead->len = sizeof(AudioData)-sizeof(RequestHead);

	return 0;
}

/**
 * @brief 音频通道线程
 * 用于向apps发送音频数据或者接收apps的数据，方向是在控制通道控制的
 * @author <王志强>
 * ＠param[in]<arg><用户的变量>
 *
 * */
void *audiochannel_rdt_thread(void *arg)
{
	int i = (int)arg/4;
	int j = (int)arg%4;
	struct tutk_iotc_session *s = &tutk_session[i];
	int icur_audio_index = 0;
	AudioData audio_data, speaker_data;
	char *paudio_data;
	int ret, iRet, Ret;
	int speaker_count = 0;

//	FILE *fp = fopen("/mnt/WZQsound.adpcm", "w");
//	if(fp == NULL){
//		printf("audiochannel_rdt_thread: fopen fail\n");
//	}
	int curTimes = 200;

	DEBUGPRINT(DEBUG_INFO, "tutk create audio thread: %d %d\n", i, j);
	memset(&audio_data, 0, sizeof(audio_data));
	set_amsg_head(304, &audio_data.head);
	audio_data.iDataLen = 160;
	//iAudioStoreFlag = 1;
	while(s->ch[j].rdt_used){
		if (tutk_session[i].audio_usb_insert == USB_INSERT) {
			tutk_session[i].audio_stop = AV_CHANNEL_STOP;
			usleep(30000);
			continue;
		}

		if(s->close_audio_ch == TUTK_AUDIO_OFF){
			usleep(30000);
			continue;
		}

		if(s->audio_dir_change == TUTK_AUDIO_DIR_CH){
			s->audio_dir_change = TUTK_AUDIO_DIR_UNCH;
			speaker_count = 1;
			audio_data.iSerialNum = 0;
		}

		if (s->audio_direction == TUTK_CAMERA_TO_MOBILE) {
			iRet = tutk_check_a_buf_size(i);
			if (iRet == 0) {
				while (1) {
					//send
					//DEBUGPRINT(DEBUG_INFO, "audiochannel_rdt_thread: .............%d\n", ret);
					ret = GetAudioFrameWithWantedIndex(&paudio_data, icur_audio_index);
					if (ret == -3) {
						icur_audio_index = 0;
						continue;
					} else if (ret == -2) {
						break;
					} else if (ret == -1) {
						break;
					} else if ((0 <= ret)  && (ret < AUDIO_BUFFER_SIZE-1)) {
						icur_audio_index++;
//						DEBUGPRINT(DEBUG_INFO, "========================Send====Index====%d==========================\n", icur_audio_index);
					} else if ((AUDIO_BUFFER_SIZE-1 <= ret) && (ret < AUDIO_MAX_INDEX)) {
						icur_audio_index += ret;
						continue;
					}
					if(paudio_data == NULL){
						break;
					}
					
					//fwrite(paudio_data + 40, 160, 1, fp);
					memcpy(audio_data.buf, paudio_data + 40, sizeof(audio_data.buf));
					tutk_send_vast(s->ch[j].rdt_id,
							(char *) &audio_data, sizeof(audio_data));
					tutk_msg.audio_val = 10 * 4096; //40K
					audio_data.iSerialNum++;
				}
			} else if (iRet == -1) {
				//Dont send
				while (1) {
					ret = GetAudioFrameWithWantedIndex(&paudio_data,
							icur_audio_index);
					if (ret == -3) {
						icur_audio_index = 0;
						continue;
					} else if (ret == -2) {

					} else if (ret == -1) {

					} else if ((0 <= ret) && (ret <= AUDIO_BUFFER_SIZE-1)) {
						icur_audio_index++;
						//						DEBUGPRINT(DEBUG_INFO, "========================Send====Index====%d==========================\n", icur_audio_index);
					} else if ((AUDIO_BUFFER_SIZE-1 < ret) && (ret < AUDIO_MAX_INDEX)) {
						icur_audio_index += ret;
						continue;
					}
					tutk_msg.audio_val = 4096; //4K
					break;
				}
			} else if (iRet > 0) {
				if (iRet - 1 == i) {
					//disconnect, thread exit
					tutk_log_s("video send exceptional");
					break;
				}
			}
			usleep(30000);
		} else if (s->audio_direction == TUTK_MOBILE_TO_CAMERA) { //
			Ret = tutk_recv_data_timeout(s->ch[j].rdt_id, (char *)&speaker_data, 200, 1000);
			if (Ret < 0 && Ret == RDT_ER_TIMEOUT) {
				continue;
			} else if (Ret < 0) {
				printf("audiochannel_rdt_thread: speaker %d\n", Ret);
				tutk_log_s("speaker exceptional");
				break;
			}
			if(speaker_count == 1 && speaker_data.iSerialNum != 0){
				continue;
			}
			speaker_count = 0;

			if(++curTimes > 100) {
				curTimes = 0;
				DEBUGPRINT(DEBUG_INFO, "++++>PlaySpeaker(audio_data.buf);\n");
			}

			PlaySpeaker(speaker_data.buf);
			usleep(30000);
		}
	}	
//	iAudioStoreFlag = 0;
//	fclose(fp);

	s->ch[j].rdt_used = RDT_USED_WAIT_EXIT;
	set_session_exit_state(i);
	IOTC_Session_Close(s->session_id);
	RDT_Destroy(s->ch[j].rdt_id);
	tutk_log_s("audiochannel_rdt_thread exit");
	DEBUGPRINT(DEBUG_INFO, "audiochannel_rdt_thread --------------------> session:%d, channel:%d exit\n", i, j);
	pthread_exit(NULL);
}
