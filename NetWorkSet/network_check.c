/**   @file []
   @brief    网路检测实现
   @note
   @author   wangfengbo
   @date     2013-11－13
   @remarks  增加该文件
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

#include "common/logfile.h"
#include "common/thread.h"
#include "common/common.h"
#include "common/common_func.h"
#include "common/msg_queue.h"
#include "common/utils.h"
#include "network_check.h"


/*******************************************************************************/
/*                              宏定义 结构体区                                       */
/*******************************************************************************/
#define NETWORK_CHECK_THREAD_USLEEP  1000000

/** 获取网关  参数 BUFSIZE */
#define BUFSIZE      186
/** 获取网关  参数 MAX_ROW */
#define MAX_ROW       10
/** 获取网关  参数 GATEWAY_ROW */
#define GATEWAY_ROW    1
/** 获取网关  参数 GATEWAY_NUM */
#define GATEWAY_NUM   10

#define PING_TIME_OUT      4
#define PING_PACKAGE_NUM   3
#define PING_RCV_PACKAGE_MIN_NORMAL_NUM 1

/*******************************************************************************/
/*                                全局变量区                                         */
/*******************************************************************************/
/** 设置网络检测标志锁  */
pthread_mutex_t g_stNetCheckFlagLock = PTHREAD_MUTEX_INITIALIZER;
/** 重设网络锁  */
pthread_mutex_t g_stResetNetFlagLock = PTHREAD_MUTEX_INITIALIZER;
/** 设置开始DHCP  */
pthread_mutex_t g_stStartDhcpFlagLock = PTHREAD_MUTEX_INITIALIZER;
/** 设置Dhcp已经ok */
pthread_mutex_t g_stDhcpOkFlagLock = PTHREAD_MUTEX_INITIALIZER;
/** 网络阻塞标志 */
pthread_mutex_t g_stNetCheckBlockFlagLock = PTHREAD_MUTEX_INITIALIZER;

/** 是否开始检测网络标志 */
volatile int g_nNetCheckFlag;
/** 是否开始重设网络标志 */
volatile int g_nResetNetFlag;
/** 是否开始dhcp */
volatile int g_nStartDhcpFlag;
/** DHCP 已OK */
volatile int g_nDhcpAlreadyOk;
/** 用于检测Network是否有阻塞现象标志 */
volatile int g_nNetCheckBlockFlag;


/* ping 相关信息 */
char g_acGateway[16];

/** dhcp之间的间隔值 这个是从popen到pclose完成的时间 */
unsigned int g_nDhcpFinishTime = 0;


/** 以下变量用于测试  */
unsigned int g_nNetIwprivEssidoff = 1;
unsigned int g_nNetIwprivFuncSet= 1;
unsigned int g_nNetGetWifiSignal = 1;
unsigned int g_nNetConnStatus = 1;
unsigned int g_nPopenFlag = 1;


/*******************************************************************************/
/*                               接口函数声明                                        */
/*******************************************************************************/
void *network_check_thread(void *arg);


/******************************************************************************/
/*                                接口函数                                          */
/******************************************************************************/
/**
  @brief      网络检测线程实现
  @author     <wfb> 
  @param[in]  <无>
  @param[out] <无>
  @return     <无>
  @remark     2013-11-13 增加该函数
  @note
*/
int NetworkCheck_Thread_Creat(void)
{
    int result;

    /*! 创建线程 */
    result = thread_start(network_check_thread);

    return result;
}


/**
  @brief      线程检测初始化函数
  @author     <wfb> 
  @param[in]  <无>
  @param[out] <无>
  @return     <无>
  @remark     2013-11-13 增加该注释
  @note
*/
int network_check_module_init(void)
{
   return 0;
}


/**
  *  @brief      获取网关
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
int get_gateway(char *pacGateWay)
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
                 strncpy(pacGateWay, ip, strlen(ip));
                 *(pacGateWay + strlen(ip)) = 0;
                 printf("gateway ip = %s\n", ip);
             }
             continue;
         }
     }

     fclose(fp);

     return num;
}


/**
  *  @brief      获取ip地址
  *  @author     <wfb> 
  *  @param[in]  <pcIpAddr: IP地址>
  *  @param[in]  <nNetType: 网络类型>
  *  @param[out] <无>
  *  @return     <EXIT_FAILURE EXIT_SUCCESS>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void get_ip_addr(char *pcIpAddr, int nNetType)
{
    int sock_get_ip;
    struct   sockaddr_in *sin;
    struct   ifreq ifr_ip;

    if ((sock_get_ip=socket(AF_INET, SOCK_STREAM, 0)) == -1)

    {
    	DEBUGPRINT(DEBUG_INFO, "socket create failse...GetLocalIp!/n");
         return ;
    }
    memset(&ifr_ip, 0, sizeof(ifr_ip));

    if (NET_TYPE_WIRED == nNetType)
    {
    	DEBUGPRINT(DEBUG_INFO, "func: get ip addr get wired ip\n");
    	strncpy(ifr_ip.ifr_name, "eth2", sizeof(ifr_ip.ifr_name) - 1);
    }
    else
    {
    	DEBUGPRINT(DEBUG_INFO, "func: get ip addr get wifi ip\n");
    	strncpy(ifr_ip.ifr_name, "ra0", sizeof(ifr_ip.ifr_name) - 1);
    }


    if( ioctl( sock_get_ip, SIOCGIFADDR, &ifr_ip) < 0 )
    {
    	 DEBUGPRINT(DEBUG_INFO, "ioctl get ip failed\n");
         *pcIpAddr = 0;
         close( sock_get_ip );
         return ;
    }

    sin = (struct sockaddr_in *)&ifr_ip.ifr_addr;
    memcpy(pcIpAddr, inet_ntoa(sin->sin_addr),16);

    close( sock_get_ip );
}


/**
  @brief network_check_start_dhcp()
  @author wfb
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-11－13 增加该函数
  @note
*/
void network_check_dhcp()
{
	int nRet = -1;
	FILE *pFile = NULL;
	char acSetCmdBuf[64] = {0};
    struct timeval stTimeVal0;
    struct timeval stTimeVal3;

    if (0 != g_nStartDhcpFlag)
    {
    	//清空标志
    	DEBUGPRINT(DEBUG_INFO, "start network_check_dhcp g_nStartDhcpFlag = %d><><><><><>\n",g_nStartDhcpFlag);

    	gettimeofday(&stTimeVal0, NULL);
    	DEBUGPRINT(DEBUG_INFO, "ready udhcpc g_stTimeVal0.tv_sec = %ld~~~~~\n", stTimeVal0.tv_sec);

        //开始dhcp 有线网 无线网
    	if (NET_TYPE_WIRED == g_nStartDhcpFlag)
    	{
    		DEBUGPRINT(DEBUG_INFO, "start  udhcpc -i eth2 <><><><><><><><><><><><><><><>\n");
    		pFile = popen("killall udhcpc; udhcpc -i eth2", "r");
    	}
    	else if (NET_TYPE_WIRELESS == g_nStartDhcpFlag)
    	{
        	if (0 == strlen(g_acWifiIpInfo))
        	{
        		sprintf(acSetCmdBuf, "killall udhcpc;udhcpc -i ra0 &");
        	}
        	else
        	{
        		sprintf(acSetCmdBuf, "killall udhcpc;udhcpc -i ra0 -r %s &", g_acWifiIpInfo);
        	}
        	DEBUGPRINT(DEBUG_INFO, "start  udhcpc %s <><><><><><><><><>\n", acSetCmdBuf);

        	pFile = popen(acSetCmdBuf, "r");
    	}
    	else
    	{
    		DEBUGPRINT(DEBUG_INFO, "wrong g_nStartDhcpFlag <><><><><><><><><>\n");
    	}

    	//清空标志
    	clear_start_dhcp_flag();

    	//关闭popen
    	if (NULL != pFile)
		{
			char acTmpBuf[128] = {0};
			int nLen = 0;
			int nRet = 0;

			//gettimeofday(&stTimeVal1, NULL);
			nLen = fread(acTmpBuf, 1, 64, pFile);
			//gettimeofday(&stTimeVal2, NULL);

			nRet = pclose(pFile);
			gettimeofday(&stTimeVal3, NULL);
			DEBUGPRINT(DEBUG_INFO, "stTimeVal0stTimeVal0stTimeVal0stTimeVal0stTimeVal0 %ld~~~~\n", stTimeVal0.tv_sec);
			DEBUGPRINT(DEBUG_INFO, "stTimeVal3stTimeVal3stTimeVal3stTimeVal3stTimeVal3 %ld~~~~\n", stTimeVal3.tv_sec);

			/* 计算dhcp的时间 */
			g_nDhcpFinishTime = (unsigned int)(stTimeVal3.tv_sec - stTimeVal0.tv_sec);

			DEBUGPRINT(DEBUG_INFO, "DhcpFinishTime DhcpFinishTime DhcpFinishTime %ld~~~~\n", g_nDhcpFinishTime);
		}

    	set_dhcp_ok_flag();
    }
}

/**
  @brief 定期检测网络是否连通
  @author wfb
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-11－13 增加该函数
  @note
*/
void network_check_net_if_still_connect()
{
	stPingInfo stPingInfo;

	strncpy(stPingInfo.acIpAddr, g_acGateway, strlen(g_acGateway));
	stPingInfo.acIpAddr[strlen(g_acGateway)] = 0;
	stPingInfo.nTimeOut = PING_TIME_OUT;
	stPingInfo.nPackageNum = PING_PACKAGE_NUM;



	if (TRUE == g_nNetCheckFlag)
	{
		/* 获取网关ip */
        /* 网络连接成功 */
		DEBUGPRINT(DEBUG_INFO, "ready to ping source func, stPingInfo.acIpAddr = %s\n",stPingInfo.acIpAddr);
		if (PING_RCV_PACKAGE_MIN_NORMAL_NUM <= ping_source_func(&stPingInfo))  //TODO WAITTING
		{
			DEBUGPRINT(DEBUG_INFO, "ping_source_func  returns  success\n");
			clear_net_check_flag();
		}
		/* 网络连接失败 */
		else
		{
			DEBUGPRINT(DEBUG_INFO, "ping_source_func  returns  failure the first time\n");
			// 1. sleep 5 秒
			thread_usleep(5000000);
			// 2. 再重新检测网络，如果仍失败，那么则重新连接
			if (PING_RCV_PACKAGE_MIN_NORMAL_NUM <= ping_source_func(&stPingInfo))
			{
				DEBUGPRINT(DEBUG_INFO, "ping_source_func  returns  success\n");
				clear_net_check_flag();
			}
			else
			{
				DEBUGPRINT(DEBUG_INFO, "ping_source_func  returns  failure the second time,reset net \n");
				set_reset_net_flag();
			}
		}
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
void network_check_clear_block_flag()
{
	clear_net_check_block_flag();
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
void *network_check_thread(void *arg)
{
	  //init
	  network_check_module_init();

	  printf("network check thread ok\n");

	  //sync
	  //thread_synchronization(&rNetWorkCheck, &rChildProcess);


	  //DEBUGPRINT(DEBUG_INFO, "<>--[%s]--<>\n", __FUNCTION__);

	  while(1)
	  {
		    //DEBUGPRINT(DEBUG_INFO, "<>--[%s ]--<>\n", __FUNCTION__);
		    network_check_dhcp();

            network_check_net_if_still_connect();

            network_check_clear_block_flag();
#if 0
            printf("Essidoff:%d, FuncSet:%d, WifiSig:%d, WifiConnStatus:%d g_nPopenFlag:%d\n",\
            		g_nNetIwprivEssidoff, g_nNetIwprivFuncSet, g_nNetGetWifiSignal, g_nNetConnStatus, g_nPopenFlag);
#endif

		    /* 线程切换出去500ms */
		    usleep(NETWORK_CHECK_THREAD_USLEEP);
	  }
}



/**
  @brief set网络检测标志
  @author wfb
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-11－13 增加该函数
  @note
*/
void set_net_check_flag()
{
	pthread_mutex_lock(&g_stNetCheckFlagLock);
	g_nNetCheckFlag = TRUE;
	pthread_mutex_unlock(&g_stNetCheckFlagLock);
}

/**
  @brief clear网络检测标志
  @author wfb
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-11－13 增加该函数
  @note
*/
void clear_net_check_flag()
{
	pthread_mutex_lock(&g_stNetCheckFlagLock);
	g_nNetCheckFlag = FALSE;
	pthread_mutex_unlock(&g_stNetCheckFlagLock);
}


/**
  @brief set重设网络标志
  @author wfb
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-11－13 增加该函数
  @note
*/
void set_reset_net_flag()
{
	pthread_mutex_lock(&g_stResetNetFlagLock);
	g_nResetNetFlag = TRUE;
	pthread_mutex_unlock(&g_stResetNetFlagLock);
}

/**
  @brief clear重设网络标志
  @author wfb
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-11－13 增加该函数
  @note
*/
void clear_reset_net_flag()
{
	pthread_mutex_lock(&g_stResetNetFlagLock);
	g_nResetNetFlag = FALSE;
	pthread_mutex_unlock(&g_stResetNetFlagLock);
}


/**
  @brief set start_dhcp
  @author wfb
  @param[in] int nFlag 高四位
  @param[out] 无
  @return 无
  @remark 2013-11-25 增加该函数
  @note
*/
void set_start_dhcp_flag(int nFlag)
{
	pthread_mutex_lock(&g_stStartDhcpFlagLock);
	g_nStartDhcpFlag = nFlag;
	pthread_mutex_unlock(&g_stStartDhcpFlagLock);
}

/**
  @brief clear start_dhcp
  @author wfb
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-11-25 增加该函数
  @note
*/
void clear_start_dhcp_flag()
{
	pthread_mutex_lock(&g_stStartDhcpFlagLock);
	g_nStartDhcpFlag = FALSE;
	pthread_mutex_unlock(&g_stStartDhcpFlagLock);
}


/**
  @brief set dhcp_ok
  @author wfb
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-11-25增加该函数
  @note
*/
void set_dhcp_ok_flag()
{
	pthread_mutex_lock(&g_stDhcpOkFlagLock);
	g_nDhcpAlreadyOk = TRUE;
	pthread_mutex_unlock(&g_stDhcpOkFlagLock);
}

/**
  @brief clear dhcp_ok
  @author wfb
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-11-25 增加该函数
  @note
*/
void clear_dhcp_ok_flag()
{
	pthread_mutex_lock(&g_stDhcpOkFlagLock);
	g_nDhcpAlreadyOk = FALSE;
	pthread_mutex_unlock(&g_stDhcpOkFlagLock);
}



/**
  @brief set_net_check_block_flag
  @author wfb
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-11-25增加该函数
  @note
*/
void set_net_check_block_flag()
{
	pthread_mutex_lock(&g_stNetCheckBlockFlagLock);
	g_nNetCheckBlockFlag = TRUE;
	pthread_mutex_unlock(&g_stNetCheckBlockFlagLock);
}

/**
  @brief clear_net_check_block_flag
  @author wfb
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-11-25 增加该函数
  @note
*/
void clear_net_check_block_flag()
{
	pthread_mutex_lock(&g_stNetCheckBlockFlagLock);
	g_nNetCheckBlockFlag = FALSE;
	pthread_mutex_unlock(&g_stNetCheckBlockFlagLock);
}

