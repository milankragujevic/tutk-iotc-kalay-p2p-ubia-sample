/** @file [VideoAlarmAdapter.c]
 *  @brief 视频报警线程的外部交互C文件
 *	@author: Summer.Wang
 *	@data: 2013-9-24
 *	@modify record: 即日创建了这套文件注释系统
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <time.h>
#include "common/thread.h"
#include "common/common.h"
#include "common/msg_queue.h"
#include "common/logfile.h"
#include "common/utils.h"
#include "common/logfile.h"
#include "common/common_config.h"
#include "HTTPServer/httpsserver.h"
#include "VideoAlarm.h"
#include "VideoAlarmAdapter.h"
#include "Video/video.h"
#include "AudioAlarm/audioalarm.h"
#include "NewsChannel/NewsChannelAdapter.h"
#include "p2p/tutk_rdt_video_server.h"


/**
 * @brief 向外交互的消息函数
 * @author Summer.Wang
 * @return Null
 * @param[out] <self><控制及查询模块属性>
 * @note 子进程调用传给控制通道和转发，报警被触发；参数为报警类型（1为视频报警）和报警状态（1为报警中，2为报警结束）
 */
void videoalarm_process_alarm(msg_container *self)
{
	VA_MsgData *msgsnd = (VA_MsgData *)self->msgdata_snd;
	char *data = (char*)msgsnd->data;

	/*-----------------控制通道-------------------*/
	msgsnd->len = 6;
	msgsnd->type = NEWS_CHANNEL_MSG_TYPE;
	msgsnd->cmd = MSG_NEWS_CHANNEL_T_NEWS_BROADCAST;
	memset(data,0,4);
	*(data+4) = 1;
	*(data+5) = VideoAlarm_list->alarm_state;
	if(msg_queue_snd(self->rmsgid, self->msgdata_snd, sizeof(struct MsgData)-sizeof(long int) + 6) == -1)
	{
		return;
	}

	msgsnd->type = P2P_MSG_TYPE;
	msgsnd->cmd = MSG_P2P_T_ALARMEVENT;
	if(msg_queue_snd(self->rmsgid, self->msgdata_snd, sizeof(struct MsgData)-sizeof(long int) + 6) == -1)
	{
		return;
	}
}

/**
 * @brief 向外交互的消息函数
 * @author Summer.Wang
 * @return Null
 * @param[out] <self><控制及查询模块属性>
 * @note 子进程调用传给HTTPS，报警取图动作完成；参数为图片存入的路径
 */
void videoalarm_process_finish(msg_container *self)
{
	VA_MsgData *msgsnd = (VA_MsgData *)self->msgdata_snd;
	int *data = (int*)msgsnd->data;

	/*---------------时间戳＋类型---------------*/
	msgsnd->len = 8;
	msgsnd->type = HTTP_SERVER_MSG_TYPE;
	msgsnd->cmd = MSG_HTTPSSEVER_T_PUSH_ALARM_2_CLOUND_CAM ;

	*data = VideoAlarm_list->timestamp;
	if(VideoAlarm_list->alarm_type == 0) 	*(data+1) = 1;//VideoAlarm_list->alarm_type = 1;//移动报警
	else 									*(data+1) = 2;//VideoAlarm_list->alarm_type = 2;//声音报警

//	*(data+1) = (int)VideoAlarm_list->alarm_type;
	VideoAlarm_list->alarm_type = 0;

#ifdef Http_LOG_ON
	videoalarm_write_log_info_to_file("In the Videoalarm Adapter,finish message,will send to HTTPS the alarm type and timestamp .\n","a+");
#endif

	if(msg_queue_snd(self->rmsgid, self->msgdata_snd, sizeof(struct MsgData)-sizeof(long int) + 8) == -1)
	{
		return;
	}

	/*-----------------图像路径-------------------*/
	char *str = VideoAlarm_list->pathname;
	msgsnd->len = strlen(str);
	msgsnd->type = HTTP_SERVER_MSG_TYPE;
	msgsnd->cmd = MSG_HTTPSSEVER_T_UPLOAD_FILE_CAM;
	memcpy(msgsnd->data,VideoAlarm_list->pathname,50);

#ifdef Http_LOG_ON
	videoalarm_write_log_info_to_file("In the Videoalarm Adapter,finish message,will send to HTTPS the file route.\n","a+");
#endif

	if(msg_queue_snd(self->rmsgid, self->msgdata_snd, sizeof(struct MsgData)-sizeof(long int)+strlen(str)) == -1)
	{
		return;
	}

#ifdef Http_LOG_ON
	videoalarm_write_log_info_to_file("In the Videoalarm Adapter,finish message,send finish.\n","a+");
#endif

}

/**
 * @brief 向外交互的消息函数
 * @author Summer.Wang
 * @return Null
 * @param[out] <self><控制及查询模块属性>
 * @note 子进程调用，停止动作完成后，将该全局变量置高
 */
void videoalarm_process_reply(msg_container *self)
{
	g_lVideoAlarm = 1;
}


/**
 * @brief 向外交互的消息函数
 * @author Summer.Wang
 * @return Null
 * @param[out] <self><控制及查询模块属性>
 * @note 留用接口
 */
void videoalarm_process_control(msg_container *self)
{
	DEBUGPRINT(DEBUG_INFO,"videoalarm :%s \n",__FUNCTION__);
}

/**
 * @brief 向外交互的消息函数
 * @author Summer.Wang
 * @return Null
 * @param[out] <self><控制及查询模块属性>
 * @note 当图像花屏或白屏时，发送给视频采集更改图像大小
 */
void videoalarm_process_frame(msg_container *self)
{
	VA_MsgData *msgsnd = (VA_MsgData *)self->msgdata_snd;
	int *data = (int*)msgsnd->data;
	int length;
	char temp = 0;

	msgsnd->len = 8;
	msgsnd->type = VIDEO_MSG_TYPE;
	msgsnd->cmd = MSG_VIDEO_P_SETTING_PARAMS;

	get_config_item_value(CONFIG_VIDEO_SIZE,&temp,&length);
	temp -= '0';

	*data = 3;
	*(data+1) = temp;
	if(msg_queue_snd(self->rmsgid, self->msgdata_snd, sizeof(struct MsgData)-sizeof(long int) + 8) == -1)
	{
		return;
	}
}

/**
 * @brief 向外交互的消息函数
 * @author Summer.Wang
 * @return Null
 * @param[out] <self><控制及查询模块属性>
 * @note 报警后通知音频报警线程，使之30秒不报警
 */
void videoalarm_process_inform(msg_container *self)
{
	VA_MsgData *msgsnd = (VA_MsgData *)self->msgdata_snd;
	msgsnd->type = AUDIO_ALARM_MSG_TYPE;
	msgsnd->cmd = MSG_AUDIOALARM_T_STOP_4_VIDEO_CAPTURE;
	if(msg_queue_snd(self->rmsgid, self->msgdata_snd, sizeof(struct MsgData)-sizeof(long int)) == -1)
	{
		return;
	}
}

/**
 * @brief 向外交互的消息函数
 * @author Summer.Wang
 * @return Null
 * @param[out] <self><控制及查询模块属性>
 * @note 音频报警开始通知控制通道和p2p
 */
void videoalarm_process_audio_start(msg_container *self)
{
	VA_MsgData *msgsnd = (VA_MsgData *)self->msgdata_snd;
	char *data = (char*)msgsnd->data;

	msgsnd->type = NEWS_CHANNEL_MSG_TYPE;
	msgsnd->cmd = MSG_NEWS_CHANNEL_T_NEWS_BROADCAST;
	msgsnd->len = 6;

	memset(data, 0, 4);
	*(data+4) = 2;
	*(data+5) = 1;
	if(msg_queue_snd(self->rmsgid, self->msgdata_snd, sizeof(struct MsgData)-sizeof(long int) + 6) == -1)
	{
		return;
	}

	msgsnd->type = P2P_MSG_TYPE;
	msgsnd->cmd = MSG_P2P_T_ALARMEVENT;
	if(msg_queue_snd(self->rmsgid, self->msgdata_snd, sizeof(struct MsgData)-sizeof(long int) + 6) == -1)
	{
		return;
	}
}

/**
 * @brief 向外交互的消息函数
 * @author Summer.Wang
 * @return Null
 * @param[out] <self><控制及查询模块属性>
 * @note 音频报警结束通知控制通道和p2p
 */
void videoalarm_process_audio_stop(msg_container *self)
{
	VA_MsgData *msgsnd = (VA_MsgData *)self->msgdata_snd;
	char *data = (char*)msgsnd->data;

	msgsnd->type = NEWS_CHANNEL_MSG_TYPE;
	msgsnd->cmd = MSG_NEWS_CHANNEL_T_NEWS_BROADCAST;
	msgsnd->len = 6;

	memset(data, 0, 4);
	*(data+4) = 2;
	*(data+5) = 2;
	if(msg_queue_snd(self->rmsgid, self->msgdata_snd, sizeof(struct MsgData)-sizeof(long int) + 6) == -1)
	{
		return;
	}

	msgsnd->type = P2P_MSG_TYPE;
	msgsnd->cmd = MSG_P2P_T_ALARMEVENT;
	if(msg_queue_snd(self->rmsgid, self->msgdata_snd, sizeof(struct MsgData)-sizeof(long int) + 6) == -1)
	{
		return;
	}
}

/**
 * @brief 向外交互的消息函数
 * @author Summer.Wang
 * @return Null
 * @param[out] <self><控制及查询模块属性>
 * @note 子进程调用可测试线程是否正常
 */
void videoalarm_process_alive(msg_container *self)
{
	DEBUGPRINT(DEBUG_INFO,"videoalarm says: I am alive! \n");
}

/** 协议实现列表 */
cmd_item videoalarm_cmd_table[] = {
		{MSG_VA_P_ALARM,videoalarm_process_alarm},
		{MSG_VA_P_FINISH,videoalarm_process_finish},
		{MSG_VA_P_REPLY,videoalarm_process_reply},
		{MSG_VA_P_CONTROL,videoalarm_process_control},
		{MSG_VA_P_FRAME,videoalarm_process_frame},
		{MSG_VA_P_INFORM,videoalarm_process_inform},
		{MSG_VA_P_AUDIO_START,videoalarm_process_audio_start},
		{MSG_VA_P_AUDIO_STOP,videoalarm_process_audio_stop},
		{MSG_VA_P_ALIVE, videoalarm_process_alive},
};

/**
 * @brief 协议查询入口
 * @author Summer.Wang
 * ＠param[in|out] <self><适配器运行环境>
 * @return Null
 */
void videoalarm_cmd_table_fun(msg_container *self) {
	MsgData *msgrcv = (MsgData *)self->msgdata_rcv;
	cmd_item icmd;
	icmd.cmd = msgrcv->cmd;
	msg_fun fun = utils_cmd_bsearch(&icmd, videoalarm_cmd_table, sizeof(videoalarm_cmd_table)/sizeof(cmd_item));
	if(fun != NULL) {
		fun(self);
	}
}
