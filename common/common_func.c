/**   @file []
   *  @brief    线程公用函数
   *  @note
   *  @author   wangfengbo
   *  @date     2013-9-2
   *  @remarks  增加注释
*/
/*******************************************************************************/
/*                                 头文件                                            */
/*******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdarg.h>

#include "common.h"
#include "msg_queue.h"
#include "pdatatype.h"
#include "common_func.h"
#include "common_config.h"
#include "time_func.h"
#include "IOCtl/ioctl.h"
#include "FlashRW/FlashRW.h"
#include "VideoAlarm/VideoAlarm.h"
#include "NetWorkSet/networkset.h"
#include "playback/speaker.h"
#include "logfile.h"

/*******************************************************************************/
/*                                全局变量区                                         */
/*******************************************************************************/
#define COMMON_LOG_BUG_LEN                             128
/** 获取指示灯状态锁 */
pthread_mutex_t g_stLedState = PTHREAD_MUTEX_INITIALIZER;
/** 获取网络状态锁*/
pthread_mutex_t g_stNetState = PTHREAD_MUTEX_INITIALIZER;

/** WIFI信号强度等级锁 */
pthread_mutex_t g_stWifiSignalLevelLock = PTHREAD_MUTEX_INITIALIZER;

/** 增加led 快山和慢闪线程锁 */
pthread_mutex_t g_stDhcpTutkLedLock = PTHREAD_MUTEX_INITIALIZER;

/** 定义网络状态信息 */
netset_state_t  g_stNetStateInfo = {0, 0, 0};

/** 网络灯当前状态 */
int g_nNetLedState = LED_CTL_TYPE_OFF;
/** 电源灯当前状态*/
int g_nPowerLedState = LED_CTL_TYPE_ON;
/** 夜视灯当前状态  */
int g_nLightLedState = LED_CTL_TYPE_ON;

/** COMMON BUF */
char g_acLogBuf[COMMON_LOG_BUG_LEN] = {0};


/*******************************************************************************/
/*                              公共函数定义区                                       */
/*******************************************************************************/
/**
  @brief 向子线程发送消息接口
  @author wfb 
  @param[in] ulThreadId : 发送线程
  @param[in] Cmd : 发送命令
  @param[in] pMsgData : 数据长度, 这里最大长度MAX_MSG_LEN
  @param[in] unDataLen : 数据指针
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
int send_msg_to_child_process(long int ulThreadId, int Cmd, void *pMsgData, unsigned int unDataLen)
{
	Child_Process_Msg stChildProcessMsg;
	int lProcessQueueId = -1;
	int lRet = EXIT_FAILURE;

	if (0 != unDataLen)
	{
		if (NULL == pMsgData)
		{
			return lRet;
		}
	}


	memset(&stChildProcessMsg, 0x00, sizeof(Child_Process_Msg));

	get_process_msg_queue_id(NULL, &lProcessQueueId);
	stChildProcessMsg.msg_type   = ulThreadId;
	stChildProcessMsg.Thread_cmd = Cmd;
	stChildProcessMsg.length     = unDataLen;

	if (0 != unDataLen)
	{
		if (MAX_MSG_LEN >= unDataLen)
		{
			memcpy(stChildProcessMsg.data, (char *)pMsgData, unDataLen);
		}
		else
		{
			//PRINT_WARNNING HERE
			memcpy(stChildProcessMsg.data, (char *)pMsgData, MAX_MSG_LEN);
		}
	}


	msg_queue_snd(lProcessQueueId, &stChildProcessMsg, (sizeof(Child_Process_Msg) - MAX_MSG_LEN - sizeof(long) + unDataLen));

	return EXIT_SUCCESS;

}

/**
  @brief 获取同子线程通信消息队列
  @author wfb
  @param[in] plThreadMsgId : 发送线程
  @param[in] plProcessMsgId : 发送命令
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
int get_process_msg_queue_id(int *plThreadMsgId,  int *plProcessMsgId)
{
	if (NULL != plThreadMsgId)
	{
		*plThreadMsgId = msg_queue_get((key_t)CHILD_THREAD_MSG_KEY);

	    if (-1 == *plThreadMsgId)
	    {
	        exit(0);
		}
	}

	if (NULL != plProcessMsgId)
	{
		*plProcessMsgId = msg_queue_get((key_t)CHILD_PROCESS_MSG_KEY);

		if (-1 == *plProcessMsgId)
		{
			exit(0);
	 	}

	}

	return EXIT_SUCCESS;
	//打印成功获取的消息
}

/**
  @brief 是否所有的线程都已回复停止使用音视频针的回复
  @author wfb 
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
int is_all_thread_return_stop_use_frame_rep(void)
{
    DEBUGPRINT(DEBUG_INFO, "g_lVideoAlarm = %d  \n", g_lVideoAlarm);
    DEBUGPRINT(DEBUG_INFO, "g_lAudioSend  = %d  \n", g_lAudioSend);
    DEBUGPRINT(DEBUG_INFO, "g_lVideoSend  = %d  \n", g_lVideoSend);
    DEBUGPRINT(DEBUG_INFO, "g_lAudioAlarm = %d  \n", g_lAudioAlarm);
    DEBUGPRINT(DEBUG_INFO, "g_lP2p        = %d  \n", g_lP2p);

	return  (TRUE == g_lVideoAlarm) &&
			(TRUE == g_lAudioSend)  &&
			(TRUE == g_lVideoSend)  &&
			(TRUE == g_lAudioAlarm) &&
			(TRUE == g_lP2p);

#if 0
	return (TRUE == g_lAudioFlag) &&
	(TRUE == g_lAudio) &&
	(TRUE == g_lAudioAlarm) &&
	(TRUE == g_lAudioSend) &&
	(TRUE == g_lDdnsServer) &&
	(TRUE == g_lFlashRW) &&
	(TRUE == g_lForwarding) &&
	(TRUE == g_lHTTPServer) &&
	(TRUE == g_lMFI) &&
	(TRUE == g_lNetWorkSet) &&
	(TRUE == g_lNewsChannel) &&
	(TRUE == g_lNTP) &&
	(TRUE == g_lplayback) &&
	(TRUE == g_lRevplayback) &&
	(TRUE == g_lSerialPorts) &&
	(TRUE == g_lTooling) &&
	(TRUE == g_lUdpserver) &&
	(TRUE == g_lUpgrade) &&
	(TRUE == g_lUPNP) &&
	(TRUE == g_lVideo) &&
	(TRUE == g_lVideoAlarm) &&
	(TRUE == g_lVideoSend) &&
	(TRUE == g_lIoCtl) &&
	(TRUE == g_lP2p);
#endif
}

/**
  @brief 清空标志
  @author wfb 
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void reset_flag()
{
	g_lAudioFlag = 0;
	g_lAudio = 0;
	g_lAudioAlarm = 0;
	g_lAudioSend = 0;
	g_lDdnsServer = 0;
	g_lFlashRW = 0;
	g_lForwarding = 0;
	g_lHTTPServer = 0;
	g_lMFI = 0;
	g_lNetWorkSet = 0;
	g_lNewsChannel = 0;
	g_lNTP = 0;
	g_lplayback = 0;
	g_lRevplayback = 0;
	g_lSerialPorts = 0;
	g_lTooling = 0;
	g_lUdpserver = 0;
	g_lUpgrade = 0;
	g_lUPNP = 0;
	g_lVideo = 0;
	g_lVideoAlarm = 0;
	g_lVideoSend = 0;
	g_lIoCtl = 0;
	g_lP2p = 0;

	/* 定义音频。视频停止采集数据 */
	g_lAudioStopFlag = 0;
}

/**
  @brief 音视频是否停止采集
  @author wfb 
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
int is_all_video_and_audio_stop_gather(void)
{
    DEBUGPRINT(DEBUG_INFO, "g_lAudioStopFlag = %d  \n", g_lAudioStopFlag);
    return ((1 == g_lAudioStopFlag) || (2 == g_lAudioStopFlag));
}

/**
  @brief 发送网络设置
  @author wfb 
  @param[in] pstNetSetInfo：网络设置信息 在common.h中定义
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
int send_message_to_set_net(netset_info_t *pstNetSetInfo)
{
    char acMsgData[MAX_MSG_QUEUE_LEN] = {0};  ///< 栈开了个大的缓存  有点浪费 以后想办法优化下内存的使用效率
    MsgData *pstMsg = (MsgData *)acMsgData;
    int lThreadMsgId = -1;

    if (NULL == pstNetSetInfo)
    {
        printf("send_message_to_set_net invalid\n");
        return EXIT_FAILURE;
    }
    printf("comeinto send_message_to_set_net\n");
    get_process_msg_queue_id(&lThreadMsgId,  NULL);

    pstMsg->type = NETWORK_SET_MSG_TYPE;
    pstMsg->len = sizeof(netset_info_t);
    /* cmd的值要与 networkset.h MSG_NETWORKSET_T_SET_NET 一致 */
    pstMsg->cmd = 1;
    memcpy((char *)(pstMsg+1), pstNetSetInfo, sizeof(netset_info_t));


    msg_queue_snd(lThreadMsgId, pstMsg, sizeof(MsgData) - sizeof(long) + pstMsg->len);

    return EXIT_SUCCESS;
}


/**
  @brief 设置状态灯状态
  @author wfb
  @param[in] nLedColor : 指示灯名称
  @param[in] nLedCtlType : 指示灯控制类型
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void set_led_state(int nLedColor, int nLedCtlType)
{
	pthread_mutex_lock(&g_stLedState);
	if (LED_TYPE_NET == nLedColor)
	{
		g_nNetLedState = nLedCtlType;
	}
	else if(LED_TYPE_NIGHT == nLedColor)
	{
		g_nLightLedState = nLedCtlType;
	}
	else
	{
		g_nPowerLedState = nLedCtlType;
	}

	pthread_mutex_unlock(&g_stLedState);
}


/**
  @brief 获取指示灯状态
  @author wfb 
  @param[in] nLedName : 指示灯名称
  @param[in] nLedCtlType : 指示灯控制类型
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
int get_led_state(int nLedName)
{
	if (LED_TYPE_NET == nLedName)
	{
		return g_nNetLedState;
	}
	else if (LED_TYPE_POWER == nLedName)
	{
		return g_nPowerLedState;
	}
	else if (LED_TYPE_NIGHT == nLedName)
	{
		return g_nPowerLedState;
	}
	else
	{
		printf("sorry, you have input an invalid LedName = %d\n", nLedName);
		return LED_CTL_TYPE_INVALID;
	}

}

/**
  @brief 获取当前网络状态
  @author wfb 
  @param[in] 无
  @param[in] 无
  @param[out] pstNetSetState:网络状态结构体
  @return EXIT_SUCCESS EXIT_FAILURE
  @remark 2013-09-02 增加该注释
  @note
*/
int get_cur_net_state(netset_state_t *pstNetSetState)
{
    if (NULL != pstNetSetState)
    {
    	pthread_mutex_lock(&g_stLedState);
    	memcpy(pstNetSetState, &g_stNetStateInfo, sizeof(netset_state_t));
    	pthread_mutex_unlock(&g_stLedState);
    	return EXIT_SUCCESS;
    }
    else
    {
    	return EXIT_FAILURE;
    }
}

/**
  @brief 设置当前网络状态，有线或无线  静态或动态  连接情况，由于是三个变量 若只设置一个变量一定要先读出然后再设置
  @author wfb 
  @param[out] 无
  @param[in] pstNetSetState:网络状态结构体
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
int set_cur_net_state(netset_state_t *pstNetSetState)
{
    if (NULL != pstNetSetState)
    {
    	pthread_mutex_lock(&g_stLedState);
    	memcpy(&g_stNetStateInfo, pstNetSetState, sizeof(netset_state_t));
    	pthread_mutex_unlock(&g_stLedState);
    	return EXIT_SUCCESS;
    }
    else
    {
    	return EXIT_FAILURE;
    }
}


/**
  @brief led灯控制  若是M2 nLedType: LED_M2_TYPE_MIXED
  @author wfb 
  @param[in] nLedType: 指示灯名称
  @param[in] nCtlType: 指示灯控制类型
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void led_control(int nLedType, int nCtlType)
{
	int lThreadMsgId = -1;
	char buf[128] = {0};
	MsgData *pstMsgSnd = (MsgData *)buf;

	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->type = IO_CTL_MSG_TYPE;
	pstMsgSnd->cmd = MSG_IOCTL_T_LED_CTL;
	pstMsgSnd->len = LED_CTL_MSG_DATA_LEN;
	*(char *)(pstMsgSnd + 1) = nLedType;
	*(((char *)(pstMsgSnd + 1))+1) = nCtlType;
	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
}

/**
  @brief 直接设置led灯的状态  若是M2 nLedType: LED_M2_TYPE_MIXED
  @author wfb 
  @param[in] nLedType: 指示灯名称
  @param[in] nCtlType: 指示灯控制类型
  @param[out] 无
  @return 无
  @remark 2013-10-31 增加该函数
  @note
*/
void direct_set_led_state(int nLedType, int nCtlType)
{
	int lThreadMsgId = -1;
	char buf[128] = {0};
	MsgData *pstMsgSnd = (MsgData *)buf;

	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->type = IO_CTL_MSG_TYPE;
	pstMsgSnd->cmd = MSG_IOCTL_T_DIRECT_SET_LED_STATE;
	pstMsgSnd->len = LED_CTL_MSG_DATA_LEN;
	*(char *)(pstMsgSnd + 1) = nLedType;
	*(((char *)(pstMsgSnd + 1))+1) = nCtlType;
	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
}


/**
  @brief M2 led灯控制  颜色(电池电量信息)
  @author wfb 
  @param[in] nLedColorType: 指示灯颜色
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void battary_power_state_report(int nBattaryPowerState)
{
	int lThreadMsgId = -1;
	char buf[128] = {0};
	MsgData *pstMsgSnd = (MsgData *)buf;

	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->type = IO_CTL_MSG_TYPE;
	pstMsgSnd->cmd = MSG_IOCTL_T_CTL_LED_STATE;
	pstMsgSnd->len = 2;

	*(char *)(pstMsgSnd + 1) = LED_CONTROL_ITEM_BATTERY_STATE;
	*(((char *)(pstMsgSnd + 1))+1) = nBattaryPowerState;

	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
}


/**
  @brief 夜间模式控制  颜色(电池电量信息)
  @author wfb 
  @param[in] nLedColorType: 指示灯颜色
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void night_mode_report(int nNightModeFlag)
{
	int lThreadMsgId = -1;
	char buf[32] = {0};
	MsgData *pstMsgSnd = (MsgData *)buf;

	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->type = IO_CTL_MSG_TYPE;
	pstMsgSnd->cmd = MSG_IOCTL_T_CTL_LED_STATE;
	pstMsgSnd->len = 2;
	*(char *)(pstMsgSnd + 1) = LED_CONTROL_ITEM_NIGHT_MODE;
	*(((char *)(pstMsgSnd + 1))+1) = nNightModeFlag;

	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
}

/**
  @brief 通过flash配置设置wifi PARAM: null
  @author wfb 
  @param[in] pstNetSetInfo: NULL
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void set_wifi_by_config(netset_info_t *pstNetSetInfo)
{
	char acTmpBuf[256] = {0};
	netset_info_t stNetSetInfo;
	int lTmpLen = 0;

	/* 查看flash 是否有可用的wifi */
	get_config_item_value(CONFIG_WIFI_ACTIVE, acTmpBuf, &lTmpLen);

	DEBUGPRINT(DEBUG_INFO, "acTmpBuf = %s, lTmpLen = %d\n", acTmpBuf, lTmpLen);

	if (strncmp(acTmpBuf, "y", 1))
	{
		DEBUGPRINT(DEBUG_INFO, "no active wifi in flash, return\n");
		/* TODO 到这里是没有网络可用的 通知 */
        return;
	}
	else
	{
		DEBUGPRINT(DEBUG_INFO, "start to set wifi,read para from flash\n");
	}
	/* 发送设置 无线网 */
	/* 发送设置 有线网络  */
	stNetSetInfo.lNetType = NET_TYPE_WIRELESS;

    /* 获取ip类型 */
	if (EXIT_FAILURE == get_config_item_value(CONFIG_WIFI_MODE, acTmpBuf, &lTmpLen))
	{
		printf(" get_config_item_value CONFIG_WIFI_MODE failed\n");
	}

	if (0 == strncmp(acTmpBuf, "S", 1))
	{
		stNetSetInfo.lIpType = IP_TYPE_STATIC;
	}
	else
	{
		stNetSetInfo.lIpType = IP_TYPE_DYNAMIC;
	}

	/* 获取ssid和密码 */
	get_config_item_value(CONFIG_WIFI_SSID,         stNetSetInfo.acSSID,      &lTmpLen);
	DEBUGPRINT(DEBUG_INFO, "CONFIG_WIFI_SSID  : %s lTmpLen is: %d\n", stNetSetInfo.acSSID, lTmpLen);

	/* 如果ssid为空 则认为ssid无效 不设置 */
	if (0 == lTmpLen)
	{
		DEBUGPRINT(DEBUG_INFO, "wifi ssid is null , return\n");
		return;
	}

	get_config_item_value(CONFIG_WIFI_ENRTYPE,      acTmpBuf,     &lTmpLen);
	DEBUGPRINT(DEBUG_INFO, "config wifi enrtyp is : %s\n", acTmpBuf);
	stNetSetInfo.lWifiType = (int)atoi(acTmpBuf);

	get_config_item_value(CONFIG_WIFI_PASSWD,       stNetSetInfo.acCode,      &lTmpLen);
	DEBUGPRINT(DEBUG_INFO, "CONFIG_WIFI_PASSWD: %s lTmpLen is: %d\n", stNetSetInfo.acCode, lTmpLen);

	if (IP_TYPE_STATIC == stNetSetInfo.lIpType)
	{
		get_config_item_value(CONFIG_WIFI_IP,      stNetSetInfo.acIpAddr,     &lTmpLen);
		get_config_item_value(CONFIG_WIFI_SUBNET,  stNetSetInfo.acSubNetMask, &lTmpLen);
		get_config_item_value(CONFIG_WIFI_GATEWAY, stNetSetInfo.acGateWay,    &lTmpLen);
		get_config_item_value(CONFIG_WIRED_DNS,    stNetSetInfo.acWiredDNS,  &lTmpLen);
		get_config_item_value(CONFIG_WIFI_DNS,     stNetSetInfo.acWifiDNS,   &lTmpLen);
	}

	printf("ready to set wifi net\n");
	/* 设置网络 */
	send_message_to_set_net(&stNetSetInfo);
}

/**
  @brief led是否工作控制
  @author wfb 
  @param[in] nLedType: 指示灯类型
  @param[in] nCtlType: 指示灯控制类型
  @param[in] nWriteFlashFlag: 是否写Flash标志
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
          2014-03-05 修改该函数，增加对夜视灯的特殊处理 夜视灯要关闭报警功能
  @note
*/
void led_if_work_control(int nLedType, int nCtlType, int nWriteFlashFlag)
{
	int lThreadMsgId = -1;
	char buf[32] = {0};
	MsgData *pstMsgSnd = (MsgData *)buf;

	//注意在这里要判断是否是夜视灯
	if (LED_TYPE_NIGHT == nLedType)
	{
		night_led_if_work_msg_handler(nCtlType);
	}
	else
	{
		get_process_msg_queue_id(&lThreadMsgId,  NULL);
		pstMsgSnd->type = IO_CTL_MSG_TYPE;
		pstMsgSnd->cmd = MSG_IOCTL_T_CTL_LED_STATE;
		pstMsgSnd->len = LED_IF_WORK_MSG_DATA_LEN;

		*(char *)(pstMsgSnd + 1) = LED_CONTROL_ITEM_IF_WORK;
		*(((char *)(pstMsgSnd + 1))+1) = nLedType;
		*(((char *)(pstMsgSnd + 1))+2) = nCtlType;
		*(((char *)(pstMsgSnd + 1))+3) = nWriteFlashFlag;

		msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
	}
}

/**
  @brief led是否工作直接控制，主要是解决夜视灯在开关的时候会报警
  @author wfb 
  @param[in] nLedType: 指示灯类型
  @param[in] nCtlType: 指示灯控制类型
  @param[in] nWriteFlashFlag: 是否写Flash标志
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
          2014-03-05 修改该函数，增加对夜视灯的特殊处理 夜视灯要关闭报警功能
  @note
*/
void led_if_work_direct_control(int nLedType, int nCtlType, int nWriteFlashFlag)
{
	int lThreadMsgId = -1;
	char buf[32] = {0};
	MsgData *pstMsgSnd = (MsgData *)buf;

	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->type = IO_CTL_MSG_TYPE;
	pstMsgSnd->cmd = MSG_IOCTL_T_CTL_LED_STATE;
	pstMsgSnd->len = LED_IF_WORK_MSG_DATA_LEN;

	*(char *)(pstMsgSnd + 1) = LED_CONTROL_ITEM_IF_WORK;
	*(((char *)(pstMsgSnd + 1))+1) = nLedType;
	*(((char *)(pstMsgSnd + 1))+2) = nCtlType;
	*(((char *)(pstMsgSnd + 1))+3) = nWriteFlashFlag;

	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);

}


/**
  @brief是否没有使用的线程
  @author wfb 
  @param[in] nThreadId: 线程ID
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
int if_not_use_thread(int nThreadId)
{
	if ((UPNP_MSG_TYPE == nThreadId) ||
		(DDNS_SERVER_MSG_TYPE == nThreadId) ||
		(FORWARDING_MSG_TYPE == nThreadId) ||
		(NTP_MSG_TYPE == nThreadId) ||
		(TOOLING_MSG_TYPE == nThreadId) ||
		(UPGRADE_MSG_TYPE == nThreadId) ||
		(CONTROL_MSG_TYPE == nThreadId))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
/**
  @brief 发送更新消息
  @author wfb 
  @param[in] nThreadId: 线程ID
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void updata_config_to_flash()
{
	int lThreadMsgId = -1;
	char buf[32] = {0};
	MsgData *pstMsgSnd = (MsgData *)buf;

	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->type = FLASH_RW_MSG_TYPE;
	pstMsgSnd->cmd = MSG_FLAHSRW_T_UPDATA_CONFIG_TO_FLASH;
	pstMsgSnd->len = 0;

	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
}


/**
  @brief 写文件 存储log信息 /etc/loginfo.txt
  @author wfb 
  @param[in] nThreadId: 线程ID
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void write_log_info_to_file(char *pStrInfo)
{
	FILE *pFile = NULL;

	if ((pStrInfo == NULL) || (0 == strlen(pStrInfo)))
	{
		return;
	}

	pFile = fopen("/etc/loginfo.txt","a+");

	if (NULL == pFile)
	{
        DEBUGPRINT(DEBUG_INFO, "OPEN loginfo failed\n");
        return;
	}

	fprintf(pFile, pStrInfo);

	fclose(pFile);

}

/**
  @brief 获取wifi信号强度, 注意这个函数是对全局变量赋值
  @author wfb 
  @param[in] 无
  @param[out] pcWifiInfo: 获取wifi信息结构体指针
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void get_wifi_signal_level(char *pcWifiInfo)
{
	if (NULL == pcWifiInfo)
	{
		DEBUGPRINT(DEBUG_INFO, "get_wifi_signal_level para invalid\n");
		return;
	}

	pthread_mutex_lock(&g_stWifiSignalLevelLock);
	memcpy(pcWifiInfo, g_acWifiSignalLevel, 9);
	pthread_mutex_unlock(&g_stWifiSignalLevelLock);
}


/**
  @brief 设置wifi信号轻度
  @author wfb 
  @param[in] 无
  @param[out] pcWifiInfo : 设置wifi信息结构体指针
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
int set_wifi_signal_level(char *pcWifiInfo)
{
	pthread_mutex_lock(&g_stWifiSignalLevelLock);
	memcpy(g_acWifiSignalLevel, pcWifiInfo, 9);
	pthread_mutex_unlock(&g_stWifiSignalLevelLock);

	//write_log_info_to_file(g_acWifiSignalLevel);
}


/**
  @brief wifi断连后，tutk和tcp停止发送数据
  @author wfb 
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void tutk_and_tcp_stop_send_data()
{
	int lThreadMsgId = -1;
	char buf[128] = {0};
	MsgData *pstMsgSnd = (MsgData *)buf;

	get_process_msg_queue_id(&lThreadMsgId,  NULL);

	//向tutk发送消息
	pstMsgSnd->type = P2P_MSG_TYPE;
	pstMsgSnd->cmd = CMD_STOP_SEND_DATA;
	pstMsgSnd->len = 0;
	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
	DEBUGPRINT(DEBUG_INFO, "already send msg to tutk TTTTTTTUUUUUUUUTTTTTTTTKKKKKKKKKK\n");

	//向tcp发送消息
	pstMsgSnd->type = NEWS_CHANNEL_MSG_TYPE;
	pstMsgSnd->cmd = CMD_STOP_SEND_DATA;
	pstMsgSnd->len = 0;
	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
	DEBUGPRINT(DEBUG_INFO, "already send msg to tcp  TTTTTTTTTCCCCCCCCCCCPPPPPPPPPPPPPP\n");
}


/**
  @brief 正在联网或无网络可用
  @author wfb 
  @param[in] nLedType : 指示灯名称
  @param[in] nCtlType： 指示灯控制类型
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void net_no_ip_set_led(void)
{
    int lThreadMsgId = -1;
 	char buf[128] = {0};
	MsgData *pstMsgSnd = (MsgData *)buf;

	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->type = IO_CTL_MSG_TYPE;
	pstMsgSnd->cmd = MSG_IOCTL_T_CTL_LED_STATE;
	pstMsgSnd->len = NET_STATE_REPORT_LEN;

	*(char *)(pstMsgSnd + 1) = LED_CONTROL_ITEM_NET_STATE;
	*((char *)((pstMsgSnd + 1))+1) = NET_STATE_NO_IP;
	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);

	common_write_log("COMMON:net_no_ip_set_led");
}

/**
  @brief 获取ip成功设置led闪灯
  @author wfb 
  @param[in] nLedType : 指示灯名称
  @param[in] nCtlType： 指示灯控制类型
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void net_get_ip_ok_set_led(void)
{
	int lThreadMsgId = -1;
	char buf[128] = {0};
	MsgData *pstMsgSnd = (MsgData *)buf;

	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->type = IO_CTL_MSG_TYPE;
	pstMsgSnd->cmd = MSG_IOCTL_T_CTL_LED_STATE;
	pstMsgSnd->len = NET_STATE_REPORT_LEN;

	*(char *)(pstMsgSnd + 1) = LED_CONTROL_ITEM_NET_STATE;
	*((char *)((pstMsgSnd + 1))+1) = NET_STATE_HAVE_IP;

	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);

	common_write_log("COMMON:net_get_ip_ok_set_led");
}

/**
  @brief 网络连接成功设置led闪灯
  @author wfb 
  @param[in] nLedType : 指示灯名称
  @param[in] nCtlType： 指示灯控制类型
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void net_connect_ok_set_led(void)
{
	int lThreadMsgId = -1;
	char buf[128] = {0};
	MsgData *pstMsgSnd = (MsgData *)buf;

	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->type = IO_CTL_MSG_TYPE;
	pstMsgSnd->cmd = MSG_IOCTL_T_CTL_LED_STATE;
	pstMsgSnd->len = NET_STATE_REPORT_LEN;


	*(char *)(pstMsgSnd + 1) = LED_CONTROL_ITEM_NET_STATE;
	*((char *)((pstMsgSnd + 1))+1) = NET_STATE_OK;
	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
}

/**
  @brief tutk登录成功 led闪灯
  @author wfb 
  @param[in] nLedType : 指示灯名称
  @param[in] nCtlType: 指示灯控制类型
  @param[out] 无
  @return 无
  @remark 2013-10-16 增加该函数
          2014-10-27 去掉变量
  @note
*/
void tutk_login_ok_set_led(int nLedType, int nCtlType)
{
	pthread_mutex_lock(&g_stDhcpTutkLedLock);
	//led_control(nLedType, nCtlType); todo
    net_connect_ok_set_led();
	//g_nTutkLoginOkFlag = TRUE; 该变量无用 modify by wangfengbo
	pthread_mutex_unlock(&g_stDhcpTutkLedLock);
}


/**
  @brief 清空tutk和http标志位
  @author wfb 
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void reset_tutk_https_set_led_flag()
{
	g_nRcvTutkLoginOKFlag = FALSE;
	g_nRcvHttpAuthenOKFlag = FALSE;
}

/**
  @brief HTTP或TUTK设置led闪灯
  @author wfb 
  @param[in] nControler： tutk或是http  LED_CONTROL_TUTK或LED_CONTROL_HTTP
  @param[out] 无
  @return 无
  @remark 2013-10-16 增加该函数 tutk和http都成功后才置灯
          2014-02-27 修改该函数 去掉无用参数
  @note
*/
void tutk_and_http_set_led(int nControler)
{
    if (NET_STATE_CONTROL_TUTK == nControler)
    {
    	common_write_log("tutk_and_http_set_led  NET_STATE_CONTROL_TUTK!!!!!!!!!!!!!");

    	g_nRcvTutkLoginOKFlag = TRUE;
    	DEBUGPRINT(DEBUG_INFO, "LED_CONTROL_TUTK  RCV..................\n");
    }
    else if (NET_STATE_CONTROL_HTTP == nControler)
    {
    	common_write_log("tutk_and_http_set_led  NET_STATE_CONTROL_HTTP!!!!!!!!!!!!!");

    	g_nRcvHttpAuthenOKFlag = TRUE;
    	DEBUGPRINT(DEBUG_INFO, "LED_CONTROL_HTTP  RCV..................\n");
    }
    else
    {
    	DEBUGPRINT(DEBUG_INFO, "wrong controler..................\n");
    	return;
    }

    //设置led
    if ((TRUE == g_nRcvTutkLoginOKFlag) && (TRUE == g_nRcvHttpAuthenOKFlag))
    {

    	common_write_log("tutk_and_http_set_led  OK!!!!!!!!!!!!!");

    	net_connect_ok_set_led();
    	g_nRcvTutkLoginOKFlag = FALSE;
    }
}


/**
  @brief 清空tutk登录成功标志
  @author wfb 
  @param[in] 无
  @param[out] 无
  @return 无
  @remark  2013-09-02 增加该注释
  @note
*/
void reset_tutk_login_flag()
{
	pthread_mutex_lock(&g_stDhcpTutkLedLock);
	g_nTutkLoginOkFlag = FALSE;
	DEBUGPRINT(DEBUG_INFO, "g_nTutkLoginOkFlag = FALSE..........\n");
	pthread_mutex_unlock(&g_stDhcpTutkLedLock);
}

/**
  @brief 向视频报警线程发送停止报警, 默认报警级别为9
  @author wfb 
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void send_msg_to_video_alarm_stop_alarm()
{
	int lThreadMsgId = -1;
	char buf[128] = {0};
	MsgData *pstMsgSnd = (MsgData *)buf;

	get_process_msg_queue_id(&lThreadMsgId,  NULL);

	//向视频报警线程发送消息
	pstMsgSnd->type = VIDEO_ALARM_MSG_TYPE;
	pstMsgSnd->cmd = MSG_VA_T_CONTROL;
	pstMsgSnd->len = 8;
	*((int *)(pstMsgSnd + 1)) = 2;       //关视频报警 默认级别9级
	*(((int *)(pstMsgSnd + 1))+1) = 9;
	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
}

/**
  @brief 向视频报警线程发送开始报警 默认报警级别为9
  @author wfb 
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void send_msg_to_video_alarm_start_alarm()
{
	int lThreadMsgId = -1;
	char buf[128] = {0};
	MsgData *pstMsgSnd = (MsgData *)buf;

	get_process_msg_queue_id(&lThreadMsgId,  NULL);

	//向视频报警线程发送消息
	pstMsgSnd->type = VIDEO_ALARM_MSG_TYPE;
	pstMsgSnd->cmd = MSG_VA_T_CONTROL;
	pstMsgSnd->len = 8;
	*((int *)(pstMsgSnd + 1)) = 1;       //开视频报警 默认级别9级
	*(((int *)(pstMsgSnd + 1))+1) = 9;
	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
}

/**
  @brief 开始检测网络
  @author wfb 
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-19-25 增加该函数
  @note
*/
void start_check_net_connect(void)
{
	int lThreadMsgId = -1;
	char buf[128] = {0};
	MsgData *pstMsgSnd = (MsgData *)buf;

	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->type = NETWORK_SET_MSG_TYPE;
	pstMsgSnd->cmd = MSG_NETWORKSER_T_NET_START_PING;
	pstMsgSnd->len = 0;
	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
}


/**
  @brief 也是灯处理函数
  @author wfb 
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void night_led_if_work_msg_handler(unsigned int nNightLedCtl)
{
    /*(1) 停止timer 停止视频报警 */
	suspend_timer_by_id(TIMER_ID_CTL_NIGHT_LED_CTL);
	suspend_timer_by_id(TIMER_ID_CTL_VIDEO_ALARM);

	DEBUGPRINT(DEBUG_INFO, "night_led_if_work_msg_handler ............\n");
	g_nNightLedCtl = nNightLedCtl;
	send_msg_to_video_alarm_stop_alarm();

	reset_timer_param_by_id(TIMER_ID_CTL_NIGHT_LED_CTL, 10, NULL);
	start_timer_by_id(TIMER_ID_CTL_NIGHT_LED_CTL);
	/*(2) 打开活关闭led灯 */
	//
	/*(3) 查看是否打开视频报警 */
	//
	common_write_log("COMMON:night_led_if_work_msg_handler");
}


/**
  @brief 进入夜间模式处理
  @author wfb 
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void night_mode_ctl_msg_handler(unsigned int nNightModeFlag)
{
    /*(1) 停止timer 停止视频报警 */
	suspend_timer_by_id(TIMER_ID_CTL_NIGHT_MODE_CTL);
	suspend_timer_by_id(TIMER_ID_CTL_VIDEO_ALARM);

	g_nNightModeFlag = nNightModeFlag;
	send_msg_to_video_alarm_stop_alarm();

	reset_timer_param_by_id(TIMER_ID_CTL_NIGHT_MODE_CTL, 10, NULL);
	start_timer_by_id(TIMER_ID_CTL_NIGHT_MODE_CTL);

	common_write_log("COMMON:night_mode_ctl_msg_handler");
	/*(2) 打开活关闭led灯 */
	//
	/*(3) 查看是否打开视频报警 */
	//
}


/**
  *  @brief      system_stop_speaker_and_reboot
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void system_stop_speaker_and_reboot()
{
	int lThreadMsgId = -1;
	char acBuf[64] = {0};
	MsgData *pstMsgSnd = (MsgData *)acBuf;

	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->type = PLAY_BACK_MSG_TYPE;
	pstMsgSnd->cmd = MSG_SPEAKER_P_REBOOT;

	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long));
}


/**
  *  @brief      void set_cur_ip_addr(char *pIpAddr)
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2014-02-26 增加该函数
  *  @note
*/
void set_cur_ip_addr(char *pIpAddr)
{
    strncpy(g_acWifiIpInfo, pIpAddr, strlen(pIpAddr)<16 ? strlen(pIpAddr):16);
    g_acWifiIpInfo[strlen(pIpAddr)<16 ? strlen(pIpAddr):16] = 0;
}

/**
  @brief      void get_cur_ip_addr(char *pIpAddr)
  @author     <wfb> 
  @param[in]  <无>
  @param[out] <无>
  @return     <无>
  @remark     2014-02-26 增加该函数
  @note
*/
void get_cur_ip_addr(char *pIpAddr)
{
	strncpy(pIpAddr, g_acWifiIpInfo, strlen(g_acWifiIpInfo));
	*(pIpAddr+strlen(g_acWifiIpInfo)) = 0;
}




void send_msg_to_io_reboot_rt5350()
{
	int lThreadMsgId = -1;
	char acBuf[64] = {0};
	MsgData *pstMsgSnd = (MsgData *)acBuf;

	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->type = IO_CTL_MSG_TYPE;
	pstMsgSnd->cmd = MSG_IOCTL_T_KERNEL_MUST_REBOOT;

	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long));
}



RunTimeStateInfo rtsi;
void runtimeinfo_init()
{
	rtsi.nightmode_state = NIGHT_MODE_OFF;
	rtsi.nightmode_start_time = 0;
	pthread_mutex_init(&rtsi.nm_mutex, NULL);
}

void inline runtimeinfo_set_nm_state(int state)
{
	pthread_mutex_lock(&rtsi.nm_mutex);
	rtsi.nightmode_state = state;
	pthread_mutex_unlock(&rtsi.nm_mutex);
}

int inline runtimeinfo_get_nm_state(void)
{
	int state;
	pthread_mutex_lock(&rtsi.nm_mutex);
	state = rtsi.nightmode_state;
	pthread_mutex_unlock(&rtsi.nm_mutex);

	return state;
}

void inline runtimeinfo_set_nm_time(int time)
{
	pthread_mutex_lock(&rtsi.nm_mutex);
	rtsi.nightmode_start_time = time;
	pthread_mutex_unlock(&rtsi.nm_mutex);
}

int inline runtimeinfo_get_nm_time(void)
{
	int time;
	pthread_mutex_lock(&rtsi.nm_mutex);
	time = rtsi.nightmode_start_time;
	pthread_mutex_unlock(&rtsi.nm_mutex);

	return time;
}

int init_Camera_info(){

	char tmp[100];
	char *p;
	char *buf1,*buf2;
	int tmp_len;
	char *ap;

	FILE *fp = fopen("/usr/share/info.txt", "r");
	if(fp == NULL){
		DEBUGPRINT(DEBUG_INFO, "open /usr/share/info.txt fail\n");
		return -1;
	}

	if (fp != NULL ) {
		while (fgets(tmp, 100, fp)) {
			p = NULL;
			if ((p = strstr(tmp, "firmware")) != NULL ) {
				tmp_len = strlen("firmware");
				buf1 = Cam_firmware;
				buf2 = Cam_firmware_HTTP;
			} else if ((p = strstr(tmp, "camera_type")) != NULL ) {
				tmp_len = strlen("camera_type");
				buf1 = Cam_camera_type;
				buf2 = Cam_camera_type_HTTP;
			} else if ((p = strstr(tmp, "hardware")) != NULL ) {
				tmp_len = strlen("hardware");
				buf1 = Cam_hardware;
				buf2 = Cam_hardware_HTTP;
			} else if ((p = strstr(tmp, "camera_model")) != NULL ) {
				tmp_len = strlen("camera_model");
				buf1 = Cam_camera_model;
				buf2 = Cam_camera_model_HTTP;
			}

			memset(buf1,'\0',SYSTME_VERSION_LEN);
			memset(buf2,'\0',SYSTME_VERSION_LEN);

			if (p != NULL ) {
				p += tmp_len;
				p += 1;
				memcpy(buf1, p, strlen(p));
				memcpy(buf2, p, strlen(p));
				buf2[strlen(p)-1] = '\0';
			}
		}

		fclose(fp);
	}
}



/**
  @brief void common_write_log(const char *fmt, ...)
  @author <wfb> 
  @param[in] <fmt>
  @param[out] <无>
  @return <无>
  @remark 2014-02-26 增加该函数
  @note
*/
void common_write_log(const char *fmt, ...)
{
    int n;
    va_list ap;

    memset(g_acLogBuf, 0, COMMON_LOG_BUG_LEN);
    va_start(ap, fmt);
    vsnprintf(g_acLogBuf, COMMON_LOG_BUG_LEN, fmt, ap);

    n = strlen(g_acLogBuf);

    if (n >= COMMON_LOG_BUG_LEN-2)
    {
         return;
    }

    g_acLogBuf[n++] = '\n';
    g_acLogBuf[n++] = '\0';
    va_end(ap);
    log_s(g_acLogBuf, n);
}
