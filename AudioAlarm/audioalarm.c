/**   @file []
   *  @brief AudioAlarm的实现
   *  @note
   *  @author:   wjn&wzq
   *  @date 2013-5-7
   *  @remarks modify record:增加注释
*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include "common/common.h"
#include "common/utils.h"
#include "common/thread.h"
#include "common/msg_queue.h"
#include "Audio/Audio.h"
#include "audioalarm.h"
#include "audioalarm_common.h"
#include "testcommon.h"
#include "VideoAlarm/VideoAlarm.h"
#include "common/common_config.h"
#include "NewsChannel/NewsUtils.h"
/**
  *  @brief  测试使用打印程序
  *  @author  <wjn> 
  *  @param[in] <str想打印的话，打印当前时间>
  *  @note <无>
*/
void audioalarm_print_current_time(char *str)
{
        struct timeval tv;
        gettimeofday(&tv, NULL);
        printf("%s: %d.%d\n",str, tv.tv_sec, tv.tv_usec);
}
/**
  *  @brief  测试使用打印程序
  *  @author  <wjn> 
  *  @param[in] <Time:1 打印时间，tag:未用，text:自定义输出>
  *  @param[out] <无>
  *  @return  <0代表成功>   <-1代表失败>
  *  @note
*/
static void getTimeAndPrint(int Time,int tag,char *text){
	time_t mytime;
	time(&mytime);
	if(Time == 1){
		printf("Time :%s o(∩_∩)o --> Message:%s\n",ctime(&mytime),text);
	}else{
		printf(
				"File:%s Line:%d Time :%s \n "
				"o(∩_∩)o --> Message:%s\n",__FILE__, __LINE__,ctime(&mytime),text);
	}
}
/**启动AudioAlarmMain线程*/
void * AudioAlarmMain(void *args);

/**
  *  @brief  创建AudioAlarmMain线程函数
  *  @author  <wjn&wzq> 
  *  @param[in] <无>
  *  @param[out] <无>
  *  @return  线程PID
  *  @note
*/
int CreateAudioAlarmThread(void) {
	int createAudioAlarm = 0;
	createAudioAlarm = thread_start(AudioAlarmMain);
	return createAudioAlarm; //记得判断哦！！！！
}

/**
  *  @brief  启动AudioAlarmMain线程
  *  @author  <wjn&wzq> 
  *  @param[in] <无>
  *  @param[out] <无>
  *  @return  线程PID
  *  @note
*/
void * AudioAlarmMain(void *args){

	int Ret = 0;
	ArgsStruct *self;
	AUDIOALARM_FLAG *flag;
	self = malloc(sizeof(ArgsStruct));
	flag = malloc(sizeof(AUDIOALARM_FLAG));
	if(NULL == flag){
		free(self);
		self=NULL;
	}else{
		flag->AudioAlarmFlag = 0;
		flag->AudioPush = 0;
		flag->Level = 0;
		flag->last_index = -1;
		flag->alarm_on = 2;//stop
		flag->bcast_comm = 2;
	}

	if(-1 == InitArgs(self)){
		getTimeAndPrint(2,__LINE__,"Init args self error!!!");
		free(flag);
		flag=NULL;		
		free(self);
		self=NULL;
	}

	audio_variable_init();

	//write_file(NULL);
	thread_synchronization(&rAudioAlarm, &rChildProcess);
	get_config_item_value(CONFIG_ALARM_AUDIO_SENSITIVITY, &flag->Level, &Ret);
	flag->Level -= '0'; //取出的是可见字符
	get_config_item_value(CONFIG_ALARM_AUDIO, &flag->AudioAlarmFlag, &Ret);

	if(flag->AudioAlarmFlag == 'y'){
		//printf("flag->AudioAlarmFlag------%d-----%c\n", flag->Level, flag->AudioAlarmFlag);
		flag->AudioAlarmFlag = 1;
	}else{
		flag->AudioAlarmFlag = 0;
	}
	//flag->AudioAlarmFlag = 0;
	printf("AudioAlarm Thread start ok v1.1\n");
//	int count = 0;
	int wait30s = 0;
	while(1){
		child_setThreadStatus(AUDIO_ALARM_MSG_TYPE, NULL);
		if(-1 == RevMSGProcess(self, AUDIO_ALARM_MSG_TYPE)){

		}else{
			//stop count,clear flag
			flag->AudioPush = 0;
			switch(self->s_rev_process->Thread_cmd){
				case MSG_AUDIOALARM_T_ALARM_LEVEL:
					//printf("AudioAlarm Thread: len=%d\n", self->s_rev_process->length);
					sscanf(self->s_rev_process->data,"%d",&(flag->Level));
//					if(flag->alarm_on == 1){
//						flag->alarm_on = 1;
//					}
					printf("AudioAlarmMain: level=%d\n", flag->Level);
					break;

				case MSG_AUDIOALARM_T_START_AUDIO_CAPTURE:
					printf("AudioAlarmMain: start\n");
					flag->AudioAlarmFlag = 1;
					flag->alarm_on = 2;

					//send start response message to process
					AudioAlarmSendStartResponse(self);
					break;

				case CMD_STOP_ALL_USE_FRAME:
					flag->bcast_comm = 2;
					g_lAudioAlarm = 1; //写全局变量
					break;
				case MSG_AUDIOALARM_T_STOP_AUDIO_CAPTURE:
					printf("AudioAlarmMain: stop\n");
					flag->AudioAlarmFlag = 0;
					flag->alarm_on = 2;

					//send stop response message to process
					AudioAlarmSendStopResponse(self);
					break;
				case MSG_AUDIOALARM_T_STOP_4_VIDEO_CAPTURE:
					wait30s = 0;
					flag->AudioPush = 1;
					break;
				case CMD_AV_STARTED:
					flag->bcast_comm = 1;
					break;
				default:
					break;
			}
		}
#if 0//test
	flag->AudioAlarmFlag = 1;
	flag->AudioPush = 0;
#endif
		//printf("flag->Level %d flag->AudioAlarmFlag %d  flag->AudioPush : %d\n",flag->Level,flag->AudioAlarmFlag,flag->AudioPush);
		if(0!=flag->Level&&1==flag->AudioAlarmFlag&&0==flag->AudioPush&&flag->bcast_comm==1&&(appInfo.speakerInfo == INFO_SPEAKER_OFF)){
			//printf("-----------------------------__FILE__:%s __LINE__:%d\n",__FILE__,__LINE__);
			Ret = handleAudioAlarm(flag);
			if(Ret == 1){
				flag->AudioPush = 1;
				printf("send alarm message to process\n");
//				//send alarm message to process
				flag->alarm_on = 1;
				AudioAlarmSendAlarmResponse(self);
			}else if(Ret == 2){
				if(flag->alarm_on == 1){
					flag->alarm_on = 2;
					AudioAlarmSendAlarmEnd(self);
				}
			}
		}

		if(flag->AudioPush == 1){
			wait30s++;
			if(wait30s%50 == 0){
				//getTimeAndPrint(2,__LINE__,"30s wait for audioalarm...");
			}
			if(wait30s == AUDIOALARM_USLEEP_COUNT){
				wait30s = 0;
				flag->AudioPush = 0;
			}
		}

		thread_usleep(AUDIOALARM_USLEEP_TIME);
	}

	return NULL;
}


/**
  *  @brief 子进程CMD解析
  *  @author  <wjn&wzq> 
  *  @param[in] <msg_container 子进程接受消息结构体>
  *  @param[out] <无>
  *  @return  线程PID
  *  @note
*/
void audioalarm_cmd_table_fun(msg_container *self)
{
//	int ret;
	MsgData *msgp = (MsgData *)self->msgdata_rcv;
	int cmd = msgp->cmd;
//	int len = msgp->len;

	switch(cmd){
		case MSG_AUDIOALARM_P_AUDIO_ALRAM_NOW:
			//send message to other thread
			printf("MSG_AUDIOALARM_P_AUDIO_ALRAM_NOW:.........\n");
			AudioAlarmSendVideoAlarm(self, 1);
			//AudioAlarmSendNewsChannel(self);
			//AudioAlarmSendP2P(self);
			break;

		case MSG_AUDIOALARM_P_AUDIO_ALRAM_END:
			printf("MSG_AUDIOALARM_P_AUDIO_ALRAM_END...........\n");
			AudioAlarmSendVideoAlarm(self, 2);
			//AudioAlarmSendNewsChannel(self);
			//AudioAlarmSendP2P(self);
			break;

		case MSG_AUDIOALARM_P_START_AUDIO_RESPONSE:
			printf("MSG_AUDIOALARM_P_START_AUDIO_RESPONSE:..........\n");
			//send message to other thread
			break;

		case MSG_AUDIOALARM_P_STOP_AUDIO_RESPONSE:
			printf("MSG_AUDIOALARM_P_STOP_AUDIO_RESPONSE..........\n");
			//send message to other thread
			break;
		case MSG_AUDIOALARM_P_STOP_4_VIDEO_RESPONSE:
			printf("MSG_AUDIOALARM_P_STOP_4_VIDEO_RESPONSE..........\n");
			//send message to other thread
			break;
		default:
			break;
	}
}

