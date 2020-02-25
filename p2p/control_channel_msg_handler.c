/*
 * control_channel_msg_handler.c
 *
 *  Created on: May 14, 2013
 *      Author: root
 */
#include <stdio.h>

#include "tutk_rdt_video_server.h"

/**
 * @brief 回复通道声明的函数
 * @author <王志强>
 * ＠param[in]<rdt_id><rdt通道的ID>
 * ＠param[in]<msg><数据的首地址>
 * ＠param[in]<ch><数据的长度>
 * @return <0: 成功>
 *
 * */
int tutk_send_open_channel_msg(int rdt_id, char *msg, int ch)
{
	RequestHead *head = (RequestHead *)msg;
	char *text = (char *)(head + 1);

	head->cmd = NEWS_CHANNEL_DECLARATION_REP;
	head->len = 10;
	*(text + 4) = ch;
	*(text + 5) = 1;
	DEBUGPRINT(DEBUG_INFO, "tutk_send_open_channel_msg .. %d, ", *(text+4));
	return tutk_send_vast(rdt_id, msg, sizeof(RequestHead)+head->len);
}

#if 0
/**
 * 未用
 *
 * */
int tutk_send_close_channel_msg(int rdt_id, char *msg, int ch)
{
	RequestHead *head = (RequestHead *)msg;
	char *text = (char *)(head + 1);

	head->cmd = NEWS_CHANNEL_CLOSE_REP;
	head->len = 6;
	*(text + 4) = ch;
	*(text + 5) = 1;
	DEBUGPRINT(DEBUG_INFO, "tutk_send_close_channel_msg .. %d, ", *(text+4));
	return tutk_send_vast(rdt_id, msg, sizeof(RequestHead)+head->len);
}
#endif

/**
 * @brief 回复通道打开的函数
 * @author <王志强>
 * ＠param[in]<rdt_id><rdt通道的ID>
 * ＠param[in]<msg><数据的首地址>
 * ＠param[in]<ch><数据的长度>
 * @return <0: 成功>
 *
 * */
int tutk_send_channel_msg_rep(int rdt_id, char *msg, int ch)
{
	RequestHead *head = (RequestHead *)msg;
	char *text = (char *)(head + 1);

	head->cmd = 2002;
	head->len = 6;
	*(text + 4) = ch;
	*(text + 5) = 1;
	DEBUGPRINT(DEBUG_INFO, "tutk_send_channel_msg_rep .. %d, ", *(text+4));
	return tutk_send_vast(rdt_id, msg, sizeof(RequestHead)+head->len);
}

#if 0
int tutk_send_heart_msg(int rdt_id, char *msg)
{
	RequestHead *head = (RequestHead *)msg;
	char *text = (char *)(head + 1);

	head->cmd = 157;
	head->len = 6;
	*(text + 4) = 1;
	*(text + 5) = 1;

	return tutk_send_vast(rdt_id, msg, sizeof(RequestHead)+head->len);
}
#endif

/**
 * @brief 回复设置音频方向的函数
 * @author <王志强>
 * ＠param[in]<rdt_id><rdt通道的ID>
 * ＠param[in]<msg><数据的首地址>
 * ＠param[in]<dir><方向的值>
 * @return <0: 成功>
 *
 * */
int tutk_send_audio_dir_req(int rdt_id, char *msg, int dir)
{
	RequestHead *head = (RequestHead *)msg;
	char *text = (char *)(head + 1);

	head->cmd = NEWS_AUDIO_TRANSMIT_REP;
	if(dir != 0){
		if(dir == 1){
			*(text + 4) = 1;
			*(text + 5) = 2;
		}else{
			*(text + 4) = 2;
			*(text + 5) = 1;
		}
	}else{

	}
	int k;
	for(k = 0; k < head->len+23; k++){
		printf("0x%x ", msg[k]);
	}
	puts("\n");
	return tutk_send_vast(rdt_id, msg, sizeof(RequestHead)+head->len);
}

/**
 * @brief 发送wifi信号的函数
 * @author <王志强>
 * ＠param[in]<rdt_id><rdt通道的ID>
 * @return <0: 成功>
 *
 * */
int send_wifi_signal_msg(int rdt_id)
{
	char buf[100];
	RequestHead *head = (RequestHead *)buf;
	char *text = (char *)(head + 1);

	memset(buf, 0, sizeof(buf));
	memcpy(head->head, ICAMERA_NETWORK_HEAD, sizeof(ICAMERA_NETWORK_HEAD));
	head->version = ICAMERA_NETWORK_VERSION;
	head->cmd = NEWS_WIFI_QUALITY_REP;
	head->len = 13;
	get_wifi_signal_level(text+4);
	return tutk_send_vast(rdt_id, buf, sizeof(RequestHead)+head->len);
}

/**
 * @brief 回复夜间模式的函数
 * @author <王志强>
 * ＠param[in]<rdt_id><rdt通道的ID>
 * ＠param[in]<state><夜间模式的状态>
 * ＠param[in]<time><夜间模式的开始时间>
 * @return <0: 成功>
 *
 * */
int send_set_nightmode_msg(int rdt_id, int state, int time)
{
	char buf[100];
	RequestHead *head = (RequestHead *)buf;
	char *text = (char *)(head + 1);

	DEBUGPRINT(DEBUG_INFO, "send_set_nightmode_msg: on=%d, time=%d\n", state, time);
	memset(buf, 0, sizeof(buf));
	memcpy(head->head, ICAMERA_NETWORK_HEAD, sizeof(ICAMERA_NETWORK_HEAD));
	head->version = ICAMERA_NETWORK_VERSION;
	head->cmd = NEWS_NIGHT_MODE_SET_REP;
	head->len = 9;
	*(text+4) = state;
	memcpy(text+5, &time, 4);
	return tutk_send_vast(rdt_id, buf, sizeof(RequestHead)+head->len);
}

/**
 * @brief 向连接的用户回复夜间模式的函数
 * @author <王志强>
 * ＠param[in]<state><夜间模式的状态>
 * ＠param[in]<time><夜间模式的开始时间>
 * @return <0: 成功>
 *
 * */
int tutk_send_set_nightmode_command(int state, int time)
{
	int i, j;
	int ret;

	for(i = 0; i < TUTK_SESSION_NUM; i++){
		pthread_mutex_lock(&tutk_session[i].ch[0].rdt_mutex);
		if(tutk_session[i].ch[0].rdt_used == 1){
			DEBUGPRINT(DEBUG_INFO, "tutk_send_set_nightmode_command..........%d, %d\n", i, tutk_session[i].ch[0].rdt_id);
			usleep(500);
			ret = send_set_nightmode_msg(tutk_session[i].ch[0].rdt_id, state, time);
			DEBUGPRINT(DEBUG_INFO, "send_set_nightmode_msg: %d\n", ret);
		}
		pthread_mutex_unlock(&tutk_session[i].ch[0].rdt_mutex);
	}
	return 0;
}

/**
 * @brief 回复查询夜间模式的函数
 * @author <王志强>
 * ＠param[in]<rdt_id><rdt通道的ID>
 * ＠param[in]<state><夜间模式的状态>
 * ＠param[in]<time><夜间模式的开始时间>
 * @return <0: 成功>
 *
 * */
int send_nightmode_req_msg(int rdt_id, int state, int time)
{
	char buf[100];
	RequestHead *head = (RequestHead *)buf;
	char *text = (char *)(head + 1);

	DEBUGPRINT(DEBUG_INFO, "send_nightmode_req_msg: state=%d, time=%d\n", state, time);
	memset(buf, 0, sizeof(buf));
	memcpy(head->head, ICAMERA_NETWORK_HEAD, sizeof(ICAMERA_NETWORK_HEAD));
	head->version = ICAMERA_NETWORK_VERSION;
	head->cmd = NEWS_NIGHT_MODE_RECORD_REP;
	head->len = 9;
	*(text+4) = state;
	memcpy(text+5, &time, 4);

	return tutk_send_vast(rdt_id, buf, sizeof(RequestHead)+head->len);
}

/**
 * @brief 向https发送UID无效消息
 * @author <王志强>
 * @return <0: 成功>
 *
 * */
int tutk_send_https_check_msg(void)
{
	char buf[100];
	MsgData *msg_snd = buf;

	msg_snd->type = HTTP_SERVER_MSG_TYPE;
	msg_snd->cmd = MSG_HTTPSSERVER_T_AUTHENTICATION_POWER;
	msg_snd->len = 0;

	return msg_queue_snd(tutk_msg.upnp_rev_process_id, msg_snd, sizeof(MsgData) - sizeof(long) + msg_snd->len);
}



