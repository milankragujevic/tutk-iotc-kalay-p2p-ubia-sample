#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "tutk_rdt_video_server.h"

#define CONTROL_WAIT_TIMEMS		500
/**
 * @brief 发送wifi信号函数
 * 比较系统时间，大于等于时间间隔才发送数据
 * @author <王志强>
 * ＠param[in]<n><会话数组的下标>
 * @return <0: 成功>
 *
 * */
int send_wifi_signal_func(int n)
{
	int i = (int)((int)n/4);
	int j = (int)n%4;
	struct tutk_iotc_session *s = (struct tutk_iotc_session *)&tutk_session[i];
	time_t now;

	now = time(NULL);
	if((now-s->wifi_time_stamp) >= s->wifi_time_interval){
		DEBUGPRINT(DEBUG_INFO, "send_wifi_signal_func: ........%d.....%d...\n", i, time(NULL));
		send_wifi_signal_msg(s->ch[j].rdt_id);
		s->wifi_time_stamp += s->wifi_time_interval;
	}

	return 0;
}

/**
 * @brief 处理控制通道消息函数
 * tutk只处理下面的消息，其他都发送到子进程中来处理
 * @author <王志强>
 * ＠param[in]<n><会话数组的下标>
 * ＠param[in]<msg><消息的首地址>
 * ＠param[in]<len><消息的长度>
 * @return <0: 成功>
 *
 * */
int tutk_ctrlchannel_msg_handler(int n, char *msg, int len)
{
	int i = (int)((int)n/4);
	int j = (int)n%4;
	RequestHead *head = (RequestHead *)msg;
	char *text = (char *)(head + 1);
	int rdt_id = tutk_session[i].ch[j].rdt_id;
	int ret;
	int dir;
	MsgData *msg_snd = (MsgData *)tutk_msg.msg_data_snd;

	tutk_log_s("username:%d, cmd:%d", tutk_session[i].user_name, head->cmd);
	switch(head->cmd){
#if 0
		case NEWS_CHANNEL_DECLARATION_REQ:
			DEBUGPRINT(DEBUG_INFO, "150 .. %d, \n", *(text+8));
			if(len < 9){
				DEBUGPRINT(DEBUG_INFO, "150 .................... len too small \n, ");
			}
			if(*(text+8) == VIDEO_CHANNEL){
				tutk_session[i].close_video_ch = TUTK_VIDEO_ON;
			}else if(*(text+8) == AUDIO_CHANNEL){
				tutk_session[i].close_audio_ch = TUTK_AUDIO_ON;
			}
			tutk_send_open_channel_msg(rdt_id, msg, *(text+8));
			break;

		case NEWS_CHANNEL_CLOSE_REQ:
			DEBUGPRINT(DEBUG_INFO, "NEWS_CHANNEL_CLOSE_REQ .. %d, ", *(text+4));
			if(*(text+4) == VIDEO_CHANNEL){
				tutk_session[i].close_video_ch = TUTK_VIDEO_OFF;
			}else if(*(text+4) == AUDIO_CHANNEL){
				tutk_session[i].close_audio_ch = TUTK_AUDIO_OFF;
			}
			tutk_send_close_channel_msg(rdt_id, msg, *(text+4));
			break;
#endif
		case 2001:
			DEBUGPRINT(DEBUG_INFO, "2001 ..............%d %d\n", *(text+4), *(text+5));
			if(*(text+4) == 1){
				tutk_session[i].close_video_ch = *(text+5);
			}else if(*(text+4) == 2){
				tutk_session[i].close_audio_ch= *(text+5);
			}
			tutk_send_channel_msg_rep(rdt_id, msg, *(text+4));
			break;

		case NEWS_AUDIO_TRANSMIT_REQ:
			if(*(text+4) == TUTK_CAMERA_TO_MOBILE){
				dir = TUTK_CAMERA_TO_MOBILE;
			}else{
				dir = TUTK_MOBILE_TO_CAMERA;
			}
			ret = set_audio_channel_dir(i, dir);
			if(ret == 0){
				if(dir == TUTK_CAMERA_TO_MOBILE){
					tutk_send_audio_dir_req(rdt_id, msg, TUTK_CAMERA_TO_MOBILE);
				}else if(dir == TUTK_MOBILE_TO_CAMERA){
					tutk_send_audio_dir_req(rdt_id, msg, TUTK_MOBILE_TO_CAMERA);
				}
			}else{
				tutk_send_audio_dir_req(rdt_id, msg, TUTK_CAMERA_TO_MOBILE);
			}
			break;

		case NEWS_WIFI_QUALITY_REQ:
			tutk_session[i].wifi_st = *(text+4);
			if(tutk_session[i].wifi_st == TUTK_WIFI_STATUS_OFF){ //关闭
				tutk_session[i].wifi_time_interval = 0;//时间清零
				tutk_session[i].wifi_time_stamp = 0;
				break;
			}
			int *val = (int *)(text+5);
			DEBUGPRINT(DEBUG_INFO, "wifi_st: %d, time: %d\n", tutk_session[i].wifi_st, *val);
			if(*val < 5){
				*val = 5;
			}
			if(*val > 60){
				*val = 60;
			}
			tutk_session[i].wifi_time_interval = *val; 
			tutk_session[i].wifi_time_stamp = time(NULL);
			break;

		default:
			strncpy(head->head, "TUTK", 4); //用于在子进程中区分

			msg_snd->type = NEWS_CHANNEL_MSG_TYPE;
			msg_snd->cmd = MSG_NEWS_CHANNEL_P_CHANNEL_REQ;
			msg_snd->len = sizeof(RequestHead) + head->len + sizeof(int);
			memcpy(msg_snd + 1, &rdt_id, sizeof(int));
			memcpy((char *)(msg_snd + 1) + sizeof(int), head, sizeof(RequestHead) + head->len);
			msg_queue_snd(tutk_msg.upnp_snd_process_id, msg_snd, sizeof(MsgData) - sizeof(long) + msg_snd->len);
			break;
	}

	return 0;
}

/**
 * @brief 控制通道线程
 * 循环接收apps消息
 * 对心跳包有计数，当连续2分钟没收到断开apps连接
 *
 * @author <王志强>
 * ＠param[in]<arg><用户的变量>
 *
 * */
void *ctrlchannel_rdt_thread(void *arg)
{
	int i = (int)((int)arg/4);
	int j = (int)arg%4;
	struct tutk_iotc_session *s = (struct tutk_iotc_session *)&tutk_session[i]; 
	char buf[512];
	RequestHead *head = (RequestHead *)buf;
	int ret;
	int timeout = 0;
	int rdt_id = s->ch[j].rdt_id;

	DEBUGPRINT(DEBUG_INFO, "tutk create control thread: %d %d %d\n", i, j, rdt_id);
	while(s->ch[j].rdt_used){
		ret = tutk_recv_data_timeout(rdt_id, buf, sizeof(RequestHead), CONTROL_WAIT_TIMEMS);
		if(ret < 0){
			if(ret == RDT_ER_TIMEOUT){
				timeout++;
				//printf("ctrlchannel_rdt_thread..................out.........................\n");
				if (timeout == 240) {
					tutk_log_s("ctrlchannel_rdt_thread timeout");
					timeout = 0;
					DEBUGPRINT(DEBUG_INFO, "ctrlchannel_rdt_thread......out.........................\n");
					break; //thread exit
				}
			}else{
				tutk_log_s("ctrlchannel_rdt_thread: read head failed %d", ret);
				printf("ctrlchannel_rdt_thread: read head failed %d\n", ret);
				break; //thread exit
			}
		}else{
			if (strncmp(head->head, "IBBM", 4) != 0 && head->len < 4) {
				//clear buf data
				tutk_recv_data_throw(rdt_id, buf, 512);
				printf("ctrlchannel_rdt_thread: recv head error\n");
			} else {
				if (head->cmd == NEWS_CHANNEL_TOUCH_REQ_V2) {
					timeout = 0;
				}

				printf("ctrlchannel_rdt_thread%d: CMD=%d len = %d\n", i, head->cmd, head->len);
				ret = tutk_recv_data_timeout(rdt_id, buf+sizeof(RequestHead), head->len, 10000);
				if (ret < 0 && ret != RDT_ER_TIMEOUT) {
					printf("ctrlchannel_rdt_thread: read head failed %d\n", ret);
					break;
				}

				tutk_ctrlchannel_msg_handler((int) arg, buf, head->len);
			}
		}

		if(tutk_session[i].wifi_st == TUTK_WIFI_STATUS_ON){
			send_wifi_signal_func((int)arg);
		}
	}
	tutk_log_s("ctrlchannel_rdt_thread ---------1-----------> session:%d, channel:%d exit\n", i, j);
	pthread_mutex_lock(&s->ch[j].rdt_mutex);
	s->ch[j].rdt_used = RDT_USED_WAIT_EXIT;
	set_session_exit_state(i);
	tutk_log_s("ctrlchannel_rdt_thread ---------2-----------> session:%d, channel:%d exit\n", i, j);
	IOTC_Session_Close(tutk_session[i].session_id);
	tutk_log_s("ctrlchannel_rdt_thread ----------3----------> session:%d, channel:%d exit\n", i, j);
	RDT_Destroy(s->ch[j].rdt_id);
	tutk_log_s("ctrlchannel_rdt_thread -----------4---------> session:%d, channel:%d exit\n", i, j);
	pthread_mutex_unlock(&s->ch[j].rdt_mutex);
	tutk_log_s("ctrlchannel_rdt_thread -----------5---------> session:%d, channel:%d exit\n", i, j);


	pthread_exit(NULL);
}


