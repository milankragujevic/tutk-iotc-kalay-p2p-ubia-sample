/**   @file []
   *  @brief (1)处理时间消息功能
   *  @note
   *  @author wangfengbo
   *  @date 2013-09-17
   *  @remarks 增加注释
*/
/*******************************************************************************/
/*                                    头文件                                         */
/*******************************************************************************/
#include <sys/time.h>

#include "common/time_func.h"
#include "common/logfile.h"
#include "common/msg_queue.h"
#include "IOCtl/ioctl.h"
#include "common/common.h"
#include "common_func.h"
#include "pdatatype.h"
#include "VideoAlarm/VideoAlarm.h"
#include "UPNP/upnp_thread.h"
#include "common/logfile.h"
#include "common/common_config.h"

#if 10
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <semaphore.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <linux/stddef.h>
#include "IOCtl/rt5350_ioctl.h"
#endif



/*******************************************************************************/
/*                                 结构体和红定义区                                  */
/*******************************************************************************/
/** 定义timer 精度 */
#define TIMER_ACCURACY                  (100000)    //100ms

/** 音视频 停止的最大时间   10s */
#define  AV_STOP_MAX_TIMEOUT             1500000     //实测
/** 子进程循环时间   */
#define TIMER_CHILD_PROCESS_SLEEP_TIME  100000  //实测 子进程循环一圈 开销35～40ms  而delay只是10ms时，目前100没测试过

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))
#endif

/** 消息处理函数 */
typedef void (*pFnTimerHandler)(char *pcData);

/** 消息处理结构体 */
typedef struct _tag_timer_handler_info_
{
    int lTimerId;                             ///<timer id, 这个timer id在TimerFunc.h中定义
	int lIsActive;                            ///<timer 是否运行
	unsigned int unTimerExpd;                 ///<定时时间     单位是100ms
	unsigned int unTimerLeft;                 ///<剩余时间     单位是100ms
	const pFnTimerHandler pFnTimerHandler;    ///<timer处理函数
	char *pTimerData;                         ///<timer处理函数的参数
}time_handler_info_t;

/** 消息处理结构体timer */
typedef struct _tag_timer_ctl_info_t_
{
	unsigned int unLoopCnts;
}timer_ctl_info_t;

/*******************************************************************************/
/*                                  函数声明区                                       */
/*******************************************************************************/
void child_process_send_start_work_msg(char *pcData);
void stop_use_frame_timer_handler(char *pcData);
void stop_gather_timer_handler(char *pcData);
void process_child_timer_test(char *pcData);
void video_and_audio_start_gather(char *pcData);
void system_reboot(char *pcData);
void night_led_ctl_timer_hanler(char *pcData);
void night_mode_ctl_timer_hanler(char *pcData);
void video_alarm_ctl_timer_hanler(char *pcData);

/*******************************************************************************/
/*                                 全局变量定义区                                    */
/*******************************************************************************/
/** 消息ID及其相关函数 */
time_handler_info_t g_stTimerHandlerInfoMap[] =
{
	{TIMER_ID_SEND_AV_START,             TIMER_ENABLE,   0,  0,  child_process_send_start_work_msg,   NULL},
	{TIMER_ID_STOP_USE_FRAME,            TIMER_DISABLE,  0,  0,  stop_use_frame_timer_handler,        NULL},
	{TIMER_ID_STOP_GATHER,               TIMER_DISABLE,  0,  0,  stop_gather_timer_handler,           NULL},
	{TIMER_ID_TEST,                      TIMER_DISABLE,  10,  10,  process_child_timer_test,   NULL},
	{TIMER_ID_START_GATHER,              TIMER_DISABLE,  0,  0,  video_and_audio_start_gather,        NULL},
	{TIMER_ID_SYSTEM_REBOOT,             TIMER_DISABLE,  0,  0,  system_reboot,                       NULL},
	{TIMER_ID_CTL_NIGHT_MODE_CTL,        TIMER_DISABLE,  0,  0,  night_mode_ctl_timer_hanler,    NULL},
	{TIMER_ID_CTL_NIGHT_LED_CTL,         TIMER_DISABLE,  0,  0,  night_led_ctl_timer_hanler,    NULL},
	{TIMER_ID_CTL_VIDEO_ALARM,           TIMER_DISABLE,  0,  0,  video_alarm_ctl_timer_hanler,        NULL},
};

/** 消息ID及其相关函数 */
timer_ctl_info_t g_stTimerCtlInfo = {0};
/** Timer公共内存 */
char g_acTimeFuncBuf[512] = {0};
/** AV停止循环次数 */
int  g_nAvStopLoopTimes = 0;
/** 测试循环次数 */
int g_TestLoopTimes = 0;

/*******************************************************************************/
/*                                   函数实现                                                                     */
/*******************************************************************************/
/**
  @brief时间消息处理函数
  @author wfb
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void chile_process_timer_handler()
{
	int i = 0;

    if (TIMER_ACCURACY/TIMER_CHILD_PROCESS_SLEEP_TIME != g_stTimerCtlInfo.unLoopCnts)
    {
    	g_stTimerCtlInfo.unLoopCnts++;
    	return;
    }
    else
    {
    	g_stTimerCtlInfo.unLoopCnts = 0;
    }

    /* 遍历表 */
    for(i = 0; i < ARRAY_SIZE(g_stTimerHandlerInfoMap); i++)
    {
        if (TIMER_ENABLE == g_stTimerHandlerInfoMap[i].lIsActive)
        {
        	//printf("i = %d enable***************************\n", i);
        	if (0 == g_stTimerHandlerInfoMap[i].unTimerLeft)
        	{
        		//printf(" unTimerLeft =++++++++++++++==== %d\n",g_stTimerHandlerInfoMap[i].unTimerLeft);
        		/* 是否要判断timer次数 TO 新需求 */
        		g_stTimerHandlerInfoMap[i].lIsActive = TIMER_DISABLE;
        		g_stTimerHandlerInfoMap[i].pFnTimerHandler(g_stTimerHandlerInfoMap[i].pTimerData);
        	}
        	else
        	{
        		//printf(" unTimerLeft = %d\n",g_stTimerHandlerInfoMap[i].unTimerLeft);
        		g_stTimerHandlerInfoMap[i].unTimerLeft--;
                //gettimeofday(&stBegin, NULL);
                //printf("chile_process_timer_handler i = %d, stBegin.tv_sec = %ld,stBegin.tv_usec = %ld\n", i, stBegin.tv_sec, stBegin.tv_usec);
        	}
        }
    }
}


/**
  @brief 重新设置timer的超时时间，但不会修改正在运行timer的剩余时间
  @author wfb
  @param[in] lTimerId : 要修改参数的timer id 所有的id都在TimeFunc.h中被定义
  @param[in] <unTimerExpd : timer的超时时间，若设为0，则默认上次不变，超时时间会在timer下次开始时使用
  @param[in] <pTimerData : timer处理函数的参数，若为NULL，则默认使用上次，设置完参数后，当该timer到期时，立即被使用
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void reset_timer_param_by_id(int lTimerId, unsigned int unTimerExpd, char *pTimerData)
{
	int i = 0;

    if((lTimerId > TIMER_ID_FIRST) && (lTimerId < TIMER_ID_LAST))
    {
         for(i = 0; i < ARRAY_SIZE(g_stTimerHandlerInfoMap); i++)
         {
        	 if (g_stTimerHandlerInfoMap[i].lTimerId == lTimerId)
        	 {

        		 if (0 != unTimerExpd)
	         	 {
         		     g_stTimerHandlerInfoMap[i].unTimerExpd = unTimerExpd;
		         }

	         	 if (NULL != pTimerData)
		         {
	     		 	 g_stTimerHandlerInfoMap[i].pTimerData = pTimerData;
         	     }

	         	 break;
        	 }
         }
    }
    else
    {
    	DEBUGPRINT(DEBUG_INFO, "timer is not exist timer id = %d\n", lTimerId);
    }
}


/**
  @brief 调用该函数将重新开始该timer，timer的超时为上次设置的值，若从没设置过，则为初始值
  @author wfb
  @param[in] lTimerId
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void start_timer_by_id(int lTimerId)
{
	int i = 0;

	for(i = 0; i < ARRAY_SIZE(g_stTimerHandlerInfoMap); i++)
	{
		if (g_stTimerHandlerInfoMap[i].lTimerId == lTimerId)
        {
			g_stTimerHandlerInfoMap[i].unTimerLeft = g_stTimerHandlerInfoMap[i].unTimerExpd;
            g_stTimerHandlerInfoMap[i].lIsActive = TIMER_ENABLE;
            break;
        }
	}

}


/**
  @brief 挂起timer
  @author wfb
  @param[in] lTimerId
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void suspend_timer_by_id(int lTimerId)
{
	int i = 0;

	if((lTimerId > TIMER_ID_FIRST) && (lTimerId < TIMER_ID_LAST))
	{
		for(i = 0; i < ARRAY_SIZE(g_stTimerHandlerInfoMap); i++)
			{
				if (g_stTimerHandlerInfoMap[i].lTimerId == lTimerId)
		        {
		            g_stTimerHandlerInfoMap[i].lIsActive = TIMER_DISABLE;
		            break;
		        }
			}
	}
	else
	{
		DEBUGPRINT(DEBUG_INFO, "timer is not exist timer id = %d\n", lTimerId);
	}

}


/* －－－－－－－－－－－－－－－－timer消息处理函数－－－－－－－－－－－－－－－－－－ */


/**
  @brief 停止使用枕时间处理函数
  @author wfb
  @param[in] pcData: NULL
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void stop_use_frame_timer_handler(char *pcData)
{
	int lThreadMsgId = 0;
	//struct timeval stBegin;
	MsgData *pstMsgSnd = (MsgData *)g_acTimeFuncBuf;

	//printf("g_lVideoAlarm = %d\n", g_lVideoAlarm);
	/* 判断是否所有线程已全部返回停止使用祯的rep  */;

    if (1 == is_all_thread_return_stop_use_frame_rep())
	//if (1)
    {
    	g_TestLoopTimes = 0;
    	DEBUGPRINT(DEBUG_INFO, "rcv STOP FRAME  ok, send msg to stop av capture  ****************************\n");
        get_process_msg_queue_id(&lThreadMsgId,  NULL);

#if 1
        /* 停止 音频采集 */
        pstMsgSnd->type = AUDIO_MSG_TYPE;
        pstMsgSnd->cmd = CMD_STOP_GATHER_AUDIO;
        pstMsgSnd->len = 0;
        msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
#endif
#if 0
        /* 停止 视频采集 */
        pstMsgSnd->type = VIDEO_MSG_TYPE;
        pstMsgSnd->cmd = 2;
        msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
#endif
        //system("echo all stop use frame, and already send stop msg to av capture >> /etc/log.txt");

        //gettimeofday(&stBegin, NULL);
        //printf("cur time is stBegin.tv_sec = %ld,stBegin.tv_usec = %ld\n", stBegin.tv_sec, stBegin.tv_usec);

    	reset_timer_param_by_id(TIMER_ID_STOP_GATHER, 1, NULL);
    	start_timer_by_id(TIMER_ID_STOP_GATHER);
    }
    /* 未返回 超时 */
    else
    {
    	g_TestLoopTimes ++;
    	if (10 == g_TestLoopTimes)
    	{
            if (0 == g_lVideoAlarm)
            {
            	//system("echo g_lVideoAlarm not return  >> /etc/log.txt");
            	write_log_info_to_file("g_lVideoAlarm not return\n");
            }
            if (0 == g_lAudioSend)
            {
            	//system("echo g_lAudioSend not return  >> /etc/log.txt");
            	write_log_info_to_file("g_lAudioSend not return\n");
            }
            if (0 == g_lVideoSend)
            {
            	//system("echo g_lVideoSend not return  >> /etc/log.txt");
            	write_log_info_to_file("g_lVideoSend not return\n");
            }
            if (0 == g_lAudioAlarm)
            {
            	//system("echo g_lAudioAlarm not return  >> /etc/log.txt");
            	write_log_info_to_file("g_lAudioAlarm not return\n");
            }
            if (0 == g_lP2p)
            {
            	//system("echo g_lP2p not return >> /etc/log.txt");
            	write_log_info_to_file("g_lP2p not return\n");
            }
    	}
    	reset_timer_param_by_id(TIMER_ID_STOP_USE_FRAME, 1, NULL);
    	start_timer_by_id(TIMER_ID_STOP_USE_FRAME);
    }

}


/**
  @brief 停止采集音视频
  @author wfb
  @param[in] pcData : NULL
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void stop_gather_timer_handler(char *pcData)
{
	int lThreadMsgId = -1;
	MsgData *pstMsgSnd = (MsgData *)g_acTimeFuncBuf;

	/* 已经返回 */
	if (1 == is_all_video_and_audio_stop_gather())
	{
        //suspend_timer_by_id(TIMER_ID_USB_SWITCH_TO_MFI_TIMEOUT);

        /*! 切换到mfi */
		DEBUGPRINT(DEBUG_INFO, "video and audio stop gather handler ok\n");

#if 0
        gettimeofday(&g_stTimeVal, NULL);
        printf("cur time is g_stTimeVal.tv_sec = %ld,stBegin.g_stTimeVal = %ld\n", g_stTimeVal.tv_sec, g_stTimeVal.tv_usec);
#endif
    	get_process_msg_queue_id(&lThreadMsgId,  NULL);
    	pstMsgSnd->type = IO_CTL_MSG_TYPE;
    	pstMsgSnd->cmd = MSG_IOCTL_T_CHECK_USB;
    	pstMsgSnd->len = CHECK_USB_REQ_MSG_DATA_LEN;
    	*(char *)(pstMsgSnd + 1) = CHECK_USB_SENDER_USB_LINK_TO_CAM;
    	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
    	//system("echo already stop av capture, start usb switch to mfi >> /etc/log.txt");
    	write_log_info_to_file("already stop av capture, start usb switch to mfi\n");
    	reset_flag();
    	g_nAvStopLoopTimes = 0;
    }
    /* 未返回 超时 */
    else
    {
    	g_nAvStopLoopTimes++;
        /* 达到最大次数 直接切换 */
    	if (AV_STOP_MAX_TIMEOUT/TIMER_ACCURACY == g_nAvStopLoopTimes)
    	{
        	get_process_msg_queue_id(&lThreadMsgId,  NULL);
        	pstMsgSnd->type = IO_CTL_MSG_TYPE;
        	pstMsgSnd->cmd = MSG_IOCTL_T_CHECK_USB;
        	pstMsgSnd->len = CHECK_USB_REQ_MSG_DATA_LEN;
        	*(char *)(pstMsgSnd + 1) = CHECK_USB_SENDER_USB_LINK_TO_CAM;
        	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
        	//system("echo Not rcv stop av capture, start usb switch to mfi >> /etc/log.txt");
        	write_log_info_to_file("Not rcv stop av capture, start usb switch to mfi\n");
        	g_nAvStopLoopTimes = 0;
        	reset_flag();
        	g_nAvErrorFlag = TRUE;
        	return;
    	}
    	//printf("else come into stop_gather_timer_handler\n");;
    	reset_timer_param_by_id(TIMER_ID_STOP_GATHER, 1, NULL);
    	start_timer_by_id(TIMER_ID_STOP_GATHER);
    }
}


/**
  @brief for test
  @author wfb
  @param[in] pcData : NULL
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void process_child_timer_test(char *pcData)
{
#if 0
	static int i = 0;
	char acBuf[32] = {0};

	snprintf(acBuf, 32, "%d:%d,%d,%d ", i++, g_nSpeakerFlag1, g_nSpeakerFlag2, g_nSpeakerFlag3);
	//write_log_info_to_file(acBuf);
	DEBUGPRINT(DEBUG_INFO, "g_nSpeakerFlag1 = %d, g_nSpeakerFlag2 = %d, g_nSpeakerFlag2 = %d\n",
			g_nSpeakerFlag1, g_nSpeakerFlag2, g_nSpeakerFlag3 );

	DEBUGPRINT(DEBUG_INFO, "g_nAudioFlag1 = %d, g_nAudioFlag2 = %d, g_nAudioFlag2 = %d, i = %d\n",
			g_nAudioFlag1, g_nAudioFlag2, g_nAudioFlag3, i);

	reset_timer_param_by_id(TIMER_ID_TEST, 50, NULL);
	start_timer_by_id(TIMER_ID_TEST);
#endif
#if 0
	static int i = 0;
	char acBuf[32] = "IhaveUid";
	if (i == 0)
	{
		printf("SETSETSETSETSETSETSETSETSETSETSETSETSETSETSETSETSETSETTSETSETSETSETSETSETSETSETSETSETSETSET\n");
		printf("SETSETSETSETSETSETSETSETSETSETSETSETSETSETSETSETSETSETTSETSETSETSETSETSETSETSETSETSETSETSET\n");
		printf("TSETSETSETSETSETSETSETSETSETSETSETSETSETSETSETSETSETSETSETSETSETSETSETSETSETSETSETSETSETSET\n");

		set_config_value_to_flash( CONFIG_P2P_UID , acBuf, strlen(acBuf));
		i =  1;
	}
	else
	{

		printf("CLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCCLEARCLEARCLEARCLEAR\n");
		printf("CLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCCLEARCLEARCLEARCLEAR\n");
		printf("CLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCCLEARCLEARCLEARCLEAR\n");
		set_config_value_to_flash( CONFIG_P2P_UID , NULL, 0);
		i = 0;
	}

	reset_timer_param_by_id(TIMER_ID_USB_SWITCH_TO_MFI_TIMEOUT, 50, NULL);
	start_timer_by_id(TIMER_ID_USB_SWITCH_TO_MFI_TIMEOUT);
#endif

#if 0
	int lThreadMsgId = -1;
	MsgData *pstMsgSnd = (MsgData *)g_acTimeFuncBuf;


    /*! 超时默认切换到mfi */
    get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->type = IO_CTL_MSG_TYPE;
	pstMsgSnd->cmd = MSG_IOCTL_T_SET_LED_IF_WORK;
	pstMsgSnd->len = LED_IF_WORK_MSG_DATA_LEN;
    *(char *)(pstMsgSnd + 1) = USB_SWITCH_TO_MFI;
	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
#endif

#if 0
    static int i = 0;

    //led_control(LED_COLOR_TYPE_NET_BLUE, LED_CTL_TYPE_FAST_FLK);
    //led_control(LED_COLOR_TYPE_POWER_RED, LED_CTL_TYPE_ON);

    if (0 == i)
    {
    	DEBUGPRINT(DEBUG_INFO, "5555555555555555555555555555555555 battery normal not work\n");
    	//led_if_work_control(LED_COMMON_TYPE_NIGHT, LED_FLAG_NOT_WORK);
    	//DEBUGPRINT(DEBUG_INFO, "LED_CTL_TYPE_DOZE NOT WORK NIGHT NOT WORK NIGHT NOT WORK NIGHT NOT WORK\n");
    	//led_control(LED_TYPE_NET, LED_CTL_TYPE_DOZE);
    	//led_control(LED_TYPE_POWER, LED_CTL_TYPE_DOZE);
    	//led_control(LED_TYPE_NET_POWER, LED_CTL_TYPE_DOZE);
    	//led_control(LED_TYPE_NIGHT, LED_CTL_TYPE_ON);
    	net_no_ip_set_led();
    	battary_power_state_report(BATTERY_POWER_STATE_NORMAL);
    	led_if_work_control(LED_TYPE_NET_POWER, LED_FLAG_NOT_WORK, TRUE);
        i = 1;
    }
    else if (1 == i)
    {
    	//DEBUGPRINT(DEBUG_INFO, "LED_CTL_TYPE_BLINK NOT WORK NIGHT NOT WORK NIGHT NOT WORK NIGHT NOT WORK\n");
		//led_control(LED_TYPE_NET, LED_CTL_TYPE_BLINK);
		//led_control(LED_TYPE_POWER, LED_CTL_TYPE_BLINK);
    	//led_control(LED_TYPE_NET_POWER, LED_CTL_TYPE_BLINK);
    	//led_if_work_control(LED_TYPE_NET_POWER, LED_FLAG_WORK, TRUE);
    	i = 2;
    }
    else if (2 == i)
	{
		//DEBUGPRINT(DEBUG_INFO, "LED_CTL_TYPE_FAST_FLK NOT WORK NIGHT NOT WORK NIGHT NOT WORK NIGHT NOT WORK\n");
		//led_control(LED_TYPE_NET, LED_CTL_TYPE_FAST_FLK);
		//led_control(LED_TYPE_POWER, LED_CTL_TYPE_FAST_FLK);
		//led_control(LED_TYPE_NET_POWER, LED_CTL_TYPE_FAST_FLK);
		//led_control(LED_TYPE_NIGHT, LED_CTL_TYPE_OFF);
		//battary_power_state_report(BATTERY_POWER_STATE_CHARGE);
		i = 3;
	}
    else if (3 == i)
	{
		DEBUGPRINT(DEBUG_INFO, "5555555555555555555555555555555555 net_connect_ok_set_led\n");
		//led_control(LED_TYPE_NET, LED_CTL_TYPE_SLOW_FLK);
		//led_control(LED_TYPE_POWER, LED_CTL_TYPE_SLOW_FLK);
		//led_control(LED_TYPE_NET_POWER, LED_CTL_TYPE_SLOW_FLK);
		net_connect_ok_set_led();
		i = 4;
	}
    else if (4 == i)
	{
		//DEBUGPRINT(DEBUG_INFO, "LED_CTL_TYPE_ON BATTERY_POWER_STATE_LOW  WORK NIGHT NOT WORK\n");
    	DEBUGPRINT(DEBUG_INFO, "5555555555555555555555555555555555 net_get_ip_ok_set_led()\n");
		//led_control(LED_TYPE_NET, LED_CTL_TYPE_ON);
		//led_control(LED_TYPE_POWER, LED_CTL_TYPE_ON);
		//led_control(LED_TYPE_NET_POWER, LED_CTL_TYPE_ON);
		//battary_power_state_report(BATTERY_POWER_STATE_LOW);
		i = 5;
	}
    else if (5 == i)
	{
		DEBUGPRINT(DEBUG_INFO, "5555555555555555555555555555555555 BATTERY_POWER_STATE_LOW\n");
		//led_control(LED_TYPE_NET, LED_CTL_TYPE_OFF);
		//led_control(LED_TYPE_POWER, LED_CTL_TYPE_OFF);
		//led_control(LED_TYPE_NET_POWER, LED_CTL_TYPE_OFF);
		//led_if_work_control(LED_TYPE_NET_POWER, LED_FLAG_WORK, TRUE);
		//led_if_work_control(LED_TYPE_NIGHT, LED_FLAG_WORK, TRUE);
		battary_power_state_report(BATTERY_POWER_STATE_LOW);
		i = 0;
	}

	reset_timer_param_by_id(TIMER_ID_TEST, 80, NULL);
	start_timer_by_id(TIMER_ID_TEST);
#endif

#if 0
	static int i = 0;
	char acBuf[5] = "ABCD";
	if (i == 0)
	{
		printf("SETSETSETSETSETSETSETSETSETSETSETSETSETSETSETSETSETSETSETSETSET\n");

		set_config_value_to_flash( CONFIG_WIFI_SSID , acBuf, strlen(acBuf));
		i =  1;
	}
	else
	{

		printf("CLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEAR\n");
		set_config_value_to_flash( CONFIG_WIFI_SSID , NULL, 0);
		printf("CLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEARCLEAR ok\n");
		i = 0;
	}

	reset_timer_param_by_id(TIMER_ID_USB_SWITCH_TO_MFI_TIMEOUT, 50, NULL);
	start_timer_by_id(TIMER_ID_USB_SWITCH_TO_MFI_TIMEOUT);
#endif

#if 0
	static int i = 0;

	if (i == 0)
	{
		DEBUGPRINT(DEBUG_INFO, "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^NOT WORK\n");
		DEBUGPRINT(DEBUG_INFO, "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^NOT WORK\n");
		DEBUGPRINT(DEBUG_INFO, "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^NOT WORK\n");
		//led_if_work_control(LED_M2_TYPE_MIXED, LED_FLAG_NOT_WORK);
		//led_if_work_control(LED_M3S_TYPE_NET, LED_FLAG_NOT_WORK);
		//led_if_work_control(LED_M3S_TYPE_POWER, LED_FLAG_NOT_WORK);
		led_if_work_control(LED_COMMON_TYPE_NIGHT, LED_FLAG_NOT_WORK);
        i = 1;
	}
	else
	{
		DEBUGPRINT(DEBUG_INFO, "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ WORK\n");
		DEBUGPRINT(DEBUG_INFO, "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ WORK\n");
		DEBUGPRINT(DEBUG_INFO, "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ WORK\n");
		//led_if_work_control(LED_M2_TYPE_MIXED, LED_FLAG_WORK);
		//led_if_work_control(LED_M3S_TYPE_NET, LED_FLAG_WORK);
		//led_if_work_control(LED_M3S_TYPE_POWER, LED_FLAG_WORK);
		led_if_work_control(LED_COMMON_TYPE_NIGHT, LED_FLAG_WORK);
        i = 0;
	}

	reset_timer_param_by_id(TIMER_ID_USB_SWITCH_TO_MFI_TIMEOUT, 10, NULL);
	start_timer_by_id(TIMER_ID_USB_SWITCH_TO_MFI_TIMEOUT);
#endif

}

/**
  @brief 发送启动线程
  @author wfb
  @param[in] pcData : NULL
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void child_process_send_start_work_msg(char *pcData)
{
	int lThreadMsgId = -1;
	MsgData *pstMsgSnd = (MsgData *)g_acTimeFuncBuf;

	DEBUGPRINT(DEBUG_INFO, "child process basic start thread flag^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
	/* 开启音视频  */
	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->type = AUDIO_MSG_TYPE;
	pstMsgSnd->len = 0;
	pstMsgSnd->cmd = 1;
	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);


#define _DISABLE_VIDEO_ALARM
#ifndef _DISABLE_VIDEO_ALARM
	/* 开始视频 报警 */
	pstMsgSnd->type = VIDEO_ALARM_MSG_TYPE;
	pstMsgSnd->len = 0;
	pstMsgSnd->cmd = MSG_VA_T_START;
	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);

	pstMsgSnd->type = VIDEO_ALARM_MSG_TYPE;
	pstMsgSnd->len = 2 * sizeof(int);
	pstMsgSnd->cmd = MSG_VA_T_CONTROL;
	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
#endif

#ifndef _DISABLE_AUDIO_ALARM
	/* 开始视频 报警 */
	pstMsgSnd->type = AUDIO_ALARM_MSG_TYPE;
	pstMsgSnd->len = 0;
	pstMsgSnd->cmd = MSG_VA_T_START;
	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
#if 0
	pstMsgSnd->type = VIDEO_ALARM_MSG_TYPE;
	pstMsgSnd->len = 2 * sizeof(int);
	pstMsgSnd->cmd = MSG_VA_T_CONTROL;
	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
#endif
#endif

#ifndef _DISABLE_UPNP
    pstMsgSnd->type = UPNP_MSG_TYPE;
	pstMsgSnd->len = 5;
	pstMsgSnd->cmd = MSG_UPNP_T_UPNPSTART;
    memcpy(pstMsgSnd + 1, "10002", 5);
	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
#endif

}


/**
  @brief 通知音视频 开始采集
  @author wfb
  @param[in] pcData: NULL
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void video_and_audio_start_gather(char *pcData)
{
	int lThreadMsgId = -1;
	MsgData *pstMsgSnd = (MsgData *)g_acTimeFuncBuf;

	/* 通知开始时候 清空表置位 */
	reset_flag();

	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->cmd = CMD_START_CAPTURE;
	pstMsgSnd->len = 0;

	/* 音频 */
    pstMsgSnd->type = AUDIO_MSG_TYPE;
    msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
    DEBUGPRINT(DEBUG_INFO,"send start gather already ok\n");

    //system("echo inform all, start capture >> /etc/log.txt");
    write_log_info_to_file("inform av, start capture \n");

    /* 只给音频发开始， 当音频准备好后会自动给视频线程发送开启视频  modify by wfb zl,2013-04-09 16:17 */
#if 0
    /* 视频 */
    pstMsgSnd->type = VIDEO_MSG_TYPE;
    msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
#endif
}

/**
  @brief 系统重启
  @author wfb
  @param[in] pcData: NULL
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void system_reboot(char *pcData)
{
	system("reboot");
}

/**
  @brief 夜间模式控制处理
  @author wfb
  @param[in] pstMsgContainer : 接收子进程消息
  @param[out] 无
  @return 无
  @remark 2013-09-03 增加该注释
  @note
*/
void night_mode_ctl_timer_hanler(char *pcData)
{
	/* 发送夜视模式 注：这里本可以不用全局变量，可以通过设置pcData来解决该问题，但
	 * 怕中间被打断后造成数据区被修改 */
	night_mode_report(g_nNightModeFlag);
	DEBUGPRINT(DEBUG_INFO, "night_mode_ctl_timer_hanler ................\n");

	reset_timer_param_by_id(TIMER_ID_CTL_VIDEO_ALARM, 10, NULL);
	start_timer_by_id(TIMER_ID_CTL_VIDEO_ALARM);

	return;
}

/**
  @brief 夜视灯控制处理
  @author wfb
  @param[in] pstMsgContainer : 接收子进程消息
  @param[out] 无
  @return 无
  @remark 2013-09-03 增加该注释
  @note
*/
void night_led_ctl_timer_hanler(char *pcData)
{
	/* 控制夜视灯 */
	DEBUGPRINT(DEBUG_INFO, "night_led_ctl_timer_hanler ................\n");
	led_if_work_direct_control(LED_TYPE_NIGHT, g_nNightLedCtl, TRUE);

	reset_timer_param_by_id(TIMER_ID_CTL_VIDEO_ALARM, 10, NULL);
	start_timer_by_id(TIMER_ID_CTL_VIDEO_ALARM);
}

/**
  @brief 视频报警控制时间处理函数
  @author wfb
  @param[in] pstMsgContainer : 接收子进程消息
  @param[out] 无
  @return 无
  @remark 2013-09-03 增加该注释
  @note
*/
void video_alarm_ctl_timer_hanler(char *pcData)
{
	char acTmpBuf[8] = {0};
	int nBufLen = 0;

	//通过flash读取
	get_config_item_value(CONFIG_ALARM_MOTION, acTmpBuf, &nBufLen);

	if (0 == strncmp(acTmpBuf, "y", 2))
	{
		/* 开始视频报警 */
		DEBUGPRINT(DEBUG_INFO, "send_msg_to_video_alarm_start_alarm()\n");
		send_msg_to_video_alarm_start_alarm();
	}
}
