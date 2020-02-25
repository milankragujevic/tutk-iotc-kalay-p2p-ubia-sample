/**   @file []
   *  @brief    网路线程实现
   *  @note
   *  @author   wangfengbo
   *  @date     2013-9-2
   *  @remarks  增加注释
*/
/*******************************************************************************/
/*                                    头文件                                   */
/*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <semaphore.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <regex.h>
#include <stdint.h>

#include "common/thread.h"
#include "common/common.h"
#include "common/msg_queue.h"
#include "common/common_func.h"
#include "common/common_config.h"
#include "common/utils.h"
#include "common/logfile.h"
#include "networkset.h"
#include "network_check.h"

#include "kernel_code/iwconfig.h"
#include "kernel_code/iwlib.h"
#include "kernel_code/iwpriv.h"


/*******************************************************************************/
/*                              宏定义 结构体区                                       */
/*******************************************************************************/
/** 定义网络线程通信的长度 */
#define NETWORKSET_MSG_LEN                   1024

/** 网络设置线程sleep 时间 */
#define NETWORKSET_THREAD_USLEEP             200000

/** 网络监控线程sleep 时间 */
#define NETMONITOR_THREAD_USLEEP             30000

/** 动态ip网络检测等待时间  */
#define DHCP_SET_TIME_INTERVAL               15000000

/** DHCP 最大检测次数 */
#define DHCP_MAX_RETRY_TIME                  3

/** DHCP不成功的最大次数，重设网络   */
#define DHCP_RESET_DEFAULT_TIMES             3

/** 定义检测网路时间  */
#define NET_CHECK_INTERVAL                   1000000

//#define DEFAULT_INVALID_IP                   "10.10.10.10"

/** 网络连接成功后定期检测时间  */
#define NET_CHECK_IF_CONNECT_INTERVAL        6000000//300000000  //5secs
#define NET_CHECK_DISCONNECT_INTERVAL        5000000


/** 定义最大的连接时间 */
#define MAX_WIFI_LOGIN_TIMES                 40 //每次是 NETWORKSET_THREAD_USLEEP

/** Wifi信息列表更新时间 */
#define WIFI_LIST_UPDATA_TIME                10000000

/** 支持最大64个wifi */
#define MAX_WIFI_LIST_NUM                    64

/** dhcp popen pclose之间的时间 若超过45则认为是异常  正常瞬间就完成了 */
#define DHCP_DEFAULT_TIMEOUT                 45

/** 检测阻塞最大值 */
#define CHECK_BLOCK_MAX_TIME                 60000000

/** 支持最大64个wifi */
typedef struct _tag_networkset_
{
	int lThreadMsgId;
	int lProcessMsgId;
	char acMsgDataRcv[NETWORKSET_MSG_LEN];
	char acMsgDataSnd[NETWORKSET_MSG_LEN];
}networkset_t;


/** 网络控制 信息 */
typedef struct _tag_networkset_info_
{
	unsigned int unNetType;                         ///<网络类型
	unsigned int unIpType;                          ///<IP类型
	unsigned int unLoopCnts;                        ///<循环次数
	unsigned int unWifiLoopCnts;                    ///<检测wifi是否连接循环次数
	unsigned int unNetState;                        ///<网络的状态
	unsigned int unCheckDhcpTimes;                  ///<dhcp的检测次数 若大于三 则  退出
	unsigned int unDhcpFailedTimes;                 ///<dhcp失败次数 若大于三 则  退出
	unsigned int unCheckNetTimes;                   ///<网络检测次数
	unsigned int unHaveIpStatus;                    ///<检测是否有ip
	unsigned int unWifiEncryTimes;                  ///<WIFI次数
	unsigned int unWifiLoginStatus;                 ///<WIFI登录的状态
	unsigned int unWifiListLoopCnts;                ///<wifi列表时间
	unsigned int unWifiDisConnectCnts;              ///<wifi断连次数
	unsigned int unNetChecktLoopCnts;               ///<网络连接检测循环次数
	unsigned int unNetDisConnectCnts;               ///<网络断连次数
	unsigned int unIfPingFlag;                      ///<是否ping
	char acInitIp[16];
	int          nFileNo;
	FILE         *pFile;
	FILE         *pTestFile;
	FILE         *pFileIp;
	FILE         *pTestFileIp;
}networkset_into_t;


/** wifi相关信息 */
typedef struct
{
    char acWifiESSID[128];
    char acWifiFreq[64];
    char acWifiQuqlity[64];
    char acWifiSignalLevel[32];
    char acWifiNoiseLevel[32];
    int  nEncryptType;
    int  nEncryptMode;
}wifi_info_t;

/** wifi 列表相关信息 */
typedef struct
{
    wifi_info_t *pstWifiListInfo;
    int nCurNo;
}wifi_list_t;

/** WIFI 状态检测返回值 */
enum
{
	WIFI_STATUS_RTN_INVALID,
	WIFI_STATUS_RTN_CONNECT,
	WIFI_STATUS_RTN_NOT_CONNECT,
};


/*******************************************************************************/
/*                                全局变量区                                         */
/*******************************************************************************/
/** 网络设置信息  */
networkset_into_t g_stNetWorkSetInfo;

/** 当网络成功设置后 这里边将会存储成功设置后的信息 */
netset_info_t  g_stLocalNetSetInfo;

/* 定义互斥锁 */
//pthread_mutex_t networkset_lock = PTHREAD_MUTEX_INITIALIZER;

/** 全局变量区 */
wifi_list_t g_stWifiList;

/** 记录当前的ip地址 */
//char g_acGateWay[16] = {0};

/** ip是否获得成功 */
int g_nGetIpFlag = FALSE;

/** dhcp 间隔次数 */
int g_nIntervalDhcpLoop = 0;

int g_nNetBlockTimes = 0;

/*******************************************************************************/
/*                               接口函数声明                                        */
/*******************************************************************************/
/* 启动networkset线程 */
void *networkset_thread(void *arg);
/* io控制模块 */
int networkset_module_init(networkset_t *pstNetWorkSet);
/* 消息处理函数 */
void networkset_msgHandler(networkset_t *pstNetWorkSet);
/* dhcp 设置 */
//int set_dhcp(int lNetType);
/* 检测dhcp 设置 */
void networkset_check_dhcp_set_result_handler();
/* 检测网络连接 */
void networkset_check_net_connect_state_handler();
/* 网络设置处理函数 */
void net_workset_msghandler(MsgData *pstRcvMsg, MsgData *pstSndMsg);
/* 设置wifi */
int netset_set_net_type_wifi(int lWiFiType, char *pcSSID, char *pcCode);
/* 设置有线网 */
int netset_set_net_type_wired();
/* 设置网络类型 */
//int network_set_net_type(netset_info_t *pstNetSetInfo, MsgData *pstSndMsg);
/* 设ip */
//int network_set_ip_type(netset_info_t *pstNetSetInfo);
/* 判断是否需要重新设置网络 */
int if_need_reset_net(netset_info_t *pstNetSetInfo);
/* 清除资源 */
void clean_net_info_msghandler();
void start_set_net_type_sendmessage_to_child_process(int nNetType);
//void report_set_net_type_sendmessage_to_child_process(int nNetType, int nSetNetResult);
void start_set_ip_sendmessage_to_child_process(int nIpType);
void report_set_ip_sendmessage_to_child_process(int nIpType, int nSetIpResult);
void set_net_connect_result_sendmessage_to_child_process(int nNetType, int nIpType, int nNetResult);
void wifi_disconnect_to_child_process(unsigned char ucTmpBuf);
// 设置网络类型还ip类型 */
//int set_net_type_and_ip_type(netset_info_t *pstNetSetInfo);
/* 发送消息到子进程 网络设置开始 */
void send_msg_to_child_start_set_net();
/* 检测wifi连接是否OK */
int check_wifi_connect_ok();
/* 定期检测网络是否连通 */
void networkset_check_net_if_still_connect();
/* 根据次数获取最新的加密类型 */
int return_mixed_WifiEntryType_by_times(int nTime);
/* 设置网络类型还ip类型 */
int set_ip_type(netset_info_t *pstNetSetInfo);
/* 设置网络类型还net类型 */
int set_net_type(netset_info_t *pstNetSetInfo);
/* 检测wifi登录状态 只有成功之后才会向下走 */
void check_wifi_login_status();
/* 接受https */
void net_connect_status_msghandler(unsigned int unSubType);
/* 开始检测网络消息处理函数 */
void net_start_check_msghandler();
/* 向子进程 发送wifi登录状态 */
void send_msg_to_child_wifi_login_state(unsigned char ucTmpBuf);
/* 更新wifi列表 */
void updata_wifi_list();
/* 定期udhcpc */
void interval_udhcpc();
void check_if_net_check_block();
/******************************************************************************/
/*                                接口函数                                          */
/******************************************************************************/
/**
  *  @brief      网络设置线程创建函数
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
int NetworkSet_Thread_Creat(void)
{
    int result;

    /*! 创建线程 */
    result = thread_start(networkset_thread);

    return result;
}

/**
  *  @brief      wifi信息初始化函数
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void wifi_list_init()
{
    g_stWifiList.pstWifiListInfo = (wifi_info_t *)malloc(sizeof(wifi_info_t) * MAX_WIFI_LIST_NUM);
    printf("wifi_list_init ok ~~~~~~~~~~~~~\n");

    if (NULL == g_stWifiList.pstWifiListInfo)
    {
        printf("wifi list failed~~~~~~~~~~~~~\n");
        //assert(0);
        return;
    }

    memset(g_stWifiList.pstWifiListInfo, 0x00, sizeof(wifi_info_t) * MAX_WIFI_LIST_NUM);
    g_stWifiList.nCurNo = -1; //这个必须被设置成－1。
}

/**
  *  @brief      线程变量初始化
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void networkset_info_init(void)
{
	memset(&g_stNetWorkSetInfo, 0x00, sizeof(networkset_into_t));
	memset(&g_stLocalNetSetInfo, 0x00, sizeof(netset_info_t));

	//wifi_list_init();
}

/**
  *  @brief      网络初始化
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void network_dev_init(void)
{
	FILE *pFile = NULL;
	//char acIpBuf[20] = {0};
	/* down掉网卡 */
	pFile = popen("ifconfig ra0 down", "r");
	if (NULL != pFile)
	{
		char acTmpBuf[128] = {0};

		fread(acTmpBuf, 1, 128, pFile);
		pclose(pFile);
		pFile = NULL;
	}
}


/**
  *  @brief      线程初始化函数
  *  @author     <wfb> 
  *  @param[in]  <pstNetWorkSet: 网络线程消息结构体>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
int networkset_module_init(networkset_t *pstNetWorkSet)
{
	/* 参数检查 */
	if (NULL == pstNetWorkSet)
	{
		exit(0);
	}

	/* 获取消息队列 id */
	if (EXIT_SUCCESS != get_process_msg_queue_id(&(pstNetWorkSet->lThreadMsgId),  &(pstNetWorkSet->lProcessMsgId)))
	{
		exit(0);
	}

	/* 增加测试 */
#if 0
	{
		char acBuf[20] = {0};
 	    printf("we will ifconfig eth2 192.168.2.2\n");
	    system("ifconfig eth2 192.168.2.2");
	    get_ip_addr(acBuf);
	    printf("get_ip_addr = %s\n", acBuf);
	}
#endif

	/* 参数init */
	networkset_info_init();

	/* 设备初始化 */
	//network_dev_init();

}


/**
  *  @brief      线程初始化函数
  *  @author     <wfb> 
  *  @param[in]  <pstNetWorkSet: 网络线程消息结构体>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void *networkset_thread(void *arg)
{
	  networkset_t *pstNetWorkSet = (networkset_t *)malloc(sizeof(networkset_t));

	  //init
	  if(-1 == networkset_module_init(pstNetWorkSet))
	  {
	      return NULL;
	  }

	  //check thread creat
	  NetworkCheck_Thread_Creat();

	  printf("network init ok, creat check thread ok\n");
	  //sync
	  thread_synchronization(&rNetWorkSet, &rChildProcess);


	  //DEBUGPRINT(DEBUG_INFO, "<>--[%s]--<>\n", __FUNCTION__);

	  while(1)
	  {
		    //DEBUGPRINT(DEBUG_INFO, "<>--[%s ]--<>\n", __FUNCTION__);
		    //printf("<>--[%s]--<>\n", __FUNCTION__);

		    /* 消息循环处理函数 */
		    networkset_msgHandler(pstNetWorkSet);

		    /* 设置网络 */
		    set_net_type(&g_stLocalNetSetInfo);

		    /* 检测  wifi是否成功 */
		    check_wifi_login_status();

		    /* 设置ip */
		    set_ip_type(&g_stLocalNetSetInfo);

		    /*  检测dhcp设置后的情况 */
		    networkset_check_dhcp_set_result_handler();

		    /* 检测网络连接情况 ping */
            //networkset_check_net_connect_state_handler(pstNetWorkSet);

            /* 定期检测网络 */
            networkset_check_net_if_still_connect();

		    /* 更新wifi列表 */
		    updata_wifi_list();

		    /* 检测网络检测线程是否有阻塞 若发现 则重启 */
		    check_if_net_check_block();

		    /* 定时udhcpc */
		    //interval_udhcpc();
		    //DEBUGPRINT(DEBUG_INFO, "net is alive .........^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");

		    child_setThreadStatus(NETWORK_SET_MSG_TYPE, NULL);

		    /* 线程切换出去200ms */
		    thread_usleep(NETWORKSET_THREAD_USLEEP);
	  }

	  return 0;
}


/**
  *  @brief      网络线程消息处理函数
  *  @author     <wfb> 
  *  @param[in]  <pstNetWorkSet: 网络线程消息结构体>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
                 2013-11-13 增加开始检测函数
  *  @note
*/
void networkset_msgHandler(networkset_t *pstNetWorkSet)
{
	MsgData *pstRcvMsg = (MsgData *)pstNetWorkSet->acMsgDataRcv;
	MsgData *pstSndMsg = (MsgData *)pstNetWorkSet->acMsgDataSnd;
	unsigned int unSubType = 0;

	/* 参数检查 */
	if (NULL == pstNetWorkSet)
	{
		return;
	}

	/* 处理接收到的消息 */
	if (-1 != msg_queue_rcv(pstNetWorkSet->lThreadMsgId, pstRcvMsg, MAX_MSG_QUEUE_LEN, NETWORK_SET_MSG_TYPE))
	{
		unSubType = *((char *)((char *)pstRcvMsg + sizeof(MsgData)));

		switch(pstRcvMsg->cmd)
		{
		case MSG_NETWORKSET_T_SET_NET:
			net_workset_msghandler(pstRcvMsg, pstSndMsg);
			break;
		case MSG_NETWORKSET_T_CLEAN_RESOURCE:
			clean_net_info_msghandler();
			break;
		case MSG_NETWORKSET_T_NET_CONNECT_STATUS:
			net_connect_status_msghandler(unSubType);
			break;
		case MSG_NETWORKSER_T_NET_START_PING:
			net_start_check_msghandler();
			break;
		default:
			break;
		}
	}

	return;
}

/**
  *  @brief      检测dhcp结果
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void networkset_check_dhcp_set_result_handler()
{
	FILE *pFile = NULL;
	char acIpBuf[20] = {0};
	unsigned int unDhcpCheckInverval = 0;

	if (0 == g_stNetWorkSetInfo.unCheckDhcpTimes)
	{
		unDhcpCheckInverval = 2000000;
	}
	else if (1 == g_stNetWorkSetInfo.unCheckDhcpTimes)
	{
		unDhcpCheckInverval = 3000000;
	}
	else if (2 == g_stNetWorkSetInfo.unCheckDhcpTimes)
	{
		unDhcpCheckInverval = 7000000;
	}
	else
	{
		unDhcpCheckInverval = 15000000;
	}


    /* 判断当前是否处于dhcp 设置状态  或是dhcp 检测状态  */
    if (SET_NET_STATE_SET_DHCP == g_stNetWorkSetInfo.unNetState)
    {
    	/* 判断是否已到时间间隔 */
    	if (unDhcpCheckInverval/NETWORKSET_THREAD_USLEEP  == g_stNetWorkSetInfo.unLoopCnts)
    	{
    		/* 获取ip地址 */
    		get_ip_addr(acIpBuf, g_stNetWorkSetInfo.unNetType);
    		DEBUGPRINT(DEBUG_INFO, "Cur time is : %d, acIpBuf = %s\n", g_stNetWorkSetInfo.unCheckDhcpTimes, acIpBuf);
    		/* 检测dhcp是否成功 */
    	    if (0 != strlen(acIpBuf))  /* TODO */
    		{
    	    	DEBUGPRINT(DEBUG_INFO, " udhcpc success \n");
    	    	//strncpy(g_acWifiIpInfo, acIpBuf, strlen(acIpBuf));
    	    	//g_acWifiIpInfo[strlen(acIpBuf)] = 0;
    	    	set_cur_ip_addr(acIpBuf);
    	    	g_stNetWorkSetInfo.unHaveIpStatus =  GET_IP_SUCCESS;
    	    	g_nGetIpFlag = TRUE;
    	    	//system("killall udhcpc");

    	    	/* 获取网关 */
    	    	get_gateway(g_acGateway);  //存在全局变量里

    			g_stNetWorkSetInfo.unNetState = SET_NET_STATE_SET_IP_SUCCESS;
    			g_stNetWorkSetInfo.unCheckDhcpTimes = 0;
    			g_stNetWorkSetInfo.unWifiEncryTimes = 0;
    			g_stNetWorkSetInfo.unLoopCnts = 0;

    			/* 发送设置dhcp成功 */
    			report_set_ip_sendmessage_to_child_process(IP_TYPE_DYNAMIC, SET_IP_SUCCESS);
                return;
    		}

    	    DEBUGPRINT(DEBUG_INFO, "HELLO COME INTO TEST CODE\n");



    	    /* 若尝试DHCP_RESET_DEFAULT_TIMES次后，仍没有获得ip 那么则重新 加入网络 */
    	    if ((DHCP_RESET_DEFAULT_TIMES == g_stNetWorkSetInfo.unDhcpFailedTimes) &&
    	    		(NET_TYPE_WIRELESS == g_stNetWorkSetInfo.unNetType))
    	    {
    	    	//清空网络资源 重新设置网络
    	    	clean_net_info_msghandler();
    	    	//重新设置网络
    	    	set_wifi_by_config(NULL);
    	    }
    	    else if (DHCP_MAX_RETRY_TIME == g_stNetWorkSetInfo.unCheckDhcpTimes)
    	    {
    	    	//如果dhcp前后的时间超过了DHCP_DEFAULT_TIMEOUT,我们默认是出现了异常，这时我们应该重新设置网络
    	    	if ((g_nDhcpFinishTime > DHCP_DEFAULT_TIMEOUT) && (NET_TYPE_WIRELESS == g_stNetWorkSetInfo.unNetType))
    	    	{
    	    		DEBUGPRINT(DEBUG_INFO, "g_nDhcpFinishTime > DHCP_DEFAULT_TIMEOUT so reset net\n");
                    //清空网络资源 重新设置网络
    	    		clean_net_info_msghandler();
    	    		//重新设置网络
    	    		set_wifi_by_config(NULL);
    	    	}
    	    	else
    	    	{
#if 0
    	    		//system("killall udhcpc");
					pFile = popen("killall udhcpc", "r");
					if (NULL != pFile)
					{
						char acTmpBuf[128] = {0};

						fread(acTmpBuf, 1, 128, pFile);
						pclose(pFile);
						pFile = NULL;
					}
#endif
					g_stNetWorkSetInfo.unNetState = SET_NET_STATE_SET_IP_FAILURE;
					g_stNetWorkSetInfo.unCheckDhcpTimes = 0;
					g_stNetWorkSetInfo.unLoopCnts = 0;

					/* 重新dhcp */
					g_stNetWorkSetInfo.unNetState = SET_NET_STATE_WIFI_LOGIN_OK;

    	    	}

    	    	g_stNetWorkSetInfo.unDhcpFailedTimes++;

    	     	return;
    	    }

    	    g_stNetWorkSetInfo.unCheckDhcpTimes++;
    		g_stNetWorkSetInfo.unLoopCnts = 0;
    	}
    	else
    	{
    		g_stNetWorkSetInfo.unLoopCnts++;
    	}
    }

}


/**
  *  @brief      检测网络连接
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void networkset_check_net_connect_state_handler(networkset_t *pstNetWorkSet)
{
    int nRet = -1;
    int nLen = 0;
    int nLenIp = 0;
    char buf[256] = {0};
    char acBufIp[256] = {0};
    //char acBuf[4] = {0};

    return;

	if (SET_NET_STATE_SET_IP_SUCCESS == g_stNetWorkSetInfo.unNetState)
	{
		DEBUGPRINT(DEBUG_INFO, "ping ....\n");
		//system("rm -rf /etc/test_ping;touch /etc/test_ping;rm -rf /etc/test_ping1;touch /etc/test_ping1");

		g_stNetWorkSetInfo.pFile = popen("ping www.google.com -c 3 | grep time= > /etc/test_ping &", "r");
		g_stNetWorkSetInfo.pTestFile = fopen("/etc/test_ping", "rw+");
		g_stNetWorkSetInfo.pFileIp = popen("ping 173.194.72.103 -c 1 | grep time= > /etc/test_ping1 &", "r");
		g_stNetWorkSetInfo.pTestFileIp = fopen("/etc/test_ping1", "rw+");

		g_stNetWorkSetInfo.unNetState = SET_NET_STATE_CHECK_NET_CONNECT;
	}
	else if (SET_NET_STATE_CHECK_NET_CONNECT == g_stNetWorkSetInfo.unNetState)
	{
        if (NET_CHECK_INTERVAL/ NETWORKSET_THREAD_USLEEP == g_stNetWorkSetInfo.unLoopCnts )
        {
        	//DEBUGPRINT(DEBUG_INFO, "TEST 11111111111111111111111111111111111111111111\n");
            fseek(g_stNetWorkSetInfo.pTestFile, 0, SEEK_SET);
            //DEBUGPRINT(DEBUG_INFO, "TEST 11111111111111111111122222222222222222222222\n");
            nLen = fread(buf, 80, 1, g_stNetWorkSetInfo.pTestFile);
            //DEBUGPRINT(DEBUG_INFO, "TEST 111111111111111111111\n");

            fseek(g_stNetWorkSetInfo.pTestFileIp, 0, SEEK_SET);
            nLenIp = fread(acBufIp, 80, 1, g_stNetWorkSetInfo.pTestFileIp);

            if((0 == nLen) && (0 == nLenIp))
            {
            	DEBUGPRINT(DEBUG_INFO, "select time out!\n");
                goto next;
            }
            else
            {
                {
                	if ((NULL == strstr(buf, "time=")) && (NULL == strstr(acBufIp, "time=")))
                	{
                		DEBUGPRINT(DEBUG_INFO, "select time out!\n");
                		goto next;
                	}
                	else
                	{
                		DEBUGPRINT(DEBUG_INFO, "acBufIp = %s, buf = %s\n",acBufIp, buf);
                		DEBUGPRINT(DEBUG_INFO, "select ok ~~~~~~~\n");
                	}

                }

                //通过https来检测外网是否可达  modify by fb.w  jn.w  zx.g 2013-06-24

                //g_stNetWorkSetInfo.unNetState = SET_NET_STATE_NET_CONNECT_SUCCESS;
                //set_net_connect_result_sendmessage_to_child_process(g_stNetWorkSetInfo.unNetType,
                         			  // g_stNetWorkSetInfo.unIpType, NET_CONNECT_SUCCESS);
                g_stNetWorkSetInfo.unLoopCnts = 0;
                g_stNetWorkSetInfo.unCheckNetTimes = 0;

         	    nRet = pclose(g_stNetWorkSetInfo.pFile);
         	    nRet = pclose(g_stNetWorkSetInfo.pFileIp);
         	    g_stNetWorkSetInfo.pFile = NULL;
         	    g_stNetWorkSetInfo.pFileIp = NULL;
         	    if(-1 == nRet)
         	    {
         	    	DEBUGPRINT(DEBUG_INFO, "close file pointer fail! @:%s %d\n", __FILE__, __LINE__);
         	        return;
         	    }
         	    nRet = fclose(g_stNetWorkSetInfo.pTestFile);
         	    nRet = fclose(g_stNetWorkSetInfo.pTestFileIp);
         	    g_stNetWorkSetInfo.pTestFile = NULL;
         	    g_stNetWorkSetInfo.pTestFileIp = NULL;
         	    if(-1 == nRet)
         	    {
         	    	DEBUGPRINT(DEBUG_INFO, "close file pointer fail! @:%s %d\n", __FILE__, __LINE__);
         	        return;
         	    }

#if 0
         	    /* 若当前是 无线网络 ， 直接将wifi 信息写入 flash */
         	    if (NET_TYPE_WIRELESS == g_stLocalNetSetInfo.lNetType)
         	    {
         	    	snprintf(acBuf, sizeof(acBuf), "%d", g_stLocalNetSetInfo.lWifiType);
         	    	DEBUGPRINT(DEBUG_INFO, "ready to set wifi info \n");
         	    	set_config_value_to_flash(CONFIG_WIFI_ENRTYPE, acBuf, strlen(acBuf));
         	    	set_config_value_to_flash(CONFIG_WIFI_SSID,   g_stLocalNetSetInfo.acSSID, strlen(g_stLocalNetSetInfo.acSSID));
         	    	set_config_value_to_flash(CONFIG_WIFI_PASSWD, g_stLocalNetSetInfo.acCode, strlen(g_stLocalNetSetInfo.acCode));
         	    	set_config_value_to_flash(CONFIG_WIFI_ACTIVE, "y", 1);
         	    	DEBUGPRINT(DEBUG_INFO, "set wifi info ok\n");
         	    }
#endif
#if 0
         	    {
         	    	/* 增加测试代码  */
                    char acTmp[64] = {0};
                    int  nLen = 0;

                    get_config_item_value(CONFIG_WIFI_ENRTYPE, acTmp, &nLen);
                    DEBUGPRINT(DEBUG_INFO, "CONFIG_WIFI_ENRTYPE is: %s ,len = %d\n", acTmp, nLen);
                    memset(acTmp, 0x00, 64);
                    get_config_item_value(CONFIG_WIFI_SSID, acTmp, &nLen);
                    DEBUGPRINT(DEBUG_INFO, "CONFIG_WIFI_SSID is: %s ,len = %d\n", acTmp, nLen);
                    memset(acTmp, 0x00, 64);
                    get_config_item_value(CONFIG_WIFI_PASSWD, acTmp, &nLen);
                    DEBUGPRINT(DEBUG_INFO, "CONFIG_WIFI_PASSWD is: %s ,len = %d\n", acTmp, nLen);
         	    }
#endif
         	    DEBUGPRINT(DEBUG_INFO, "select ok end end end end ~~~~~~~\n");
         	    return;
            }

next:       g_stNetWorkSetInfo.unLoopCnts = 0;
            g_stNetWorkSetInfo.unCheckNetTimes++;

            /* 若 每20s 内不回 则 重来 两次不成功则 退出 失败 */
            //if (51 == g_stNetWorkSetInfo.unCheckNetTimes)
            if (0)
            {
         		//system("killall ping");
         		nRet = pclose(g_stNetWorkSetInfo.pFile);
         		nRet = pclose(g_stNetWorkSetInfo.pFileIp);
         		g_stNetWorkSetInfo.pFile = NULL;
         		g_stNetWorkSetInfo.pFileIp = NULL;
         	    if(-1 == nRet)
         	    {
         	    	DEBUGPRINT(DEBUG_INFO, "close file pointer fail! @:%s %d\n", __FILE__, __LINE__);
         	        return;
         	    }
         	    nRet = fclose(g_stNetWorkSetInfo.pTestFile);
         	    nRet = fclose(g_stNetWorkSetInfo.pTestFileIp);
         	    g_stNetWorkSetInfo.pTestFile = NULL;
         	    g_stNetWorkSetInfo.pTestFileIp = NULL;
         	    if(-1 == nRet)
         	    {
         	    	DEBUGPRINT(DEBUG_INFO, "close file pointer fail! @:%s %d\n", __FILE__, __LINE__);
         	        return;
         	    }
                g_stNetWorkSetInfo.unCheckNetTimes = 0;
                //g_stNetWorkSetInfo.unNetState = SET_NET_STATE_NET_CONNECT_FAILURE;
                //set_net_connect_result_sendmessage_to_child_process(g_stNetWorkSetInfo.unNetType,
                //g_stNetWorkSetInfo.unIpType, NET_CONNECT_FAILURE);
            }
            else if (0 == g_stNetWorkSetInfo.unCheckNetTimes%25 )
            {
            	g_stNetWorkSetInfo.unNetState = SET_NET_STATE_SET_IP_SUCCESS;
         	    nRet = pclose(g_stNetWorkSetInfo.pFile);
         	    nRet = pclose(g_stNetWorkSetInfo.pFileIp);
         	    g_stNetWorkSetInfo.pFile = NULL;
         	    g_stNetWorkSetInfo.pFileIp = NULL;
         		//system("killall ping");
         	    if(-1 == nRet)
         	    {
         	    	DEBUGPRINT(DEBUG_INFO, "close file pointer fail! @:%s %d\n", __FILE__, __LINE__);
         	        return;
         	    }
         	    nRet = fclose(g_stNetWorkSetInfo.pTestFile);
         	    nRet = fclose(g_stNetWorkSetInfo.pTestFileIp);
         	    g_stNetWorkSetInfo.pTestFile = NULL;
         	    g_stNetWorkSetInfo.pTestFileIp = NULL;
         	    if(-1 == nRet)
         	    {
         	    	DEBUGPRINT(DEBUG_INFO, "close file pointer fail! @:%s %d\n", __FILE__, __LINE__);
         	        return;
         	    }
            }

        }
        else
        {
        	g_stNetWorkSetInfo.unLoopCnts ++;
        }
	}
}

/**
  @brief 定期检测网络是否连通
  @author wfb
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-09-03 增加该注释
          2013-10-16 网络联通后，每分钟检测一次，当检测到断开后，再隔5s检测
          2013-10-23 活取IP后 若tutk允许ping，则ping
  @note
*/
void networkset_check_net_if_still_connect()
{
	//判断如果需要重设网络向子进程发送消息
    if ((GET_IP_SUCCESS == g_stNetWorkSetInfo.unHaveIpStatus) &&
    	(TRUE == g_nResetNetFlag)                             &&
    	(NET_TYPE_WIRELESS == g_stNetWorkSetInfo.unNetType))
	{
    	//网络检测知针对 无线网
		DEBUGPRINT(DEBUG_INFO, "TUTK AND TCP STOP SEND DATA ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
		wifi_disconnect_to_child_process(WIFI_DISCONNECT_STOP_SEND_DATA);
		clean_net_info_msghandler();
	}

	return;
#if 0
	int nRet = -1;
    int nLen = 0;
    int nWifiStatus = WIFI_STATUS_RTN_INVALID;
    char acCmdBuf[256] = {0};
    char acReadBuf[256] = {0};
    char acMsgData[MAX_MSG_QUEUE_LEN] = {0};
    int nLoops = 0;


    /* 从获取ip成功之后,且tutk允许ping开始获取 */
    if ((GET_IP_SUCCESS == g_stNetWorkSetInfo.unHaveIpStatus) &&
    		(TRUE == g_stNetWorkSetInfo.unIfPingFlag))
    {

    	//若网络正常连接，则检测时间为1分钟，若已经检测到一次断开，则检测时间为5s
    	if (0 == g_stNetWorkSetInfo.unNetDisConnectCnts)
    	{
    		nLoops = NET_CHECK_IF_CONNECT_INTERVAL/NETWORKSET_THREAD_USLEEP;
    	}
    	else
    	{
    		nLoops = NET_CHECK_DISCONNECT_INTERVAL/NETWORKSET_THREAD_USLEEP;
    	}

    	//查看定时是否已到
    	if (nLoops == g_stNetWorkSetInfo.unNetChecktLoopCnts)
    	{
    		g_stNetWorkSetInfo.unNetChecktLoopCnts = 0;
    	}
    	else
    	{
    		if (NET_CHECK_DISCONNECT_INTERVAL/NETWORKSET_THREAD_USLEEP == \
    				(nLoops - g_stNetWorkSetInfo.unNetChecktLoopCnts))
    		{
    			//打开ping
    			sprintf(acCmdBuf, "echo \"\" > /etc/test_ping1;ping %s -c 3 -w 5 | grep time= > /etc/test_ping1", g_acGateWay);
    			g_stNetWorkSetInfo.pFileIp = popen(acCmdBuf, "r");
    		}

    		g_stNetWorkSetInfo.unNetChecktLoopCnts++;
    		return;
    	}

    	//查看是否有断ping的情况,哲理要判断第一次进入 131016更改，第一次进入在函数开始时处理
        if (NULL != g_stNetWorkSetInfo.pFileIp)
        {
        	/* 关闭ping */
        	pclose(g_stNetWorkSetInfo.pFileIp);
        	g_stNetWorkSetInfo.pFileIp = NULL;

        	/* 检查ping的返回值 */
        	if (NULL != (g_stNetWorkSetInfo.pTestFileIp = fopen("/etc/test_ping1", "rw+")))
        	{
        		fseek(g_stNetWorkSetInfo.pTestFileIp, 0, SEEK_SET);
        		nLen = fread(acReadBuf, 128, 1, g_stNetWorkSetInfo.pTestFileIp);
        		/* 写入数据 将本次的数据覆盖掉 */
        		fclose(g_stNetWorkSetInfo.pTestFileIp);
        		g_stNetWorkSetInfo.pTestFileIp = NULL;

        		/* 查看网络连接的状态  */
        		if (NULL == strstr(acReadBuf, "time="))
        		{
        		    DEBUGPRINT(DEBUG_INFO, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~not rcv from ping!\n");
        		    g_stNetWorkSetInfo.unNetDisConnectCnts++;
        		}
        		else
        		{
        			DEBUGPRINT(DEBUG_INFO, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~rcv from ping  ok! %s %s\n",acReadBuf, g_acWifiIpInfo);
        			g_stNetWorkSetInfo.unNetDisConnectCnts = 0;
        			g_stNetWorkSetInfo.unIfPingFlag = FALSE;
        		}

        		/* 若连续2次则 认为网络断开 */
        		if (2 == g_stNetWorkSetInfo.unNetDisConnectCnts )
        		{
        			/* 当前是wifi */
        			if (NET_TYPE_WIRELESS == g_stNetWorkSetInfo.unNetType)
        			{
        				//若当前为无限则停止发送 TODO 有限网怎么办？
        				DEBUGPRINT(DEBUG_INFO, "TUTK AND TCP STOP SEND DATA ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
        				wifi_disconnect_to_child_process(WIFI_DISCONNECT_STOP_SEND_DATA);
        				clean_net_info_msghandler();
        			}
        		}
        	}
        }
    }
#endif //modify by wfb  use worker thread to check if the net is still connect

#if 0   //20130815 通过ping来检测网络
    /* 这里是定时监测  但目前不监测     2013－05－27  制定人：郭智欣 王丰博 */
    if (WIFI_LOGIN_SUCCESS ==  g_stNetWorkSetInfo.unWifiLoginStatus)
    {
    	/* 检测网络是否仍然连接 每隔五秒检测一次  */
        if (NET_CHECK_IF_CONNECT_INTERVAL/NETWORKSET_THREAD_USLEEP == g_stNetWorkSetInfo.unWifiLoopCnts)
        {
        	nWifiStatus = check_wifi_connect_ok();
        	//wifi连接无效
        	if (WIFI_STATUS_RTN_INVALID == nWifiStatus)
        	{
        		;
        	}
        	else if (WIFI_STATUS_RTN_NOT_CONNECT == nWifiStatus)
            {
        		g_stNetWorkSetInfo.unWifiDisConnectCnts++;

        		//如果检测到未连接的次数达到两次 那么则停止发送数据
        		if (2 == g_stNetWorkSetInfo.unWifiDisConnectCnts)
        		{
        			//向tutk和newchannel发送消息
        			wifi_disconnect_to_child_process(WIFI_DISCONNECT_STOP_SEND_DATA);
        		}
        		//若三次则重连接
        		else if (3 == g_stNetWorkSetInfo.unWifiDisConnectCnts)
        		{
        			clean_net_info_msghandler();
        			g_stNetWorkSetInfo.unWifiLoginStatus = WIFI_LOGIN_FAILURE;
        			/* 重新设置网络  */
        			//set_wifi_by_config((netset_info_t *)acMsgData);
        			DEBUGPRINT(DEBUG_INFO, "already dis conn three times  reset wifi+++++++++++++++++++++++++\n");
        			wifi_disconnect_to_child_process(WIFI_DISCONNECT_RECONNECT);
        			g_stNetWorkSetInfo.unWifiDisConnectCnts = 0;
        		}

            }
        	//wifi依然连接
        	else
        	{
        		g_stNetWorkSetInfo.unWifiDisConnectCnts = 0;
        	}

        	g_stNetWorkSetInfo.unWifiLoopCnts = 0;

        }
        else
        {
        	g_stNetWorkSetInfo.unWifiLoopCnts++;
        }
    }
#endif

#if 0
    if (SET_NET_STATE_NET_CONNECT_SUCCESS == g_stNetWorkSetInfo.unNetState)
    {
        /* 判断间隔是否到 */
    	if (NET_CHECK_IF_CONNECT_INTERVAL/NETWORKSET_THREAD_USLEEP == g_stNetWorkSetInfo.unLoopCnts)
    	{
#if 0
            if (TRUE == wifi_connect_ok())
            {
            	g_stNetWorkSetInfo.unLoopCnts = 0;
                return;
            }
            else
            {
            	/* 网络连接不成功 */
            	g_stNetWorkSetInfo.unNetState = SET_NET_STATE_NET_CONNECT_FAILURE;
            	set_net_connect_result_sendmessage_to_child_process(g_stNetWorkSetInfo.unNetType,
            	         			   g_stNetWorkSetInfo.unIpType, NET_CONNECT_DISCONNECT);
            	return;
            }
#endif
            /* 以下代码不用 */
            DEBUGPRINT(DEBUG_INFO, "check net if still connect, ping ....\n");
    		//system("rm -rf /etc/test_connect");
    		//system("touch /etc/test_connect");
    		g_stNetWorkSetInfo.pFile = popen("ping www.google.com -c 3 | grep time= > /etc/test_connect &", "r");
    		g_stNetWorkSetInfo.pTestFile = fopen("/etc/test_connect", "rw+");
    		g_stNetWorkSetInfo.unNetState = SET_NET_STATE_NET_INTERVAL_CHECK;
    		g_stNetWorkSetInfo.unCheckNetTimes = 0;
    		g_stNetWorkSetInfo.unLoopCnts = 0;
    	}
    	else
    	{
    		g_stNetWorkSetInfo.unLoopCnts++;
    	}
    }
    else if (SET_NET_STATE_NET_INTERVAL_CHECK == g_stNetWorkSetInfo.unNetState)
    {
    	 //printf("g_stNetWorkSetInfo.unLoopCnts = %d\n", g_stNetWorkSetInfo.unLoopCnts);
		 if (NET_CHECK_INTERVAL/NETWORKSET_THREAD_USLEEP == g_stNetWorkSetInfo.unLoopCnts )
		 {
		     fseek(g_stNetWorkSetInfo.pTestFile, 0, SEEK_SET);
			 nLen = fread(buf, 80, 1, g_stNetWorkSetInfo.pTestFile);

			 if(0 == nRet)
			 {
			     goto next;
			 }
			 else
			 {

				if (NULL == strstr(buf, "time="))
				{
					DEBUGPRINT(DEBUG_INFO, "select time out!\n");
					goto next;
				}
				else
				{
					DEBUGPRINT(DEBUG_INFO, "buf = %s\n", buf);
					DEBUGPRINT(DEBUG_INFO, "select ok ~~~~~~~\n");
				}


				g_stNetWorkSetInfo.unNetState = SET_NET_STATE_NET_CONNECT_SUCCESS;
				g_stNetWorkSetInfo.unCheckNetTimes = 0;
				g_stNetWorkSetInfo.unLoopCnts = 0;

				nRet = pclose(g_stNetWorkSetInfo.pFile);
				g_stNetWorkSetInfo.pFile = NULL;
				if(-1 == nRet)
				{
					DEBUGPRINT(DEBUG_INFO, "close file pointer fail! @:%s %d\n", __FILE__, __LINE__);
					return;
				}
				nRet = fclose(g_stNetWorkSetInfo.pTestFile);
				g_stNetWorkSetInfo.pTestFile = NULL;
				if(-1 == nRet)
				{
					DEBUGPRINT(DEBUG_INFO, "close file pointer fail! @:%s %d\n", __FILE__, __LINE__);
					return;
				}

				return;
			}

next:       g_stNetWorkSetInfo.unLoopCnts = 0;
			g_stNetWorkSetInfo.unCheckNetTimes++;

			DEBUGPRINT(DEBUG_INFO, "cg_stNetWorkSetInfo.unCheckNetTimes = %d\n",g_stNetWorkSetInfo.unCheckNetTimes);
			/* 若 30s 内不回 则  失败 */
			if (30 == g_stNetWorkSetInfo.unCheckNetTimes )
			{
				//system("killall ping");
				nRet = pclose(g_stNetWorkSetInfo.pFile);
				g_stNetWorkSetInfo.pFile = NULL;
				if(-1 == nRet)
				{
					DEBUGPRINT(DEBUG_INFO, "close file pointer fail! @:%s %d\n", __FILE__, __LINE__);
					return;
				}
				nRet = fclose(g_stNetWorkSetInfo.pTestFile);
				g_stNetWorkSetInfo.pTestFile = NULL;
				if(-1 == nRet)
				{
					DEBUGPRINT(DEBUG_INFO, "close file pointer fail! @:%s %d\n", __FILE__, __LINE__);
					return;
				}

				/* 无网络可用 */
				/* 若当前为无线 */
				if (NET_TYPE_WIRELESS == g_stNetWorkSetInfo.unNetType)
				{
					;
					DEBUGPRINT(DEBUG_INFO, "set wifi n @:%s %d\n", __FILE__, __LINE__);
					//set_config_value_to_ram(CONFIG_WIFI_ACTIVE,"n",1);
				}
				else
				{
					/* 尝试设置无线网络 TODO */
					//set_wifi_by_config();
				}

				/* 网络连接不成功 */
				g_stNetWorkSetInfo.unNetState = SET_NET_STATE_NET_CONNECT_FAILURE;
				set_net_connect_result_sendmessage_to_child_process(g_stNetWorkSetInfo.unNetType,
								   g_stNetWorkSetInfo.unIpType, NET_CONNECT_DISCONNECT);

			}
		 }
		 else
		 {
			 g_stNetWorkSetInfo.unLoopCnts++;
		 }
	}
#endif
}


/**
  *  @brief      将wifi列表信息存入文件
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
int achieve_wifi_list_to_file()
{
	FILE *pFile = NULL;

	pFile = popen("iwconfig ra0> /tmp/wifilist.txt", "r");

	if (NULL != pFile)
	{
        pclose(pFile);
        return EXIT_SUCCESS;
	}
	else
	{
		DEBUGPRINT(DEBUG_INFO, "popen iwlist ra0 scanning > /tmp/wifilist.txt failed\n");
		DEBUGPRINT(DEBUG_INFO, "popen iwlist ra0 scanning > /tmp/wifilist.txt failed\n");
		DEBUGPRINT(DEBUG_INFO, "popen iwlist ra0 scanning > /tmp/wifilist.txt failed\n");
		DEBUGPRINT(DEBUG_INFO, "popen iwlist ra0 scanning > /tmp/wifilist.txt failed\n");
		DEBUGPRINT(DEBUG_INFO, "popen iwlist ra0 scanning > /tmp/wifilist.txt failed\n");
		DEBUGPRINT(DEBUG_INFO, "popen iwlist ra0 scanning > /tmp/wifilist.txt failed\n");
		DEBUGPRINT(DEBUG_INFO, "popen iwlist ra0 scanning > /tmp/wifilist.txt failed\n");
		DEBUGPRINT(DEBUG_INFO, "popen iwlist ra0 scanning > /tmp/wifilist.txt failed\n");
		DEBUGPRINT(DEBUG_INFO, "popen iwlist ra0 scanning > /tmp/wifilist.txt failed\n");
		DEBUGPRINT(DEBUG_INFO, "popen iwlist ra0 scanning > /tmp/wifilist.txt failed\n");
		DEBUGPRINT(DEBUG_INFO, "popen iwlist ra0 scanning, error is %d, descrip: %s\n", errno, strerror(errno));

		return EXIT_FAILURE;
	}
}

/**
  *  @brief      解析wifi列表信息到ram
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void analysis_wifi_list_to_ram()
{
    FILE *pFile = NULL;
	char buf[512];
	regex_t re_ssid;
	regex_t re_freq;
	regex_t re_qu;
	regex_t re_signal_level;
	regex_t re_noise_level;
	regmatch_t match_ssid[1];
	regmatch_t match_freq[1];
	regmatch_t match_qu[1];
	regmatch_t match_signal_level[1];
	regmatch_t match_noise_level[1];

	if (NULL == g_stWifiList.pstWifiListInfo)
	{
		DEBUGPRINT(DEBUG_INFO, "g_stWifiList.pstWifiListInfo  \n");
	}

    if (NULL == (pFile = fopen("/tmp/wifilist.txt", "rw+")))
    {
    	DEBUGPRINT(DEBUG_INFO, "fopen /tmp/wifilist.txt failed\n");
    	return;
    }

	regcomp(&re_ssid,         "ESSID:\"[^\"]*\"",              REG_EXTENDED);
	regcomp(&re_freq,         "Frequency[:=][^\"]*",              REG_EXTENDED);
	regcomp(&re_qu,           "Quality=[0-9]{1,3}",            REG_EXTENDED);
	regcomp(&re_signal_level, "Signal level[:=]-[0-9]{1,3}",      REG_EXTENDED);
	regcomp(&re_noise_level,  "Noise level[:=]-[0-9]{1,3}",       REG_EXTENDED);

	while(NULL != fgets(buf, sizeof(buf), pFile))
    {

		if (0 == regexec(&re_ssid, buf, 1, match_ssid, 0))
        {
			g_stWifiList.nCurNo++;
			if (MAX_WIFI_LIST_NUM < g_stWifiList.nCurNo)
			{
				printf("already up to the max\n");
				break;
			}
			//printf("1111111111111111cur no is : %d, buf is %s\n",  g_stWifiList.nCurNo, buf);

			strncpy(((wifi_info_t *)(g_stWifiList.pstWifiListInfo) + g_stWifiList.nCurNo)->acWifiESSID,\
                              buf + 7 + match_ssid[0].rm_so, match_ssid[0].rm_eo - match_ssid[0].rm_so -8);
		}
        else if (0 == regexec(&re_freq, buf, 1, match_freq, 0))
        {
			//printf("2222222222222222cur no is : %d, buf is %s\n", g_stWifiList.nCurNo, buf);
			strncpy(((wifi_info_t *)(g_stWifiList.pstWifiListInfo) + g_stWifiList.nCurNo)->acWifiFreq,\
				  buf + 10 + match_freq[0].rm_so, 5/*match_freq[0].rm_eo - match_freq[0].rm_so*/);
        }
		else if (0 == regexec(&re_qu, buf, 1, match_qu, 0))
        {
   			strncpy(((wifi_info_t *)(g_stWifiList.pstWifiListInfo) + g_stWifiList.nCurNo)->acWifiQuqlity,\
                              buf + 8 + match_qu[0].rm_so, match_qu[0].rm_eo - match_qu[0].rm_so - 8);
            //printf("3333333333333333cur no is : %d, buf is %s\n", g_stWifiList.nCurNo, buf);

			if (0 == regexec(&re_signal_level, buf, 1, match_signal_level, 0))
			{
				  strncpy(((wifi_info_t *)(g_stWifiList.pstWifiListInfo) + g_stWifiList.nCurNo)->acWifiSignalLevel,\
					   buf + 13 + match_signal_level[0].rm_so, match_signal_level[0].rm_eo - match_signal_level[0].rm_so - 13);
				  //printf("44444444444444444cur no is : %d, buf is %s\n", g_stWifiList.nCurNo, buf);
			}

			if (0 == regexec(&re_noise_level, buf, 1, match_noise_level, 0))
			{
				  strncpy(((wifi_info_t *)(g_stWifiList.pstWifiListInfo) + g_stWifiList.nCurNo)->acWifiNoiseLevel,\
					   buf + 12 + match_noise_level[0].rm_so, match_noise_level[0].rm_eo - match_noise_level[0].rm_so - 12);
				  //printf("55555555555555555cur no is : %d, buf is %s\n", g_stWifiList.nCurNo, buf);
			}
		}
	}
#if 10
	if (NULL != pFile)
	{
	    fclose(pFile);
	}
#endif
	regfree(&re_ssid);
	regfree(&re_freq);
	regfree(&re_qu);
	regfree(&re_signal_level);
	regfree(&re_noise_level);
}


/**
  *  @brief      打包wifi信号信息
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void package_wifi_signal_info()
{
	char acSSID[64] = {0};
	char acWifiInfo[10] = {0};
	int nBufLen = 0;
	int i = 0;

	get_config_item_value(CONFIG_WIFI_SSID,      acSSID,     &nBufLen);

	//printf("CONFIG_WIFI_SSID is : %s...........................\n", acSSID);

	/* 查找相关信息 */
	//for (i = 0; i < g_stWifiList.nCurNo; i++)
	do{
		 //printf("(g_stWifiList.pstWifiListInfo + i)->acWifiESSID = %s\n", (g_stWifiList.pstWifiListInfo + i)->acWifiESSID);
         if (0 == strcmp(acSSID, (g_stWifiList.pstWifiListInfo + i)->acWifiESSID))
         {
        	 strncpy(acWifiInfo, (g_stWifiList.pstWifiListInfo + i)->acWifiFreq, 6);

        	 acWifiInfo[6] = atoi((g_stWifiList.pstWifiListInfo + i)->acWifiQuqlity);
        	 acWifiInfo[7] = atoi((g_stWifiList.pstWifiListInfo + i)->acWifiSignalLevel);
        	 acWifiInfo[8] = atoi((g_stWifiList.pstWifiListInfo + i)->acWifiNoiseLevel);

        	 set_wifi_signal_level(acWifiInfo);
        	 printf("freq: %s...qu: %d sig: %d nosie:%d\n", acWifiInfo, acWifiInfo[6], acWifiInfo[7], acWifiInfo[8]);
        	 break;
         }
	//}
	}while(++i < g_stWifiList.nCurNo);
}

/**
  *  @brief      清空wifi列表资源
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void clear_wifi_list()
{
	memset(g_stWifiList.pstWifiListInfo, 0x00, sizeof(wifi_info_t) * MAX_WIFI_LIST_NUM);
	g_stWifiList.nCurNo = -1;
}

/**
  *  @brief      检测网络信息
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *              2013-11-14 通过函数获取wifi值
  *  @note
*/
void updata_wifi_list()
{
	char acWifiInfo[10] = {0};


	if (WIFI_LIST_UPDATA_TIME/NETWORKSET_THREAD_USLEEP == g_stNetWorkSetInfo.unWifiListLoopCnts)
    {
		g_stNetWorkSetInfo.unWifiListLoopCnts = 0;
		g_nNetGetWifiSignal = 0;
		iw_get_wifi_signal(acWifiInfo);
		g_nNetGetWifiSignal = 1;
		set_wifi_signal_level(acWifiInfo);

	}
	else
	{
		 g_stNetWorkSetInfo.unWifiListLoopCnts++;
	}



    return;
#if 0
	 if (WIFI_LIST_UPDATA_TIME/NETWORKSET_THREAD_USLEEP == g_stNetWorkSetInfo.unWifiListLoopCnts)
	 {
		 g_stNetWorkSetInfo.unWifiListLoopCnts = 0;

		 //清空wifi资源
		 clear_wifi_list();

	     //将wifi列表信息存入文件
		 if (EXIT_FAILURE == achieve_wifi_list_to_file())
		 {
			 return;
		 }

		 //解析wifi列表信息到ram
		 analysis_wifi_list_to_ram();
		 //根据需求的填充数据
		 //(1)发送给apps的wifi信息
		 package_wifi_signal_info();
	 }
	 else
	 {
		 g_stNetWorkSetInfo.unWifiListLoopCnts++;
	 }
#endif
}


/**
  *  @brief      定时check_if_net_check_block
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void check_if_net_check_block()
{
	if (CHECK_BLOCK_MAX_TIME/NETWORKSET_THREAD_USLEEP == g_nNetBlockTimes)
	{
		DEBUGPRINT(DEBUG_INFO, "check_if_net_check_block  CHECK REALY BLOCK\n");
        //重启系统
		//对空指针操作，然后通过保护进程将其启动
		//strcpy(NULL, "ABC");
		DEBUGPRINT(DEBUG_INFO, "HERE SHOULD CAUSE SEGMENT FAULT\n");
		system_stop_speaker_and_reboot();
	}

	//FALSE不阻塞
	if (FALSE == g_nNetCheckBlockFlag)
	{
		g_nNetBlockTimes = 0;
		set_net_check_block_flag();
	}
	//阻塞
	else
	{
		g_nNetBlockTimes ++;
	}
}
/**
  *  @brief      定时udhcpc
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void interval_udhcpc()
{
	char acCmdBuf[64] = {0};

	if ((TRUE == g_nGetIpFlag) || (NET_TYPE_WIRED == g_stLocalNetSetInfo.lNetType))
	{
         if (100 == g_nIntervalDhcpLoop)
         {
             system("killall -9 udhcpc");
             sprintf(acCmdBuf, "udhcpc -i ra0 -r %s &", g_acWifiIpInfo);
             system(acCmdBuf);
             DEBUGPRINT(DEBUG_INFO, "reset udhcpc   %s\n", acCmdBuf);
             g_nIntervalDhcpLoop = 0;
         }
         else
         {
        	 g_nIntervalDhcpLoop++;
         }
	}

}


/**
  *  @brief      网络线程监测网络设置结果
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void network_check_report_handler()
{
;
}

/**
  *  @brief      网络类型
  *  @author     <wfb> 
  *  @param[in]  <pstRcvMsg: 接收消息>
  *  @param[in]  <pstSndMsg: 发送消息>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void net_workset_msghandler(MsgData *pstRcvMsg, MsgData *pstSndMsg)
{
	netset_info_t *pstNetSetInfo = NULL;
	int lRet = EXIT_SUCCESS;

	if (NULL == pstRcvMsg || NULL == pstSndMsg)
	{
		DEBUGPRINT(DEBUG_INFO, "func:net_workset_msghandler invalid\n");
	}

	pstNetSetInfo = (netset_info_t *)(pstRcvMsg +1);


	/* 此时判断是否需要重新设置，若当前连接成功的是有线网，那么 发送来的若是无线网则不设置 只将无限的信息存起来
	 * 若当前连接成功的是无线网，那么发送国来的要设置的是有线网 那么则要重新设置，若发送过来的是无线网，则要和
	 * 目前连接的进行对比，若设置的是同一个网络，那么则不设置（目前这种情况重设）
	*/
	//if (SET_NET_STATE_NET_CONNECT_SUCCESS == g_stNetWorkSetInfo.unNetState)
	{
		if (FALSE == if_need_reset_net(pstNetSetInfo))
		{
			return;
		}
		else
		{
			/* 清空网络信息 */
			clean_net_info_msghandler();
			g_nGetIpFlag = FALSE;
		}
	}

	do{
        /* 清空资源 */
		memcpy(&g_stLocalNetSetInfo, pstNetSetInfo, sizeof(netset_info_t));

		/* 通知子进程开始设置网络  */
		send_msg_to_child_start_set_net();

		/* 开始设置网络 */
		g_stNetWorkSetInfo.unNetState = SET_NET_STATE_READY_SET;

		/* 向子进程发送消息 当前网络状态（正在设置网络<有线 或是 wifi>） */
		start_set_net_type_sendmessage_to_child_process(pstNetSetInfo->lNetType);
		/* 开始设置 */
		//lRet = set_net_type_and_ip_type(pstNetSetInfo);

	}while(0);

	if (lRet != EXIT_SUCCESS)
	{
        //向子进程发送设置失败结果
		set_net_connect_result_sendmessage_to_child_process(pstNetSetInfo->lNetType,
				pstNetSetInfo->lIpType, NET_CONNECT_FAILURE);
	}
}

/**
  *  @brief      设置网络类型  同时记录网络类型
  *  @author     <wfb> 
  *  @param[in]  <pstNetSetInfo: 网络信息结构体>
  *  @param[in]  <pstSndMsg: 发送消息结构体>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
int  network_set_net_type(netset_info_t *pstNetSetInfo, MsgData *pstSndMsg)
{
	int lRet = EXIT_FAILURE;
	FILE *pFile = NULL;

    /* 参数检查 */
	if (NULL == pstNetSetInfo)
	{
		DEBUGPRINT(DEBUG_INFO, "func: network_set_net_type invalid\n");
        return lRet;
	}

	DEBUGPRINT(DEBUG_INFO, "func :network_set_net_type \n");

    do{
        if (NET_TYPE_WIRED == pstNetSetInfo->lNetType)
        {
        	DEBUGPRINT(DEBUG_INFO, "set wired type\n");
            lRet = netset_set_net_type_wired();
            g_stNetWorkSetInfo.unNetState = SET_NET_STATE_SET_WIRED_TYPE_OK;
        }
        else if (NET_TYPE_WIRELESS== pstNetSetInfo->lNetType)
        {
        	DEBUGPRINT(DEBUG_INFO, "set WIRELESS type\n");
        	DEBUGPRINT(DEBUG_INFO, "ssid:   %s\n", pstNetSetInfo->acSSID);
        	DEBUGPRINT(DEBUG_INFO, "code:   %s\n", pstNetSetInfo->acCode);
        	DEBUGPRINT(DEBUG_INFO, "type:   %d\n", pstNetSetInfo->lWifiType);
        	/* 先断开网 */
#ifndef __IWPRIV_H_
        	//system("iwconfig ra0 essid off");
        	pFile = popen("iwconfig ra0 essid off", "r");
        	if (NULL != pFile)
			{
				char acTmpBuf[128] = {0};

				fread(acTmpBuf, 1, 128, pFile);
				pclose(pFile);
				pFile = NULL;
			}
#else
        	g_nNetIwprivEssidoff = 0;
        	iwconfig_essid_off("ra0");
        	g_nNetIwprivEssidoff = 1;
#endif
        	{
        		netset_set_net_type_wifi(pstNetSetInfo->lWifiType, pstNetSetInfo->acSSID, pstNetSetInfo->acCode);
        		g_stNetWorkSetInfo.unNetState = SET_NET_STATE_SET_WIFI_OK;
        	}
        }
        else
        {
        	DEBUGPRINT(DEBUG_INFO, "invalid net type \n");
            break;
        }

        /* 记录当前网络类型 */
        g_stNetWorkSetInfo.unNetType = pstNetSetInfo->lNetType;
        lRet = EXIT_SUCCESS;

    }while(0);

    if (EXIT_FAILURE == lRet)
    {
    	memset(&g_stNetWorkSetInfo, 0x00, sizeof(networkset_into_t));
    }

    return lRet;
}

/**
  *  @brief      设置静态ip
  *  @author     <wfb> 
  *  @param[in]  <pstNetSetInfo: 网络信息结构体>
  *  @param[in]  <pstSndMsg: 发送消息结构体>
  *  @param[out] <无>
  *  @return     <EXIT_FAILURE>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
int set_static_ip(netset_info_t *pstNetSetInfo)
{
    int lRet = EXIT_FAILURE;
    char acSetCmdBuf[256] = {0};
    FILE *pFile = NULL;

    if (NULL == pstNetSetInfo)
    {
    	DEBUGPRINT(DEBUG_INFO, "func:set_static_ip invalid param\n");
    	return lRet;
    }

    /* 设置状态  */
    g_stNetWorkSetInfo.unNetState = SET_NET_STATE_SET_STATIC_IP;

    /* 设置 有线 */
    if (NET_TYPE_WIRED == pstNetSetInfo->lNetType)
    {
    	memset(acSetCmdBuf, 0x00, sizeof(acSetCmdBuf));
    	snprintf(acSetCmdBuf, sizeof(acSetCmdBuf), "ifconfig eth2 %s netmask %s%c",\
    			pstNetSetInfo->acIpAddr, pstNetSetInfo->acSubNetMask, '\0');
    	//system(acSetCmdBuf);
    	pFile = popen(acSetCmdBuf, "r");
    	if (NULL != pFile)
		{
			char acTmpBuf[128] = {0};

			fread(acTmpBuf, 1, 128, pFile);
			pclose(pFile);
			pFile = NULL;
		}

    	memset(acSetCmdBuf, 0x00, sizeof(acSetCmdBuf));
    	snprintf(acSetCmdBuf, sizeof(acSetCmdBuf), "route add default gw %s dev eth2%c",\
    			pstNetSetInfo->acGateWay, '\0');
    	//system(acSetCmdBuf);
    	pFile = popen(acSetCmdBuf, "r");
    	if (NULL != pFile)
		{
			char acTmpBuf[128] = {0};

			fread(acTmpBuf, 1, 128, pFile);
			pclose(pFile);
			pFile = NULL;
		}

    	memset(acSetCmdBuf, 0x00, sizeof(acSetCmdBuf));
    	snprintf(acSetCmdBuf, sizeof(acSetCmdBuf), "echo \"nameserver %s \nnameserver %s\">/etc/resolv.conf%c",\
    			pstNetSetInfo->acWiredDNS, pstNetSetInfo->acWifiDNS, '\0');
    	//system(acSetCmdBuf);
    	pFile = popen(acSetCmdBuf, "r");
    	if (NULL != pFile)
		{
			char acTmpBuf[128] = {0};

			fread(acTmpBuf, 1, 128, pFile);
			pclose(pFile);
			pFile = NULL;
		}
    }
    else
    {
    	memset(acSetCmdBuf, 0x00, sizeof(acSetCmdBuf));
        snprintf(acSetCmdBuf, sizeof(acSetCmdBuf), "ifconfig ra0 %s netmask %s%c",\
        		pstNetSetInfo->acIpAddr, pstNetSetInfo->acSubNetMask, '\0');
    	//system(acSetCmdBuf);
    	pFile = popen(acSetCmdBuf, "r");
    	if (NULL != pFile)
		{
			char acTmpBuf[128] = {0};

			fread(acTmpBuf, 1, 128, pFile);
			pclose(pFile);
			pFile = NULL;
		}

    	snprintf(acSetCmdBuf, sizeof(acSetCmdBuf), "route add default gw %s dev ra0%c", \
    			pstNetSetInfo->acGateWay, '\0');
    	memset(acSetCmdBuf, 0x00, sizeof(acSetCmdBuf));
    	//system(acSetCmdBuf);
    	pFile = popen(acSetCmdBuf, "r");
    	if (NULL != pFile)
		{
			char acTmpBuf[128] = {0};

			fread(acTmpBuf, 1, 128, pFile);
			pclose(pFile);
			pFile = NULL;
		}

    	memset(acSetCmdBuf, 0x00, sizeof(acSetCmdBuf));
    	snprintf(acSetCmdBuf, sizeof(acSetCmdBuf), "echo \"nameserver %s \nnameserver %s\">/etc/resolv.conf%c",\
    			pstNetSetInfo->acWiredDNS, pstNetSetInfo->acWifiDNS, '\0');
    	//system(acSetCmdBuf);
    	pFile = popen(acSetCmdBuf, "r");
    	if (NULL != pFile)
		{
			char acTmpBuf[128] = {0};

			fread(acTmpBuf, 1, 128, pFile);
			pclose(pFile);
			pFile = NULL;
		}
    }

    /* 静态默认ip是设置成功的 */
    g_stNetWorkSetInfo.unHaveIpStatus =  GET_IP_SUCCESS;
    g_stNetWorkSetInfo.unNetState = SET_NET_STATE_SET_IP_SUCCESS;
    report_set_ip_sendmessage_to_child_process(IP_TYPE_STATIC, SET_IP_SUCCESS);

    return EXIT_SUCCESS;
}

/**
  *  @brief      设置dhcp
  *  @author     <wfb> 
  *  @param[in]  <lNetType: 网络类型>
  *  @param[out] <无>
  *  @return     <EXIT_FAILURE>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
int set_dhcp(int lNetType)
{
    int lRet = EXIT_FAILURE;
    //int lCmdRtnVal = 0;
    FILE *pFile = NULL;
    char acSetCmdBuf[128] = {0};
    struct timeval stTimeVal0;
    //struct timeval stTimeVal1;
    //struct timeval stTimeVal2;
    struct timeval stTimeVal3;

    //在这里设置标志
    set_start_dhcp_flag(lNetType);
#if 0
    /* 设置 有线  dhcp的命令均在后台运行 */
    memset(acSetCmdBuf, 0x00, sizeof(acSetCmdBuf));
    if (NET_TYPE_WIRED == lNetType)
    {
    	DEBUGPRINT(DEBUG_INFO, "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$come into net type wired\n");
    	//lCmdRtnVal = system("udhcpc -q -i eth2 &");
    	pFile = popen("udhcpc -i eth2 &", "r");
    	if (NULL != pFile)
		{
			char acTmpBuf[128] = {0};

			fread(acTmpBuf, 1, 128, pFile);
			pclose(pFile);
			pFile = NULL;
		}

    }
    else if (NET_TYPE_WIRELESS == lNetType)
    {
    	DEBUGPRINT(DEBUG_INFO, "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$come into net type wireless\n");
        //lCmdRtnVal = system("udhcpc -q -i ra0 &");
    	if (0 == strlen(g_acWifiIpInfo))
    	{
    		sprintf(acSetCmdBuf, "udhcpc -i ra0 &");
    	}
    	else
    	{
    		sprintf(acSetCmdBuf, "udhcpc -i ra0 -r %s &", g_acWifiIpInfo);
    	}

    	DEBUGPRINT(DEBUG_INFO, "#############################################acSetCmdBuf  =   %s\n", acSetCmdBuf);

    	gettimeofday(&stTimeVal0, NULL);
    	DEBUGPRINT(DEBUG_INFO, "ready udhcpc g_stTimeVal0.tv_sec = %ld~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n", stTimeVal0.tv_sec);
    	pFile = popen(acSetCmdBuf, "r");
    	if (NULL != pFile)
		{
			char acTmpBuf[128] = {0};
			int nLen = 0;
			int nRet = 0;

			//gettimeofday(&stTimeVal1, NULL);
			//DEBUGPRINT(DEBUG_INFO, "1111111111 stTimeVal1.tv_sec = %ld~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n", stTimeVal1.tv_sec);
			nLen = fread(acTmpBuf, 1, 64, pFile);
			//gettimeofday(&stTimeVal2, NULL);
			//DEBUGPRINT(DEBUG_INFO, "2222222222 stTimeVal2.tv_sec = %ld nLen = %d~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n", stTimeVal2.tv_sec, nLen);
			nRet = pclose(pFile);
			gettimeofday(&stTimeVal3, NULL);
			DEBUGPRINT(DEBUG_INFO, "ready udhcpc stTimeVal3.tv_sec = %ld nLen = %d, return =  %d~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n", stTimeVal3.tv_sec, nLen, nRet);

			/* 计算dhcp的时间 */
			g_nDhcpFinishTime = stTimeVal3.tv_sec - stTimeVal0.tv_sec;

			pFile = NULL;
		}
    }
    else
    {
    	DEBUGPRINT(DEBUG_INFO, "invalid lNetType = %d\n", lNetType);
        return lRet;
    }
#endif

    DEBUGPRINT(DEBUG_INFO, "set_dhcp flag ok\n");

    g_stNetWorkSetInfo.unNetType = lNetType;
    /* 设置状态  个人认为这里在设置dhcp（后台）后，再置状态比较好  add by wfb */
    g_stNetWorkSetInfo.unNetState = SET_NET_STATE_SET_DHCP;
    g_stNetWorkSetInfo.unLoopCnts = 0;

    return EXIT_SUCCESS;
}

/**
  *  @brief      设置ip
  *  @author     <wfb> 
  *  @param[in]  <pstNetSetInfo: 网络结构体>
  *  @param[out] <无>
  *  @return     <EXIT_FAILURE EXIT_SUCCESS>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
int network_set_ip_type(netset_info_t *pstNetSetInfo)
{
	int lRet = EXIT_FAILURE;
	int nSetDhcpFlag = 0;
	//char acSetIpBuf[32] = {0};

	if (NULL == pstNetSetInfo)
	{
		DEBUGPRINT(DEBUG_INFO, "func: network_set_ip_type pstNetSetInfo \n");
        return lRet;
	}

	do {
		if (IP_TYPE_STATIC == pstNetSetInfo->lIpType)
		{
			/* set wired ip */
			DEBUGPRINT(DEBUG_INFO, "set wired static ip: %s \n", pstNetSetInfo->acIpAddr);
            lRet = set_static_ip(pstNetSetInfo);
		}
		else if(IP_TYPE_DYNAMIC== pstNetSetInfo->lIpType)
		{
			/* dhcp */
			DEBUGPRINT(DEBUG_INFO, "set dynamic ip \n");
			/* 先设置一个ip 然后看获取之后的 若dhcp成功 则会更改 */
			lRet = set_dhcp(pstNetSetInfo->lNetType);
		}
		else
		{
			DEBUGPRINT(DEBUG_INFO, "func:network_set_ip_type, invalid ip type\n");
		    break;
		}

	}while(0);

	/* 记录当前网络类型 */
	g_stNetWorkSetInfo.unIpType = pstNetSetInfo->lIpType;

    return lRet;
}

/**
  *  @brief      设置有线网
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <EXIT_FAILURE EXIT_SUCCESS>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
int netset_set_net_type_wired()
{
	int lRet = EXIT_SUCCESS;
    FILE *pFile = NULL;

	/* 若连接则要断开无线网 */
	//system("ifconfig ra0 0.0.0.0 up");
	//system("iwconfig ra0 essid off");

	/* set def ip */
	//system("ifconfig eth2 10.10.10.10");

	pFile = popen("ifconfig ra0 0.0.0.0 up;iwconfig ra0 essid off", "r");
	if (NULL != pFile)
	{
		char acTmpBuf[128] = {0};

		fread(acTmpBuf, 1, 128, pFile);
		pclose(pFile);
		pFile = NULL;
	}

	return lRet;
}

/**
  *  @brief      设置wifi
  *  @author     <wfb> 
  *  @param[in]  <lWiFiType: wifi加密类型>
  *  @param[in]  <pcSSID：    SSID>
  *  @param[in]  <pcCode：    密码>
  *  @param[out] <无>
  *  @return     <EXIT_FAILURE EXIT_SUCCESS>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
int netset_set_net_type_wifi(int lWiFiType, char *pcSSID, char *pcCode)
{
    int lRet = EXIT_SUCCESS;
    int lWifiEncryType = 0;
    char acTmpBuf[512] = {0};
    FILE *pFile = NULL;
    char acIpAddr[16] = {0};

	memset(acTmpBuf, '\0', sizeof(acTmpBuf));

    if (NULL == pcSSID)
    {
    	DEBUGPRINT(DEBUG_INFO, "nWiFiType:%d, in file:%s line:%d\n", lWiFiType, __FILE__, __LINE__);
    	return lRet;
    }

    /* 设置wifi 不可用 这里为了加快wifi流程所以直接使用后台执行 modify by wfb */
    //system("nvram_set 2860 \"Wifi_Active\" \"n\" &");
    //set_config_value_to_flash(CONFIG_WIFI_ACTIVE, "n", 1);
    /* 这里得先down 掉有线网 */
    //system("ifconfig eth2 down ");
    //system("ifconfig eth2 0.0.0.0");
    g_nPopenFlag = 0;
    //如果eht2没有ip地址 那么则不执行
    get_ip_addr(acIpAddr, NET_TYPE_WIRED);
    if (0 != strlen(acIpAddr))
    {
    	pFile = popen("ifconfig eth2 0.0.0.0", "r");
    	if (NULL != pFile)
    	{
    		char acTmpBuf[128] = {0};

    		fread(acTmpBuf, 1, 128, pFile);
    		pclose(pFile);
    		pFile = NULL;
    	}
    }
        g_nPopenFlag = 1;
    //system("ifconfig ra0 10.10.10.10");

    /* 每次进来 wifi次数加1 */
    g_stNetWorkSetInfo.unWifiEncryTimes++;

    switch(lWiFiType)
    {
        case 0:
        	lWifiEncryType = WIFI_TYPE_OPEN_NONE;
        	break;
        case 1:
        	if (g_stNetWorkSetInfo.unWifiEncryTimes%2)
        	{
        		lWifiEncryType = WIFI_TYPE_WEP_OPEN;
        	}
        	else
        	{
        		lWifiEncryType = WIFI_TYPE_WEP_SHARED;
        	}

        	break;
        case 2:
        	if (g_stNetWorkSetInfo.unWifiEncryTimes%2)
        	{
        		lWifiEncryType = WIFI_TYPE_WPAPSK_TKIP;
        	}
        	else
        	{
        		lWifiEncryType = WIFI_TYPE_WPAPSK_AES;
        	}
        	break;
        case 3:
        	if (g_stNetWorkSetInfo.unWifiEncryTimes%2)
        	{
        		lWifiEncryType = WIFI_TYPE_WPA2PSK_AES;
        	}
        	else
        	{
        		lWifiEncryType = WIFI_TYPE_WPA2PSK_TKIP;
        	}
        	break;
        case 4:
        	lWifiEncryType = return_mixed_WifiEntryType_by_times(g_stNetWorkSetInfo.unWifiEncryTimes);
        	break;
        default:
        	lWifiEncryType = return_mixed_WifiEntryType_by_times(g_stNetWorkSetInfo.unWifiEncryTimes);
        	{
            	//默认设置成4
            	set_config_value_to_flash(CONFIG_WIFI_ENRTYPE, "4", 1);
        	}

        	DEBUGPRINT(DEBUG_INFO, "unknow lWifiType = %d, this is not wrong, but we do not deal with it\n", lWiFiType);
        	//return EXIT_FAILURE;
        	break;
    }

    /* 设置网络类型 */
    switch(lWifiEncryType)
    {
        case WIFI_TYPE_OPEN_NONE:     // 1
        	DEBUGPRINT(DEBUG_INFO, "WIFI_TYPE_OPEN_NONE, in file:%s line:%d\n", __FILE__, __LINE__);
#ifndef __IWPRIV_H_
			snprintf(acTmpBuf, sizeof(acTmpBuf),
				"iwpriv ra0 set NetworkType=Infra;\
             	iwpriv ra0 set AuthMode=OPEN;\
             	iwpriv ra0 set EncrypType=NONE;\
             	iwpriv ra0 set SSID=\"%s\"",
             	pcSSID);
#else
			g_nNetIwprivFuncSet = 0;
			iwpriv_func(IWPRIV_CMD_SET, "ra0", "NetworkType=Infra", NULL);
			iwpriv_func(IWPRIV_CMD_SET, "ra0", "AuthMode=OPEN", NULL);
			iwpriv_func(IWPRIV_CMD_SET, "ra0", "EncrypType=NONE", NULL);
			memset(acTmpBuf, 0x00, sizeof(acTmpBuf));
			snprintf(acTmpBuf, sizeof(acTmpBuf),"SSID=%s", pcSSID);
			iwpriv_func(IWPRIV_CMD_SET, "ra0",  acTmpBuf, NULL);
			g_nNetIwprivFuncSet = 1;
#endif
            break;
	   case WIFI_TYPE_WEP_OPEN:   // 2
		   DEBUGPRINT(DEBUG_INFO, "WIFI_TYPE_WEP_OPEN, in file:%s line:%d\n", __FILE__, __LINE__);
#ifndef __IWPRIV_H_
			snprintf(acTmpBuf, sizeof(acTmpBuf),
				"iwpriv ra0 set NetworkType=Infra;\
             	iwpriv ra0 set AuthMode=OPEN;\
             	iwpriv ra0 set EncrypType=WEP;\
             	iwpriv ra0 set DefaultKeyID=1;\
             	iwpriv ra0 set Key1=\"%s\";\
             	iwpriv ra0 set SSID=\"%s\"",
             	pcCode, pcSSID);
#else
			g_nNetIwprivFuncSet = 0;
			iwpriv_func(IWPRIV_CMD_SET, "ra0", "NetworkType=Infra", NULL);
			iwpriv_func(IWPRIV_CMD_SET, "ra0", "AuthMode=OPEN", NULL);
			iwpriv_func(IWPRIV_CMD_SET, "ra0", "EncrypType=WEP", NULL);
			iwpriv_func(IWPRIV_CMD_SET, "ra0", "DefaultKeyID=1", NULL);

			memset(acTmpBuf, 0x00, sizeof(acTmpBuf));
			snprintf(acTmpBuf, sizeof(acTmpBuf),"Key1=%s", pcCode);
			iwpriv_func(IWPRIV_CMD_SET, "ra0",  acTmpBuf, NULL);

			memset(acTmpBuf, 0x00, sizeof(acTmpBuf));
			snprintf(acTmpBuf, sizeof(acTmpBuf),"SSID=%s", pcSSID);
			iwpriv_func(IWPRIV_CMD_SET, "ra0",  acTmpBuf, NULL);
			g_nNetIwprivFuncSet = 1;
#endif
			break;
	   case WIFI_TYPE_WEP_SHARED:	// 3
		   DEBUGPRINT(DEBUG_INFO, "WIFI_TYPE_WEP_SHARED, in file:%s line:%d\n", __FILE__, __LINE__);
#ifndef __IWPRIV_H_
			snprintf(acTmpBuf, sizeof(acTmpBuf),
				"iwpriv ra0 set NetworkType=Infra;\
             	iwpriv ra0 set AuthMode=SHARED;\
             	iwpriv ra0 set EncrypType=WEP;\
             	iwpriv ra0 set DefaultKeyID=1;\
             	iwpriv ra0 set Key1=\"%s\";\
             	iwpriv ra0 set SSID=\"%s\"",
             	pcCode, pcSSID);
#else
			g_nNetIwprivFuncSet = 0;
			iwpriv_func(IWPRIV_CMD_SET, "ra0", "NetworkType=Infra", NULL);
			iwpriv_func(IWPRIV_CMD_SET, "ra0", "AuthMode=SHARED", NULL);
			iwpriv_func(IWPRIV_CMD_SET, "ra0", "EncrypType=WEP", NULL);
			iwpriv_func(IWPRIV_CMD_SET, "ra0", "DefaultKeyID=1", NULL);

			memset(acTmpBuf, 0x00, sizeof(acTmpBuf));
			snprintf(acTmpBuf, sizeof(acTmpBuf),"Key1=%s", pcCode);
			iwpriv_func(IWPRIV_CMD_SET, "ra0",  acTmpBuf, NULL);

			memset(acTmpBuf, 0x00, sizeof(acTmpBuf));
			snprintf(acTmpBuf, sizeof(acTmpBuf),"SSID=%s", pcSSID);
			iwpriv_func(IWPRIV_CMD_SET, "ra0",  acTmpBuf, NULL);
			g_nNetIwprivFuncSet = 1;
#endif
			break;
        case WIFI_TYPE_WPAPSK_TKIP:   // 4
        	DEBUGPRINT(DEBUG_INFO, "WIFI_TYPE_WPAPSK_TKIP, in file:%s line:%d\n", __FILE__, __LINE__);
#ifndef __IWPRIV_H_
			snprintf(acTmpBuf, sizeof(acTmpBuf),
				"iwpriv ra0 set NetworkType=Infra;\
             	iwpriv ra0 set AuthMode=WPAPSK;\
             	iwpriv ra0 set EncrypType=TKIP;\
				iwpriv ra0 set SSID=\"%s\";\
             	iwpriv ra0 set WPAPSK=\"%s\";\
             	iwpriv ra0 set SSID=\"%s\"",
             	pcSSID, pcCode, pcSSID);
#else
			g_nNetIwprivFuncSet = 0;
			iwpriv_func(IWPRIV_CMD_SET, "ra0", "NetworkType=Infra", NULL);
			iwpriv_func(IWPRIV_CMD_SET, "ra0", "AuthMode=WPAPSK", NULL);
			iwpriv_func(IWPRIV_CMD_SET, "ra0", "EncrypType=TKIP", NULL);

			memset(acTmpBuf, 0x00, sizeof(acTmpBuf));
			snprintf(acTmpBuf, sizeof(acTmpBuf),"WPAPSK=%s", pcCode);
			iwpriv_func(IWPRIV_CMD_SET, "ra0",  acTmpBuf, NULL);

			memset(acTmpBuf, 0x00, sizeof(acTmpBuf));
			snprintf(acTmpBuf, sizeof(acTmpBuf),"SSID=%s", pcSSID);
			iwpriv_func(IWPRIV_CMD_SET, "ra0",  acTmpBuf, NULL);
			g_nNetIwprivFuncSet = 1;
#endif
            break;

        case WIFI_TYPE_WPAPSK_AES:    //5
        	DEBUGPRINT(DEBUG_INFO, "WIFI_TYPE_WPAPSK_AES, in file:%s line:%d\n", __FILE__, __LINE__);
#ifndef __IWPRIV_H_
			snprintf(acTmpBuf, sizeof(acTmpBuf),
				"iwpriv ra0 set NetworkType=Infra;\
             	iwpriv ra0 set AuthMode=WPAPSK;\
             	iwpriv ra0 set EncrypType=AES;\
             	iwpriv ra0 set SSID=\"%s\";\
             	iwpriv ra0 set WPAPSK=\"%s\";\
				iwpriv ra0 set SSID=\"%s\"",
             	pcSSID, pcCode, pcSSID);
#else
			g_nNetIwprivFuncSet = 0;
			iwpriv_func(IWPRIV_CMD_SET, "ra0", "NetworkType=Infra", NULL);
			iwpriv_func(IWPRIV_CMD_SET, "ra0", "AuthMode=WPAPSK", NULL);
			iwpriv_func(IWPRIV_CMD_SET, "ra0", "EncrypType=AES", NULL);

			memset(acTmpBuf, 0x00, sizeof(acTmpBuf));
			snprintf(acTmpBuf, sizeof(acTmpBuf),"WPAPSK=%s", pcCode);
			iwpriv_func(IWPRIV_CMD_SET, "ra0",  acTmpBuf, NULL);

			memset(acTmpBuf, 0x00, sizeof(acTmpBuf));
			snprintf(acTmpBuf, sizeof(acTmpBuf),"SSID=%s", pcSSID);
			iwpriv_func(IWPRIV_CMD_SET, "ra0",  acTmpBuf, NULL);
			g_nNetIwprivFuncSet = 1;
#endif
            break;

        case WIFI_TYPE_WPA2PSK_TKIP:  // 6
        	DEBUGPRINT(DEBUG_INFO, "WIFI_TYPE_WPA2PSK_TKIP, in file:%s line:%d\n", __FILE__, __LINE__);
#ifndef __IWPRIV_H_
			snprintf(acTmpBuf, sizeof(acTmpBuf),
				"iwpriv ra0 set NetworkType=Infra;\
             	iwpriv ra0 set AuthMode=WPA2PSK;\
             	iwpriv ra0 set EncrypType=TKIP;\
             	iwpriv ra0 set SSID=\"%s\";\
             	iwpriv ra0 set WPAPSK=\"%s\";\
				iwpriv ra0 set SSID=\"%s\"",
             	pcSSID, pcCode, pcSSID);
#else
			g_nNetIwprivFuncSet = 0;
			iwpriv_func(IWPRIV_CMD_SET, "ra0", "NetworkType=Infra", NULL);
			iwpriv_func(IWPRIV_CMD_SET, "ra0", "AuthMode=WPA2PSK", NULL);
			iwpriv_func(IWPRIV_CMD_SET, "ra0", "EncrypType=TKIP", NULL);

			memset(acTmpBuf, 0x00, sizeof(acTmpBuf));
			snprintf(acTmpBuf, sizeof(acTmpBuf),"WPAPSK=%s", pcCode);
			iwpriv_func(IWPRIV_CMD_SET, "ra0",  acTmpBuf, NULL);

			memset(acTmpBuf, 0x00, sizeof(acTmpBuf));
			snprintf(acTmpBuf, sizeof(acTmpBuf),"SSID=%s", pcSSID);
			iwpriv_func(IWPRIV_CMD_SET, "ra0",  acTmpBuf, NULL);
			g_nNetIwprivFuncSet = 1;
#endif
            break;

        case WIFI_TYPE_WPA2PSK_AES:  // 7
        	DEBUGPRINT(DEBUG_INFO, "WIFI_TYPE_WPA2PSK_AES, in file:%s line:%d\n", __FILE__, __LINE__);
#ifndef __IWPRIV_H_
			snprintf(acTmpBuf, sizeof(acTmpBuf),
				"iwpriv ra0 set NetworkType=Infra;\
             	iwpriv ra0 set AuthMode=WPA2PSK;\
             	iwpriv ra0 set EncrypType=AES;\
             	iwpriv ra0 set SSID=\"%s\";\
             	iwpriv ra0 set WPAPSK=\"%s\";\
             	iwpriv ra0 set SSID=\"%s\"",
             	pcSSID, pcCode, pcSSID);
#else
			g_nNetIwprivFuncSet = 0;
			iwpriv_func(IWPRIV_CMD_SET, "ra0", "NetworkType=Infra", NULL);
			iwpriv_func(IWPRIV_CMD_SET, "ra0", "AuthMode=WPA2PSK", NULL);
			iwpriv_func(IWPRIV_CMD_SET, "ra0", "EncrypType=AES", NULL);

			memset(acTmpBuf, 0x00, sizeof(acTmpBuf));
			snprintf(acTmpBuf, sizeof(acTmpBuf),"WPAPSK=%s", pcCode);
			iwpriv_func(IWPRIV_CMD_SET, "ra0",  acTmpBuf, NULL);

			memset(acTmpBuf, 0x00, sizeof(acTmpBuf));
			snprintf(acTmpBuf, sizeof(acTmpBuf),"SSID=%s", pcSSID);
			iwpriv_func(IWPRIV_CMD_SET, "ra0",  acTmpBuf, NULL);
			g_nNetIwprivFuncSet = 1;
#endif
			break;

        default :
        	lRet = EXIT_FAILURE;
        	break;
    }

#ifndef __IWPRIV_H_
    //if (EXIT_SUCCESS == lRet)
    {
    	//system(acTmpBuf);
    	pFile = popen(acTmpBuf, "r");
    	if (NULL != pFile)
		{
			char acTmpBuf[128] = {0};

			fread(acTmpBuf, 1, 128, pFile);
			pclose(pFile);
			pFile = NULL;
		}
    }
#endif

	//gWiFiSetState = WiFi_Set_NetworkType;

    return lRet;
}

/**
  *  @brief      设置网络类型(有线或无线)
  *  @author     <wfb> 
  *  @param[in]  <pstNetSetInfo: 网络设置结构体>
  *  @param[out] <无>
  *  @return     <EXIT_FAILURE EXIT_SUCCESS>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
int set_net_type(netset_info_t *pstNetSetInfo)
{
	int nRet = EXIT_FAILURE;


	if (SET_NET_STATE_READY_SET == g_stNetWorkSetInfo.unNetState)
	{
		/* 从消息队列里取出的 这个不会存在 */
        if (NULL == pstNetSetInfo)
        {
	        DEBUGPRINT(DEBUG_INFO, "INVALID pstNetSetInfo ****************************\n");
	        return nRet;
        }
	}
	else
	{
        return EXIT_SUCCESS;
	}

	do{
		/* 设置网络类型      */
		if (EXIT_SUCCESS != network_set_net_type(pstNetSetInfo, NULL))
		{
			DEBUGPRINT(DEBUG_INFO, " set network_set_net_type failed\n");
		    break;
		}

		nRet = EXIT_SUCCESS;

	}while(0);

	return nRet;
}

/**
  *  @brief      设置ip类型
  *  @author     <wfb> 
  *  @param[in]  <pstNetSetInfo: 网络设置结构体>
  *  @param[out] <无>
  *  @return     <EXIT_FAILURE EXIT_SUCCESS>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
int set_ip_type(netset_info_t *pstNetSetInfo)
{
	int nRet = EXIT_FAILURE;

	/* 有限望络或是无限登录ok均要设置ip */
	if ((SET_NET_STATE_WIFI_LOGIN_OK == g_stNetWorkSetInfo.unNetState) ||
			(SET_NET_STATE_SET_WIRED_TYPE_OK == g_stNetWorkSetInfo.unNetState))
	{
		/* 从消息队列里取出的 这个不会存在 */
        if (NULL == pstNetSetInfo)
        {
	        DEBUGPRINT(DEBUG_INFO, "INVALID pstNetSetInfo ****************************\n");
	        return nRet;
        }
	}
	else
	{
        return EXIT_SUCCESS;
	}

	do{
		start_set_ip_sendmessage_to_child_process(pstNetSetInfo->lIpType);

		/* 设置网络  ip/dhcp */
		if (EXIT_SUCCESS != network_set_ip_type(pstNetSetInfo))
		{
			DEBUGPRINT(DEBUG_INFO, " set network_set_ip_type failed\n");
		    break;
		}

		nRet = EXIT_SUCCESS;

	}while(0);

	return nRet;
}

/**
  *  @brief      检测wifi登录状态 只有成功之后才会向下走
  *  @author     <wfb> 
  *  @param[in]  <pstNetSetInfo: 网络设置结构体>
  *  @param[out] <无>
  *  @return     <EXIT_FAILURE EXIT_SUCCESS>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void check_wifi_login_status()
{
	/* 判断状态 */
	if (SET_NET_STATE_SET_WIFI_OK != g_stNetWorkSetInfo.unNetState)
	{
         return;
	}

	/* 开始检测  每循环一次检测一次 */
	/* 检测网络连接情况 */
	if (TRUE == check_wifi_connect_ok())
	{
		/* 设置状态  */
		g_stNetWorkSetInfo.unNetState = SET_NET_STATE_WIFI_LOGIN_OK;
		DEBUGPRINT(DEBUG_INFO, "check wifi login ok ****************\n");
		g_stNetWorkSetInfo.unLoopCnts = 0;
		g_stNetWorkSetInfo.unWifiLoginStatus = WIFI_LOGIN_SUCCESS;
	}
	else
	{
		if (MAX_WIFI_LOGIN_TIMES == g_stNetWorkSetInfo.unLoopCnts)
		{
            /* 设置新的网络类型 */
			g_stNetWorkSetInfo.unNetState = SET_NET_STATE_READY_SET;
			DEBUGPRINT(DEBUG_INFO, "login not succ this time, try it next time\n");
			//g_stNetWorkSetInfo.unLoopCnts++;
			g_stNetWorkSetInfo.unLoopCnts = 0;

			/* 如果连续四次（一圈）没有登录成功 则通知子进程没有成功 */
			if (4 == g_stNetWorkSetInfo.unWifiEncryTimes)
			{
				send_msg_to_child_wifi_login_state(WIFI_LOGIN_FAILURE);
			}
		}
		else
		{
			g_stNetWorkSetInfo.unLoopCnts++;
			return;
		}
	}
}


#if 0
/*! \fn       void check_net_connect_handler()
 *
 *  \brief    网络检测处理函数
 *
 *  \param
 *
 *  \exception
 *
 *  \return
 *
 *  \b 历史记录:
 *
 *     <作者>           <时间>        <修改记录> <br>
 *     wangfengbo      2013-03-20    创建该函数 <br>
 */
void check_net_connect_handler()
{
    ;
}
#endif

/**
  *  @brief      根据次数获取最新的加密类型
  *  @author     <wfb> 
  *  @param[in]  <nTime: 尝试次数>
  *  @param[out] <无>
  *  @return     <EXIT_FAILURE EXIT_SUCCESS>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
int return_mixed_WifiEntryType_by_times(int nTime)
{
    if (1 == nTime%4)
    {
        return WIFI_TYPE_WPA2PSK_AES;
    }
    else if (2 == nTime%4)
    {
    	return WIFI_TYPE_WPA2PSK_TKIP;
    }
    else if (3 == nTime%4)
    {
    	return WIFI_TYPE_WPAPSK_AES;
    }
    else if (0 == nTime%4)
    {
    	return WIFI_TYPE_WPAPSK_TKIP;
    }
    else
    {
    	return WIFI_TYPE_WPA2PSK_AES;
    }
}

/**
  *  @brief      判断是否需要重新设置网络
  *  @author     <wfb> 
  *  @param[in]  <pstNetSetInfo: 网络结构体>
  *  @param[out] <无>
  *  @return     <EXIT_FAILURE EXIT_SUCCESS>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
int if_need_reset_net(netset_info_t *pstNetSetInfo)
{
	netset_state_t stNetSetState;

	get_cur_net_state(&stNetSetState);

	/* 如果当前正在设置有线  那么则退出 因为不可能设置有线时在发个设置有线 若设置无线则不理 */
	//if ((NET_TYPE_WIRED == g_stNetWorkSetInfo.unNetType) && (SET_NET_STATE_NET_CONNECT_FAILURE != g_stNetWorkSetInfo.unNetState))
	if (NET_TYPE_WIRED == g_stNetWorkSetInfo.unNetType)
	{
		DEBUGPRINT(DEBUG_INFO, "Cur is wired, or is setting wired, so do not need set wifi\n");
		return FALSE;
	}
	else
	{
		DEBUGPRINT(DEBUG_INFO, "need set net\n");
        return TRUE;
	}
}


/**
  *  @brief     发送消息到子线程通知正在设置网络类型
  *             aucTmpBuf[0]:设置网络类型 开始或是结束（上报）
  *             aucTmpBuf[1]:设置网络类型 有线或是无线
  *  @author     <wfb> 
  *  @param[in]  <nNetType: 网络类型>
  *  @param[out] <无>
  *  @return     <EXIT_FAILURE EXIT_SUCCESS>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void start_set_net_type_sendmessage_to_child_process(int nNetType)
{
	unsigned char aucTmpBuf[4] = {0};

	/* 反馈有线或是无线  */
	aucTmpBuf[0] = STEP_SET_START;
	aucTmpBuf[1] = nNetType;

	send_msg_to_child_process(NETWORK_SET_MSG_TYPE,
	    		MSG_NETWORKSET_P_SET_NET_TYPE, aucTmpBuf, 2);
}

/**
  *  @brief      发送消息到子线程通知正在设置网络类型
  *  @author     <wfb> 
  *  @param[in]  <nNetType: 网络类型>
  *  @param[in]  <nSetNetResult: 设置网络结果>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void report_set_net_type_sendmessage_to_child_process(int nNetType, int nSetNetResult)
{
	unsigned char aucTmpBuf[4] = {0};

	/* 反馈有线或是无线  */
	aucTmpBuf[0] = STEP_SET_END;
	aucTmpBuf[1] = nNetType;
	aucTmpBuf[2] = nSetNetResult;

	send_msg_to_child_process(NETWORK_SET_MSG_TYPE,
	    		MSG_NETWORKSET_P_SET_NET_TYPE, aucTmpBuf, 3);
}


/*  发送消息到子线程通知正在设置ip
 *  aucTmpBuf[0]:设置网络类型 开始或是结束（上报）
 *  aucTmpBuf[1]:设置网络类型 动态或是静态
*/
/**
  *  @brief      发送消息到子线程通知正在设置网络类型
  *  @author     <wfb> 
  *  @param[in]  <nNetType: 网络类型>
  *  @param[in]  <nSetNetResult: 设置网络结果>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void start_set_ip_sendmessage_to_child_process(int nIpType)
{
	unsigned char aucTmpBuf[4] = {0};

	/* 反馈动态或是静态  */
	aucTmpBuf[0] = STEP_SET_START;
	aucTmpBuf[1] = nIpType;

	send_msg_to_child_process(NETWORK_SET_MSG_TYPE,
			MSG_NETWORKSET_P_SET_IP_TYPE, aucTmpBuf, 2);
}

/**
  *  @brief      发送消息到子线程通知正在设置网络类型
  *  @author     <wfb> 
  *  @param[in]  <nIpType:      IP类型>
  *  @param[in]  <nSetIpResult: 设置IP结果>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void report_set_ip_sendmessage_to_child_process(int nIpType, int nSetIpResult)
{
	unsigned char aucTmpBuf[4] = {0};

	/* 反馈动态或是静态  */
	aucTmpBuf[0] = STEP_SET_END;
	aucTmpBuf[1] = nIpType;
	aucTmpBuf[2] = nSetIpResult;

	send_msg_to_child_process(NETWORK_SET_MSG_TYPE,
			MSG_NETWORKSET_P_SET_IP_TYPE, aucTmpBuf, 2);
}


/**
  *  @brief      发送消息到子线程通知正在设置网络类型
  *  @author     <wfb> 
  *  @param[in]  <nNetType:      网络类型>
  *  @param[in]  <nIpType:       IP类型>
  *  @param[in]  <nNetResult:    网络连接结果>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void set_net_connect_result_sendmessage_to_child_process(int nNetType, int nIpType, int nNetResult)
{
	unsigned char aucTmpBuf[4] = {0};

	/* 反馈动态或是静态  */
	aucTmpBuf[0] = nNetType;
	aucTmpBuf[1] = nIpType;
	aucTmpBuf[2] = nNetResult;

	send_msg_to_child_process(NETWORK_SET_MSG_TYPE,
			MSG_NETWORKSET_P_REPORT_CONNECT_RESULT, aucTmpBuf, 3);
}

/**
  *  @brief      清理资源
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void clean_net_info_msghandler()
{
	//FILE *pFile = NULL;

	DEBUGPRINT(DEBUG_INFO, "ready to clean net resource\n");
    //system("killall udhcpc");
    //system("killall ping");

#if 0
    pFile = popen("killall udhcpc", "r");
    if (NULL != pFile)
	{
		char acTmpBuf[128] = {0};

		fread(acTmpBuf, 1, 128, pFile);
		pclose(pFile);
		pFile = NULL;
	}
#endif
    /* 关闭  */
    if (NULL != g_stNetWorkSetInfo.pFile)
    {
        pclose(g_stNetWorkSetInfo.pFile);
        g_stNetWorkSetInfo.pFile = NULL;
    }
    if (NULL != g_stNetWorkSetInfo.pTestFile)
    {
        fclose(g_stNetWorkSetInfo.pTestFile);
        g_stNetWorkSetInfo.pTestFile = NULL;
    }
    if (NULL != g_stNetWorkSetInfo.pFileIp)
    {
        pclose(g_stNetWorkSetInfo.pFileIp);
        g_stNetWorkSetInfo.pFileIp = NULL;
    }
    if (NULL != g_stNetWorkSetInfo.pTestFileIp)
    {
    	fclose(g_stNetWorkSetInfo.pTestFileIp);
    	g_stNetWorkSetInfo.pTestFileIp = NULL;
    }

    g_nDhcpFinishTime = 0;
    g_nIntervalDhcpLoop = 0;

    reset_tutk_login_flag();

    reset_tutk_https_set_led_flag();

    clear_net_check_flag();

    clear_reset_net_flag();

    clear_start_dhcp_flag();

    clear_dhcp_ok_flag();

    g_nNetBlockTimes = 0;

    memset(&g_stNetWorkSetInfo, 0x00, sizeof(networkset_into_t));
}

/**
  *  @brief      接受https网络连接状态
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void net_connect_status_msghandler(unsigned int unSubType)
{
	DEBUGPRINT(DEBUG_INFO, "RVC from https unSUBtYPE = %d~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n", unSubType);
	/* 首先判度判断是否接收该消息 */
	if (SET_NET_STATE_SET_IP_SUCCESS == g_stNetWorkSetInfo.unNetState ||
			SET_NET_STATE_NET_CONNECT_FAILURE == g_stNetWorkSetInfo.unNetState)
	{
		/* 收到https的网络连接情况 0代表网路成功 */
		set_net_connect_result_sendmessage_to_child_process(g_stNetWorkSetInfo.unNetType,
			g_stNetWorkSetInfo.unIpType, (0 == unSubType) ? NET_CONNECT_SUCCESS:NET_CONNECT_FAILURE);

		if (0 == unSubType)
		{
			g_stNetWorkSetInfo.unNetState = SET_NET_STATE_NET_CONNECT_SUCCESS;
			DEBUGPRINT(DEBUG_INFO, "unNetState = SET_NET_STATE_NET_CONNECT_SUCCESS\n");
		}
		else
		{
			g_stNetWorkSetInfo.unNetState = SET_NET_STATE_NET_CONNECT_FAILURE;
			DEBUGPRINT(DEBUG_INFO, "unNetState = SET_NET_STATE_NET_CONNECT_FAILURE\n");
		}
	}

}


/**
  *  @brief      接受https网络连接状态
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void net_start_check_msghandler()
{
	if ((GET_IP_SUCCESS == g_stNetWorkSetInfo.unHaveIpStatus))
    {
		DEBUGPRINT(DEBUG_INFO, "net_start_check_msghandler\n");
		set_net_check_flag();
		//设置led
		//net_get_ip_ok_set_led();
    }
	else
	{
		DEBUGPRINT(DEBUG_INFO, "NOT GET IP YET, SO DO NOT START CHECK\n");
	}
}


/**
  *  @brief      发送消息到子进程 网络设置开始
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void send_msg_to_child_start_set_net()
{
	send_msg_to_child_process(NETWORK_SET_MSG_TYPE,
			MSG_NETWORKSET_P_START_SET_NET, NULL, 0);
}

/**
  *  @brief      通知子进程  wifi没有加入成功
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void send_msg_to_child_wifi_login_state(unsigned char ucTmpBuf)
{
	unsigned char aucTmpBuf[4] = {0};

	aucTmpBuf[0] = ucTmpBuf;
	send_msg_to_child_process(NETWORK_SET_MSG_TYPE,
			MSG_NETWORKSET_P_WIFI_LOGIN_STATUS, aucTmpBuf, 1);
}

/**
  *  @brief      wifi 断连通知子进程  0:断开  1：重连
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void wifi_disconnect_to_child_process(unsigned char ucTmpBuf)
{
	unsigned char aucTmpBuf[4] = {0};

	aucTmpBuf[0] = ucTmpBuf;
	send_msg_to_child_process(NETWORK_SET_MSG_TYPE,
			MSG_NETWORKSET_P_WIFI_DISCONNECT, aucTmpBuf, 1);
}

/**
  *  @brief      检测wifi连接是否OK
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
int check_wifi_connect_ok()
{
	int nRet = 0;
	//return TRUE;
	g_nNetConnStatus = 0;
	nRet = iwpriv_connStatus("ra0");
	g_nNetConnStatus = 1;

	return nRet;
#if 0
	if (TRUE == iwpriv_connStatus("ra0"))
	{
		return WIFI_STATUS_RTN_CONNECT;
	}
	else
	{
		return WIFI_STATUS_RTN_NOT_CONNECT;
	}
#endif
#if 0
	char acReadBuf[256] = {0};
	FILE *pfPtr = NULL;

	pfPtr = popen("iwpriv ra0 connStatus", "r");

	if (pfPtr == NULL)
	{
		DEBUGPRINT(DEBUG_INFO, "pfPtr is NULL, error is %d, descrip: %s\n", errno, strerror(errno));
		return WIFI_STATUS_RTN_INVALID;
	}

	thread_usleep(10000);

	//fscanf(fptr,"%[^\n]s", acReadBuf);/*读取一行不带换行符的数据*/
	//fgets(acReadBuf, 256, pfPtr);
	fread(acReadBuf, 1, 256, pfPtr);
	DEBUGPRINT(DEBUG_INFO, "acReadBuf = %s\n", acReadBuf);

	if (0 == strlen(acReadBuf))
	{
		pclose(pfPtr);
		return WIFI_STATUS_RTN_INVALID;
	}

	if (NULL == strstr(acReadBuf, "Connected"))
	{
		DEBUGPRINT(DEBUG_INFO, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~connect not ok ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
		pclose(pfPtr);
		return WIFI_STATUS_RTN_NOT_CONNECT;
	}
	else
	{
		DEBUGPRINT(DEBUG_INFO, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~connect ok ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
		pclose(pfPtr);
		return WIFI_STATUS_RTN_CONNECT;
	}
#endif
}

#if 0
/**
  *  @brief      获取网关
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *              2013-11-14:将该函数移植到network_check里边
  *  @note
*/
int get_gateway()
{
    FILE *fp = NULL;
    unsigned char buf[BUFSIZE] = {0};
    unsigned long route[MAX_ROW] = {0};
    struct sockaddr_in gateway_addr[GATEWAY_NUM];
    unsigned char *ip = NULL;
    char iface[8] = {0};
    int lines = 0;
    int nitems = 0;
    int num = 0;
    struct in_addr gate_addr;

    if (gateway_addr == NULL)
    {
        return -1;
    }

    buf[BUFSIZE-1] = 0;
    fp = fopen("/proc/net/route", "rb");
    if (!fp)
    {
        fprintf(stderr, "Unable to open route table\n");
        return -1;
    }

    memset(gateway_addr, 0x00, sizeof(struct sockaddr_in)*GATEWAY_NUM);
    lines = 0;

    while (1)
    {
        //if (!fscanf(fp, "%[^\n]s", buf))  //fscanf(fptr,"%[^\n]s", acReadBuf)
        if (!fgets(buf, BUFSIZE -1, fp))
        {
            fclose(fp);
            break;
        }

        lines++;
        nitems = sscanf(buf, "%s %lx %lx", iface, route, route+1);

        printf("nitems = %d\n",nitems);

        if (nitems == 3 && lines > 1)
        {
             if (route[GATEWAY_ROW])
             {
                 gate_addr.s_addr = route[GATEWAY_ROW];
                 (gateway_addr + num)->sin_addr = gate_addr;
                 num++;
                 ip = inet_ntoa(gate_addr);
                 strncpy(g_acGateWay, ip, strlen(ip));
                 g_acGateWay[strlen(ip)] = 0;
                 printf("ip = %s\n", ip);
             }
             continue;
         }
     }

     fclose(fp);

     return num;
}
#endif

void test_net_set()
{
    int lRet = EXIT_SUCCESS;
    int lWifiEncryType = 0;
    char acTmpBuf[512] = {0};
    FILE *pFile = NULL;

    //iwpriv_func(IWPRIV_CMD_CONNSTATUS, "ra0", "connStatus", NULL);

	//sleep(2);
#if 0
	iwpriv_connStatus("ra0");


	sleep(2);
	iwconfig_essid_off("ra0");


#if 10
	memset(acTmpBuf, '\0', sizeof(acTmpBuf));

	iwpriv_func(IWPRIV_CMD_SET, "ra0", "NetworkType=Infra", NULL);
	iwpriv_func(IWPRIV_CMD_SET, "ra0", "AuthMode=WPA2PSK", NULL);
	iwpriv_func(IWPRIV_CMD_SET, "ra0", "EncrypType=AES", NULL);

	memset(acTmpBuf, 0x00, sizeof(acTmpBuf));
	snprintf(acTmpBuf, sizeof(acTmpBuf),"WPAPSK=%s", "88888888");
	iwpriv_func(IWPRIV_CMD_SET, "ra0",  acTmpBuf, NULL);

	memset(acTmpBuf, 0x00, sizeof(acTmpBuf));
	snprintf(acTmpBuf, sizeof(acTmpBuf),"SSID=%s", "superfood");
	iwpriv_func(IWPRIV_CMD_SET, "ra0",  acTmpBuf, NULL);
#endif

	sleep(2);

	iwpriv_connStatus("ra0");
#endif
}
