/**	
   @file NewsAdapter.c
   @brief 应用程序主文件
   @note
   author: yangguozheng
   date: 2013-09-16
   mcdify record: creat this file.
   author: yangguozheng
   date: 2013-09-17
   modify record: add a function
 */
#include <stdlib.h>
#include "NewsAdapter.h"
#include "common/msg_queue.h"
#include "common/common.h"
#include "VideoSend/VideoSend.h"
#include "SerialPorts/SerialAdapter.h"
#include "AudioAlarm/audioalarm.h"
#include "common/common_config.h"
#include "common/common_func.h"
#include "VideoAlarm/VideoAlarm.h"
#include "AudioSend/AudioSend.h"
#include "Video/video.h"
#include "NewsUtils.h"

#if 1
#define TEST_PRINT(format, arg...)\
	DEBUGPRINT(DEBUG_INFO, format, ##arg)
#else
#define TEST_PRINT(format, arg...)
#endif

//typedef void (*newsFun)(void *self);
int NewsAdapter_cmd_compar(const void *left, const void *right);
void *NewsAdapter_cmd_bsearch(NewsCmdItem *cmd, NewsCmdItem *cmd_table, size_t count);
void NewsAdapter_cmd_table_fun(void *self);
void NewsAdapter_newsChannelDeclarationReq(void *self);
void NewsAdapter_newsAudioTransmitReq(void *self);

/**
   @brief 对比函数
   @author yangguozheng
   @param[in] left 左值
   @param[in] right 右值
   @return left - right
   @note
 */
int NewsAdapter_cmd_compar(const void *left, const void *right) {
	NewsCmdItem *a = (NewsCmdItem *)left;
	NewsCmdItem *b = (NewsCmdItem *)right;
	return a->cmd - b->cmd;
}

/**
   @brief 二分法查找
   @author yangguozheng
   @param[in] cmd 项目
   @param[in] cmd_table 项目表
   @param[in] count 项目数量
   @return 查找项
   @note
 */
void *NewsAdapter_cmd_bsearch(NewsCmdItem *cmd, NewsCmdItem *cmd_table, size_t count) {
	NewsCmdItem *icmd = bsearch(cmd, cmd_table, count, sizeof(NewsCmdItem), NewsAdapter_cmd_compar);
	if(icmd == NULL) {
		return NULL;
	}
	return icmd->data;
}

void NewsAdapter_newsChannelDeclarationReq(void *self);
void NewsAdapter_newsChannelNetlightReq (void *self);
void NewsAdapter_newsChannelPowerlightReq (void *self);
void NewsAdapter_newsChannelNightlightReq(void *self);
void NewsAdapter_newsChannelAudioCtrl(void *self);
void NewsAdapter_newsChannelVideoCtrl(void *self);
void NewsAdapter_newsChannelCameraCtrl(void *self);
void NewsAdapter_newsChannelAlarmCtrl(void *self);

void NewsAdapter_newsAudioTransmitOut(void *self);
void NewsAdapter_newsAudioTransmitIn(void *self);
void NewsAdapter_newsAudioTransmitReq(void *self);

void NewsAdapter_newsCameraStaminaReq(void *self);
void NewsAdapter_newsCameranightmodesetreq(void *self);
void NewsAdapter_newsCameranightmoderecordreq(void *self);
void NewsAdapter_newsChannelTouchReqV2(void *self);

/** 协议表 */
NewsCmdItem NewsAdapter_cmd_table[] = {
		{NEWS_CHANNEL_DECLARATION_REQ, NewsAdapter_newsChannelDeclarationReq},
		{NEWS_CHANNEL_DECLARATION_REP, 0},
		{NEWS_CHANNEL_CLOSE_REQ, 0},
		{NEWS_CHANNEL_CLOSE_REP, 0},
		{NEWS_CHANNEL_TOUCH_REQ, 0},
		{NEWS_CHANNEL_WRIGGLE_REP, 0},
		{NEWS_CHANNEL_TOUCH_REQ_V2, NewsAdapter_newsChannelTouchReqV2},
		{NEWS_CTL_CHANNEL_NET_LED_QUERY_REQ, NewsAdapter_newsChannelNetlightReq},
		{NEWS_CTL_CHANNEL_NET_LED_QUERY_REP, 0},
		{NEWS_CTL_CHANNEL_NET_LED_SET_REQ, NewsAdapter_newsChannelNetlightReq},
		{NEWS_CTL_CHANNEL_NET_LED_SET_REP, 0},

		{NEWS_CTL_CHANNEL_POWER_LED_QUERY_REQ, NewsAdapter_newsChannelPowerlightReq},
		{NEWS_CTL_CHANNEL_POWER_LED_SET_REQ, NewsAdapter_newsChannelPowerlightReq},

		{NEWS_CTL_CHANNEL_NIGHT_LED_QUERY_REQ, NewsAdapter_newsChannelNightlightReq},
		{NEWS_CTL_CHANNEL_NIGHT_LED_SET_REQ, NewsAdapter_newsChannelNightlightReq},

		{NEWS_AUDIO_SET_REQ, NewsAdapter_newsChannelAudioCtrl},
		{NEWS_AUDIO_QUERY_REQ, NewsAdapter_newsChannelAudioCtrl},
		{NEWS_AUDIO_TRANSMIT_OUT, NewsAdapter_newsAudioTransmitOut},
		{NEWS_AUDIO_TRANSMIT_IN, NewsAdapter_newsAudioTransmitIn},
		{NEWS_AUDIO_TRANSMIT_REQ, NewsAdapter_newsAudioTransmitReq},
		{NEWS_VIDEO_SET_REQ, NewsAdapter_newsChannelVideoCtrl},
		{NEWS_VIDEO_QUERY_REQ, NewsAdapter_newsChannelVideoCtrl},

		//camera
		{NEWS_CAMERA_CRUISE, NewsAdapter_newsChannelCameraCtrl},
		{NEWS_CAMERA_MOVE_REL, NewsAdapter_newsChannelCameraCtrl},

		//alarm
		{NEWS_ALARM_QUERY_REQ, NewsAdapter_newsChannelAlarmCtrl},
//		{NEWS_ALARM_QUERY_REP, NewsAdapter_newsChannelAlarmCtrl},
		{NEWS_ALARM_CONFIG_REQ, NewsAdapter_newsChannelAlarmCtrl},
//		{NEWS_ARARM_CONFIG_REP, NewsAdapter_newsChannelAlarmCtrl},
		{NEWS_ALARM_EVENT_NOTIFY, NewsAdapter_newsChannelAlarmCtrl},
    {NEWS_CAMERA_STAMINA_REQ, NewsAdapter_newsCameraStaminaReq},
		{NEWS_NIGHT_MODE_SET_REQ, NewsAdapter_newsCameranightmodesetreq},
		{NEWS_NIGHT_MODE_RECORD_REQ, NewsAdapter_newsCameranightmoderecordreq},
};

/**
   @brief 协议查询入口
   @author yangguozheng
   @param[in] self 适配器运行环境
   @return 查找项
   @note
 */
void NewsAdapter_cmd_table_fun(void *self) {
	msg_container *context = (msg_container *)self;
	MsgData *msgRcv = (MsgData *)context->msgdata_rcv;
	int *fd = (int *)(msgRcv + 1);
	RequestHead *head = (RequestHead *)(fd + 1);
	NewsCmdItem icmd;
	icmd.cmd = head->cmd;
	printf("NewsAdapter_cmd_table_fun..........%d\n", icmd.cmd);
	DEBUGPRINT(DEBUG_INFO, "NewsAdapter_cmd_table_fun..........%d\n", icmd.cmd);
	newsFun fun = NewsAdapter_cmd_bsearch(&icmd, NewsAdapter_cmd_table, sizeof(NewsAdapter_cmd_table)/sizeof(NewsCmdItem));
	if(fun != NULL) {
		fun(self);
	}
	else {
		DEBUGPRINT(DEBUG_INFO, "################################################################");
		DEBUGPRINT(DEBUG_INFO, "%s: cannot find command\n", __FUNCTION__);
		DEBUGPRINT(DEBUG_INFO, "################################################################");
	}
}

/**
   @brief 发送数据到网络
   @author yangguozheng
   @param[in] sockfd 通道描述符
   @param[in] cmd 命令项
   @param[in] data 数据项
   @param[in] len 数据长度
   @param[in] path 发送路径
   @return -1: 失败, 0: 成功
   @note
 */
int send_msg_to_network(int sockfd, int cmd, char *data, int len, int path)
{
	RequestHead netmsg;
	char buf[1024];

	memset(&netmsg, 0, sizeof(netmsg));
	memset(buf, 0, sizeof(buf));
	memcpy(netmsg.head, ICAMERA_NETWORK_HEAD, sizeof(ICAMERA_NETWORK_HEAD));
	netmsg.version = ICAMERA_NETWORK_VERSION;
	netmsg.cmd = cmd;
	netmsg.len = len;
	memcpy(buf, &netmsg, sizeof(netmsg));
	if (len > 0 && data != NULL ) {
		memcpy(&buf[sizeof(netmsg)], data, len);
		len += sizeof(netmsg);
	} else {
		len = sizeof(netmsg);
	}
	if(path == 1){
		return NewsUtils_sendToNet(sockfd, buf, len, 0);
	}else if(path == 2){
		return tutk_send_vast(sockfd, buf, len);
	}
}

/**
   @brief 发送数据线程消息队列
   @author yangguozheng
   @param[in|out] self 上下文
   @param[in] type 类型
   @param[in] cmd 命令项
   @param[in] data 数据项
   @param[in] len 数据长度
   @return -1: 失败, 0: 成功
   @note
 */
int send_msg_to_otherthread(msg_container *self, int type, int cmd, void *data, int len)
{
	MsgData *msgSnd = (MsgData *) self->msgdata_snd;

	msgSnd->type = type; //AUDIO_ALARM_MSG_TYPE;
	msgSnd->cmd = cmd;
	switch(cmd){
		case MSG_AUDIOALARM_T_START_AUDIO_CAPTURE:
			msgSnd->len = 0;
			break;

		case MSG_AUDIOALARM_T_STOP_AUDIO_CAPTURE:
			msgSnd->len = 0;
			break;

		case MSG_AUDIOALARM_T_ALARM_LEVEL:
			memcpy(msgSnd+1, data, len);
			msgSnd->len = len;
			break;

		case MSG_VIDEO_P_SETTING_PARAMS:
			memcpy(msgSnd+1, data, len);
			msgSnd->len = len;
			break;

		default:
//			memcpy(msgSnd+1, data, len);
//			msgSnd->len = len;
			break;
	}
	return msg_queue_snd(self->rmsgid, msgSnd,
					sizeof(MsgData) - sizeof(long) + msgSnd->len);
}

/**
   @brief 通道声明
   @author yangguozheng
   @param[in|out] self 运行环境
   @return
   @note
 */
void NewsAdapter_newsChannelDeclarationReq(void *self) {
	msg_container *context = (msg_container *)self;
	MsgData *msgRcv = (MsgData *)context->msgdata_rcv;
	MsgData *msgSnd = (MsgData *)context->msgdata_snd;
	int *fd = (int *)(msgRcv + 1);
	RequestHead *head = (RequestHead *)(fd + 1);
	char *text = (char *)(head + 1);
//	DEBUGPRINT(DEBUG_INFO, "NewsAdapter_newsChannelDeclarationReq %d\n", *(text + sizeof(int) + 1 + 1));
	DEBUGPRINT(DEBUG_INFO, "%d, %d, %d\n", *(int *)text, *(text + 4), *(text + 8));
	switch(*(text + 8)) {
		case CONTROL_CHANNEL:
			DEBUGPRINT(DEBUG_INFO, "CONTROL_CHANNEL\n");
#if 0
			head->cmd = 151;
			memcpy(head->head, "IBBM", 4);
			head->version = 1;
			head->len = 10;
			*(int *)text = 0;
			*(text + 4) = 1;
			*(text + 5) = 1;
			*(int *)(text + 6) = 0;
			if(-1 == NewsUtils_sendToNet(*fd, head, sizeof(RequestHead) + head->len, 0)) {
				DEBUGPRINT(DEBUG_INFO, "send error \n");
			}
#endif
			break;
		case VIDEO_CHANNEL:
			DEBUGPRINT(DEBUG_INFO, "VIDEO_CHANNEL\n");
#if 0
			head->cmd = 151;
			memcpy(head->head, "IBBM", 4);
			head->version = 1;
			head->len = 10;
			*(int *)text = 0;
			*(text + 4) = 2;
			*(text + 5) = 1;
			*(int *)(text + 6) = 0;
			if(-1 == NewsUtils_sendToNet(*fd, head, sizeof(RequestHead) + head->len, 0)) {
				DEBUGPRINT(DEBUG_INFO, "send error \n");
			}
#endif
#ifndef _DISABLE_VIDEO_SEND
			//DEBUGPRINT(DEBUG_INFO, "VIDEO_CHANNEL\n");
			msgSnd->type = VIDEO_SEND_MSG_TYPE;
			msgSnd->cmd = MSG_VIDEOSEND_P_VIDEO_SEND;
			*(int *)(msgSnd + 1) = *fd;
			msgSnd->len = sizeof(int);
			msg_queue_snd(context->rmsgid, msgSnd, sizeof(MsgData) - sizeof(long) + msgSnd->len);
            DEBUGPRINT(DEBUG_INFO, "VIDEO_CHANNEL: %X\n", *(int *)(head->head));
#endif
			break;
		case AUDIO_CHANNEL:
			DEBUGPRINT(DEBUG_INFO, "AUDIO_CHANNEL\n");
#if 0
			head->cmd = 151;
			memcpy(head->head, "IBBM", 4);
			head->version = 1;
			head->len = 10;
			*(int *)text = 0;
			*(text + 4) = 3;
			*(text + 5) = 1;
			*(int *)(text + 6) = 0;
			TEST_PRINT("accept: %d\n", *fd);
			if(0 >= NewsUtils_sendToNet(*fd, head, sizeof(RequestHead) + head->len, 0)) {
				DEBUGPRINT(DEBUG_ERROR, "send error %s\n", errstr);
			}
#endif
#if 1
			msgSnd->type = AUDIO_SEND_MSG_TYPE;
			msgSnd->cmd = MSG_AUDIOSEND_P_AUDIO_SEND;
			*(int *)(msgSnd + 1) = *fd;
			msgSnd->len = sizeof(int);
//			TEST_PRINT("accept: %d\n", *fd);
			msg_queue_snd(context->rmsgid, msgSnd, sizeof(MsgData) - sizeof(long) + msgSnd->len);
#endif
			break;
	}
//	DEBUGPRINT(DEBUG_INFO, "%s: %d\n", __FUNCTION__, *(int *)text);
//	DEBUGPRINT(DEBUG_INFO, "%s: %d\n", __FUNCTION__, *(text + sizeof(int)));
//	DEBUGPRINT(DEBUG_INFO, "%s: %d\n", __FUNCTION__, *(text + 1 + sizeof(int)));
}

/**
   @brief 网路状态指示灯查询请求
   @author yangguozheng
   @param[in|out] self 运行环境
   @return
   @note
 */
void NewsAdapter_newsChannelNetlightReq (void *self)
{
	msg_container *context = (msg_container *) self;
	MsgData *msgRcv = (MsgData *) context->msgdata_rcv;
	MsgData *msgSnd = (MsgData *) context->msgdata_snd;
	int *fd = (int *) (msgRcv + 1);
	RequestHead *head = (RequestHead *) (fd + 1);
	char *text = (char *) (head + 1);
	int led_st;
	int len;
	int n;
	int path;

	if(strncmp(head->head, "TUTK", 4) == 0){
		path = 2;
	}else{
		path = 1;
	}

	switch(head->cmd){
		case NEWS_CTL_CHANNEL_NET_LED_QUERY_REQ:
			len = 0;
			memset(&context->msgdata_snd[len], 0, 4); //动态口令  0
			len += 4;
			led_st = 0;
			get_config_item_value(CONFIG_LIGHT_NET, &led_st, &n);
			//printf("net led_st =================== %c %d\n", led_st, led_st);
			if (led_st == 'n') {
				context->msgdata_snd[len] = 2; //关闭
			} else {
				context->msgdata_snd[len] = 1; //开启
			}
			len += 1;
			send_msg_to_network(*fd, NEWS_CTL_CHANNEL_NET_LED_QUERY_REP, context->msgdata_snd, len, path);
			break;

		case NEWS_CTL_CHANNEL_NET_LED_SET_REQ:
			led_st = *(text+4);
//			printf("NEWS_CTL_CHANNEL_NET_LED_SET_REQ......... %d\n", led_st);
			if(led_st == 1){
				if(g_nDeviceType == DEVICE_TYPE_M2){
					led_if_work_control(LED_TYPE_NET_POWER, LED_FLAG_WORK, TRUE);
				}else if(g_nDeviceType == DEVICE_TYPE_M3S){
					led_if_work_control(LED_TYPE_NET, LED_FLAG_WORK, TRUE);
				}
			}else if(led_st == 2){
				if (g_nDeviceType == DEVICE_TYPE_M2) {
					led_if_work_control(LED_TYPE_NET_POWER, LED_FLAG_NOT_WORK, TRUE);
				} else if (g_nDeviceType == DEVICE_TYPE_M3S) {
					led_if_work_control(LED_TYPE_NET, LED_FLAG_NOT_WORK, TRUE);
				}
			}
			len = 0;
			memset(&context->msgdata_snd[len], 0, 4); //动态口令  0
			len += 4;
#if 0
			led_st = get_led_state(LED_COLOR_TYPE_NET_BLUE);
			if (led_st == *(text+4)) {
				context->msgdata_snd[len] = 1; //成功
			} else {
				context->msgdata_snd[len] = 2; //失败
			}
#else
			context->msgdata_snd[len] = 1; //成功
#endif
			len += 1;
			send_msg_to_network(*fd, NEWS_CTL_CHANNEL_NET_LED_SET_REP, context->msgdata_snd, len, path);
			break;

		default:
			break;
	}
}

/**
   @brief 电源状态指示灯查询请求
   @author yangguozheng
   @param[in|out] self 运行环境
   @return
   @note
 */
void NewsAdapter_newsChannelPowerlightReq (void *self)
{
	msg_container *context = (msg_container *) self;
	MsgData *msgRcv = (MsgData *) context->msgdata_rcv;
	MsgData *msgSnd = (MsgData *) context->msgdata_snd;
	int *fd = (int *) (msgRcv + 1);
	RequestHead *head = (RequestHead *) (fd + 1);
	char *text = (char *) (head + 1);
	int led_st;
	int len;
	int n;
	//char buf_tmp;
	int path;

	if (strncmp(head->head, "TUTK", 4) == 0) {
		path = 2;
	} else {
		path = 1;
	}

	switch(head->cmd){
		case NEWS_CTL_CHANNEL_POWER_LED_QUERY_REQ:
			len = 0;
			memset(&context->msgdata_snd[len], 0, 4); //动态口令  0
			len += 4;
			led_st = 0;
			get_config_item_value(CONFIG_LIGHT_POWER, &led_st, &n);
			//printf("led_st: ------- %d\n", led_st);
			if(led_st == 'n'){
				context->msgdata_snd[len] = 2;//关闭
			}else{
				context->msgdata_snd[len] = 1;//开启
			}
			len += 1;
			send_msg_to_network(*fd, NEWS_CTL_CHANNEL_POWER_LED_QUERY_REP, context->msgdata_snd, len, path);
			break;

		case NEWS_CTL_CHANNEL_POWER_LED_SET_REQ:
			led_st = *(text+4);
//			printf("NEWS_CTL_CHANNEL_POWER_LED_SET_REQ %d\n", led_st);
			if(led_st == 1){
				if (g_nDeviceType == DEVICE_TYPE_M2) {

				}else if(g_nDeviceType == DEVICE_TYPE_M3S){
					led_if_work_control(LED_TYPE_POWER, LED_FLAG_WORK, TRUE);
				}
			}else if(led_st == 2){
				if (g_nDeviceType == DEVICE_TYPE_M2) {

				} else if (g_nDeviceType == DEVICE_TYPE_M3S) {
					led_if_work_control(LED_TYPE_POWER, LED_FLAG_NOT_WORK, TRUE);
				}
			}
			len = 0;
			memset(&context->msgdata_snd[len], 0, 4); //动态口令  0
			len += 4;
#if 0
			led_st = get_led_state(LED_COLOR_TYPE_POWER_RED);
			if (led_st == *(text+4)) {
				context->msgdata_snd[len] = 1; //成功
			} else {
				context->msgdata_snd[len] = 2; //失败
			}
#else
			context->msgdata_snd[len] = 1; //成功
#endif
			len += 1;
			send_msg_to_network(*fd, NEWS_CTL_CHANNEL_POWER_LED_SET_REP, context->msgdata_snd, len, path);
			break;

		default:
			break;
	}
}

/**
   @brief 夜视灯状态指示灯查询请求
   @author yangguozheng
   @param[in|out] self 运行环境
   @return
   @note
 */
void NewsAdapter_newsChannelNightlightReq (void *self)
{
	msg_container *context = (msg_container *) self;
	MsgData *msgRcv = (MsgData *) context->msgdata_rcv;
	MsgData *msgSnd = (MsgData *) context->msgdata_snd;
	int *fd = (int *) (msgRcv + 1);
	RequestHead *head = (RequestHead *) (fd + 1);
	char *text = (char *) (head + 1);
	int led_st;
	int len;
	int n;
	//char buf_tmp;
	int path;

	if (strncmp(head->head, "TUTK", 4) == 0) {
		path = 2;
	} else {
		path = 1;
	}

	switch(head->cmd){
		case NEWS_CTL_CHANNEL_NIGHT_LED_QUERY_REQ:
			len = 0;
			memset(&context->msgdata_snd[len], 0, 4); //动态口令  0
			len += 4;
			led_st = 0;
			get_config_item_value(CONFIG_LIGHT_NIGHT, &led_st, &n);
			//printf("led_st: ------- %d\n", led_st);
			if(led_st == 'n'){
				context->msgdata_snd[len] = 2;//关闭
			}else{
				context->msgdata_snd[len] = 1;//开启
			}
			len += 1;
			send_msg_to_network(*fd, NEWS_CTL_CHANNEL_NIGHT_LED_QUERY_REP, context->msgdata_snd, len, path);
			break;

		case NEWS_CTL_CHANNEL_NIGHT_LED_SET_REQ:
			led_st = *(text+4);
//			printf("NEWS_CTL_CHANNEL_NIGHT_LED_SET_REQ %d\n", led_st);
			if(led_st == 1){
				night_led_if_work_msg_handler(LED_FLAG_WORK);
			}else if(led_st == 2){
				night_led_if_work_msg_handler(LED_FLAG_NOT_WORK);
			}
			len = 0;
			memset(&context->msgdata_snd[len], 0, 4); //动态口令  0
			len += 4;
#if 0
			led_st = get_led_state(LED_COLOR_TYPE_POWER_RED);
			if (led_st == *(text+4)) {
				context->msgdata_snd[len] = 1; //成功
			} else {
				context->msgdata_snd[len] = 2; //失败
			}
#else
			context->msgdata_snd[len] = 1; //成功
#endif
			len += 1;
			send_msg_to_network(*fd, NEWS_CTL_CHANNEL_NIGHT_LED_SET_REP, context->msgdata_snd, len, path);
			break;

		default:
			break;
	}
}

/**
   @brief 音频操作
   @author yangguozheng
   @param[in|out] self 运行环境
   @return
   @note CMD: 300~399
 */
void NewsAdapter_newsChannelAudioCtrl(void *self)
{
	msg_container *context = (msg_container *) self;
	MsgData *msgRcv = (MsgData *) context->msgdata_rcv;
	MsgData *msgSnd = (MsgData *) context->msgdata_snd;
	int *fd = (int *) (msgRcv + 1);
	RequestHead *head = (RequestHead *) (fd + 1);
//	char *text = (char *) (head + 1);
//	short *data = (short *) (msgSnd + 1);
	int len;
	int path;

	if (strncmp(head->head, "TUTK", 4) == 0) {
		path = 2;
	} else {
		path = 1;
	}

	switch(head->cmd){
		case NEWS_AUDIO_SET_REQ:
			len = 0;
			memset(&context->msgdata_snd[len], 0, 4); //动态口令  0
			len += 4;
			context->msgdata_snd[len] = 1;  //录音音量设置结果
			len += 1;
			context->msgdata_snd[len] = 1;  //采样率设置结果
			len += 1;
			context->msgdata_snd[len] = 1;  //声道设置结果
			len += 1;
			send_msg_to_network(*fd, NEWS_AUDIO_SET_REP, context->msgdata_snd, len, path);
			break;

		case NEWS_AUDIO_QUERY_REQ:
			len = 0;
			memset(&context->msgdata_snd[len], 0, 4); //动态口令  0
			len += 4;
			context->msgdata_snd[len] = 1; //结果
			len += 1;
			context->msgdata_snd[len] = 1;  //录音音量
			len += 1;
			context->msgdata_snd[len] = 1;  //采样率 －－－－－－－1：8K 2：16K
			len += 1;
			context->msgdata_snd[len] = 2;  //声道－－－－－> 1:单声道   2：立体声
			len += 1;
			memset(&context->msgdata_snd[len], 0, 16);
			len += 16;
			send_msg_to_network(*fd, NEWS_AUDIO_QUERY_REP, context->msgdata_snd, len, path);
			break;

		default:
			break;
	}
}

/**
   @brief 视频设置和查询
   @author yangguozheng
   @param[in|out] self 运行环境
   @return
   @note
 */
void NewsAdapter_newsChannelVideoCtrl(void *self)
{
	msg_container *context = (msg_container *) self;
	MsgData *msgRcv = (MsgData *) context->msgdata_rcv;
	MsgData *msgSnd = (MsgData *) context->msgdata_snd;
	int *fd = (int *) (msgRcv + 1);
	RequestHead *head = (RequestHead *) (fd + 1);
	char *text = (char *) (head + 1);
	int brightness, contrast, size, hflip, vflip;
	char data_buf[100], data_tmp[8];
	int len, n;
	int bright_set, constract_set, size_set;
	int hflip_set, vflip_set;
	int path;

	if (strncmp(head->head, "TUTK", 4) == 0) {
		path = 2;
	} else {
		path = 1;
	}

	bright_set = 0;
	constract_set = 0;
	size_set = 0;
	hflip_set = 0;
	vflip_set = 0;

//	int i;
//	printf("cmd: %d, len = %d\n", head->cmd, head->len);
//	for(i = 0; i < head->len; i++){
//		printf("%d ", *(text+i));
//	}
//	printf("\n");
	switch(head->cmd){
		case NEWS_VIDEO_SET_REQ:
#define VIDEO_FPS_OFFSET		4
#define VIDEO_BRIGHT_OFFSET 	5   	//亮度
#define VIDEO_CONSTRACT_OFFSET	6   	//对比度
#define VIDEO_SIZE_OFFSET		7       //图像大小
#define VIDEO_HFLIP_OFFSET		8       //图像水平翻转
#define VIDEO_VFLIP_OFFSET		9       //图像垂直翻转
#define VIDEO_SIMPLE_OFFSET		10		//采样率

			if(*(text + VIDEO_FPS_OFFSET) == 1
					|| *(text + VIDEO_FPS_OFFSET) == 2
					|| *(text + VIDEO_FPS_OFFSET) == 3
					|| *(text + VIDEO_FPS_OFFSET) == 4
					|| *(text + VIDEO_FPS_OFFSET) == 5
					|| *(text + VIDEO_FPS_OFFSET) == 10
					|| *(text + VIDEO_FPS_OFFSET) == 15
					|| *(text + VIDEO_FPS_OFFSET) == 20
					|| *(text + VIDEO_FPS_OFFSET) == 30){
				memset(data_tmp, 0, sizeof(data_tmp));
				len = sprintf(data_tmp, "%d", *(text + VIDEO_FPS_OFFSET));
				set_config_value_to_ram(CONFIG_VIDEO_SIMPLE, data_tmp, len);
			}

			if(*(text + VIDEO_SIMPLE_OFFSET) == 50
					|| *(text + VIDEO_SIMPLE_OFFSET) == 60){
				len = sprintf(data_tmp, "%d", *(text + VIDEO_SIMPLE_OFFSET));
				set_config_value_to_ram(CONFIG_VIDEO_RATE, data_tmp, len);
			}

			get_config_item_value(CONFIG_VIDEO_BRIGHT, data_tmp, &n);
			if (*(text + VIDEO_BRIGHT_OFFSET) != atoi(data_tmp)) {
				printf("CONFIG_VIDEO_BRIGHT: %d, %d\n", *(text + VIDEO_BRIGHT_OFFSET), atoi(data_tmp));
				if (*(text + VIDEO_BRIGHT_OFFSET) > 0
						&& *(text + VIDEO_BRIGHT_OFFSET) < 16) {
					memset(data_buf, 0 ,sizeof(data_buf));
					len = 0;
					data_buf[len] = BRIGHT;
					len += 4;
					data_buf[len] = *(text + VIDEO_BRIGHT_OFFSET)*15;
					len += 4;
					send_msg_to_otherthread(self, VIDEO_MSG_TYPE,
							MSG_VIDEO_P_SETTING_PARAMS, data_buf, len);
					len = sprintf(data_tmp, "%d", *(text + VIDEO_BRIGHT_OFFSET));
					set_config_value_to_ram(CONFIG_VIDEO_BRIGHT,
							data_tmp, len);
					bright_set = 1;//set ok
				}else{
					bright_set = 0;//not set
				}
			}

			get_config_item_value(CONFIG_VIDEO_CONSTRACT, data_tmp, &n);
			if (*(text + VIDEO_CONSTRACT_OFFSET) != atoi(data_tmp)) {
				printf("CONFIG_VIDEO_CONSTRACT: %d, %d\n", *(text + VIDEO_CONSTRACT_OFFSET), atoi(data_tmp));
				if (*(text + VIDEO_CONSTRACT_OFFSET) > 0
						&& *(text + VIDEO_CONSTRACT_OFFSET) < 7) {
					memset(data_buf, 0 ,sizeof(data_buf));
					len = 0;
					data_buf[len] = CONTRAST;
					len += 4;
					data_buf[len] = (*(text + VIDEO_CONSTRACT_OFFSET)-1) * 20;
					len += 4;
					send_msg_to_otherthread(self, VIDEO_MSG_TYPE,
							MSG_VIDEO_P_SETTING_PARAMS, data_buf, len);
					len = sprintf(data_tmp, "%d", *(text + VIDEO_CONSTRACT_OFFSET));
					set_config_value_to_ram(CONFIG_VIDEO_CONSTRACT,
							data_tmp, len);
					constract_set = 1;//set ok
				}else{
					constract_set = 0;//not set
				}
			}

			get_config_item_value(CONFIG_VIDEO_SIZE, data_tmp, &n);
			if (*(text + VIDEO_SIZE_OFFSET) != atoi(data_tmp)) {
				printf("CONFIG_VIDEO_SIZE: %d, %d\n", *(text + VIDEO_SIZE_OFFSET), atoi(data_tmp));
				if (*(text + VIDEO_SIZE_OFFSET) > 0
						&& *(text + VIDEO_SIZE_OFFSET) < 4) {
					memset(data_buf, 0 ,sizeof(data_buf));
					len = 0;
					data_buf[len] = SIZE;
					len += 4;
					data_buf[len] = *(text + VIDEO_SIZE_OFFSET);
					len += 4;
					send_msg_to_otherthread(self, VIDEO_MSG_TYPE,
							MSG_VIDEO_P_SETTING_PARAMS, data_buf, len);
					len = sprintf(data_tmp, "%d", *(text + VIDEO_SIZE_OFFSET));
					set_config_value_to_ram(CONFIG_VIDEO_SIZE,
							data_tmp, len);
					size_set = 1;//set ok
				}else{
					size_set = 0;  //not set
				}
			}

			get_config_item_value(CONFIG_VIDEO_HFLIP, data_tmp, &n);
			if (*(text + VIDEO_HFLIP_OFFSET) != atoi(data_tmp)) {
				printf("CONFIG_VIDEO_HFLIP: %d, %d\n", *(text + VIDEO_HFLIP_OFFSET), atoi(data_tmp));
				if (*(text + VIDEO_HFLIP_OFFSET) > 0
						&& *(text + VIDEO_HFLIP_OFFSET) < 3) {
					memset(data_buf, 0 ,sizeof(data_buf));
					len = 0;
					data_buf[len] = HFLIP;
					len += 4;
					data_buf[len] = *(text + VIDEO_HFLIP_OFFSET) - 1;
					len += 4;
					send_msg_to_otherthread(self, VIDEO_MSG_TYPE,
							MSG_VIDEO_P_SETTING_PARAMS, data_buf, len);
					len = sprintf(data_tmp, "%d", *(text + VIDEO_SIZE_OFFSET));
					set_config_value_to_ram(CONFIG_VIDEO_HFLIP,
							data_tmp, len);
					hflip_set = 1;//set ok
				}else{
					hflip_set = 0;//not set
				}
			}

			get_config_item_value(CONFIG_VIDEO_VFLIP, data_tmp, &n);
			if (*(text + VIDEO_VFLIP_OFFSET) != atoi(data_tmp)) {
				printf("CONFIG_VIDEO_VFLIP: %d, %d\n", *(text + VIDEO_VFLIP_OFFSET), atoi(data_tmp));
				if (*(text + VIDEO_VFLIP_OFFSET) > 0
						|| *(text + VIDEO_VFLIP_OFFSET) < 3) {
					memset(data_buf, 0 ,sizeof(data_buf));
					len = 0;
					data_buf[len] = VFLIP;
					len += 4;
					data_buf[len] = *(text + VIDEO_VFLIP_OFFSET) - 1;
					len += 4;
					send_msg_to_otherthread(self, VIDEO_MSG_TYPE,
							MSG_VIDEO_P_SETTING_PARAMS, data_buf, len);
					len = sprintf(data_tmp, "%d", *(text + VIDEO_SIZE_OFFSET));
					set_config_value_to_ram(CONFIG_VIDEO_VFLIP,
							data_tmp, len);
					vflip_set = 1; //set ok
				}else{
					vflip_set = 0;//not set
				}
			}

			memset(data_buf, 0, sizeof(data_buf));
			len = 4;
			data_buf[len] = 0;
			len += 1;
			data_buf[len] = bright_set;
			len += 1;
			data_buf[len] = constract_set;
			len += 1;
			data_buf[len] = size_set;
			len += 1;
			data_buf[len] = hflip_set;
			len += 1;
			data_buf[len] = vflip_set;
			len += 1;
			data_buf[len] = 0;
			len += 1;
			send_msg_to_network(*fd, NEWS_VIDEO_SET_REP, data_buf, len, path);
			break;

		case NEWS_VIDEO_QUERY_REQ:
			memset(data_buf, 0, sizeof(data_buf));
			len = 4;
			data_buf[len] = 1;
			len += 1;
			//真率
			memset(data_tmp, 0, sizeof(data_tmp));
			get_config_item_value(CONFIG_VIDEO_SIMPLE, data_tmp, &n);
			data_buf[len] = atoi(data_tmp);
//			DEBUGPRINT(DEBUG_INFO, "a: %d\n", data_buf[len]);
			len += 1;
			//亮度
			memset(data_tmp, 0, sizeof(data_tmp));
			get_config_item_value(CONFIG_VIDEO_BRIGHT, data_tmp, &n);
			data_buf[len] = atoi(data_tmp);
			//DEBUGPRINT(DEBUG_INFO, "b: %s\n", data_tmp);
			DEBUGPRINT(DEBUG_INFO, "b: %d\n", data_buf[len]);
			len += 1;
			//对比度
			memset(data_tmp, 0, sizeof(data_tmp));
			get_config_item_value(CONFIG_VIDEO_CONSTRACT, data_tmp, &n);
			data_buf[len] = atoi(data_tmp);
			DEBUGPRINT(DEBUG_INFO, "c: %d\n", data_buf[len]);
			len += 1;
			//图像大小
			memset(data_tmp, 0, sizeof(data_tmp));
			get_config_item_value(CONFIG_VIDEO_SIZE, data_tmp, &n);
			data_buf[len] = atoi(data_tmp);
			DEBUGPRINT(DEBUG_INFO, "d: %d\n", data_buf[len]);
			len += 1;
			//图像水平翻转
			memset(data_tmp, 0, sizeof(data_tmp));
			get_config_item_value(CONFIG_VIDEO_HFLIP, data_tmp, &n);
			data_buf[len] = atoi(data_tmp);
			DEBUGPRINT(DEBUG_INFO, "e: %d\n", data_buf[len]);
			len += 1;
			//图像垂直翻转
			memset(data_tmp, 0, sizeof(data_tmp));
			get_config_item_value(CONFIG_VIDEO_VFLIP, data_tmp, &n);
			data_buf[len] = atoi(data_tmp);
			DEBUGPRINT(DEBUG_INFO, "f: %d\n", data_buf[len]);
			len += 1;
			//采样率
			memset(data_tmp, 0, sizeof(data_tmp));
			get_config_item_value(CONFIG_VIDEO_RATE, data_tmp, &n);
			data_buf[len] = atoi(data_tmp);
			DEBUGPRINT(DEBUG_INFO, "g: %d\n", data_buf[len]);
			len += 1;

			send_msg_to_network(*fd, NEWS_VIDEO_QUERY_REP, data_buf, len, path);
			break;

		default:
			break;
	}
}

/**
   @brief 控制云台的命令
   @author yangguozheng
   @param[in|out] self 运行环境
   @return
   @note cmd:500~504
 */
void NewsAdapter_newsChannelCameraCtrl(void *self)
{
	msg_container *context = (msg_container *) self;
	MsgData *msgRcv = (MsgData *) context->msgdata_rcv;
	MsgData *msgSnd = (MsgData *) context->msgdata_snd;
	int *fd = (int *) (msgRcv + 1);
	RequestHead *head = (RequestHead *) (fd + 1);
	char *text = (char *) (head + 1);
	short *data = (short *) (msgSnd + 1);

	msgSnd->type = SERIAL_PORTS_MSG_TYPE;
	switch(head->cmd){
//	case NEWS_CAMERA_MOVE_ABS:
//		msgSnd->cmd = NEWS_CHANNEL_DECLARATION_REQ;
//		*data = *(short *)(text+4);
//		*(data+1) = *(short *)(text+6);
//		msgSnd->len = 4;
//		msg_queue_snd(context->rmsgid, msgSnd, sizeof(MsgData) - sizeof(long) + msgSnd->len);
//		break;
//
//	case NEWS_CAMERA_POSITION:
//		msgSnd->cmd = MSG_SP_T_MOVE_ABS;
//		if(*(text+4) == CAMERA_HORIZONTAL){
//			*data = 0;
//		}else if(*(text+4) == CAMERA_VERTICALITY){
//			*(data+1) = 0;
//		}else if(*(text+4) == CAMERA_MIDDLE){
//			*data = 0;
//			*(data+1) = 0;
//		}
//		msgSnd->len = 4;
//		msg_queue_snd(context->rmsgid, msgSnd, sizeof(MsgData) - sizeof(long) + msgSnd->len);
//		break;
//
	case NEWS_CAMERA_CRUISE:
//		msgSnd->cmd = MSG_SP_T_CRUISE;
//		msgSnd->len = 2;
//		msg_queue_snd(context->rmsgid, msgSnd, sizeof(MsgData) - sizeof(long) + msgSnd->len);
		if(appInfo.cameraMode == INFO_FACTORY_MODE){
			int on;
			if(*(text+5) == 0){
				on = 1;
			}else{
				on = 0;
			}
			serialport_foctory_mode(on);
		}else{
			char dir;
			if(*(text+4) == 1){//水平
				dir = 0x0C;
			}else if(*(text+4)== 2){//垂直
				dir = 0x03;
			}else if(*(text+4) == 3){//水平垂直
				dir = 0x0F;
			}
			serialport_cruise(*(text+5), dir);
		}
		break;

	case NEWS_CAMERA_MOVE_REL:
		msgSnd->cmd = MSG_SP_T_MOVE_REL_SPEED;
		msgSnd->len = 6;
		memcpy(msgSnd + 1, text+4, msgSnd->len);
		msg_queue_snd(context->rmsgid, msgSnd, sizeof(MsgData) - sizeof(long) + msgSnd->len);
		break;

//	case NEWS_CAMERA_MOVE_RES:
//			break;
	default:
		break;
	}
}

/**
   @brief 读取报警信息
   @author yangguozheng
   @param[in|out] self 运行环境
   @return
   @note
 */
int read_flash_alarmdata(char *data)
{
	int len, n;
	char data_buf[8];

	len = 0;
	memset(&data[len], 0, 4); //动态口令  0
	len += 4;

	memset(data_buf, 0, sizeof(data_buf));
	get_config_item_value(CONFIG_ALARM_MOTION, data_buf, &n);
	//DEBUGPRINT(DEBUG_INFO, "video alarm switch: %c\n", data[0]);
	if(data_buf[0] == 'y'){
		data[len] = 1;
	}else {
		data[len] = 2;
	}
	//data[len] = atoi(data_buf); //移动报警     4
	len += 1;

	memset(data_buf, 0, sizeof(data_buf));
	get_config_item_value(CONFIG_ALARM_MOTION_SENSITIVITY, data_buf, &n);
	//DEBUGPRINT(DEBUG_INFO, "video level: %c\n", data[0]);
	data[len] = atoi(data_buf); //移动报警灵敏度    5
	len += 1;
	memset(&data[len], 0, 16); //坐标   6
	len += 16;

	memset(data_buf, 0, sizeof(data_buf));
	get_config_item_value(CONFIG_ALARM_AUDIO, data_buf, &n);
	//DEBUGPRINT(DEBUG_INFO, "audio alarm switch: %c\n", data[0]);
	if (data_buf[0] == 'y') {
		data[len] = 1;
	} else {
		data[len] = 2;
	}
	//data[len] = atoi(data_buf); //声音报警    22
	len += 1;

	memset(data_buf, 0, sizeof(data_buf));
	get_config_item_value(CONFIG_ALARM_AUDIO_SENSITIVITY, data_buf, &n);
	//DEBUGPRINT(DEBUG_INFO, "audio alarm level: %c\n", data[0]);
	data[len] = atoi(data_buf); //声音灵敏度报警   23
	len += 1;

	memset(data_buf, 0, sizeof(data_buf));
	get_config_item_value(CONFIG_ALARM_BATTERY, data_buf, &n);
	if (data_buf[0] == 'y') {
		data[len] = 1;
	} else {
		data[len] = 2;
	}
	//data[len] = 1; //电池报警   24
	len += 1;
	data[len] = 1; //报警图片上传  25
	len += 1;
	data[len] = 1; //报警录像上传  26
	len += 1;
#if 0
	data[len] = 1; //报警消息推送   27
	len += 1;
#endif
	memset(data_buf, 0, sizeof(data_buf));
	get_config_item_value(CONFIG_ALARM_START_TIME, data_buf, &n);
	//memcpy(&data[len], data_buf, 4); //报警开始时间  28
	*(int *)(&(data[len])) = atoi(data_buf);
	len += 4;

	memset(data_buf, 0, sizeof(data_buf));
	get_config_item_value(CONFIG_ALARM_END_TIME, data_buf, &n);
	//memcpy(&data[len], data_buf, 4); //报警结束时间  32
	*(int *)(&(data[len])) = atoi(data_buf);
	len += 4;

	return len;
}

/**
   @brief 报警设置和查询
   @author yangguozheng
   @param[in|out] self 运行环境
   @return
   @note
 */
void NewsAdapter_newsChannelAlarmCtrl(void *self)
{
	msg_container *context = (msg_container *) self;
	MsgData *msgRcv = (MsgData *) context->msgdata_rcv;
	MsgData *msgSnd = (MsgData *) context->msgdata_snd;
	int *fd = (int *) (msgRcv + 1);
	RequestHead *head = (RequestHead *) (fd + 1);
	char *text = (char *) (head + 1);
	short *data = (short *) (msgSnd + 1);
	int len;
	char data_buf[8];
	int n;
	int ret;
	int path;

	if (strncmp(head->head, "TUTK", 4) == 0) {
		path = 2;
	} else {
		path = 1;
	}

	switch(head->cmd){
	case NEWS_ALARM_QUERY_REQ:
		memset(context->msgdata_snd, 0, sizeof(context->msgdata_snd));
		len = read_flash_alarmdata(context->msgdata_snd);
//		DEBUGPRINT(DEBUG_INFO, "alarm switch: %d, %d, %d, %d", context->msgdata_snd[4], context->msgdata_snd[5], context->msgdata_snd[22], context->msgdata_snd[23]);
		send_msg_to_network(*fd, NEWS_ALARM_QUERY_REP, context->msgdata_snd, len, path);
		break;

//	case NEWS_ALARM_QUERY_REP:
//		break;
	case NEWS_ALARM_CONFIG_REQ:
		//判断音视频报警灵敏度的范围
		if(*(text+5) < 0 || *(text+5) > 9){
			DEBUGPRINT(DEBUG_ERROR, "motion alarm level error... value:%d\n ", *(text+5));
			break;
		}
		if(*(text+23) < 0 || *(text+23) > 9){
			DEBUGPRINT(DEBUG_ERROR, "audio alarm level error...value:%d\n ", *(text+23));
			break;
		}

		//移动报警开关设置
		int on;
		get_config_item_value(CONFIG_ALARM_MOTION, data_buf, &n);
		if (data_buf[0] == 'y') {
			on = 1;
		} else {
			on = 2;
		}
		//移动报警灵敏度设置
		get_config_item_value(CONFIG_ALARM_MOTION_SENSITIVITY, data_buf, &n);
		int level = atoi(data_buf);
		//printf("video: on:%d  level:%d\n", *(text+4), *(text+5));
		if(*(text+4) != on || *(text+5) != level){
			DEBUGPRINT(DEBUG_INFO, "video: on:%d  level:%d\n", on, level);
		//	memcpy(data_buf, text+4, 1);
			*(int*)data_buf = (int)(*(text+4));
		//	memcpy(data_buf+4, text+5, 1); //level
			*(int*)(data_buf+4) = (int)(*(text+5));
			send_msg_to_otherthread(self, VIDEO_ALARM_MSG_TYPE, MSG_VA_T_CONTROL, data_buf, 8);
			if(*(text+4) == 1){
				data_buf[0] = 'y';
			}else{
				data_buf[0] = 'n';
			}
			ret = set_config_value_to_ram(CONFIG_ALARM_MOTION,
					data_buf, 1);
			data_buf[0] = *(text+5)+'0';
			ret = set_config_value_to_ram(CONFIG_ALARM_MOTION_SENSITIVITY,
					data_buf, 1);
		}

		//音频报警开关设置
		get_config_item_value(CONFIG_ALARM_AUDIO, data_buf, &n);
		if (data_buf[0] == 'y') {
			on = 1;
		} else {
			on = 2;
		}
		//printf("audio alarm: on:%d level:%d\n", *(text+22), *(text + 23));
		DEBUGPRINT(DEBUG_INFO, "audio alarm: on:%d level:%d\n", *(text+22), *(text + 23));
		if(*(text+22) != on){// || *(text+23) != atoi(data_buf)){
			if (*(text+22) == 1) {
				if(send_msg_to_otherthread(self, AUDIO_ALARM_MSG_TYPE, MSG_AUDIOALARM_T_START_AUDIO_CAPTURE, NULL, 0) < 0){
					printf("send AUDIOALARM start failed\n");
				}
			}else if (*(text+22) == 2) {
				if(send_msg_to_otherthread(self, AUDIO_ALARM_MSG_TYPE, MSG_AUDIOALARM_T_STOP_AUDIO_CAPTURE, NULL, 0) < 0){
					printf("send AUDIOALARM stop failed\n");
				}
			}
			if(*(text+22) == 1){
				data_buf[0] = 'y';
			}else{
				data_buf[0] = 'n';
			}
			ret = set_config_value_to_ram(CONFIG_ALARM_AUDIO,
					data_buf, 1);
		}

		get_config_item_value(CONFIG_ALARM_AUDIO_SENSITIVITY, data_buf, &n);
		if(*(text+23) != atoi(data_buf)){
			//音频报警灵敏度设置
			int level = *(text+23) + '0';
			memcpy(data_buf, &level, sizeof(level));
			if (send_msg_to_otherthread(self, AUDIO_ALARM_MSG_TYPE, MSG_AUDIOALARM_T_ALARM_LEVEL,
						data_buf, 4) < 0) {
				DEBUGPRINT(DEBUG_ERROR, "send AUDIOALARM level failed\n");
			}
			data_buf[0] = *(text+23)+'0';
			ret = set_config_value_to_ram(CONFIG_ALARM_AUDIO_SENSITIVITY,
					data_buf, 1);
		}

		get_config_item_value(CONFIG_ALARM_BATTERY, data_buf, &n);
		if (data_buf[0] == 'y') {
			on = 1;
		} else {
			on = 2;
		}
		if(*(text+24) != on){
			//电池报警开关设置
			if (send_msg_to_otherthread(self, SERIAL_PORTS_MSG_TYPE, MSG_SP_T_BATTERY_ALARM,
					text+24, 1) < 0) {
				DEBUGPRINT(DEBUG_ERROR, "send AUDIOALARM level failed\n");
			}

			if (*(text + 24) == 1) {
				data_buf[0] = 'y';
			} else{
				data_buf[0] = 'n';
			}
			ret = set_config_value_to_ram(CONFIG_ALARM_BATTERY, data_buf, 1);
		}

		memset(context->msgdata_snd, 0, sizeof(context->msgdata_snd));
		len = 0;
		memset(&context->msgdata_snd[len], 0, 4); //动态口令  0
		len += 4;
		context->msgdata_snd[len] = 1;
		len += 1;
		send_msg_to_network(*fd, NEWS_ARARM_CONFIG_REP, context->msgdata_snd, len, path);
		break;
//	case NEWS_ARARM_CONFIG_REP:
//		break;
//	case NEWS_ALARM_EVENT_NOTIFY:
//		break;
	}
}

#if 0
void NewsAdapter_newsAudioTransmitReq(void *self) {
	msg_container *context = (msg_container *)self;
	MsgData *msgRcv = (MsgData *)context->msgdata_rcv;
	MsgData *msgSnd = (MsgData *)context->msgdata_snd;
	int *fd = (int *)(msgRcv + 1);
	RequestHead *head = (RequestHead *)(fd + 1);
	char *text = (char *)(head + 1);
	TEST_PRINT("accept: %d\n", *fd);
//	if(*(text + 4) == 1) {
//
//	}
//	else {
//
//	}
//	if(*(text + 4 + 1) == 1) {
//
//	}
//	else {
//
//	}
	head->cmd = NEWS_AUDIO_TRANSMIT_REP;
	memcpy(head->head, "IBBM", 4);
	head->version = 1;
	head->len = 6;
	*(int *)text = 0;
	*(text + 4) = 1;
	*(text + 5) = 2;
	//*(int *)(text + 6) = 0;
	if(-1 == NewsUtils_sendToNet(*fd, head, sizeof(RequestHead) + head->len, 0)) {
		DEBUGPRINT(DEBUG_INFO, "send error \n");
	}
	TEST_PRINT("accept: %d\n", *fd);
}
#endif

/**
   @brief 音频输出
   @author yangguozheng
   @param[in|out] self 运行环境
   @return
   @note
 */
void NewsAdapter_newsAudioTransmitOut(void *self) {
	msg_container *context = (msg_container *)self;
	MsgData *msgRcv = (MsgData *)context->msgdata_rcv;
	MsgData *msgSnd = (MsgData *)context->msgdata_snd;
	int *fd = (int *)(msgRcv + 1);
	RequestHead *head = (RequestHead *)(fd + 1);
	char *text = (char *)(head + 1);
	DEBUGPRINT(DEBUG_INFO, "%s\n", __FUNCTION__);
}

/**
   @brief 音频输入
   @author yangguozheng
   @param[in|out] self 运行环境
   @return
   @note
 */
void NewsAdapter_newsAudioTransmitIn(void *self) {
	msg_container *context = (msg_container *)self;
	MsgData *msgRcv = (MsgData *)context->msgdata_rcv;
	MsgData *msgSnd = (MsgData *)context->msgdata_snd;
	int *fd = (int *)(msgRcv + 1);
	//int *audioFd = fd + 1;
	RequestHead *head = (RequestHead *)(fd + 1);
//	int *audioFd = (int *)((char *)head + sizeof(RequestHead) + head->len);
	char *text = (char *)(head + 1);
	DEBUGPRINT(DEBUG_INFO, "%s\n", __FUNCTION__);
	DEBUGPRINT(DEBUG_INFO, "head->len: %d\n", head->len);
//	DEBUGPRINT(DEBUG_INFO, "fd: %d, audio fd: %d\n", *fd, *audioFd);
}

/**
   @brief 音频请求
   @author yangguozheng
   @param[in|out] self 运行环境
   @return
   @note
 */
void NewsAdapter_newsAudioTransmitReq(void *self) {
	DEBUGPRINT(DEBUG_INFO, "NewsAdapter_newsAudioTransmitReq\n");
	msg_container *context = (msg_container *)self;
	MsgData *msgRcv = (MsgData *)context->msgdata_rcv;
	MsgData *msgSnd = (MsgData *)context->msgdata_snd;
	int *fd = (int *)(msgRcv + 1);
	//int *audioFd = fd + 1;
	RequestHead *head = (RequestHead *)(fd + 1);
//	int *audioFd = (int *)((char *)head + sizeof(RequestHead) + head->len);
	char *text = (char *)(head + 1);
    DEBUGPRINT(DEBUG_INFO, "%s audio: %d, speaker: %d\n", __FUNCTION__, *(text + 4), *(text + 5));
#if 0
	if(*(text + 4) == 1) {
#ifndef _DISABLE_AUDIO_SEND
		int *audioFd = (int *)((char *)head + sizeof(RequestHead) + head->len);
		msgSnd->type = AUDIO_SEND_MSG_TYPE;
		msgSnd->cmd = MSG_AUDIOSEND_P_AUDIO_SEND;
		*(int *)(msgSnd + 1) = *audioFd;
		DEBUGPRINT(DEBUG_INFO, "audio fd: %d\n", *audioFd);
		msgSnd->len = sizeof(int);
//			TEST_PRINT("accept: %d\n", *fd);
		msg_queue_snd(context->rmsgid, msgSnd, sizeof(MsgData) - sizeof(long) + msgSnd->len);
#endif
	}else if(*(text + 4) == 2){
		int *audioFd = (int *)((char *)head + sizeof(RequestHead) + head->len);
		msgSnd->type = AUDIO_SEND_MSG_TYPE;
		msgSnd->cmd = MSG_AUDIOSEND_T_AUDIO_ONE_STOP;
		*(int *) (msgSnd + 1) = *audioFd;
		DEBUGPRINT(DEBUG_INFO, "audio fd: %d\n", *audioFd);
		msgSnd->len = sizeof(int);
		//			TEST_PRINT("accept: %d\n", *fd);
		msg_queue_snd(context->rmsgid, msgSnd,
				sizeof(MsgData) - sizeof(long) + msgSnd->len);
	}
	if(*(text + 5) == 1) {

	}
#endif

//	DEBUGPRINT(DEBUG_INFO, "type: %d, qq: %d", *(text + 4), *(text + 5));
	head->cmd = NEWS_AUDIO_TRANSMIT_REP;
	memcpy(head->head, "IBBM", 4);
	head->version = 1;
	head->len = 6;
//	*(int *)text = 0;
//	*(text + 4) = 1;
//	*(text + 5) = 1;
//	*(int *)(text + 6) = 0;
//	DEBUGPRINT(DEBUG_INFO, "head->cmd: %d\n", head->cmd);
	if(-1 == NewsUtils_sendToNet(*fd, head, sizeof(RequestHead) + head->len, 0)) {
		DEBUGPRINT(DEBUG_INFO, "send error \n");
	}
}

/**
   @brief 电量查询
   @author yangguozheng
   @param[in|out] self 运行环境
   @return
   @note
 */
void NewsAdapter_newsCameraStaminaReq(void *self) {
    msg_container *context = (msg_container *)self;
	MsgData *msgRcv = (MsgData *)context->msgdata_rcv;
	MsgData *msgSnd = (MsgData *)context->msgdata_snd;
	int *fd = (int *)(msgRcv + 1);
	RequestHead *head = (RequestHead *)(fd + 1);
    head->len = 5;
    head->cmd = NEWS_CAMERA_STAMINA_REP;
	char *text = (char *)(head + 1);
    int vStamina = serialport_get_battery_state();
    DEBUGPRINT(DEBUG_INFO, "camera stamina: %X\n", vStamina);
    *(text + 4) = *(((char *)&vStamina) + 2);
    if(-1 == NewsUtils_sendToNet(*fd, head, sizeof(RequestHead) + head->len, 0)) {
		DEBUGPRINT(DEBUG_INFO, "send error \n");
	}
}

/**
   @brief 设置夜间模式
   @author yangguozheng
   @param[in|out] self 运行环境
   @return
   @note
 */
void NewsAdapter_newsCameranightmodesetreq(void *self)
{
	msg_container *context = (msg_container *)self;
	MsgData *msgRcv = (MsgData *)context->msgdata_rcv;
	MsgData *msgSnd = (MsgData *)context->msgdata_snd;
	int *fd = (int *)(msgRcv + 1);
	RequestHead *head = (RequestHead *)(fd + 1);
	head->len = 5;
	head->cmd = NEWS_CAMERA_STAMINA_REP;
	char *text = (char *)(head + 1);
	char led_st[4];
	int n;

	DEBUGPRINT(DEBUG_INFO, "NEWS_NIGHT_MODE_REQ: %d, %d, %d, %d\n", *(text+4), *(int*)(text+5),
						runtimeinfo_get_nm_state(), runtimeinfo_get_nm_time())
	if(*(text+4) == NIGHT_MODE_ON){
		if(runtimeinfo_get_nm_state() == NIGHT_MODE_OFF){
			int *val = (int *) (text + 5);
			runtimeinfo_set_nm_time(*val);

			night_mode_ctl_msg_handler(LED_NIGHT_MODE);

		}
//		DEBUGPRINT(DEBUG_INFO, "NewsAdapter_newsCameranightmodesetreq test print M3S why not work+++ NIGHT_MODE_ON\n");
	}else{
		if(runtimeinfo_get_nm_state() == NIGHT_MODE_ON){
//			DEBUGPRINT(DEBUG_INFO, "NewsAdapter_newsCameranightmodesetreq test print M3S why not work+++ NIGHT_MODE_ON in\n");
			runtimeinfo_set_nm_time(0);
			night_mode_ctl_msg_handler(LED_NOT_NIGHT_MODE);
#if 0
			//开灯
			if(g_nDeviceType == DEVICE_TYPE_M2){
				get_config_item_value(CONFIG_LIGHT_NIGHT, led_st, &n);
				if(strncmp(led_st, "y", n) == 0){
					night_light_state_msg_handler();
				}
				get_config_item_value(CONFIG_LIGHT_NET, led_st, &n);
				if(strncmp(led_st, "y", n) == 0){
					led_if_work_control(LED_TYPE_NET_POWER, LED_FLAG_WORK, FALSE);
				}
			}else{
				get_config_item_value(CONFIG_LIGHT_NIGHT, led_st, &n);
				if(strncmp(led_st, "y", n) == 0){
					night_light_state_msg_handler();
				}

				get_config_item_value(CONFIG_LIGHT_NET, led_st, &n);
				if(strncmp(led_st, "y", n) == 0){
					led_if_work_control(LED_TYPE_NET, LED_FLAG_WORK, FALSE);
				}

//				DEBUGPRINT(DEBUG_INFO, "NewsAdapter_newsCameranightmodesetreq test print M3S why not work+++ before get config\n");
				get_config_item_value(CONFIG_LIGHT_POWER, led_st, &n);
//				DEBUGPRINT(DEBUG_INFO, "NewsAdapter_newsCameranightmodesetreq test print M3S why not work+++ after get config\n");
				if(strcmp(led_st, "y") == 0){ //not 'y'
//					DEBUGPRINT(DEBUG_INFO, "NewsAdapter_newsCameranightmodesetreq test print M3S why not work+++ after if\n");
					led_if_work_control(LED_TYPE_POWER, LED_FLAG_WORK, FALSE);
//					DEBUGPRINT(DEBUG_INFO, "NewsAdapter_newsCameranightmodesetreq test print M3S why not work+++ after if ==\n");
				}
//				DEBUGPRINT(DEBUG_INFO, "NewsAdapter_newsCameranightmodesetreq test print M3S why not work+++ after out if\n");
			}
#endif
		}
//		DEBUGPRINT(DEBUG_INFO, "NewsAdapter_newsCameranightmodesetreq test print M3S why not work+++ NIGHT_MODE_ON else");
	}

	runtimeinfo_set_nm_state(*(text+4));
	DEBUGPRINT(DEBUG_INFO, "NEWS_NIGHT_MODE_REQ: %d, %d\n", runtimeinfo_get_nm_state(), runtimeinfo_get_nm_time())
	tutk_send_set_nightmode_command(runtimeinfo_get_nm_state(), runtimeinfo_get_nm_time());
//	DEBUGPRINT(DEBUG_INFO, "NewsAdapter_newsCameranightmodesetreq test print M3S why not work+++");
	NewsChannel_nightModeAction(runtimeinfo_get_nm_state());
}

/**
   @brief 获取夜间模式记录
   @author yangguozheng
   @param[in|out] self 运行环境
   @return
   @note
 */
void NewsAdapter_newsCameranightmoderecordreq(void *self)
{
	msg_container *context = (msg_container *) self;
	MsgData *msgRcv = (MsgData *) context->msgdata_rcv;
	MsgData *msgSnd = (MsgData *) context->msgdata_snd;
	int *fd = (int *) (msgRcv + 1);
	RequestHead *head = (RequestHead *) (fd + 1);
	int path, len;

	if (strncmp(head->head, "TUTK", 4) == 0) {
		path = 2;
	} else {
		path = 1;
	}

	memset(context->msgdata_snd, 0, 4);
	len = 4;
	context->msgdata_snd[len] = runtimeinfo_get_nm_state();
	len += 1;
	int timeval = runtimeinfo_get_nm_time();
	memcpy(&context->msgdata_snd[len], &timeval, sizeof(timeval));
	len += 4;
	DEBUGPRINT(DEBUG_INFO, "NEWS_NIGHT_MODE_RECORD_REP.......%d.. %d\n", runtimeinfo_get_nm_state(), timeval);
	send_msg_to_network(*fd, NEWS_NIGHT_MODE_RECORD_REP, context->msgdata_snd, len, path);
}

/**
   @brief 心跳报2
   @author yangguozheng
   @param[in|out] self 运行环境
   @return
   @note
 */
void NewsAdapter_newsChannelTouchReqV2(void *self)
{
	msg_container *context = (msg_container *) self;
	MsgData *msgRcv = (MsgData *) context->msgdata_rcv;
	MsgData *msgSnd = (MsgData *) context->msgdata_snd;
	int *fd = (int *) (msgRcv + 1);
	RequestHead *head = (RequestHead *) (fd + 1);
	int path, len;

	if (strncmp(head->head, "TUTK", 4) == 0) {
		path = 2;
	} else {
		path = 1;
	}

	memset(context->msgdata_snd, 0, 4);
	context->msgdata_snd[4] = 1;
	context->msgdata_snd[5] = 1;
	len = 6;
	send_msg_to_network(*fd, NEWS_CHANNEL_WRIGGLE_REP_V2, context->msgdata_snd, len, path);
}
