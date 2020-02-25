/**   @file []
   *  @brief    (1)Flash线程 自动更新
   *  @note
   *  @author   wangfengbo
   *  @date     2013-9-3
   *  @remarks  增加注释
*/
/*******************************************************************************/
/*                                    头文件                                   */
/*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <semaphore.h>
#include <fcntl.h>
#include <termios.h>
#include <linux/stddef.h>

#include "common/thread.h"
#include "common/common.h"
#include "common/msg_queue.h"
#include "common/common_func.h"
#include "common/common_config.h"
#include "common/utils.h"
#include "FlashRW.h"


/*******************************************************************************/
/*                              宏定义 结构体区                                       */
/*******************************************************************************/
#define FLASHRW_MSG_LEN     1024

/** flash线程sleep 时间       */
#define FLASH_RW_THREAD_USLEEP             100000

/** 定期更新间隔 */
#define UPDATA_TIME_INTERVAL               300000000

typedef struct _tag_flash_rw_
{
	int lThreadMsgId;
	int lProcessMsgId;
	char acMsgDataRcv[FLASHRW_MSG_LEN];
	char acMsgDataSnd[FLASHRW_MSG_LEN];
}flash_rw_t;

typedef int (*pFnFlashRwMsgHandler)(MsgData *);


/** 定义消息及其处理函数 */
typedef struct _tag_flash_entry_
{
    int lCmd;
    pFnFlashRwMsgHandler fnFlashRwMsgHandler;

}flash_entry_t;

/** flahs 读写信息 */
typedef struct _tag_flash_wr_info_
{
	int unLoopCnts;
}flash_wr_info_t;


/*******************************************************************************/
/*                               接口函数声明                                        */
/*******************************************************************************/
/** 启动flash_wr线程 */
void *flash_wr_thread(void *arg);
/** io控制模块 */
int flash_wr_module_init(flash_rw_t *pstIoCtl);
/** 消息处理函数 */
void flash_wr_msgHandler(flash_rw_t *pstIoCtl);

void flash_rw_info_init(void);

void updata_flash_per_time_interval(void);

/** 写mac消息处理函数 */
int flash_wr_write_macs_msg_handler(MsgData *pstMsg);
/** 读mac 消息处理函数 */
int flash_wr_read_macs_msg_handler(MsgData *pstMsg);
/** 写配置到flash 消息处理函数 */
int flash_wr_write_config_to_flash_msg_handler(MsgData *pstMsg);
/** 写配置到ram 消息处理函数 */
int flash_wr_write_config_to_ram_msg_handler(MsgData *pstMsg);
/** 读 配置 消息处理函数 */
int flash_wr_read_config_msg_handler(MsgData *pstMsg);
/** 读 pb key 消息处理函数 */
int flash_wr_read_pb_key_msg_handler(MsgData *pstMsg);
/** 写pb key 消息处理函数 */
int flash_wr_write_pb_key_msg_handler(MsgData *pstMsg);
/** 更新ram到flash 消息处理函数 */
int flash_wr_write_updata_config_to_flash_msg_handler(MsgData *pstMsg);



/*******************************************************************************/
/*                                全局变量区                                         */
/*******************************************************************************/
/** Flash 读写 控制 信息  */
flash_wr_info_t g_stFlashWrInfo;

/** flash 消息处理函数的map */
flash_entry_t g_stFlashRwMap[] =
{
	{MSG_FLASHRW_T_WRITE_MACS,              flash_wr_write_macs_msg_handler},

	{MSG_FLASHRW_T_READ_MACS,               flash_wr_read_macs_msg_handler},

	{MSG_FLASHRW_T_WRITE_CONFIG_TO_RAM,     flash_wr_write_config_to_ram_msg_handler},

	{MSG_FLASHRW_T_WRITE_CONFIG_TO_FLASH,   flash_wr_write_config_to_flash_msg_handler},

	{MSG_FLASHRW_T_READ_CONFIG,             flash_wr_read_config_msg_handler},

	{MSG_FLAHSRW_T_READ_PB_KEY,             flash_wr_read_pb_key_msg_handler},

	{MSG_FLAHSRW_T_WRITE_PB_KEY,            flash_wr_write_pb_key_msg_handler},

	{MSG_FLAHSRW_T_UPDATA_CONFIG_TO_FLASH,  flash_wr_write_updata_config_to_flash_msg_handler},
};



/******************************************************************************/
/*                                接口函数                                      */
/******************************************************************************/
/**
  *  @brief      FLASH读写线程创建函数
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <创建线程ID>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
int flash_wr_thread_creat(void)
{
    int result;
	
    /* 创建线程 */
    result = thread_start(flash_wr_thread);
	
    return result;
}

/**
  *  @brief      线程信息初始化
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void flash_rw_info_init(void)
{
    /* 参数初始化 */
	g_stFlashWrInfo.unLoopCnts = 0;

}

/**
  *  @brief      线程初始化
  *  @author     <wfb> 
  *  @param[in]  <pstFlashRW: 线程消息>
  *  @param[out] <无>
  *  @return     <成功返回1>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
int flash_wr_module_init(flash_rw_t *pstFlashRW)
{
	/* 参数检查 */
	if (NULL == pstFlashRW)
	{
		exit(0);
	}

	/* 若不成功 则退出 所以这里不判断 */
    get_process_msg_queue_id(&(pstFlashRW->lThreadMsgId),  &(pstFlashRW->lProcessMsgId));

	/* 参数init */
    flash_rw_info_init();

	printf("comeinto flash module init\n");

	return 1;
}


/**
  *  @brief      线程框架实现
  *  @author     <wfb> 
  *  @param[in]  <pstFlashRW: 线程消息>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void *flash_wr_thread(void *arg)
{
	flash_rw_t *pstFlashRW = (flash_rw_t *)malloc(sizeof(flash_rw_t));

	if (NULL == pstFlashRW)
	{
	  	  printf("[%s] : malloc failed\n", __FUNCTION__);
	}

	/* init */
	flash_wr_module_init(pstFlashRW);
	  
	/* sync */
	thread_synchronization(&rFlashRW, &rChildProcess);
	  
	//DEBUGPRINT(DEBUG_INFO, "<>--[%s]--<>\n", __FUNCTION__);
	printf("flash module sync ok\n");
	  
	while(1)
	{
	    //DEBUGPRINT(DEBUG_INFO, "<>--[%s]--<>\n", __FUNCTION__);
	    //printf("<>--[%s]--<>\n", __FUNCTION__);

	    /* 消息循环处理函数 */
		flash_wr_msgHandler(pstFlashRW);
        //printf("ioctl comeinto  111\n");

		/* 定时写入 */
	    updata_flash_per_time_interval();

	    /* 线程切换出去20ms */
	    thread_usleep(FLASH_RW_THREAD_USLEEP);

	  }	  
	  
	  return 0;
}


/**
  *  @brief      线程框架实现
  *  @author     <wfb> 
  *  @param[in]  <pstFlashRW: 线程消息>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void flash_wr_msgHandler(flash_rw_t *pstFlashRW)
{
	MsgData *pstMsg = (MsgData *)pstFlashRW->acMsgDataRcv;
	int i = 0;

	/* 参数检查 */
	if (NULL == pstFlashRW)
	{
		exit(0);
	}

	/* 处理接收到的消息 */
	if (-1 != msg_queue_rcv(pstFlashRW->lThreadMsgId, pstMsg, FLASHRW_MSG_LEN, FLASH_RW_MSG_TYPE))
	{
        for(i = 0; i < sizeof(g_stFlashRwMap)/sizeof(g_stFlashRwMap[0]); i++)
        {
            if (g_stFlashRwMap[i].lCmd == pstMsg->cmd)
            {
                 g_stFlashRwMap[i].fnFlashRwMsgHandler(pstMsg);
            }
        }

	}

	return;
}


/**
  *  @brief      定时写入flash
  *  @author     <wfb> 
  *  @param[in]  <pstFlashRW: 线程消息>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void updata_flash_per_time_interval()
{
	if (UPDATA_TIME_INTERVAL/FLASH_RW_THREAD_USLEEP == g_stFlashWrInfo.unLoopCnts)
	{
		updata_ram_config_to_flash();
		g_stFlashWrInfo.unLoopCnts = 0;
	}
	else
	{
		g_stFlashWrInfo.unLoopCnts++;
	}
}


/**
  *  @brief      写mac消息处理函数
  *  @author     <wfb> 
  *  @param[in]  <pstMsg: 消息数据>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
int flash_wr_write_macs_msg_handler(MsgData *pstMsg)
{
	int lRet = EXIT_SUCCESS;

	if (EXIT_SUCCESS != set_mac_addr_value((char *)(pstMsg+sizeof(MsgData))))
	{
         lRet = EXIT_FAILURE;
	}

	return 0;
}

/**
  *  @brief      读mac 消息处理函数
  *  @author     <wfb> 
  *  @param[in]  <pstMsg: 消息数据>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
int flash_wr_read_macs_msg_handler(MsgData *pstMsg)
{
	char acTmpBuf[MAX_MSG_LEN] = {0};
	int lMacLen = 0;
	int lRet = EXIT_SUCCESS;
	//int lProcessMsgId = -1;

	if (EXIT_FAILURE == get_mac_addr_value(acTmpBuf, &lMacLen))
	{
        lRet = EXIT_FAILURE;
	}

	/* 发送消息 */
	send_msg_to_child_process(FLASH_RW_MSG_TYPE, MSG_FLASHRW_P_READ_MACS, acTmpBuf, lMacLen);

	return lRet;
}

/**
  *  @brief      写配置到ram 消息处理函数
  *  @author     <wfb> 
  *  @param[in]  <pstMsg: 消息数据>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
int flash_wr_write_config_to_ram_msg_handler(MsgData *pstMsg)
{
	int lItemIndex = -1;
	int lRet = EXIT_SUCCESS;

	lItemIndex = *(char *)(pstMsg + 1);

	/* 长度 减一 因为第一个字节是index */
	if (EXIT_FAILURE == set_config_value_to_ram(lItemIndex, (char *)((char *)(pstMsg + 1) + 1), pstMsg->len -1 ))
	{
         lRet = EXIT_FAILURE;
	}

	return lRet;
}

/**
  *  @brief      写配置到flash 消息处理函数
  *  @author     <wfb> 
  *  @param[in]  <pstMsg: 消息数据>
  *  @param[out] <无>
  *  @return     <EXIT_SUCCESS EXIT_FAILURE>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
int flash_wr_write_config_to_flash_msg_handler(MsgData *pstMsg)
{
	int lItemIndex = -1;
	int lRet = EXIT_SUCCESS;

	lItemIndex = *(char *)(pstMsg + 1);

	/* 长度 减一 因为第一个字节是index */
	if (EXIT_FAILURE == set_config_value_to_flash(lItemIndex, (char *)((char *)(pstMsg + 1) + 1), pstMsg->len -1 ))
	{
	     lRet = EXIT_FAILURE;
	}

    return lRet;
}

/**
  *  @brief      读 配置 消息处理函数
  *  @author     <wfb> 
  *  @param[in]  <pstMsg: 消息数据>
  *  @param[out] <无>
  *  @return     <EXIT_SUCCESS EXIT_FAILURE>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
int flash_wr_read_config_msg_handler(MsgData *pstMsg)
{
	char acTmpBuf[MAX_MSG_LEN] = {0};
	int lConfigItmeLen = 0;
	int lItemIndex = -1;
	int lRet = EXIT_SUCCESS;
	//int lProcessMsgId = -1;

	lItemIndex = *(char *)(pstMsg + 1);


	if (EXIT_FAILURE == get_config_item_value(lItemIndex, acTmpBuf, &lConfigItmeLen))
	{
        lRet = EXIT_FAILURE;
	}

	/* 发送消息 */
	send_msg_to_child_process(FLASH_RW_MSG_TYPE, MSG_FLASHRW_P_READ_CONFIG, acTmpBuf, lConfigItmeLen);

	return lRet;
}

/**
  *  @brief      读 pb key 消息处理函数
  *  @author     <wfb> 
  *  @param[in]  <pstMsg: 消息数据>
  *  @param[out] <无>
  *  @return     <EXIT_SUCCESS EXIT_FAILURE>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
int flash_wr_read_pb_key_msg_handler(MsgData *pstMsg)
{
	char acTmpBuf[MAX_MSG_LEN] = {0};
	int lPbKeyLen = 0;
	int lRet = EXIT_SUCCESS;
	//int lProcessMsgId = -1;

	if (EXIT_FAILURE == get_pb_key_value(acTmpBuf, &lPbKeyLen))
	{
        lRet = EXIT_FAILURE;
	}

	/* 发送消息 */
	send_msg_to_child_process(FLASH_RW_MSG_TYPE, MSG_FLAHSRW_P_READ_PB_KEY, acTmpBuf, lPbKeyLen);

	return lRet;
}

/**
  *  @brief      写pb key 消息处理函数
  *  @author     <wfb> 
  *  @param[in]  <pstMsg: 消息数据>
  *  @param[out] <无>
  *  @return     <0>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
int flash_wr_write_pb_key_msg_handler(MsgData *pstMsg)
{
	int lRet = EXIT_SUCCESS;

	if (EXIT_SUCCESS != set_pb_key_value((char *)(pstMsg+sizeof(MsgData)), pstMsg->len))
	{
         lRet = EXIT_FAILURE;
	}

	return 0;
}

/**
  *  @brief      更新ram到flash 消息处理函数
  *  @author     <wfb> 
  *  @param[in]  <pstMsg: 消息数据>
  *  @param[out] <无>
  *  @return     <0>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
int flash_wr_write_updata_config_to_flash_msg_handler(MsgData *pstMsg)
{
	//updata_ram_config_to_flash();
	fast_updata_config_by_flag(FALSE);
    g_stFlashWrInfo.unLoopCnts = 0;
	return 0;
}
