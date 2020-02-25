/**   @file []
   *  @brief    网路子进程实现
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
#include "common/utils.h"
#include "network_adapter.h"
#include "common/common.h"
#include "common/msg_queue.h"
#include "common/time_func.h"
#include "common/common_func.h"
#include "common/common_config.h"
#include "HTTPServer/adapter_httpserver.h"
#include "p2p/tutk_rdt_video_server.h"
#include "networkset.h"

/******************************************************************************/
/*                                 宏定义                                                                           */
/******************************************************************************/
/* 同mfi 通信使用 CMD: 5 */
enum
{
	TO_MFI_SETTING_WIFI = 1,         ///< 设wifi
	TO_MFI_SETTING_IP,               ///<获取ip
	TO_MFI_SET_WIFI_FAILURE,         ///<设置wifi失败
	TO_MFI_COMMUNICATE_WITH_CLOUDY,  ///<同云交互
};

/** 同mfi 通信使用 CMD: 6 */
enum
{
	TO_MFI_CAN_NOT_CONNETC_INTERNET = 1,
	TO_MFI_GET_ENCRYTION_FAILED = 3,
	TO_MFI_READY_TO_CHECK_NET = 4,
};

/******************************************************************************/
/*                               内部函数声明                                        */
/******************************************************************************/
void network_report_result_msghandler(msg_container *pstMsgContainer);
void network_set_ip_type_msghandler(msg_container *pstMsgContainer);
void network_set_net_type_msghandler(msg_container *pstMsgContainer);
void send_net_state_msg_to_mfi(int nCmd, unsigned char unNetState);
void network_start_set_net_msghandler();
void network_wifi_login_state_msghandler();
void network_wifi_disconnect_msghandler();
void net_ok_sendmsg_to_tutk();


/** 子进程接受来自io线程的消息及处理函数表 */
cmd_item network_cmd_table[] =
{
	{MSG_NETWORKSET_P_SET_NET_TYPE,               network_set_net_type_msghandler},
	{MSG_NETWORKSET_P_SET_IP_TYPE,                network_set_ip_type_msghandler},
	{MSG_NETWORKSET_P_REPORT_CONNECT_RESULT,      network_report_result_msghandler},
	{MSG_NETWORKSET_P_START_SET_NET,              network_start_set_net_msghandler},
	{MSG_NETWORKSET_P_WIFI_LOGIN_STATUS,          network_wifi_login_state_msghandler},
	{MSG_NETWORKSET_P_WIFI_DISCONNECT,            network_wifi_disconnect_msghandler},
};


/** net_adapter通用的发送 buf */
char g_acNetAdapter[MAX_MSG_LEN] = {0};

/******************************************************************************/
/*                                 接口函数                                         */
/******************************************************************************/
/**
  *  @brief      网络子进程消息处理函数
  *  @author     <wfb> 
  *  @param[in]  <pstMsgContainer: 接收子进程消息>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void net_cmd_table_fun(msg_container *pstMsgContainer)
{
	MsgData *pstMsgRcv = (MsgData *)pstMsgContainer->msgdata_rcv;
	cmd_item icmd;
	icmd.cmd = pstMsgRcv->cmd;

	msg_fun fun = utils_cmd_bsearch(&icmd, network_cmd_table, sizeof(network_cmd_table)/sizeof(cmd_item));

	if(NULL != fun)
	{
		fun(pstMsgContainer);
	}
}

/**
  *  @brief      子进程接收设置网络类型处理函数
  *  @author     <wfb> 
  *  @param[in]  <pstMsgContainer: 接收子进程消息>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void network_set_net_type_msghandler(msg_container *pstMsgContainer)
{
	int nNetStage  = -1;
    int nNetType   = -1;
    int nNetResult = -1;
    netset_state_t stNetSetState;

    if (NULL == pstMsgContainer)
    {
        printf("invalid pointer \n");
        return;
    }

	MsgData *pstMsgRcv = (MsgData *)pstMsgContainer->msgdata_rcv;

	/* 获取数据 */
	nNetStage  = (int)(*((char *)(pstMsgRcv + 1)));
	nNetType   = (int)(*((char *)(pstMsgRcv + 1) + 1));
	nNetResult = (int)(*((char *)(pstMsgRcv + 1) + 2));

	if (STEP_SET_START == nNetStage)
	{
		/* 若是无线网 发送正在设置wifi */
		if (NET_TYPE_WIRELESS == nNetType)
		{
			send_net_state_msg_to_mfi(5, TO_MFI_SETTING_WIFI);
		}
		get_cur_net_state(&stNetSetState);
		stNetSetState.lNetType = nNetType;
		stNetSetState.lNetConnectState = NET_CONNECT_FAILURE;
		set_cur_net_state(&stNetSetState);
	}
	else if (STEP_SET_END == nNetStage)
	{
        /* 若成功则设置当前的网络类型 */
		get_cur_net_state(&stNetSetState);
		stNetSetState.lNetType = nNetType;
		stNetSetState.lNetConnectState = NET_CONNECT_FAILURE;
		set_cur_net_state(&stNetSetState);
	}
	else
	{
        printf("invalid nNetStage = %d\n", nNetStage);
	}
}

/**
  *  @brief      设置ip类型
  *  @author     <wfb> 
  *  @param[in]  <pstMsgContainer: 接收子进程消息>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void network_set_ip_type_msghandler(msg_container *pstMsgContainer)
{
	int nNetStage  = -1;
    int nIpType   = -1;
    int nNetResult = -1;
	int nBufLen = 0;
	char acTmpBuf[4] = {0};
    netset_state_t stNetSetState;

    if (NULL == pstMsgContainer)
    {
        printf("invalid pointer \n");
        return;
    }

	MsgData *pstMsgRcv = (MsgData *)pstMsgContainer->msgdata_rcv;

	/* 获取数据 */
	nNetStage  = (int)(*((char *)(pstMsgRcv + 1)));
	nIpType    = (int)(*((char *)(pstMsgRcv + 1) +1));
	nNetResult = (int)(*((char *)(pstMsgRcv + 1) +2));

	if (STEP_SET_START == nNetStage)
	{
		net_no_ip_set_led();

		/* 通知mfi正在设置ip */
		send_net_state_msg_to_mfi(5, TO_MFI_SETTING_IP);
	}
	else if (STEP_SET_END == nNetStage)
	{
		if (SET_IP_FAILURE == nNetResult)
		{
			/* 若是静态ip */
            ;
		}
		else
		{
	        /* 若成功则设置当前的网络类型 */
			get_cur_net_state(&stNetSetState);
			stNetSetState.lIpType = nIpType;
			set_cur_net_state(&stNetSetState);

			/* 网络灯由网络线程控制，tutk登录成功后黄灯快闪  2013-05-27   确认人： 郭智欣 王丰博 */
			/* 向TUTK 发送开始 连接 TODO */
			DEBUGPRINT(DEBUG_INFO, "net_get_ip_ok_set_led OK ..... \n");
			net_get_ip_ok_set_led();
		    /* 向http发送网络设置已经ok */
			{
				int lThreadMsgId = -1;
				char acTmpBuf[20] = {0};
				MsgData *pstMsgSnd = (MsgData *)acTmpBuf;

				get_process_msg_queue_id(&lThreadMsgId,  NULL);
				pstMsgSnd->type = HTTP_SERVER_MSG_TYPE;
				pstMsgSnd->cmd = 13;
				pstMsgSnd->len = 0;

				 if(msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(struct MsgData)-sizeof(long int)) == -1)
				 {
					 DEBUGPRINT(DEBUG_INFO, "send msg 2 httpserver fail\n");
				 }
				 else
				 {
					 DEBUGPRINT(DEBUG_INFO, "send msg 2 httpserver SUCCESS\n");
				 }

				 HttpsSeverVerifycam(pstMsgContainer);
			}

			/* 网络成功 通知 tutk */
			net_ok_sendmsg_to_tutk();
#if 0
			{
				char acBuf[64] = "already send net ok msg to tutk and https\n";
				write_log_info_to_file(acBuf);
			}
#endif

			/* mfi设置正在检测网络通知*/
			send_net_state_msg_to_mfi(6, TO_MFI_READY_TO_CHECK_NET);
		}

	}
	else
	{
        printf("invalid nNetStage = %d\n", nNetStage);
	}
}

/**
  *  @brief      网络连接结果消息处理
  *  @author     <wfb> 
  *  @param[in]  <pstMsgContainer: 接收子进程消息>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void network_report_result_msghandler(msg_container *pstMsgContainer)
{
	int nNetType = 0;
	int nIpType= 0;
	int nNetResult = 0;
	int nBufLen = 0;
	char acTmpBuf[4] = {0};
    netset_state_t stNetSetState;

    if (NULL == pstMsgContainer)
    {
        printf("invalid pointer \n");
        return;
    }

	MsgData *pstMsgRcv = (MsgData *)pstMsgContainer->msgdata_rcv;
	MsgData *pstMsgSnd = (MsgData *)pstMsgContainer->msgdata_snd;
	netset_info_t *pstNetSetInfo = (netset_info_t *)(pstMsgSnd+1);

	/* 获取数据 */
	nNetType  = (int)(*((char *)(pstMsgRcv + 1)));
	nIpType    = (int)(*((char *)(pstMsgRcv + 1) +1));
	nNetResult = (int)(*((char *)(pstMsgRcv + 1) +2));

	if (NET_CONNECT_SUCCESS == nNetResult)
	{
		/* led指示灯的控制放在tutk那里来控制  2013-05-26           确认人： 郭智欣 王丰博 */
		/* 网络灯由网络线程控制，tutk登录成功后黄灯快闪  2013-05-27   确认人： 郭智欣 王丰博 */

		DEBUGPRINT(DEBUG_INFO, "send message to set led tip\n");

		stNetSetState.lIpType = nIpType;
		stNetSetState.lNetType = nNetType;
		stNetSetState.lNetConnectState = nNetResult;
		set_cur_net_state(&stNetSetState);
	}
	else if (NET_CONNECT_FAILURE == nNetResult)
	{
		/* 若无线网 设置不成功 那么则回复mfi */
		if (NET_TYPE_WIRELESS == nNetType)
		{
			printf("send message to mfi, net set failed\n");
			send_net_state_msg_to_mfi(6, TO_MFI_CAN_NOT_CONNETC_INTERNET);
		}
		else
		{
			/* 当有限网不可用时，不去设置 2013-06-24 但没和也老商量 同gzx */
			/* 有线网失败 则尝试设置无线网 */
			//set_wifi_by_config(pstNetSetInfo);
		}
	}
	else if (NET_CONNECT_DISCONNECT == nNetResult)
	{
		net_no_ip_set_led(); 
	}
}

/**
  *  @brief      向mfi发送设置网络情况
  *  @author     <wfb> 
  *  @param[in]  <pstMsgContainer: 接收子进程消息>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void send_net_state_msg_to_mfi(int nCmd, unsigned char unNetState)
{
	int lThreadMsgId = -1;
	char acTmpBuf[20] = {0};
	MsgData *pstMsgSnd = (MsgData *)acTmpBuf;

	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->type = MFI_MSG_TYPE;
	pstMsgSnd->cmd = nCmd;
	pstMsgSnd->len = 1;

	*(unsigned char *)(pstMsgSnd+1) = unNetState;

    msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);

    DEBUGPRINT(DEBUG_INFO, "NET TO MFI MSG NET TO MFI MSG NET TO MFI MSG NET TO MFI MSG\n");
    DEBUGPRINT(DEBUG_INFO, "           CMD = %d,         STATE = %d\n", nCmd, unNetState);
    DEBUGPRINT(DEBUG_INFO, "NET TO MFI MSG NET TO MFI MSG NET TO MFI MSG NET TO MFI MSG\n");
}

/**
  *  @brief      开始设置网络
  *  @author     <wfb> 
  *  @param[in]  <pstMsgContainer: 接收子进程消息>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void network_start_set_net_msghandler()
{
	int nBufLen = 0;
	char acTmpBuf[4] = {0};

	net_no_ip_set_led();
}

/**
  *  @brief      wifi 登录失败
  *  @author     <wfb> 
  *  @param[in]  <pstMsgContainer: 接收子进程消息>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void network_wifi_login_state_msghandler(msg_container *pstMsgContainer)
{
	//发送到mfi
	int nLoginState = 0;

	MsgData *pstMsgRcv = (MsgData *)pstMsgContainer->msgdata_rcv;


	/* 获取数据 */
	nLoginState  = (int)(*((char *)(pstMsgRcv + 1)));

	/* 登录失败*/
	if (WIFI_LOGIN_FAILURE == nLoginState)
	{
		send_net_state_msg_to_mfi(5, TO_MFI_SET_WIFI_FAILURE);
	}
	else if (WIFI_LOGIN_SUCCESS == nLoginState)
	{
        ;
	}

}


/**
  *  @brief      wifi断连处理
  *  @author     <wfb> 
  *  @param[in]  <pstMsgContainer: 接收子进程消息>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void network_wifi_disconnect_msghandler(msg_container *pstMsgContainer)
{
	//发送到mfi
	int nDisConnectStep = 0;

	MsgData *pstMsgRcv = (MsgData *)pstMsgContainer->msgdata_rcv;
	nDisConnectStep  = (int)(*((char *)(pstMsgRcv + 1)));

	if (WIFI_DISCONNECT_STOP_SEND_DATA == nDisConnectStep)
	{
		net_no_ip_set_led();

		tutk_and_tcp_stop_send_data();

		set_wifi_by_config((netset_info_t *)g_acNetAdapter);
	}
	else if (WIFI_DISCONNECT_RECONNECT == nDisConnectStep)
	{
		set_wifi_by_config((netset_info_t *)g_acNetAdapter);
	}
	else
	{
        ;
	}
}

/**
  *  @brief      网络成功 发送消息到tutk
  *  @author     <wfb> 
  *  @param[in]  <pstMsgContainer: 接收子进程消息>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void net_ok_sendmsg_to_tutk()
{
	int lThreadMsgId = -1;
	char acTmpBuf[20] = {0};
	MsgData *pstMsgSnd = (MsgData *)acTmpBuf;

	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->type = P2P_MSG_TYPE;
	pstMsgSnd->cmd = MSG_P2P_T_NETCHANGE;
	pstMsgSnd->len = 0;

    msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);

    DEBUGPRINT(DEBUG_INFO, "NET OK, SEND MSG TO TUTK OK\n");
}



