/**   @file []
   *  @brief    IO子进程
   *  @note
   *  @author   wangfengbo
   *  @date     2013-9-2
   *  @remarks  增加注释
*/
/*******************************************************************************/
/*                                    头文件                                   */
/*******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "common/utils.h"
#include "common/logfile.h"
#include "ioctl_adapter.h"
#include "common/common.h"
#include "common/msg_queue.h"
#include "common/time_func.h"
#include "common/common_func.h"
#include "common/common_config.h"
#include "NetWorkSet/networkset.h"
#include "playback/speaker.h"
#include "ioctl.h"


/******************************************************************************/
/*                               内部函数声明                                        */
/******************************************************************************/
void ioctl_usb_monitor_process_msghandler(msg_container *pstMsgContainer);
void ioctl_net_monitor_process_msghandler(msg_container *pstMsgContainer);
void ioctl_usb_switch_process_msghandler(msg_container *pstMsgContainer);
void ioctl_i2c_get_cp_device_id_msghandler(msg_container *pstMsgContainer);
void ioctl_i2c_get_authentication_msghandler(msg_container *pstMsgContainer);
void ioctl_check_usb_result_msghandler(msg_container *pstMsgContainer);
void ioctl_inform_set_wifi_msghandler(msg_container *pstMsgContainer);
void ioctl_qbuf_switch_usb_msghandler(msg_container *pstMsgContainer);
void broadcast_all_thread_stop_use_frame(void);
void usb_switch_handler(int lSwitchTo);
void inform_mfi_usb_device_out(void);
void inform_mfi_usb_device_in(void);
void usb_port_monitor_ctl(unsigned char ucCtl);
//void set_wifi(netset_info_t *pstNetSetInfo);
void set_wired(netset_info_t *pstNetSetInfo);
void sendmsg_clean_net_info();
void stop_speaker();

/* 子进程接受来自io线程的消息及处理函数表 */
cmd_item ioctl_cmd_table[] =
{
	{MSG_IOCTL_P_USB_SWITCH,             ioctl_usb_switch_process_msghandler},
	{MSG_IOCTL_P_USB_MONITOR,            ioctl_usb_monitor_process_msghandler},
	{MSG_IOCTL_P_NET_MONITOR,            ioctl_net_monitor_process_msghandler},
	{MSG_IOCTL_P_I2C_DEVICE_ID,          ioctl_i2c_get_cp_device_id_msghandler},
	{MSG_IOCTL_P_I2C_GET_AUTHENTICATION, ioctl_i2c_get_authentication_msghandler},
	{MSG_IOCTL_P_CHECK_USB_RESULT,       ioctl_check_usb_result_msghandler},
	{MSG_IOCTL_P_INFORM_SET_WIRELESS,    ioctl_inform_set_wifi_msghandler},
	{MSG_IOCTL_P_QBUF_SWITCH_USB,        ioctl_qbuf_switch_usb_msghandler},
};

/** io_adapter通用的发送 buf */
char g_acIoAdapter[MAX_MSG_LEN] = {0};

/******************************************************************************/
/*                                 接口函数                                         */
/******************************************************************************/
/**
  *  @brief      查找cmd对应的处理函数
  *  @author     <wfb> 
  *  @param[in]  <pstMsgContainer: 接受消息>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void ioctl_cmd_table_fun(msg_container *pstMsgContainer)
{
	MsgData *pstMsgRcv = (MsgData *)pstMsgContainer->msgdata_rcv;
	cmd_item icmd;
	icmd.cmd = pstMsgRcv->cmd;

	msg_fun fun = utils_cmd_bsearch(&icmd, ioctl_cmd_table, sizeof(ioctl_cmd_table)/sizeof(cmd_item));

	if(NULL != fun)
	{
		fun(pstMsgContainer);
	}
}


/**
  *  @brief      IO子进程usb检测处理函数
  *  @author     <wfb> 
  *  @param[in]  <pstMsgContainer: 接受消息>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void ioctl_usb_monitor_process_msghandler(msg_container *pstMsgContainer)
{
    int lUsbAction = DEVICE_ACTION_INVALID;

    if (NULL == pstMsgContainer)
    {
    	DEBUGPRINT(DEBUG_INFO, "invalid pointer \n");
        return;
    }

	MsgData *pstMsgRcv = (MsgData *)pstMsgContainer->msgdata_rcv;
	if (STATE_MONITOR_MSG_DATA_LEN != pstMsgRcv->len)
	{
		DEBUGPRINT(DEBUG_INFO, "STATE_MONITOR_MSG_DATA_LEN != pstMsgRcv->len\n");
		return;
	}

	/* 获取数据 */
	lUsbAction = *((char *)(pstMsgRcv + 1));

	if (DEVICE_ACTION_IN == lUsbAction)
	{
        /* 停止所有音视频  */
		DEBUGPRINT(DEBUG_INFO, "broad msg to stop use frame\n");
		reset_flag();
        broadcast_all_thread_stop_use_frame();
        //system("echo ioctl child process start broad stop use frame >> /etc/log.txt");
        write_log_info_to_file("ioctl child process start broad stop use frame\n");
        common_write_log("USB: broad stop use frame~~");
#if 0
        struct timeval stBegin;
        gettimeofday(&stBegin, NULL);
        printf("cur time is stBegin.tv_sec = %ld,stBegin.tv_usec = %ld\n", stBegin.tv_sec, stBegin.tv_usec);
#endif
        reset_timer_param_by_id(TIMER_ID_STOP_USE_FRAME, 2, NULL);
        start_timer_by_id(TIMER_ID_STOP_USE_FRAME);

	}
	else if(DEVICE_ACTION_OUT == lUsbAction)
	{
		DEBUGPRINT(DEBUG_INFO, "device action out \n");
		//usb_switch_handler(USB_SWITCH_TO_CAM);
		/* 这里告诉mfi 若苹果设置已经打开 则关闭 等mfi返回后 在切换 */
		inform_mfi_usb_device_out();

		if (TRUE == g_nAvErrorFlag)
		{
			g_nAvErrorFlag = FALSE;
			DEBUGPRINT(DEBUG_INFO, "REBOOT REBOOT REBOOTREBOOT REBOOT REBOOT \n");
			DEBUGPRINT(DEBUG_INFO, "REBOOT REBOOT REBOOTREBOOT REBOOT REBOOT \n");
			DEBUGPRINT(DEBUG_INFO, "REBOOT REBOOT REBOOTREBOOT REBOOT REBOOT \n");
			DEBUGPRINT(DEBUG_INFO, "REBOOT REBOOT REBOOTREBOOT REBOOT REBOOT \n");
	        reset_timer_param_by_id(TIMER_ID_SYSTEM_REBOOT, 5, NULL);
	        start_timer_by_id(TIMER_ID_SYSTEM_REBOOT);
	        stop_speaker();
			DEBUGPRINT(DEBUG_INFO, "REBOOT REBOOT REBOOTREBOOT REBOOT OK \n");
		}

		//io_test_fun(LED_CTL_TYPE_FAST_FLK);
		//system("echo inform mfi usb device out >> /etc/log.txt");
		write_log_info_to_file("inform mfi usb device out\n");
		common_write_log("USB: inform mfi usb device out~~");
	}
	else
	{
		DEBUGPRINT(DEBUG_INFO, "come the wrong area device action\n");
	}

}

/**
  *  @brief      IO子进程网络监测处理函数
  *  @author     <wfb> 
  *  @param[in]  <pstMsgContainer: 接受消息>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *              2013-11-18 增加M2外网
  *  @note
*/
void ioctl_net_monitor_process_msghandler(msg_container *pstMsgContainer)
{
	int lNetAction = DEVICE_ACTION_INVALID;
	netset_state_t stNetSetState;

	if (NULL == pstMsgContainer)
	{
		DEBUGPRINT(DEBUG_INFO, "invalid pointer \n");
	    return;
	}

	MsgData *pstMsgRcv = (MsgData *)pstMsgContainer->msgdata_rcv;
	MsgData *pstMsgSnd = (MsgData *)pstMsgContainer->msgdata_snd;
	netset_info_t *pstNetSetInfo = (netset_info_t *)(pstMsgSnd+1); /* 只把这片buf暂当数据区用 */
	memset(&stNetSetState, 0x00, sizeof(netset_state_t));

#if 0  /* not used */
	if (STATE_MONITOR_MSG_DATA_LEN != pstMsgRcv->len)
	{
	    printf("STATE_MONITOR_MSG_DATA_LEN != pstMsgRcv->len\n");
		return;
	}
#endif
	/* 获取数据 */
	lNetAction = *((char *)(pstMsgRcv + 1));

	if (DEVICE_ACTION_IN == lNetAction)
	{
		set_wired(pstNetSetInfo);
	}
	else if(DEVICE_ACTION_OUT == lNetAction)
	{
		/* 当网线拔出时 重设网络信息全局变量 */
		set_cur_net_state(&stNetSetState);
		//增加M2的外网控制  方便调试
		net_no_ip_set_led();
#if 0
		if(DEVICE_TYPE_M3S == g_nDeviceType)
		{
		    /* 发送灭蓝灯 黄灯常量 */
		    led_control(LED_M3S_TYPE_NET, LED_CTL_TYPE_FAST_FLK);
		    led_control(LED_M3S_TYPE_POWER, LED_CTL_TYPE_ON);
		}
		else
		{
			led_control(LED_M2_TYPE_MIXED, LED_CTL_TYPE_FAST_FLK);
		}
#endif
        /* 清理有线网的资源 */
		sendmsg_clean_net_info();
        /* 设置wifi */
		set_wifi_by_config(pstNetSetInfo);

	}
	else
	{
		DEBUGPRINT(DEBUG_INFO, "come the wrong area device action\n");
	}

}


/**
  *  @brief      IO子进程usb切换处理函数
  *  @author     <wfb> 
  *  @param[in]  <pstMsgContainer: 接受消息>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void ioctl_usb_switch_process_msghandler(msg_container *pstMsgContainer)
{
    int lUsbSwitchType   = -1;
    int lUsbSwitchResult = -1;
    int lUsbPortState    = -1;


    if (NULL == pstMsgContainer)
    {
    	DEBUGPRINT(DEBUG_INFO, "invalid pointer \n");
        return;
    }

	MsgData *pstMsgRcv = (MsgData *)pstMsgContainer->msgdata_rcv;

	if (USB_SWITCH_RETURN_MSG_DATA_LEN != pstMsgRcv->len)
	{
		DEBUGPRINT(DEBUG_INFO, "USB_SWITCH_RETURN_MSG_DATA_LEN != pstMsgRcv->len\n");
		return;
	}

	/* 获取数据 */
	lUsbSwitchType = *((unsigned char *)(pstMsgRcv + 1));
	lUsbSwitchResult = *((unsigned char *)(pstMsgRcv + 1) + 1);
	lUsbPortState =  *((unsigned char *)(pstMsgRcv + 1) + 2);



	if (USB_SWITCH_RETURN_SUCCESS == lUsbSwitchResult)
	{
		if (CHECK_USB_SENDER_USB_LINK_TO_CAM == lUsbSwitchType)
		{
			if (PORT_STATE_CONNECT == lUsbPortState)
			{
				DEBUGPRINT(DEBUG_INFO, "switch result: to cam switch usb\n");
	            /* 切换到mfi */
				usb_switch_handler(USB_SWITCH_TO_MFI);
				//system("echo usb port connect, switch to mfi >> /etc/log.txt");
			}
			else if(PORT_STATE_NOT_CONNECT == lUsbPortState)
			{
				DEBUGPRINT(DEBUG_INFO, "switch result: to cam , start gather \n");
				/* 清空标志位 */
				reset_flag();
	            /* 通知可以使用音视频    增加timer */
				reset_timer_param_by_id(TIMER_ID_START_GATHER, 20, NULL);
				start_timer_by_id(TIMER_ID_START_GATHER);
			}
			else
			{
				DEBUGPRINT(DEBUG_INFO, "invalid unUsbPortState cam\n");
		        return;
			}

		}
		else if(CHECK_USB_SENDER_USB_LINK_TO_MFI == lUsbSwitchType)
		{
			if (PORT_STATE_CONNECT == lUsbPortState)
			{
				DEBUGPRINT(DEBUG_INFO, "switch result: to mfi  to apple\n");
	            /* 向mfi发送消息 通知打开苹果设备 */
				inform_mfi_usb_device_in();

				//system("echo send msg to mfi thread usb in >> /etc/log.txt");

				/* 通知usb monitor start */
				usb_port_monitor_ctl(USB_PORT_START_MONITOR);
			}
			else if(PORT_STATE_NOT_CONNECT == lUsbPortState)
			{
	            /* 切换回cam */
				DEBUGPRINT(DEBUG_INFO, "switch result: to mfi  back to cam\n");
				usb_switch_handler(USB_SWITCH_TO_CAM);
				//system("echo send msg to io thread, back to cam >> /etc/log.txt");
			}
			else
			{
				DEBUGPRINT(DEBUG_INFO, "invalid unUsbPortState mfi \n");
		        return;
			}
		}
		else
		{
			DEBUGPRINT(DEBUG_INFO, "invalid unSendType\n");
	        return;
		}

	}
	else
	{
		printf("switch failure\n");
	}
}


/**
  *  @brief      cp认证 设备ID
  *  @author     <wfb> 
  *  @param[in]  <pstMsgContainer: 接受消息>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void ioctl_i2c_get_cp_device_id_msghandler(msg_container *pstMsgContainer)
{
	int lThreadMsgId = -1;
	MsgData *pstMsgSnd = (MsgData *)g_acIoAdapter;
	MsgData *pstMsgRcv = (MsgData *)pstMsgContainer->msgdata_rcv;

	memcpy((char *)(pstMsgSnd + 1), (char *)(pstMsgRcv + 1), 4);
	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->type = MFI_MSG_TYPE;
	pstMsgSnd->cmd = 3;
	pstMsgSnd->len = 4;
	DEBUGPRINT(DEBUG_INFO, "send cp device id\n");
	DEBUGPRINT(DEBUG_INFO, "data[0] = %d\n", *(char *)(pstMsgSnd + 1));
	DEBUGPRINT(DEBUG_INFO, "data[1] = %d\n", *((char *)(pstMsgSnd + 1) + 1));
	DEBUGPRINT(DEBUG_INFO, "data[2] = %d\n", *((char *)(pstMsgSnd + 1) + 2));
	DEBUGPRINT(DEBUG_INFO, "data[3] = %d\n", *((char *)(pstMsgSnd + 1) + 3));
	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
}


/**
  *  @brief      处理获得usb端口状态后的消息
  *  @author     <wfb> 
  *  @param[in]  <pstMsgContainer: 接受消息>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void ioctl_check_usb_result_msghandler(msg_container *pstMsgContainer)
{
	int lThreadMsgId = -1;
	MsgData *pstMsgSnd = NULL;
	MsgData *pstMsgRcv = NULL;
	unsigned int unSendType = 0;
	unsigned int unUsbPortState = PORT_STATE_INVALID;

	if (NULL == pstMsgContainer)
	{
		printf("ioctl_check_usb_result_msghandler, invalid pointer\n");
        return;
	}

	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd = (MsgData *)g_acIoAdapter;
	pstMsgRcv = (MsgData *)pstMsgContainer->msgdata_rcv;

	/* 获取状态      */
	unUsbPortState = *(unsigned char *)(pstMsgRcv+1);
	/* 获取sender */
	unSendType = *((unsigned char *)(pstMsgRcv+1)+1);

	if (CHECK_USB_SENDER_USB_LINK_TO_CAM == unSendType)
	{
		if (PORT_STATE_CONNECT == unUsbPortState)
		{
            /* 切换到mfi */
			printf("switch to mfi\n");
			usb_switch_handler(USB_SWITCH_TO_MFI);
		}
		else if(PORT_STATE_NOT_CONNECT == unUsbPortState)
		{
			printf("set timer inform start gather av\n");
			/* 通知可以使用音视频  add timer  */
			reset_timer_param_by_id(TIMER_ID_START_GATHER, 10, NULL);
			start_timer_by_id(TIMER_ID_START_GATHER);
            /* 通知usb monitor start */
			//usb_port_monitor_ctl(USB_PORT_START_MONITOR);
		}
		else
		{
	        printf("invalid unUsbPortState cam\n");
	        return;
		}

	}
	else
	{
        printf("invalid unSendType\n");
        return;
	}

}

/**
  *  @brief      处理获得usb端口状态后的消息处理函数
  *  @author     <wfb> 
  *  @param[in]  <pstMsgContainer: 接受消息>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void ioctl_i2c_get_authentication_msghandler(msg_container *pstMsgContainer)
{
	int lThreadMsgId = -1;
	MsgData *pstMsgSnd = (MsgData *)g_acIoAdapter;
	MsgData *pstMsgRcv = (MsgData *)pstMsgContainer->msgdata_rcv;

	memcpy((char *)(pstMsgSnd + 1), (char *)(pstMsgRcv + 1), pstMsgRcv->len);

	get_process_msg_queue_id(&lThreadMsgId,  NULL);

	pstMsgSnd->type = MFI_MSG_TYPE;
	pstMsgSnd->cmd = 4;
	pstMsgSnd->len = 4;
	printf("send get cp authentication\n");

	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
}


/**
  *  @brief      通知设置wifi
  *  @author     <wfb> 
  *  @param[in]  <pstMsgContainer: 接受消息>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void ioctl_inform_set_wifi_msghandler(msg_container *pstMsgContainer)
{
	MsgData *pstMsgSnd = (MsgData *)pstMsgContainer->msgdata_snd;
	netset_info_t *pstNetSetInfo = (netset_info_t *)(pstMsgSnd+1); /* 只把这片buf暂当数据区用 */

	set_wifi_by_config(pstNetSetInfo);
}

/**
  *  @brief      qbug切换usb消息
  *  @author     <wfb> 
  *  @param[in]  <pstMsgContainer: 接受消息>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void ioctl_qbuf_switch_usb_msghandler(msg_container *pstMsgContainer)
{
	int lThreadMsgId = -1;
	char acTmpBuf[32] = {0};
	MsgData *pstMsgSnd = (MsgData *)acTmpBuf;

	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->cmd = CMD_START_CAPTURE;
	pstMsgSnd->len = 0;

	/* 音频 */
	pstMsgSnd->type = AUDIO_MSG_TYPE;
	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
}



/**
  *  @brief      usb切换处理函数
  *  @author     <wfb> 
  *  @param[in]  <lSwitchTo: 切换方向>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void usb_switch_handler(int lSwitchTo)
{
	int lThreadMsgId = -1;
	MsgData *pstMsgSnd = (MsgData *)g_acIoAdapter;

	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->type = IO_CTL_MSG_TYPE;
	pstMsgSnd->cmd = MSG_IOCTL_T_USB_SWITCH;
	pstMsgSnd->len = USB_SWITCH_MSG_DATA_LEN;
	*(char *)(pstMsgSnd + 1) = lSwitchTo;
	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
}

/**
  *  @brief      通知mfi usb拔出
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void inform_mfi_usb_device_out()
{
	int lThreadMsgId = -1;
	MsgData *pstMsgSnd = (MsgData *)g_acIoAdapter;

	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->type = MFI_MSG_TYPE;
	pstMsgSnd->cmd = 2;
	pstMsgSnd->len = 0;

	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
}


/**
  *  @brief      通知mfi usb插入
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void inform_mfi_usb_device_in()
{
	int lThreadMsgId = -1;
	MsgData *pstMsgSnd = (MsgData *)g_acIoAdapter;

	get_process_msg_queue_id(&lThreadMsgId,  NULL);

	pstMsgSnd->type = MFI_MSG_TYPE;
	pstMsgSnd->cmd = 1;
	pstMsgSnd->len = 0;

	/* 发消息到mfi */
	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
}

/**
  *  @brief      向所有线程发送停止使用侦
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void broadcast_all_thread_stop_use_frame(void)
{
	int i = 0;
	int lThreadMsgId = -1;
	MsgData *pstMsgSnd = (MsgData *)g_acIoAdapter;

	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->cmd = CMD_STOP_ALL_USE_FRAME;
	pstMsgSnd->len = 0;


    for(i = CHILD_PROCESS_MSG_TYPE+1; i < LAST_TYPE; i++)
    {
    	if (TRUE == if_not_use_thread(i))
    	{
    		continue;
    	}
    	pstMsgSnd->type = i;
    	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
    }
}

/**
  *  @brief      usb端口检测控制
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void usb_port_monitor_ctl(unsigned char ucCtl)
{
	int lThreadMsgId = -1;
	MsgData *pstMsgSnd = (MsgData *)g_acIoAdapter;

	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->type = IO_CTL_MSG_TYPE;
	pstMsgSnd->cmd = MSG_IOCTL_T_CTL_USB_MONITOR;
	pstMsgSnd->len = 1;

	*(unsigned char *)(pstMsgSnd+1) = ucCtl;

    msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
}

/**
  *  @brief      设置网络
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void set_wired(netset_info_t *pstNetSetInfo)
{
	char acTmpBuf[32] = {0};
	int lTmpLen = 0;

	printf("process rcv net device action  \n");
	/* 发送设置 有线网络  */
	pstNetSetInfo->lNetType = NET_TYPE_WIRED;

	/* 获取ip类型 */
	if (EXIT_FAILURE == get_config_item_value(CONFIG_WIRED_MODE, acTmpBuf, &lTmpLen))
	{
	     printf(" get_config_item_value CONFIG_Wired_Mode failed\n");
	}

	if (0 == strncmp(acTmpBuf, "S", 1))
	{
	    pstNetSetInfo->lIpType = IP_TYPE_STATIC;
	}
	else
	{
		pstNetSetInfo->lIpType = IP_TYPE_DYNAMIC;
	}

	if (IP_TYPE_STATIC == pstNetSetInfo->lIpType)
	{
		get_config_item_value(CONFIG_WIRED_IP,      pstNetSetInfo->acIpAddr,     &lTmpLen);
		get_config_item_value(CONFIG_WIRED_SUBNET,  pstNetSetInfo->acSubNetMask, &lTmpLen);
		get_config_item_value(CONFIG_WIRED_GATEWAY, pstNetSetInfo->acGateWay,    &lTmpLen);
		get_config_item_value(CONFIG_WIRED_DNS,     pstNetSetInfo->acWiredDNS,   &lTmpLen);
		get_config_item_value(CONFIG_WIFI_DNS,      pstNetSetInfo->acWifiDNS,    &lTmpLen);
	}

	printf("ready to set wired net\n");
	/* 设置网络 */
	send_message_to_set_net(pstNetSetInfo);

}

/**
  *  @brief      清空网络信息
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void sendmsg_clean_net_info()
{
	int lThreadMsgId = -1;
	MsgData *pstMsgSnd = (MsgData *)g_acIoAdapter;

	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->type = NETWORK_SET_MSG_TYPE;
	pstMsgSnd->cmd = MSG_NETWORKSET_T_CLEAN_RESOURCE;

    msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long));
}

/**
  *  @brief      停止speaker
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void stop_speaker()
{
	int lThreadMsgId = -1;
	MsgData *pstMsgSnd = (MsgData *)g_acIoAdapter;

	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->type = PLAY_BACK_MSG_TYPE;
	pstMsgSnd->cmd = MSG_SPEAKER_P_REBOOT;

	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long));
}
