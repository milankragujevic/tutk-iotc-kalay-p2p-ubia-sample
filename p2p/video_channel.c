#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "tutk_rdt_video_server.h"

VideoData video_data;
/**
 * @brief 初始化视频协议头的数据
 * @author <王志强>
 * ＠param[in]<nCmd><发送视频数据的命令>
 * ＠param[in]<pHead><视频数据的首地址>
 * @return <0: 成功>
 *
 * */
int set_vmsg_head(short int nCmd, RequestHead *pHead) 
{
	if (NULL == pHead) {
		return -1;
	}
	memset(pHead, 0, sizeof(RequestHead));
	memcpy(pHead->head, ICAMERA_NETWORK_HEAD, sizeof(ICAMERA_NETWORK_HEAD));
	pHead->version = ICAMERA_NETWORK_VERSION;
	pHead->cmd = nCmd;

	return 0;
}

/**
 * @brief 视频通道线程
 * 用于向apps发送视频数据或者接收apps的数据
 * @author <王志强>
 * ＠param[in]<arg><用户的变量>
 *
 * */
void *videochannel_rdt_thread(void *arg)
{
	int i = (int)arg/4;
	int j = (int)arg%4;
	struct tutk_iotc_session *s = &tutk_session[i];
	int icur_video_index = 0;
	int prev_index = -1;
	int idata_len = 0;
	char *pvideo_data;
	int ret;
	int count = 0;
	int first_frame = 1;
	unsigned int send_num = 0;
	unsigned int time_st;

#define INTERVEL_TIME	30000//ms
	DEBUGPRINT(DEBUG_INFO, "tutk create video thread: %d %d\n", i, j);
	set_vmsg_head(400, &video_data.head);
	time_st = getTimeStamp();
	while(s->ch[j].rdt_used){
		if (tutk_session[i].video_usb_insert == USB_INSERT) {
			tutk_session[i].video_stop = AV_CHANNEL_STOP;
		} else {
			if (tutk_session[i].close_video_ch == TUTK_VIDEO_OFF) {
				//DEBUGPRINT(DEBUG_INFO, "videochannel_rdt_thread.......%d..%d.\n", i, j);
			} else {
#ifdef TUTK_DEBUG_LOG
				if(getTimeStamp() - time_st >= INTERVEL_TIME){
						TUTK_DEBUG("TUTK RDT: send picture %d, 30s\n", send_num);
						send_num = 0;
						time_st = getTimeStamp();
				}
#endif
				ret = tutk_check_v_buf_size(i);
				if (ret == 0) {
					if (count == 0) {
						count++;
						usleep(30000);
						continue;
					}
					count = 0;
					//send
					if (-1
							== GetNewestVideoFrame(&pvideo_data, &idata_len,
									&icur_video_index)) {
						printf(
								"GetNewestVideoFrame failed, in file:%s line:%d, %d\n",
								__FILE__, __LINE__, arg);
						usleep(30000);
						continue;
					}

					if (prev_index == icur_video_index) {
						ReleaseNewestVideoFrame(icur_video_index);
						usleep(30000);
						continue;
					}
					//printf("video ***********************%d*************\n", i);
					prev_index = icur_video_index;
					//memcpy(video_data.buf, pvideo_data, idata_len);
					video_data.head.len = idata_len + 9;
					video_data.iDataLen = idata_len;
					video_data.byteFormat = 1;
					if (video_data.head.len < 0) {
						DEBUGPRINT(DEBUG_INFO, "videochannel_rdt_thread: videoData.head.iDataLen < 0\n");
						continue;
					} else if (video_data.head.len + 23 >= 100 * 1024) {
						//DEBUGPRINT(DEBUG_INFO, "videochannel_rdt_thread: videoData.head.iDataLen + 23 >= 70 * 1024\n");
						continue;
					}

					tutk_send_vast(s->ch[j].rdt_id, (char*) &video_data,
							23 + 9);
					tutk_send_vast(s->ch[j].rdt_id, pvideo_data, idata_len);
					if(first_frame == 1){
#ifdef TUTK_DEBUG_LOG
						tutk_session[i].send_picture_time = getTimeStamp() - tutk_session[i].send_picture_time;
						TUTK_DEBUG("TUTK RDT: first, send picture time:%d\n", tutk_session[i].send_picture_time);
#endif
						first_frame = 0;
					}
					send_num++;
					ReleaseNewestVideoFrame(icur_video_index);
				} else if (ret == -1) {
					//Don't send
				} else if (ret > 0) { //返回值就是会话的ID+1
					if (ret - 1 == i) {
						tutk_log_s("audio send exceptional");
						//disconnect, thread exit
						break;
					}
				}
			}
		}
		usleep(30000);
	}

	s->ch[j].rdt_used = RDT_USED_WAIT_EXIT;
	set_session_exit_state(i);
	IOTC_Session_Close(tutk_session[i].session_id);
	RDT_Destroy(tutk_session[i].ch[j].rdt_id);
	tutk_log_s("videochannel_rdt_thread exit");
	DEBUGPRINT(DEBUG_INFO, "videochannel_rdt_thread --------------------> session:%d, channel:%d exit\n", i, j);
	pthread_exit(NULL);
}

