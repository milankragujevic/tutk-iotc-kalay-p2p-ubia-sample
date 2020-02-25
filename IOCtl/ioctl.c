/**   @file []
   *  @brief    (1)usb以及网口灯插拔监测  (2)状态灯设置 (3)usb切换
   *  @note
   *  @author   wangfengbo
   *  @date     2013-9-2
   *  @remarks  增加注释
*/

/*******************************************************************************/
/*                                    头文件                                         */
/*******************************************************************************/
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

#include "common/thread.h"
#include "common/common.h"
#include "common/common_func.h"
#include "common/msg_queue.h"
#include "common/utils.h"
#include "common/pdatatype.h"
#include "common/common_config.h"
#include "common/logfile.h"
#include "NewsChannel/NewsUtils.h"
#include "Hardware/hardware.h"
#include "ioctl.h"
#include "MFI/MFI.h"



/*******************************************************************************/
/*                              宏定义 结构体区                                       */
/*******************************************************************************/
/** 定义IO线程通信的长度 */
#define IOCTL_MSG_LEN                512

/** 指示灯快闪频率 */
#define LED_FAST_FLICKER             4

/** 指示灯慢闪频率 */
#define LED_SLOW_FLICKER             1

/** 状态变化连续次数 */
#define MONITOR_STATE_CONTINUE_TIMES 3

/** 监测时间间隔 */
#define MONITOR_TIME_INTERVAL            20000

/** usb插入检测硬件响应时间 */
#define USB_CHECK_POWER_RESP_TIME        500000

/** 关闭电源到切换的时间 */
#define USB_POWEROFF_TO_SWITCH_TIME      200000

/** 关闭电源到切换的时间 */
#define USB_SWITCH_TO_POWERON_TIME       200000

/** 打开电源到重新检测状态时间 */
#define USB_CHECK_RESPONSE_LATENCY_TIME  1000000





/** USB抗插把时间 5秒 */
#define USB_RESIST_FAST_SWITCH_INTERVAL   5000000

/** i2c设备 */
#define RT5350_I2C_DEVICE_PATH       "/dev/i2cM0"


/** IO线程sleep 时间       */
#define IO_SEPARATE_SLEEP_PER_10     1
#ifdef  IO_SEPARATE_SLEEP_PER_10
#define IO_THREAD_USLEEP             30000
#else
#define IO_THREAD_USLEEP             20000
#endif

/** IO进程状态 */
enum
{
	IOCTL_THREAD_STATE_RUN= 0,       ///<IO线程正常运行
	IOCTL_THREAD_STATE_SUSPEND,      ///<IO线程挂起，只接受消息，不执行动作
};


/** 指示灯状态  */
enum
{
	LED_STATE_INVALID = 0,          ///<将电源开关打开无效状态
	LED_STATE_ON,                   ///<将电源开关打开指示灯亮
	LED_STATE_OFF,                  ///<将电源开关打开指示灯灭
};

/** USB 的切换过程  */
enum
{
	SWITCH_USB_STATE_INVALID = 0,
	SWITCH_TO_MFI_START,
	SWITCH_TO_MFI_STATE_POWEROFF_USB,  ///<将电源开关打开
	SWITCH_TO_MFI_STATE_SWITCH_USB,    ///<切换到usb
	SWITCH_TO_MFI_STATE_POWERON_USB,   ///<将电源开关闭合

	SWITCH_TO_CAM_START,
	SWITCH_TO_CAM_STATE_POWEROFF_USB,  ///<将电源开关打开
	SWITCH_TO_CAM_STATE_SWITCH_USB,    ///<切换到cam
	SWITCH_TO_CAM_STATE_POWERON_USB,   ///<将电源开关打开

};

/** 定义当前usb连接的设备类型 */
enum
{
    USB_LINK_TO_CAM = 1,       ///<usb 切换在cam端
    USB_LINK_TO_MFI,           ///<usb 切换在mfi端
};

/** usb监测模式 */
enum
{
	USB_MONITOR_MODE_IO = 1,   ///< 监测usb方式：IO口
	USB_MONITOR_MODE_FILE,     ///< 检测usb方式：文件
};

/** ioctl 线程状态 ： run 或是 suspend，当suspend时，只接收消息但是不执行 这些消息都被记录起来，当run后开始执行  */
/** IOCTL线程状态 结构体 */
typedef struct _tag_io_thread_state_
{
    unsigned int unThreadState;  ///< 线程状态
}io_thread_state_t;

/** */
/** 指示灯结构体 */
typedef struct _tag_led_control_info_
{
	unsigned int unCtlTypeOld;                   ///<指示灯控制状态   开 关 快闪 慢闪
    unsigned int unCtlTypeNew;                   ///<指示灯控制状态   开 关 快闪 慢闪
    unsigned int unState;                        ///<指示灯当前状态   亮  不亮
    unsigned int unLedType;                      ///<指示灯类型  是网络灯 电源灯 或是网络电源灯 unColorType
    unsigned int unLedColor;                     ///<指示灯类型  这个目前只针对网络电源灯有效（3种颜色）
    unsigned int unLoopCnts;                     ///<循环次数 用来计时用
    unsigned int unLedOnCircleCnts;              ///<LED on  半个周期循环次数
    unsigned int unLedOffCircleCnts;             ///<LED off 半个周期循环次数

}led_control_info_t;


/** usb切换  */
typedef struct _tag_usb_switch_info_
{
	unsigned int unUsbSwitchState;      ///< usb切换状态 阶段
	unsigned int unUsbCurLinker;        ///< usb当前切换到的方向（cam或是mfi）
	unsigned int unLoopCnts;            ///< usb切换计数器
	unsigned int unUsbPowerState;       ///< usb电源开关状态
}usb_switch_info_t;

/** 监测usb口变化 */
typedef struct _tag_usb_port_monitor_
{
	unsigned int unPortState;           ///< usb端口状态（有usb设备插入或没有）
	unsigned int unLoopCnts;            ///< usb端口监测计数器
	unsigned int unContinuCnts;         ///< usb连续变化次数
	unsigned int unStopMonitorFlag;     ///< 停止检测usb端口变化
	unsigned int unUsbMonitorMode;      ///< usb检测模式  通过文件或是io口
}usb_port_monitor_t;

/** usb端口检测 */
typedef struct _tag_check_usb_state
{
	unsigned int unCheckUsbFlag;        ///< 是否检测usb端口
	unsigned int unSenderType;          ///< 记录向io发送检测消息的id
}check_usb_state_t;

/** 网络端口监测 */
typedef struct _tag_net_port_monitor_
{
	unsigned int unPortState;           ///< 网口状态
	unsigned int unLoopCnts;            ///< 网络端口监测计数器
	unsigned int unContinuCnts;         ///< 网口状态发生改变 连续变化次数
	unsigned int unWifiLoopCnts;        ///< WIFI计数器
	unsigned int unSetWifiFlag;         ///< 是否要设置WIFI
}net_port_monitor_t;

/** 指示灯控制(正常工作或是关闭) */
typedef struct _tag_led_work_flag_
{
    unsigned int unNetLedWorkFlag;      ///< 网络灯是否工作标志
    unsigned int unPowerLedWorkFlag;    ///< 电源灯是否工作标志
    unsigned int unNightLedWorkFlag;    ///< 夜视灯是否工作标志
    unsigned int unNetPowerLedWorkFlag; ///< 网络电源指示灯是否工作标志
    unsigned int unNightModeFlag;       ///< 是否处于夜间模式状态
    unsigned int unNetConnectState;     ///< 网络连接状态
    unsigned int unBatteryPowerState;   ///< 电池状态
}led_state_control_t;

/** IO 口控制 信息 */
typedef struct _tag_ioctl_info_
{
	io_thread_state_t   stIoCtlThreadState;          ///< IO线程状态
	usb_switch_info_t   stUsbSwitchInfo;             ///< USB切换信息
	usb_port_monitor_t  stUsbPortMonitorInfo;        ///< USB端口监测信息
	net_port_monitor_t  stNetPortMonitorInfo;        ///< 网络端口监测信息
	check_usb_state_t   stCheckUsbState;             ///< 检测USB端口状态（有无插入usb设备）
	led_state_control_t stLedStateControl;           ///< LED指示灯控制信息
	led_control_info_t  stPowerLedControlInfo;       ///< M3S电源灯控制信息
	led_control_info_t  stNetLedControlInfo;         ///< M3S网络灯控制信息
	led_control_info_t  stNightLedControlInfo;       ///< M2／M3S夜视灯控制信息
	led_control_info_t  stNetPowerLedControlInfo;    ///< M2指示灯控制信息
}ioctl_info_t;


/** IO线程相关消息 */
typedef struct _tag_ioctl_
{
	int threadMsgId;                            ///< 线程消息队列ID
	int processMsgId;                           ///< 进程消息队列ID
	char acMsgDataRcv[IOCTL_MSG_LEN];           ///< 接收消息数据
	char acMsgDataSnd[IOCTL_MSG_LEN];           ///< 发送消息数据
}io_ctl_t;


/*******************************************************************************/
/*                                全局变量区                                         */
/*******************************************************************************/
/** I/O 控制 信息  */
ioctl_info_t g_stIoCtlInfo;
int ioctl_getLedStatus(int *status) {
	*status = g_stIoCtlInfo.stNetLedControlInfo.unCtlTypeNew;
	*(status + 1) = g_stIoCtlInfo.stNetPowerLedControlInfo.unCtlTypeNew;
	return 0;
}


/*******************************************************************************/
/*                               接口函数声明                                        */
/*******************************************************************************/
/* 启动ioctl线程 */
void *ioctl_thread(void *arg);
/* io控制模块 */
int ioctl_module_init(io_ctl_t *pstIoCtl);
/* 消息处理函数 */
void ioctl_msgHandler(io_ctl_t *pstIoCtl);
/* 监测USB状态 */
void ioctl_usb_port_monitor(io_ctl_t *pstIoCtl);
/* 监测网口状态 */
void ioctl_net_port_monitor(io_ctl_t *pstIoCtl);
/* 指示灯状态控制函数 */
void ioctl_led_control(void);
/* usb切换控制函数 */
void ioctl_usb_switch_control(void);
/* i2c 操作 */
int ioctl_i2c(char *pstData, unsigned int unLen);
/* 获取cp认证消息 */
int get_cp_authentication_info(int lFd, char *pcBuf, int ucDataLen);
/* 检测usb端口状态 */
void ioctl_check_usb_port(io_ctl_t *pstIoCtl);
/* 获取usb端口状态 */
int get_usb_port_state(void);
/* 根据io获取 */
int get_usb_port_state_by_io(void);
/* 根据文件获取 */
int get_usb_port_state_by_file(void);
/* 获取该闪等状态的周期时间  */
void get_blink_time(int nLedCtlType, int *pnLedOnTimes, int *pnLedOffTimes);
/* 判断是否需要闪烁  */
int led_ctl_type_if_need_flk(int nLedCtlType);
void kernel_must_reboot_msghandler();

int ioctl_set_led(unsigned int unLedType, unsigned int unLedColor, unsigned int unLedState);
int ioctl_set_net_led(unsigned int unLedState);
int ioctl_set_power_led(unsigned int unLedState);
int ioctl_set_night_led(unsigned int unLedState);
int ioctl_set_net_power_led(unsigned int unLedColor, unsigned int unLedState);
int usb_switch_to_mfi(void);
int usb_switch_to_cam(void);
int poweron_usb(void);
int poweroff_usb(void);
/******************************************************************************/
/*                                接口函数                                          */
/******************************************************************************/
/**
  *  @brief     IO线程创建函数
  *  @author    <wfb> 
  *  @param[in] <无>
  *  @param[out]<无>
  *  @return    <线程ID号>
  *  @remark    2013-09-02 增加该注释
  *  @note
*/
int ioctl_thread_creat(void) 
{
    int result;
	
    /**> 创建线程 */
    result = thread_start(ioctl_thread);
	
    return result;
}

/**
  *  @brief     led初始化m2
  *  @author    <wfb> 
  *  @param[in] <无>
  *  @param[out]<无>
  *  @return    <无>
  *  @remark    2013-09-02 增加该注释
  *  @note
*/
void ioctl_led_init_m2(void)
{
	int nNightLedState = LED_STATE_INVALID;
	/* M2指示灯 */
	if (EXIT_SUCCESS != ioctl_set_net_power_led(LED_COLOR_TYPE_GREEN, LED_STATE_ON))
	{
		thread_usleep(500000);
		ioctl_set_net_power_led(LED_COLOR_TYPE_GREEN, LED_STATE_ON);
	}

	/* 夜视灯 */
	if (LED_FLAG_NOT_WORK == g_stIoCtlInfo.stLedStateControl.unNightLedWorkFlag)
	{
		nNightLedState = LED_STATE_OFF;
	}
	else
	{
		nNightLedState = LED_STATE_ON;
	}

	if (EXIT_SUCCESS != ioctl_set_night_led(nNightLedState))
	{
		thread_usleep(500000);
		ioctl_set_night_led(nNightLedState);
	}
}

/**
  *  @brief     led初始化m3s
  *  @author    <wfb> 
  *  @param[in] <无>
  *  @param[out]<无>
  *  @return    <无>
  *  @remark    2013-09-02 增加该注释
  *  @note
*/
void ioctl_led_init_m3s(void)
{
	int nPowerLedState = LED_STATE_INVALID;
	int nNightLedState = LED_STATE_INVALID;

	/* 电源灯的初始状态要根据flash */
	if (LED_FLAG_NOT_WORK == g_stIoCtlInfo.stLedStateControl.unPowerLedWorkFlag)
	{
		nPowerLedState = LED_STATE_OFF;
	}
	else
	{
		nPowerLedState = LED_STATE_ON;
	}

	/* 电源灯 */
	if (EXIT_SUCCESS != ioctl_set_power_led(nPowerLedState))
	{
		thread_usleep(500000);
		ioctl_set_power_led(nPowerLedState);
	}

	/* 夜视灯 */
	if (LED_FLAG_NOT_WORK == g_stIoCtlInfo.stLedStateControl.unNightLedWorkFlag)
	{
		nNightLedState = LED_STATE_OFF;
	}
	else
	{
		nNightLedState = LED_STATE_ON;
	}

	if (EXIT_SUCCESS != ioctl_set_night_led(nNightLedState))
	{
		thread_usleep(500000);
		ioctl_set_night_led(nNightLedState);
	}

	/* 网络灯 */
	if (EXIT_SUCCESS != ioctl_set_net_led(LED_STATE_ON))
	{
		thread_usleep(500000);
		ioctl_set_net_led(LED_STATE_ON);
	}
}

/**
  *  @brief     led初始化
  *  @author    <wfb> 
  *  @param[in] <无>
  *  @param[out]<无>
  *  @return    <无>
  *  @remark    2013-09-02 增加该注释
  *  @note
*/
void ioctl_led_init(void)
{
	/* 这里判断是M3S 还是M2 */
	if (DEVICE_TYPE_M3S == g_nDeviceType)
	{
		ioctl_led_init_m3s();
	}
	else if (DEVICE_TYPE_M2 == g_nDeviceType)
	{
		ioctl_led_init_m2();
	}
	else
	{
		DEBUGPRINT(DEBUG_INFO, "ioctl_led_init wrong type\n");
	}
}


/**
  @brief     初始化M2指示灯参数
  @author    <wfb> 
  @param[in] <无>
  @param[out]<无>
  @return    <无>
  @remark    2014-02-27 增加该函数
  @note
*/
void ioctl_info_init_led_m2(void)
{
	char acTmpBuf[8] = {0};
	int nBufLen = 0;


	g_stIoCtlInfo.stNetPowerLedControlInfo.unState = 0;
	if (INFO_FACTORY_MODE == appInfo.cameraMode)
	{
		g_stIoCtlInfo.stNetPowerLedControlInfo.unCtlTypeOld = LED_CTL_TYPE_BLINK;
		g_stIoCtlInfo.stNetPowerLedControlInfo.unCtlTypeNew = LED_CTL_TYPE_BLINK;
	}
	else
	{
		g_stIoCtlInfo.stNetPowerLedControlInfo.unCtlTypeOld = LED_CTL_TYPE_ON;
		g_stIoCtlInfo.stNetPowerLedControlInfo.unCtlTypeNew = LED_CTL_TYPE_FAST_FLK;
	}

	g_stIoCtlInfo.stNetPowerLedControlInfo.unLedType = LED_TYPE_NET_POWER;
	g_stIoCtlInfo.stNetPowerLedControlInfo.unLedColor = LED_COLOR_TYPE_GREEN;
	g_stIoCtlInfo.stNetPowerLedControlInfo.unLoopCnts = 0;

	g_stIoCtlInfo.stNightLedControlInfo.unState = 0;
	g_stIoCtlInfo.stNightLedControlInfo.unCtlTypeOld = LED_CTL_TYPE_ON;
	g_stIoCtlInfo.stNightLedControlInfo.unCtlTypeNew = LED_CTL_TYPE_ON;
	g_stIoCtlInfo.stNightLedControlInfo.unLedType = LED_TYPE_NIGHT;
	g_stIoCtlInfo.stNightLedControlInfo.unLoopCnts = 0;

	/* M2是使用网络灯作为是否工作的  */
	/* 通过flash获取是否工作work Flag */
	get_config_item_value(CONFIG_LIGHT_NET,      acTmpBuf,     &nBufLen);
	if (!strncmp(acTmpBuf, "n", 1))
	{
		g_stIoCtlInfo.stLedStateControl.unNetPowerLedWorkFlag =  LED_FLAG_NOT_WORK;
	}
	else
	{
		g_stIoCtlInfo.stLedStateControl.unNetPowerLedWorkFlag = LED_FLAG_WORK;
	}

	/* 夜视灯 */
	get_config_item_value(CONFIG_LIGHT_NIGHT,     acTmpBuf,     &nBufLen);
	if (!strncmp(acTmpBuf, "n", 1))
	{
		g_stIoCtlInfo.stLedStateControl.unNightLedWorkFlag =  LED_FLAG_NOT_WORK;
	}
	else
	{
		g_stIoCtlInfo.stLedStateControl.unNightLedWorkFlag = LED_FLAG_WORK;
	}

	g_stIoCtlInfo.stLedStateControl.unNetConnectState = NET_STATE_NO_IP;
	g_stIoCtlInfo.stLedStateControl.unBatteryPowerState = BATTERY_POWER_STATE_NORMAL;
	g_stIoCtlInfo.stLedStateControl.unNightModeFlag = LED_NOT_NIGHT_MODE;
}


/**
  @brief     初始化M3S指示灯参数
  @author    <wfb> 
  @param[in] <无>
  @param[out]<无>
  @return    <无>
  @remark    2014-02-27 增加该函数
  @note
*/
void ioctl_info_init_led_m3s(void)
{
	char acTmpBuf[8] = {0};
	int nBufLen = 0;

	/* 通过flash获取是否工作work Flag */
	get_config_item_value(CONFIG_LIGHT_NET,      acTmpBuf,     &nBufLen);
	if (!strncmp(acTmpBuf, "n", 1))
	{
		g_stIoCtlInfo.stLedStateControl.unNetLedWorkFlag =  LED_FLAG_NOT_WORK;
	}
	else
	{
		g_stIoCtlInfo.stLedStateControl.unNetLedWorkFlag = LED_FLAG_WORK;
	}

	get_config_item_value(CONFIG_LIGHT_POWER,    acTmpBuf,     &nBufLen);
	if (!strncmp(acTmpBuf, "n", 1))
	{
		g_stIoCtlInfo.stLedStateControl.unPowerLedWorkFlag =  LED_FLAG_NOT_WORK;
	}
	else
	{
		g_stIoCtlInfo.stLedStateControl.unPowerLedWorkFlag = LED_FLAG_WORK;
	}

	/* 夜视灯 */
	get_config_item_value(CONFIG_LIGHT_NIGHT,     acTmpBuf,     &nBufLen);
	if (!strncmp(acTmpBuf, "n", 1))
	{
		g_stIoCtlInfo.stLedStateControl.unNightLedWorkFlag =  LED_FLAG_NOT_WORK;
	}
	else
	{
		g_stIoCtlInfo.stLedStateControl.unNightLedWorkFlag = LED_FLAG_WORK;
	}

	g_stIoCtlInfo.stLedStateControl.unNetConnectState = NET_STATE_NO_IP;
	g_stIoCtlInfo.stLedStateControl.unBatteryPowerState = BATTERY_POWER_STATE_NORMAL;
	g_stIoCtlInfo.stLedStateControl.unNightModeFlag = LED_NOT_NIGHT_MODE;


	/* 初始化指示灯状态 */
	g_stIoCtlInfo.stPowerLedControlInfo.unState = 0;
	if (INFO_FACTORY_MODE == appInfo.cameraMode)
	{
		g_stIoCtlInfo.stPowerLedControlInfo.unCtlTypeOld = LED_CTL_TYPE_BLINK;
		g_stIoCtlInfo.stPowerLedControlInfo.unCtlTypeNew = LED_CTL_TYPE_BLINK;
	}
	else
	{
		/* */
		if (LED_FLAG_NOT_WORK == g_stIoCtlInfo.stLedStateControl.unPowerLedWorkFlag)
		{
			g_stIoCtlInfo.stPowerLedControlInfo.unCtlTypeOld = LED_CTL_TYPE_OFF;
			g_stIoCtlInfo.stPowerLedControlInfo.unCtlTypeNew = LED_CTL_TYPE_OFF;
		}
		else
		{
			g_stIoCtlInfo.stPowerLedControlInfo.unCtlTypeOld = LED_CTL_TYPE_ON;
			g_stIoCtlInfo.stPowerLedControlInfo.unCtlTypeNew = LED_CTL_TYPE_ON;
		}
	}

	g_stIoCtlInfo.stPowerLedControlInfo.unLedType = LED_TYPE_POWER;
	g_stIoCtlInfo.stPowerLedControlInfo.unLoopCnts = 0;

	g_stIoCtlInfo.stNetLedControlInfo.unState = 0;
	if (INFO_FACTORY_MODE == appInfo.cameraMode)
	{
		g_stIoCtlInfo.stNetLedControlInfo.unCtlTypeOld = LED_CTL_TYPE_BLINK;
		g_stIoCtlInfo.stNetLedControlInfo.unCtlTypeNew = LED_CTL_TYPE_BLINK;
	}
	else
	{
		g_stIoCtlInfo.stNetLedControlInfo.unCtlTypeOld = LED_CTL_TYPE_INVALID;
		g_stIoCtlInfo.stNetLedControlInfo.unCtlTypeNew = LED_CTL_TYPE_FAST_FLK;
	}

	g_stIoCtlInfo.stNetLedControlInfo.unLedType = LED_TYPE_NET;
	g_stIoCtlInfo.stNetLedControlInfo.unLoopCnts = 0;


	g_stIoCtlInfo.stNightLedControlInfo.unState = 0;
	if (LED_FLAG_NOT_WORK == g_stIoCtlInfo.stLedStateControl.unNightLedWorkFlag)
	{
		g_stIoCtlInfo.stNightLedControlInfo.unCtlTypeOld = LED_CTL_TYPE_OFF;
		g_stIoCtlInfo.stNightLedControlInfo.unCtlTypeNew = LED_CTL_TYPE_OFF;
	}
	else
	{
		g_stIoCtlInfo.stNightLedControlInfo.unCtlTypeOld = LED_CTL_TYPE_ON;
		g_stIoCtlInfo.stNightLedControlInfo.unCtlTypeNew = LED_CTL_TYPE_ON;
	}


	g_stIoCtlInfo.stNightLedControlInfo.unLedType = LED_TYPE_NIGHT;
	g_stIoCtlInfo.stNightLedControlInfo.unLoopCnts = 0;



}

/**
  @brief     线程信息函数
  @author    <wfb> 
  @param[in] <无>
  @param[out]<无>
  @return    <无>
  @remark    2013-09-02 增加该注释
  @note
*/
void ioctl_info_init_led(void)
{
	if (DEVICE_TYPE_M3S == g_nDeviceType)
	{
		ioctl_info_init_led_m3s();
	}
	else if (DEVICE_TYPE_M2 == g_nDeviceType)
	{
		ioctl_info_init_led_m2();
	}
}


/**
  @brief     初始化端口检测 网络及usb
  @author    <wfb> 
  @param[in] <无>
  @param[out]<无>
  @return    <无>
  @remark    2013-09-02 增加该注释
  @note
*/
void ioctl_info_init_port_monitor(void)
{
	g_stIoCtlInfo.stNetPortMonitorInfo.unPortState = PORT_STATE_NOT_CONNECT;
	g_stIoCtlInfo.stNetPortMonitorInfo.unContinuCnts = 0;
	g_stIoCtlInfo.stNetPortMonitorInfo.unLoopCnts = 0;
	g_stIoCtlInfo.stNetPortMonitorInfo.unWifiLoopCnts = 0;
	g_stIoCtlInfo.stNetPortMonitorInfo.unSetWifiFlag = TRUE;

	g_stIoCtlInfo.stUsbPortMonitorInfo.unPortState = PORT_STATE_NOT_CONNECT;
	g_stIoCtlInfo.stUsbPortMonitorInfo.unContinuCnts = 0;
	g_stIoCtlInfo.stUsbPortMonitorInfo.unLoopCnts = 0;
	g_stIoCtlInfo.stUsbPortMonitorInfo.unStopMonitorFlag = USB_PORT_START_MONITOR;
	g_stIoCtlInfo.stUsbPortMonitorInfo.unUsbMonitorMode = USB_MONITOR_MODE_IO;
}

/**
  @brief     初始化端口检测 网络及usb
  @author    <wfb> 
  @param[in] <无>
  @param[out]<无>
  @return    <无>
  @remark    2013-09-02 增加该注释
  @note
*/
void ioctl_info_init_usb_switch(void)
{
	g_stIoCtlInfo.stUsbSwitchInfo.unLoopCnts = 0;
	g_stIoCtlInfo.stUsbSwitchInfo.unUsbCurLinker = USB_LINK_TO_CAM;
	g_stIoCtlInfo.stUsbSwitchInfo.unUsbSwitchState = SWITCH_USB_STATE_INVALID;

	/* 第一次检测usb状态时 将电源打开 */
	g_stIoCtlInfo.stUsbSwitchInfo.unUsbPowerState = USB_POWER_STATE_OFF;
}

/**
  @brief     线程信息函数
  @author    <wfb> 
  @param[in] <无>
  @param[out]<无>
  @return    <无>
  @remark    2013-09-02 增加该注释
  @note
*/
void ioctl_info_init(void)
{
	ioctl_info_init_led();

	ioctl_info_init_port_monitor();

	ioctl_info_init_usb_switch();

	/* 初始化usb检测flag为否 */
	g_stIoCtlInfo.stCheckUsbState.unCheckUsbFlag = FALSE;

	/* IO初始状态为run */
	g_stIoCtlInfo.stIoCtlThreadState.unThreadState = IOCTL_THREAD_STATE_RUN;


}

/**
  *  @brief     线程初始化函数
  *  @author    <wfb> 
  *  @param[in] <pstIoCtl IO线程结构体>
  *  @param[out]<无>
  *  @return    <无>
  *  @remark    2013-09-02 增加该注释
  *  @note
*/
int ioctl_module_init(io_ctl_t *pstIoCtl)
{
	/* 参数检查 */
	if (NULL == pstIoCtl)
	{
		exit(0);
	}

	/* 若不成功 则退出 所以这里不判断 */
    get_process_msg_queue_id(&(pstIoCtl->threadMsgId),  &(pstIoCtl->processMsgId));

	/* 参数init */
	ioctl_info_init();

	/* led初始化 */
	ioctl_led_init();

	//kernel_must_reboot_msghandler();

	DEBUGPRINT(DEBUG_INFO, "comeinto ioctl module init&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");

	return 1;
}

/**
  *  @brief     IO线程主函数
  *  @author    <wfb> 
  *  @param[in] <pstIoCtl IO线程结构体>
  *  @param[out]<无>
  *  @return    <无>
  *  @remark    2013-09-02 增加该注释
  *  @note
*/
void *ioctl_thread(void *arg)
{
	io_ctl_t *pstIoCtl = (io_ctl_t *)malloc(sizeof(io_ctl_t));
	  
	if (NULL == pstIoCtl)
	{
	  	  //printf("[%s] : malloc failed\n", __FUNCTION__, );
	}
	  
	/* init */
	if(-1 == ioctl_module_init(pstIoCtl))
	{
        return NULL;
	}
	  
	/* sync */
	thread_synchronization(&rIoCtl, &rChildProcess);
	  
	/* DEBUGPRINT(DEBUG_INFO, "<>--[%s]--<>\n", __FUNCTION__); */
	DEBUGPRINT(DEBUG_INFO, "io module sync ok\n");

	/* 此时io不急于开启，在这里切出去2s，考虑开启音视频 add by wfb, 2013-04-15 */
	thread_usleep(2000000);
	  
	while(1)
	{
	    //DEBUGPRINT(DEBUG_INFO, "<>--[%s]--<>\n", __FUNCTION__);
	    //printf("<>--[%s]--<>\n", __FUNCTION__);

	    /* 消息循环处理函数 */
        ioctl_msgHandler(pstIoCtl);

        /* 若线程处于suspend状态 则直接休眠 */
        if (IOCTL_THREAD_STATE_SUSPEND == g_stIoCtlInfo.stIoCtlThreadState.unThreadState)
        {
        	thread_usleep(IO_THREAD_USLEEP);
        	continue;
        }
        //printf("ioctl comeinto  000\n");
        /* 检测是否有usb插入 */
        ioctl_check_usb_port(pstIoCtl);

        //printf("ioctl comeinto  111\n");
	    /* 监测usb口状态 */
	    ioctl_usb_port_monitor(pstIoCtl);
	    //printf("ioctl comeinto  222\n");
#ifdef  IO_SEPARATE_SLEEP_PER_10
        /* 线程切换10ms */
	    thread_usleep(10000);
#endif

	    /* 监测网口状态 */
        ioctl_net_port_monitor(pstIoCtl);
        //printf("ioctl comeinto  333\n");

#ifdef  IO_SEPARATE_SLEEP_PER_10
        /* 线程切换10ms */
	    thread_usleep(10000);
#endif

	    /* 指示灯状态控制函数 */
	    ioctl_led_control();
	    //printf("ioctl comeinto  444\n");

#ifdef  IO_SEPARATE_SLEEP_PER_10
        /**> 线程切换10ms */
	    thread_usleep(10000);
#endif
		    
	    /* usb切换控制函数 */
	    ioctl_usb_detect_hadler();
	    ioctl_usb_switch_control();
	    //printf("ioctl comeinto  555\n");

	    //gettimeofday(&g_stTimeVal, NULL);
	    //printf("IO THREAD g_stTimeVal.tv_sec = %ld,g_stTimeVal.tv_usec = %ld\n",
	    		//g_stTimeVal.tv_sec, g_stTimeVal.tv_usec);
#ifndef  IO_SEPARATE_SLEEP_PER_10
	    /*! 线程切换出去20ms */
	  	thread_usleep(IO_THREAD_USLEEP);
#endif


	  }	  
	  
	  return 0;
}

//detect
int ioctl_usb_detect_hadler() {
	return 0;
}

/**
  @brief int if_low_power(void) 检测当前是否低电
  @author    <wfb> 
  @param[in] <无>
  @return    <无>
  @remark    2014-02-27 增加该函数
  @note
*/
int if_low_power(void)
{
	return (BATTERY_POWER_STATE_LOW == g_stIoCtlInfo.stLedStateControl.unBatteryPowerState);
}

/* 打印信息 */
void print_cur_info()
{
     DEBUGPRINT(DEBUG_INFO, "unNetConnectState = %d\n",    g_stIoCtlInfo.stLedStateControl.unNetConnectState);
     DEBUGPRINT(DEBUG_INFO, "unBatteryPowerState = %d\n",  g_stIoCtlInfo.stLedStateControl.unBatteryPowerState);
     DEBUGPRINT(DEBUG_INFO, "unNightModeFlag = %d\n",      g_stIoCtlInfo.stLedStateControl.unNightModeFlag);
     DEBUGPRINT(DEBUG_INFO, "unNetLedWorkFlag = %d\n",     g_stIoCtlInfo.stLedStateControl.unNetLedWorkFlag);
     DEBUGPRINT(DEBUG_INFO, "unPowerLedWorkFlag = %d\n",   g_stIoCtlInfo.stLedStateControl.unPowerLedWorkFlag);
     DEBUGPRINT(DEBUG_INFO, "unNetPowerLedWorkFlag = %d\n",g_stIoCtlInfo.stLedStateControl.unNetPowerLedWorkFlag);
     DEBUGPRINT(DEBUG_INFO, "unNightLedWorkFlag = %d\n",   g_stIoCtlInfo.stLedStateControl.unNightLedWorkFlag);
}

/**
  @brief 更新 指示灯状态m2,目前的处理是所有的指示灯在这都刷新一遍，该出可以考虑优化
  @author <wfb> 
  @param[in] <pstLedControlInfo IO控制指示灯结构体>
  @param[in] <unLedMsgType IO指示灯控制类型>
  @param[out]<无>
  @return <无>
  @remark 2013-03-03 增加函数
  @note
*/
void update_m2_led_state_by_msg()
{
	//(一) 指示灯
	if (NET_STATE_NO_IP == g_stIoCtlInfo.stLedStateControl.unNetConnectState ||
				NET_STATE_HAVE_IP == g_stIoCtlInfo.stLedStateControl.unNetConnectState)
	{
		/* 网络无ip  */
		if (NET_STATE_NO_IP == g_stIoCtlInfo.stLedStateControl.unNetConnectState)
		{
			/* 指示灯快闪  */
			DEBUGPRINT(DEBUG_INFO, "NET_STATE_NO_IP \n");
			g_stIoCtlInfo.stNetPowerLedControlInfo.unCtlTypeNew = LED_CTL_TYPE_FAST_FLK;
		}
		/* 网络有ip  */
		else if (NET_STATE_HAVE_IP == g_stIoCtlInfo.stLedStateControl.unNetConnectState)
		{
			/* 指示灯慢闪  */
			DEBUGPRINT(DEBUG_INFO, "NET_STATE_HAVE_IP \n");
			g_stIoCtlInfo.stNetPowerLedControlInfo.unCtlTypeNew = LED_CTL_TYPE_SLOW_FLK;
		}
	}
	/* 网络连接正常 */
	else
	{
		/* 网络正常 低电  指示灯常量 */
		if (if_low_power())
		{
			DEBUGPRINT(DEBUG_INFO, "if_low_power()\n");
            /* 指示灯常量 当设置常量后需要重新初始化状态  */
			g_stIoCtlInfo.stNetPowerLedControlInfo.unCtlTypeOld = LED_CTL_TYPE_INVALID;
			g_stIoCtlInfo.stNetPowerLedControlInfo.unCtlTypeNew = LED_CTL_TYPE_ON;
		}
		/* 网络正常 非低电 */
		else
		{
			/* 网络正常 非低电 夜间模式  关闭指示灯 */
            if (LED_NIGHT_MODE == g_stIoCtlInfo.stLedStateControl.unNightModeFlag)
            {
            	DEBUGPRINT(DEBUG_INFO, "55555555555555555   LED_NIGHT_MODE\n");
            	g_stIoCtlInfo.stNetPowerLedControlInfo.unCtlTypeNew = LED_CTL_TYPE_OFF;
            }
            /* 网络正常 非低电 非夜间模式 */
            else
            {
            	/* 网络正常 非低电 非夜间模式 工作 开指示灯*/
            	if (LED_FLAG_WORK == g_stIoCtlInfo.stLedStateControl.unNetPowerLedWorkFlag)
            	{
            		DEBUGPRINT(DEBUG_INFO, "66666666666666666 LED_CTL_TYPE_ON \n");
            		g_stIoCtlInfo.stNetPowerLedControlInfo.unCtlTypeOld = LED_CTL_TYPE_INVALID;
            		g_stIoCtlInfo.stNetPowerLedControlInfo.unCtlTypeNew = LED_CTL_TYPE_ON;
            	}
            	/* 网络正常 非低电 非夜间模式 不工作 关示灯 */
            	else
            	{
            		DEBUGPRINT(DEBUG_INFO, "777777777777777777777 LED_CTL_TYPE_OFF\n");
            		g_stIoCtlInfo.stNetPowerLedControlInfo.unCtlTypeNew = LED_CTL_TYPE_OFF;
            	}
            }
		}
	}

	//(二) 夜视灯
	/* 打开夜视灯 */
    if (LED_FLAG_WORK == g_stIoCtlInfo.stLedStateControl.unNightLedWorkFlag &&
    		LED_NOT_NIGHT_MODE == g_stIoCtlInfo.stLedStateControl.unNightModeFlag)
    {
    	DEBUGPRINT(DEBUG_INFO, "888888888888888888888\n");
    	g_stIoCtlInfo.stNightLedControlInfo.unCtlTypeNew = LED_CTL_TYPE_ON;
    }
    /* 关闭夜视灯 */
    else
    {
    	DEBUGPRINT(DEBUG_INFO, "999999999999999999999999\n");
    	g_stIoCtlInfo.stNightLedControlInfo.unCtlTypeNew = LED_CTL_TYPE_OFF;
    }
}

/**
  @brief 更新 指示灯状态m3s
  @author <wfb> 
  @param[in] <pstLedControlInfo IO控制指示灯结构体>
  @param[in] <unLedMsgType IO指示灯控制类型>
  @param[out]<无>
  @return <无>
  @remark 2013-03-03 增加函数
  @note
*/
void update_m3s_led_state_by_msg()
{
	//(一) 网络指示灯
	if (NET_STATE_NO_IP == g_stIoCtlInfo.stLedStateControl.unNetConnectState ||
				NET_STATE_HAVE_IP == g_stIoCtlInfo.stLedStateControl.unNetConnectState)
	{
		/* 网络无ip  */
		if (NET_STATE_NO_IP == g_stIoCtlInfo.stLedStateControl.unNetConnectState)
		{
			/* 指示灯快闪  */
			g_stIoCtlInfo.stNetLedControlInfo.unCtlTypeNew = LED_CTL_TYPE_FAST_FLK;
		}
		/* 网络有ip  */
		else if (NET_STATE_HAVE_IP == g_stIoCtlInfo.stLedStateControl.unNetConnectState)
		{
			/* 指示灯慢闪  */
			g_stIoCtlInfo.stNetLedControlInfo.unCtlTypeNew = LED_CTL_TYPE_SLOW_FLK;
		}
	}
	/* 网络连接正常 */
	else
	{
		/* 网络正常 夜间模式  关闭指示灯 */
		if (LED_NIGHT_MODE == g_stIoCtlInfo.stLedStateControl.unNightModeFlag)
		{
			g_stIoCtlInfo.stNetLedControlInfo.unCtlTypeNew = LED_CTL_TYPE_OFF;
		}
		/* 网络正常 非夜间模式 */
		else
		{
			/* 网络正常 非夜间模式 工作 开指示灯*/
			if (LED_FLAG_WORK == g_stIoCtlInfo.stLedStateControl.unNetLedWorkFlag)
			{
				g_stIoCtlInfo.stNetLedControlInfo.unCtlTypeOld = LED_CTL_TYPE_INVALID;
				g_stIoCtlInfo.stNetLedControlInfo.unCtlTypeNew = LED_CTL_TYPE_ON;
			}
			/* 网络正常 非夜间模式 不工作 关示灯 */
			else
			{
				g_stIoCtlInfo.stNetLedControlInfo.unCtlTypeNew = LED_CTL_TYPE_OFF;
			}
		}
	}

	//(二) 电源指示灯
	if (LED_FLAG_WORK == g_stIoCtlInfo.stLedStateControl.unPowerLedWorkFlag &&
			LED_NOT_NIGHT_MODE == g_stIoCtlInfo.stLedStateControl.unNightModeFlag)
	{
		g_stIoCtlInfo.stPowerLedControlInfo.unCtlTypeNew = LED_CTL_TYPE_ON;
	}
	else
	{
		g_stIoCtlInfo.stPowerLedControlInfo.unCtlTypeNew = LED_CTL_TYPE_OFF;
	}

	//(三) 夜视灯
	/* 打开夜视灯 */
	if (LED_FLAG_WORK == g_stIoCtlInfo.stLedStateControl.unNightLedWorkFlag &&
			LED_NOT_NIGHT_MODE == g_stIoCtlInfo.stLedStateControl.unNightModeFlag)
	{
		g_stIoCtlInfo.stNightLedControlInfo.unCtlTypeNew = LED_CTL_TYPE_ON;
	}
	/* 关闭夜视灯 */
	else
	{
		g_stIoCtlInfo.stNightLedControlInfo.unCtlTypeNew = LED_CTL_TYPE_OFF;
	}

}


/**
  @brief 更新 指示灯状态
  @author <wfb> 
  @param[in] <pstLedControlInfo IO控制指示灯结构体>
  @param[in] <unLedMsgType IO指示灯控制类型>
  @param[out]<无>
  @return <无>
  @remark 2013-09-02 增加该注释
  @note
*/
void update_led_state_by_msg()
{
	print_cur_info();
    /* 指示灯的整体控制逻辑均在该处 */
	/* 网络连接异常 联网中或有ip */
	if (DEVICE_TYPE_M2 == g_nDeviceType)
	{
		update_m2_led_state_by_msg();
	}
	else if (DEVICE_TYPE_M3S == g_nDeviceType)
	{
		update_m3s_led_state_by_msg();
	}
}

/**
  @brief 指示灯消息处理函数——led控制
  @author <wfb> 
  @param[in] <pstLedControlInfo IO控制指示灯结构体>
  @param[in] <unLedMsgType IO指示灯控制类型>
  @param[out]<无>
  @return <无>
  @remark 2013-09-02 增加该注释
  @note
*/
void ioctl_led_msg_handler_type(led_control_info_t *pstLedControlInfo, unsigned int unLedCtlType)
{
    /* 参数检查 */
	if (NULL == pstLedControlInfo)
	{
        return;
	}

	/* 主意该类型 要跟随同子进程定义的cmd 而定 使用switch 是为了print */
	switch(unLedCtlType)
	{
	case LED_CTL_TYPE_ON:
		DEBUGPRINT(DEBUG_INFO, "come into ioctl_led_msg_handler_type LED_CTL_TYPE_ON\n");
	    pstLedControlInfo->unCtlTypeNew = LED_CTL_TYPE_ON;
	    break;
	case LED_CTL_TYPE_OFF:
		DEBUGPRINT(DEBUG_INFO, "come into ioctl_led_msg_handler_type LED_CTL_TYPE_OFF\n");
	    pstLedControlInfo->unCtlTypeNew = LED_CTL_TYPE_OFF;
	    break;
	case LED_CTL_TYPE_FAST_FLK:
		DEBUGPRINT(DEBUG_INFO, "come into ioctl_led_msg_handler_type LED_CTL_TYPE_FAST_FLK\n");
	    pstLedControlInfo->unCtlTypeNew = LED_CTL_TYPE_FAST_FLK;
	    break;
	case LED_CTL_TYPE_SLOW_FLK:
		DEBUGPRINT(DEBUG_INFO, "come into ioctl_led_msg_handler_type LED_CTL_TYPE_SLOW_FLK\n");
	    pstLedControlInfo->unCtlTypeNew = LED_CTL_TYPE_SLOW_FLK;
	    break;
	case LED_CTL_TYPE_BLINK:
		DEBUGPRINT(DEBUG_INFO, "come into ioctl_led_msg_handler_type LED_CTL_TYPE_BLINK\n");
		pstLedControlInfo->unCtlTypeNew = LED_CTL_TYPE_BLINK;
		break;
	case LED_CTL_TYPE_DOZE:
		DEBUGPRINT(DEBUG_INFO, "come into ioctl_led_msg_handler_type LED_CTL_TYPE_DOZE\n");
		pstLedControlInfo->unCtlTypeNew = LED_CTL_TYPE_DOZE;
		break;
	default:
		DEBUGPRINT(DEBUG_INFO, "ioctl_led_msg_handler_type default\n");
	    break;
	}
}

/**
  *  @brief     指示灯消息处理函数
  *  @author    <wfb> 
  *  @param[in] <unLedMsgType       IO指示灯控制类型>
  *  @param[out]<无>
  *  @return    <无>
  *  @remark    2013-09-02 增加该注释
  *  @note
*/
void ioctl_led_msg_handler(unsigned int unLedTypeColor, unsigned int unLedMsgType)
{
    if (LED_TYPE_NET == unLedTypeColor)
    {
    	DEBUGPRINT(DEBUG_INFO, "come into ioctl_led_msg_handler   LED_COLOR_TYPE_NET_BLUE\n");
    	ioctl_led_msg_handler_type(&(g_stIoCtlInfo.stNetLedControlInfo), unLedMsgType);
    }
    else if(LED_TYPE_POWER == unLedTypeColor)
    {
    	DEBUGPRINT(DEBUG_INFO, "come into ioctl_led_msg_handler   LED_COLOR_TYPE_POWER_RED \n");
    	ioctl_led_msg_handler_type(&(g_stIoCtlInfo.stPowerLedControlInfo), unLedMsgType);
    }
    else if (LED_TYPE_NIGHT == unLedTypeColor)
    {
    	DEBUGPRINT(DEBUG_INFO, "come into ioctl_led_msg_handler   LED_COMMON_TYPE_NIGHT \n");
    	ioctl_led_msg_handler_type(&(g_stIoCtlInfo.stNightLedControlInfo), unLedMsgType);
    }
    else if (LED_TYPE_NET_POWER  == unLedTypeColor )
    {
    	ioctl_led_msg_handler_type(&(g_stIoCtlInfo.stNetPowerLedControlInfo), unLedMsgType);
    }
    else
    {
    	DEBUGPRINT(DEBUG_INFO, "come into ioctl_led_msg_handler   default\n");
    }

    //判断是否需要更新状态
    //update_led_state_by_msg();

    /* 设置指示灯状态 */
    set_led_state(unLedTypeColor, unLedMsgType);

}


/**
  *  @brief     指示灯是否工作消息处理函数
  *  @author    <wfb> 
  *  @param[in] <unSubType       指示灯类型>
  *  @param[in] <unSubPara       指示灯是否工作>
  *  @param[in] <unSubPara1      是否写FLASH标志>
  *  @return    <无>
  *  @remark    2013-09-02 增加该注释
  *  @note
*/
void led_worker_msghandler(unLedType, unIfWorkFlag, unIfWriteFlashFlag)
{
	int nConfigItemIndex = -1;
	char acTmpBuf[4] = {0};


    if (LED_TYPE_NET == unLedType)
    {
    	g_stIoCtlInfo.stLedStateControl.unNetLedWorkFlag = unIfWorkFlag;
    	nConfigItemIndex = CONFIG_LIGHT_NET;
    }
    else if (LED_TYPE_POWER == unLedType)
    {
    	g_stIoCtlInfo.stLedStateControl.unPowerLedWorkFlag = unIfWorkFlag;
    	nConfigItemIndex = CONFIG_LIGHT_POWER;
    }
    else if (LED_TYPE_NIGHT == unLedType)
    {
    	g_stIoCtlInfo.stLedStateControl.unNightLedWorkFlag = unIfWorkFlag;
    	nConfigItemIndex = CONFIG_LIGHT_NIGHT;
    }
    else if (LED_TYPE_NET_POWER == unLedType)
    {
     	g_stIoCtlInfo.stLedStateControl.unNetPowerLedWorkFlag = unIfWorkFlag;
     	nConfigItemIndex = CONFIG_LIGHT_NET;
    }
    else
    {
    	DEBUGPRINT(DEBUG_INFO, "error ,invalid unSubType = %d\n", unLedType);
    	return;
    }

    if (TRUE == unIfWriteFlashFlag)
    {
    	if (LED_FLAG_WORK == unIfWorkFlag)
		{
			strncpy(acTmpBuf, "y", 1);
		}
		else if (LED_FLAG_NOT_WORK == unIfWorkFlag)
		{
			strncpy(acTmpBuf, "n", 1);
		}

    	/* 将更新的状态写入flash */
    	set_config_value_to_flash(nConfigItemIndex, acTmpBuf, strlen(acTmpBuf));
    }

    /* 将这些信息记录到全局结构体 */
    //g_stIoCtlInfo.stLedWorkFlag.unWorkFlag = TRUE;
    //g_stIoCtlInfo.stLedWorkFlag.unWorkLedType = unSubType;
    //g_stIoCtlInfo.stLedWorkFlag.unWorkCtlType = unSubPara;
}

/**
  @brief  void report_net_state_msghandler(int nNetState)
  @author     <wfb> 
  @param[in]  <nNetState: 网络状态>
  @param[out] <无>
  @return     <无>
  @remark 2013-10-31增加该函数
  @note
*/
void report_net_state_msghandler(int nNetState)
{
	g_stIoCtlInfo.stLedStateControl.unNetConnectState = nNetState;
}

void report_night_mode_msghandler(int nNightModeFlag)
{
	g_stIoCtlInfo.stLedStateControl.unNightModeFlag = nNightModeFlag;
}

/**
  @brief     电源状态消息处理函数
  @author    <wfb> 
  @param[in] <unSubType M2电池状态>
  @return    <无>
  @remark    2013-09-02 增加该注释
  @note
*/
void battery_power_state_msghandler(unSubType)
{
	//DEBUGPRINT(DEBUG_INFO, "come into power_state_msghandler  \n");
    if (g_stIoCtlInfo.stLedStateControl.unBatteryPowerState != unSubType)
    {
    	DEBUGPRINT(DEBUG_INFO, "come into power_state_msghandler color not eq  \n");
    	g_stIoCtlInfo.stLedStateControl.unBatteryPowerState = unSubType;

    	/* 指示灯置颜色 */
    	if (BATTERY_POWER_STATE_NORMAL == unSubType)
    	{
    		g_stIoCtlInfo.stNetPowerLedControlInfo.unLedColor = LED_COLOR_TYPE_GREEN;
    	}
    	else if (BATTERY_POWER_STATE_CHARGE == unSubType)
    	{
    		g_stIoCtlInfo.stNetPowerLedControlInfo.unLedColor = LED_COLOR_TYPE_ORANGE;
    	}
    	else if (BATTERY_POWER_STATE_LOW == unSubType)
    	{
    		g_stIoCtlInfo.stNetPowerLedControlInfo.unLedColor = LED_COLOR_TYPE_RED;
    	}
    }
}

/**
  @brief led_control_state_msghandler(unsigned int unSubType, unsigned intunSubPara, unsigned int unSubPara1, unsigned int unSubPara2)
  @author    <wfb> 
  @param[in] <无>
  @return    <无>
  @remark    2014-02-28 增加该函数
  @note
*/
void led_control_state_msghandler(unsigned int unCmd, unsigned int unSubPara,\
		unsigned int unSubPara1, unsigned int unSubPara2)
{
    switch(unCmd)
    {
    case LED_CONTROL_ITEM_NET_STATE:
    	DEBUGPRINT(DEBUG_INFO, "00000000000000000000LED_CONTROL_ITEM_NET_STATE:unSubPara= %d \n", unSubPara);
    	report_net_state_msghandler(unSubPara);
    	break;
    case LED_CONTROL_ITEM_NIGHT_MODE:
    	DEBUGPRINT(DEBUG_INFO, "111111111111111111111: LED_CONTROL_ITEM_NIGHT_MODE\n", unSubPara);
    	report_night_mode_msghandler(unSubPara);
    	break;
    case LED_CONTROL_ITEM_IF_WORK:
    	DEBUGPRINT(DEBUG_INFO, "3333333333333333333333LED_CONTROL_ITEM_IF_WORK: %d, %d, %d\n", unSubPara, unSubPara1, unSubPara1);
    	led_worker_msghandler(unSubPara, unSubPara1, unSubPara2);
    	break;
    case LED_CONTROL_ITEM_BATTERY_STATE:
    	DEBUGPRINT(DEBUG_INFO, "3333333333333333333333LED_CONTROL_ITEM_BATTERY_STATE: unSubPara=%d\n", unSubPara);
    	battery_power_state_msghandler(unSubPara);
    	break;
    default:
    	break;
    }

    update_led_state_by_msg();
}

/**
  *  @brief      qbuf 切换usb
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void qbuf_switch_usb_msghandler()
{
	printf("qbuf_switch_usb_msghandler");
    /**> 关闭电源 */
	poweroff_usb();
	thread_usleep(100000);
	/**> 切换到mfi */
	usb_switch_to_mfi();
	thread_usleep(100000);
	/**> 切换到cam */
	usb_switch_to_cam();
	thread_usleep(100000);
	/**> 打开电压 */
	poweron_usb();
	thread_usleep(1000000);

	/**> 发送消息 */
	send_msg_to_child_process(IO_CTL_MSG_TYPE, MSG_IOCTL_P_USB_SWITCH, NULL, 0);
}


/**
  *  @brief  direct_set_led_state_msghandler()  直接设置led灯的状态,这个函数不可乱用的
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark 2013-10-31增加该函数
  *  @note
*/
void direct_set_led_state_msghandler(unsigned int unLedTypeColor, unsigned int unLedMsgType)
{
#if 0
	DEBUGPRINT(DEBUG_INFO, "RCV DIRECT SET LED ..................\n");

    //出了开和关之外，其他指令不接受
	if ((LED_CTL_TYPE_ON != unLedMsgType) &&
		(LED_CTL_TYPE_OFF != unLedMsgType))
	{
         return;
	}

	if (LED_TYPE_NET == unLedTypeColor)
	{
		ioctl_set_led_state(unLedTypeColor, unLedMsgType);
	}
	else if (LED_TYPE_NET_POWER == unLedTypeColor)
	{
		ioctl_set_net_power_led(unLedTypeColor, unLedMsgType);
	}
	else
	{
		DEBUGPRINT(DEBUG_INFO, "sorry, you put a invalid led name\n");
	}
#endif
}


/**
  @brief  void kernel_must_reboot_msghandler()
  @author     <wfb> 
  @param[in]  <无>
  @param[out] <无>
  @return     <无>
  @remark 2013-10-31增加该函数
  @note
*/
void kernel_must_reboot_msghandler()
{
	return;
}

/**
  *  @brief      指示灯消息处理函数
  *  @author     <wfb> 
  *  @param[in]  <unSwitchType: usb切换类型>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void ioctl_usb_switch_msg_handler(unsigned int unSwitchType)
{
	/**> 如果消息中usb要切换到的设备  和 目前usb所连接的设备相同  那么不切换 */

    switch(unSwitchType)
    {
    case USB_SWITCH_TO_MFI:
    	if (USB_LINK_TO_MFI == g_stIoCtlInfo.stUsbSwitchInfo.unUsbCurLinker)
    	{
    		g_stIoCtlInfo.stUsbPortMonitorInfo.unStopMonitorFlag = USB_PORT_START_MONITOR;
    		DEBUGPRINT(DEBUG_INFO, "usb already switch to mfi\n");
            break;
    	}
    	DEBUGPRINT(DEBUG_INFO, "come into ioctl_usb_switch_msg_handler to mfi \n");
    	g_stIoCtlInfo.stUsbSwitchInfo.unUsbSwitchState = SWITCH_TO_MFI_START;
    	break;
    case USB_SWITCH_TO_CAM:
    	if (USB_LINK_TO_CAM == g_stIoCtlInfo.stUsbSwitchInfo.unUsbCurLinker)
    	{
    		g_stIoCtlInfo.stUsbPortMonitorInfo.unStopMonitorFlag = USB_PORT_START_MONITOR;
    		DEBUGPRINT(DEBUG_INFO, "usb already switch to cam\n");
    	    break;
    	}
    	DEBUGPRINT(DEBUG_INFO, "come into ioctl_usb_switch_msg_handler to cam\n");
    	g_stIoCtlInfo.stUsbSwitchInfo.unUsbSwitchState = SWITCH_TO_CAM_START;
    	break;
    default:
    	break;
    }

    return;
}

/**
  *  @brief      IO线程消息处理函数
  *  @author     <wfb> 
  *  @param[in]  <pstIoCtl: IO线程消息结构体>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
                 2013-10-31 增加直接控制led的消息处理函数
  *  @note
*/
void ioctl_msgHandler(io_ctl_t *pstIoCtl)
{
	MsgData *pstMsg = (MsgData *)pstIoCtl->acMsgDataRcv;
	unsigned int unSubType = 0;
	unsigned int unSubPara = 0;
	unsigned int unSubPara1 = 0;
	unsigned int unSubPara2 = 0;

	/**> 参数检查 */
	if (NULL == pstIoCtl)
	{
		exit(0);
	}

	/**> 处理接收到的消息 */
	if (-1 != msg_queue_rcv(pstIoCtl->threadMsgId, pstMsg, IOCTL_MSG_LEN, IO_CTL_MSG_TYPE))
	{
		unSubType  = *((char *)((char *)pstMsg + sizeof(MsgData)));
		unSubPara  = *((char *)((char *)pstMsg + sizeof(MsgData) + 1));
		unSubPara1 = *((char *)((char *)pstMsg + sizeof(MsgData) + 2));
		unSubPara2 = *((char *)((char *)pstMsg + sizeof(MsgData) + 3));

		//printf("unSubType = %ld,   unSubPara = %ld", unSubType, unSubPara);

		//printf("comeinto msg_queue_rcv \n");

		switch(pstMsg->cmd)
		{
		case MSG_IOCTL_T_LED_CTL:
			ioctl_led_msg_handler(unSubType, unSubPara);
			break;
		case MSG_IOCTL_T_USB_SWITCH:
			ioctl_usb_switch_msg_handler(unSubType);
			break;
		case MSG_IOCTL_T_I2C:
			DEBUGPRINT(DEBUG_INFO, "MSG_IOCTL_T_I2C pstMsg->type = %d\n", (int)pstMsg->type);
			ioctl_i2c(((char *)pstMsg + sizeof(MsgData)), pstMsg->len);
			break;
		case MSG_IOCTL_T_SUSPEND:  /* suspend here */
			DEBUGPRINT(DEBUG_INFO, "ioctl thread suspend\n");
			break;
		case MSG_IOCTL_T_START:
			DEBUGPRINT(DEBUG_INFO, "rcv start ~~~~\n");
			g_stIoCtlInfo.stIoCtlThreadState.unThreadState = IOCTL_THREAD_STATE_RUN;
			break;
		case MSG_IOCTL_T_CHECK_USB:
			DEBUGPRINT(DEBUG_INFO, "rcv check usb\n");
			g_stIoCtlInfo.stCheckUsbState.unCheckUsbFlag = TRUE;
			g_stIoCtlInfo.stCheckUsbState.unSenderType = unSubType;
			break;
		case MSG_IOCTL_T_CTL_USB_MONITOR:
			DEBUGPRINT(DEBUG_INFO, "IOCTL_T_CTL_USB_MONITOR, unSubType = %d\n", unSubType);
			g_stIoCtlInfo.stUsbPortMonitorInfo.unStopMonitorFlag = unSubType;
			break;
		case CMD_AV_STARTED:
			DEBUGPRINT(DEBUG_INFO, "ioctl rcv av start, start usb monitor\n");
			//system("echo Io rcv av start, start monitor >> /etc/log.txt");
			g_stIoCtlInfo.stUsbPortMonitorInfo.unStopMonitorFlag = USB_PORT_START_MONITOR;
			break;
		case MSG_IOCTL_T_QBUF_SWITCH_USB:
			qbuf_switch_usb_msghandler();
			break;
		case MSG_IOCTL_T_DIRECT_SET_LED_STATE:
			direct_set_led_state_msghandler(unSubType, unSubPara);
			break;
		case MSG_IOCTL_T_KERNEL_MUST_REBOOT:
			kernel_must_reboot_msghandler();
			break;
		case MSG_IOCTL_T_CTL_LED_STATE:
			DEBUGPRINT(DEBUG_INFO, "MSG_IOCTL_T_CTL_LED_STATE..... ..... \n");
			log_printf("before led_control_state_msghandler unSubPara %d\n", unSubPara);
            led_control_state_msghandler(unSubType, unSubPara, unSubPara1, unSubPara2);
            break;
		default:
			break;
		}
	}

	return;
}

/**
  *  @brief      led指示灯更新状态
  *  @author     <wfb> 
  *  @param[in]  <pstLedControlInfo: 指示灯控制结构体>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/

void led_state_updata(led_control_info_t *pstLedControlInfo)
{
	switch(pstLedControlInfo->unCtlTypeNew )
	{
	case LED_CTL_TYPE_OFF:
		ioctl_set_led(pstLedControlInfo->unLedType, pstLedControlInfo->unLedColor, LED_STATE_OFF);
		pstLedControlInfo->unState = LED_STATE_OFF;
		break;
	case LED_CTL_TYPE_ON:
	case LED_CTL_TYPE_FAST_FLK:
	case LED_CTL_TYPE_SLOW_FLK:
	case LED_CTL_TYPE_BLINK:
	case LED_CTL_TYPE_DOZE:
		DEBUGPRINT(DEBUG_INFO, "TEST UPDATA  LED_CTL_TYPE_BLINK   M2 ............................... \n");
		ioctl_set_led(pstLedControlInfo->unLedType, pstLedControlInfo->unLedColor, LED_STATE_ON);
		pstLedControlInfo->unState = LED_STATE_ON;
		break;
	default:
		break;
	}

	/**> 每重设一次状态后 将循环次数置位 */
	pstLedControlInfo->unLoopCnts = 0;
	pstLedControlInfo->unCtlTypeOld = pstLedControlInfo->unCtlTypeNew;

	/**> 更新状态后 务必重新设置半周期亮灭时间 */
	get_blink_time(pstLedControlInfo->unCtlTypeNew,
			&(pstLedControlInfo->unLedOnCircleCnts), &(pstLedControlInfo->unLedOffCircleCnts));

}


/**
  *  @brief      led指示灯状态控制
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void led_state_control(led_control_info_t *pstLedControlInfo)
{
	unsigned int unTimeOutFlag = 0;  ///<若时间已经到半个周期则被set为1

	/**> 每次进入计数增加1 */
	pstLedControlInfo->unLoopCnts++;

	//printf("pstLedControlInfo->unCtlTypeNew = %ld\n", pstLedControlInfo->unCtlTypeNew);

	/**> 判断是否需要闪烁 */
	if (TRUE == led_ctl_type_if_need_flk(pstLedControlInfo->unCtlTypeNew))
	{
		if (pstLedControlInfo->unLoopCnts ==
					((LED_STATE_ON == pstLedControlInfo->unState) ?
							pstLedControlInfo->unLedOnCircleCnts:pstLedControlInfo->unLedOffCircleCnts))
			{
				unTimeOutFlag = 1;
			}
	}

	/**> 半个周期时间到 */
	if (unTimeOutFlag)
	{
		if (LED_STATE_ON == pstLedControlInfo->unState)
		{
			pstLedControlInfo->unState = LED_STATE_OFF;
		}
		else
		{
			pstLedControlInfo->unState = LED_STATE_ON;
		}

		ioctl_set_led(pstLedControlInfo->unLedType, pstLedControlInfo->unLedColor, pstLedControlInfo->unState);

		/**> 将循环次数置位 */
		pstLedControlInfo->unLoopCnts = 0;

		unTimeOutFlag = 0;
	}

}

/**
  *  @brief      指示灯状态控制
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void ioctl_led_control_if_work()
{
#if 0
	if (TRUE == g_stIoCtlInfo.stLedWorkFlag.unWorkFlag)
	{
		//DEBUGPRINT(DEBUG_INFO, "TRUE == g_stIoCtlInfo.stLedWorkFlag.unWorkFlag \n");
		/**> 如果是停止 */
		if (LED_FLAG_NOT_WORK == g_stIoCtlInfo.stLedWorkFlag.unWorkCtlType)
		{
			if (DEVICE_TYPE_M2 == g_nDeviceType)
			{
				/**> 夜视灯和指示灯 */
				if (LED_M2_TYPE_MIXED == g_stIoCtlInfo.stLedWorkFlag.unWorkLedType)
				{
					//DEBUGPRINT(DEBUG_INFO, "ioctl_set_net_power_led(0, LED_STATE_OFF)\n");
					ioctl_set_net_power_led(0, LED_STATE_OFF);
				}
				else if(LED_COMMON_TYPE_NIGHT == g_stIoCtlInfo.stLedWorkFlag.unWorkLedType)
				{
					ioctl_set_led_state(g_stIoCtlInfo.stLedWorkFlag.unWorkLedType, LED_STATE_OFF);
				}
			}
			else
			{
				DEBUGPRINT(DEBUG_INFO, "ioctl_set_M3S_led_state LED_STATE_OFF ");
				ioctl_set_led_state(g_stIoCtlInfo.stLedWorkFlag.unWorkLedType, LED_STATE_OFF);
			}
		}
		else
		{
			if (DEVICE_TYPE_M2 == g_nDeviceType)
			{
				if (LED_COMMON_TYPE_NIGHT == g_stIoCtlInfo.stLedWorkFlag.unWorkLedType)
				{
					led_state_updata(&(g_stIoCtlInfo.stNightLedControlInfo));
				}
				else
				{
					led_state_updata(&(g_stIoCtlInfo.stM2LedControlInfo));
				}
			}
			else
			{
				if (LED_COLOR_TYPE_NET_BLUE == g_stIoCtlInfo.stLedWorkFlag.unWorkLedType)
				{
					led_state_updata(&(g_stIoCtlInfo.stNetLedControlInfo));
				}
				else if (LED_M3S_TYPE_POWER_ORANGE == g_stIoCtlInfo.stLedWorkFlag.unWorkLedType)
				{
					led_state_updata(&(g_stIoCtlInfo.stPowerLedControlInfo));
				}
				else if (LED_COMMON_TYPE_NIGHT == g_stIoCtlInfo.stLedWorkFlag.unWorkLedType)
				{
					led_state_updata(&(g_stIoCtlInfo.stNightLedControlInfo));
				}
			}
		}

		g_stIoCtlInfo.stLedWorkFlag.unWorkFlag = FALSE;
	}
#endif
}


/**
  *  @brief      指示灯状态控制－类型
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void ioctl_led_control_type(unsigned int unLedType)
{
	led_control_info_t *pstLedControlInfo = NULL;

	if (LED_TYPE_NET == unLedType)
	{
		pstLedControlInfo = &(g_stIoCtlInfo.stNetLedControlInfo);
    }
	else if(LED_TYPE_POWER == unLedType)
	{
		pstLedControlInfo = &(g_stIoCtlInfo.stPowerLedControlInfo);
	}
	else if(LED_TYPE_NIGHT == unLedType)
	{
		pstLedControlInfo = &(g_stIoCtlInfo.stNightLedControlInfo);
	}
	else if (LED_TYPE_NET_POWER  == unLedType)
	{
		pstLedControlInfo = &(g_stIoCtlInfo.stNetPowerLedControlInfo);
	}
	else
	{
		/**> 打印出错消息 */
		DEBUGPRINT(DEBUG_INFO, "wronf led type ............................... \n");
		return;
	}

	/**> 如果新旧状态不一致，那么则要更新到新状态 */
	if (pstLedControlInfo->unCtlTypeNew != pstLedControlInfo->unCtlTypeOld)
	{
		DEBUGPRINT(DEBUG_INFO, "led_state_updata ................ \n");
		led_state_updata(pstLedControlInfo);
	}
	else
	{
		//DEBUGPRINT(DEBUG_INFO, "led_state_control ................ \n");
		led_state_control(pstLedControlInfo);
	}

}

/**
  *  @brief      USB切换控制函数
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void ioctl_led_control(void)
{
	//ioctl_led_control_if_work();

	/**> 判断是否是M2或是M3S */
	if (DEVICE_TYPE_M2 == g_nDeviceType)
	{
		//DEBUGPRINT(DEBUG_INFO, "ioctl_led_control m2 \n");
		ioctl_led_control_type(LED_TYPE_NET_POWER);
		ioctl_led_control_type(LED_TYPE_NIGHT);
	}
	else if (DEVICE_TYPE_M3S == g_nDeviceType)
	{
		//DEBUGPRINT(DEBUG_INFO, "ioctl_led_control m3s \n");
		ioctl_led_control_type(LED_TYPE_NET);
		ioctl_led_control_type(LED_TYPE_POWER);
		ioctl_led_control_type(LED_TYPE_NIGHT);
	}
	else
	{
		DEBUGPRINT(DEBUG_INFO, "WRONG DEV TYPE \n");
	}
}

/**
  @brief      设置led net 状态
  @author     <wfb> 
  @param[in]  <unLedState: 指示灯开或关>
  @param[out] <无>
  @return     <成功：EXIT_SUCCESS   失败：EXIT_FAILURE>
  @remark     2013-09-02 增加该注释
  @note
*/
int ioctl_set_led(unsigned int unLedType, unsigned int unLedColor, unsigned int unLedState)
{
   switch(unLedType)
   {
   case LED_TYPE_NET:
	   ioctl_set_net_led(unLedState);
	   break;
   case LED_TYPE_POWER:
	   ioctl_set_power_led(unLedState);
   	   break;
   case LED_TYPE_NIGHT:
	   ioctl_set_night_led(unLedState);
   	   break;
   case LED_TYPE_NET_POWER:
	   ioctl_set_net_power_led(unLedColor, unLedState);
   	   break;
   default:
	   break;

   }

   return 0;
}

/**
  @brief      设置led net 状态
  @author     <wfb> 
  @param[in]  <unLedState: 指示灯开或关>
  @param[out] <无>
  @return     <成功：EXIT_SUCCESS   失败：EXIT_FAILURE>
  @remark     2013-09-02 增加该注释
  @note
*/
int ioctl_set_net_led(unsigned int unLedState)
{
	int lRet = EXIT_FAILURE;

	if (LED_STATE_ON == unLedState)
	{
		lRet = ioctl_set_net_led_on();
	}
	else
	{
		lRet = ioctl_set_net_led_off();
	}

	return lRet;
}

/**
  @brief      设置led power 状态
  @author     <wfb> 
  @param[in]  <unLedState: 指示灯开或关>
  @param[out] <无>
  @return     <成功：EXIT_SUCCESS   失败：EXIT_FAILURE>
  @remark     2013-09-02 增加该注释
  @note
*/
int ioctl_set_power_led(unsigned int unLedState)
{
	int lRet = EXIT_FAILURE;

	if (LED_STATE_ON == unLedState)
	{
		lRet = ioctl_set_power_led_on();
	}
	else
	{
		lRet = ioctl_set_power_led_off();
	}

	return lRet;
}


/**
  @brief      设置led night 状态 m2和M3S的正好相反
  @author     <wfb> 
  @param[in]  <unLedState: 指示灯开或关>
  @param[out] <无>
  @return     <成功：EXIT_SUCCESS   失败：EXIT_FAILURE>
  @remark     2013-09-02 增加该注释
  @note
*/
int ioctl_set_night_led(unsigned int unLedState)
{
	int lRet = EXIT_FAILURE;

	if (LED_STATE_ON == unLedState)
	{
		//printf("ononononononononononononononononononononioctl_set_night_led_on \n");
		if (DEVICE_TYPE_M3S == g_nDeviceType)
		{
			lRet = ioctl_set_night_led_on();
		}
		else if (DEVICE_TYPE_M2 == g_nDeviceType)
		{
			lRet = ioctl_set_night_led_off();
		}
	}
	else
	{
		//printf("offoffoffoffoffoffoffoffoffoffoffoffoffoffoffioctl_set_night_led_off\n");
		if (DEVICE_TYPE_M3S == g_nDeviceType)
		{
			lRet = ioctl_set_night_led_off();
		}
		else if (DEVICE_TYPE_M2 == g_nDeviceType)
		{
			lRet = ioctl_set_night_led_on();
		}
	}

	return lRet;
}

/**
  *  @brief      设置网络电源灯状态
  *  @author     <wfb> 
  *  @param[in]  <unLedType: 指示灯类型>
  *  @param[in]  <unLedState:指示灯状态>
  *  @param[out] <无>
  *  @return     <成功：EXIT_SUCCESS   失败：EXIT_FAILURE>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int ioctl_set_net_power_led(unsigned int unLedColor, unsigned int unLedState)
{
	int lRet = EXIT_FAILURE;

	if (LED_STATE_ON == unLedState)
	{
		/* 设置绿灯 */
		if (LED_COLOR_TYPE_GREEN == unLedColor)
		{
			//DEBUGPRINT(DEBUG_INFO, "ioctl_set_net_power_led_green .........\n");
			lRet = ioctl_set_net_power_led_green();
		}
		/* 设置黄灯 */
		else if(LED_COLOR_TYPE_ORANGE == unLedColor)
		{
			//DEBUGPRINT(DEBUG_INFO, "ioctl_set_net_power_led_ORANGE .........\n");
			lRet = ioctl_set_net_power_led_orange();
		}
		/* 设置红灯 LED_COLOR_TYPE_RED */
		else
		{
			//DEBUGPRINT(DEBUG_INFO, "ioctl_set_net_power_led_RED .........\n");
			lRet = ioctl_set_net_power_led_red();
		}
	}
	else if(LED_STATE_OFF == unLedState)
	{
        /* 关闭都是统一的 */
		lRet = ioctl_set_net_power_led_off();
	}
	else
	{
		DEBUGPRINT(DEBUG_INFO, "comeinto def unLedState = %ld\n", (long int)unLedState);
	}

	return lRet;
}


/**
  *  @brief      usb切换处理控制
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <成功：EXIT_SUCCESS   失败：EXIT_FAILURE>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void ioctl_usb_switch_control(void)
{
	char acMsgDataBuf[MAX_MSG_LEN] = {0};
	int i = 0;

	if (SWITCH_USB_STATE_INVALID  == g_stIoCtlInfo.stUsbSwitchInfo.unUsbSwitchState)
	{
        ;
	}
	/* 切换到mfi */
	else if((SWITCH_TO_MFI_START  <= g_stIoCtlInfo.stUsbSwitchInfo.unUsbSwitchState) &&
			(SWITCH_TO_MFI_STATE_POWERON_USB  >= g_stIoCtlInfo.stUsbSwitchInfo.unUsbSwitchState))
	{
		g_stIoCtlInfo.stUsbSwitchInfo.unLoopCnts++;

		/* 闭合usb电源 */
		if (SWITCH_TO_MFI_START  == g_stIoCtlInfo.stUsbSwitchInfo.unUsbSwitchState)
		{
			if (EXIT_FAILURE == poweroff_usb())
			{
				thread_usleep(10000);
                return;
			}
			DEBUGPRINT(DEBUG_INFO, "to mfi poweroff usb\n");
			//system("echo to mfi poweroff usb >> /etc/log.txt");
			write_log_info_to_file("to mfi poweroff usb \n");
			common_write_log("USB: to mfi poweroff usb~~");
			thread_usleep(10000);
	    	g_stIoCtlInfo.stUsbSwitchInfo.unLoopCnts = 0;
	    	g_stIoCtlInfo.stUsbSwitchInfo.unUsbSwitchState = SWITCH_TO_MFI_STATE_POWEROFF_USB;
		}
		/* 当usb电源关闭后1秒钟左右再切换usb，切换后大约0.1秒后在打开电源开关 */
		else if (SWITCH_TO_MFI_STATE_POWEROFF_USB  == g_stIoCtlInfo.stUsbSwitchInfo.unUsbSwitchState)
		{
			if ((USB_POWEROFF_TO_SWITCH_TIME/IO_THREAD_USLEEP) == g_stIoCtlInfo.stUsbSwitchInfo.unLoopCnts)
			{
				if (EXIT_FAILURE == usb_switch_to_mfi())
				{
					g_stIoCtlInfo.stUsbSwitchInfo.unLoopCnts =
							USB_POWEROFF_TO_SWITCH_TIME/IO_THREAD_USLEEP - 1;
					thread_usleep(10000);
					return;
				}
				DEBUGPRINT(DEBUG_INFO, "to mfi switch to mfi\n");
				//system("echo to mfi poweroff switch to mfi >> /etc/log.txt");
				write_log_info_to_file("to mfi poweroff switch to mfi\n");
				common_write_log("USB: switch to mfi~~");
				thread_usleep(10000);
				g_stIoCtlInfo.stUsbSwitchInfo.unLoopCnts = 0;
				g_stIoCtlInfo.stUsbSwitchInfo.unUsbSwitchState = SWITCH_TO_MFI_STATE_SWITCH_USB;
			}
		}
		else if (SWITCH_TO_MFI_STATE_SWITCH_USB  == g_stIoCtlInfo.stUsbSwitchInfo.unUsbSwitchState)
		{
			/* 切换后1s打开usb电源 */
			if ((USB_SWITCH_TO_POWERON_TIME/IO_THREAD_USLEEP) == g_stIoCtlInfo.stUsbSwitchInfo.unLoopCnts)
			{
				if (EXIT_FAILURE == poweron_usb())
				{
					g_stIoCtlInfo.stUsbSwitchInfo.unLoopCnts =
							USB_SWITCH_TO_POWERON_TIME/IO_THREAD_USLEEP - 1;
					thread_usleep(10000);
					return;
				}
				DEBUGPRINT(DEBUG_INFO, "to mfi poweron usb\n");
				//system("echo to mfi poweron usb >> /etc/log.txt");
				write_log_info_to_file("to mfi poweron usb\n");
				common_write_log("USB: mfi poweron usb~~");
				thread_usleep(10000);
				g_stIoCtlInfo.stUsbSwitchInfo.unLoopCnts = 0;
				g_stIoCtlInfo.stUsbSwitchInfo.unUsbSwitchState = SWITCH_TO_MFI_STATE_POWERON_USB;
				gettimeofday(&g_stTimeVal, NULL);
				DEBUGPRINT(DEBUG_INFO, "TO MFI POWERON USB g_stTimeVal.tv_sec = %ld,g_stTimeVal.tv_usec = %ld\n",
								    		g_stTimeVal.tv_sec, g_stTimeVal.tv_usec);
			}

		}
		else if (SWITCH_TO_MFI_STATE_POWERON_USB  == g_stIoCtlInfo.stUsbSwitchInfo.unUsbSwitchState)
		{
			/* 切换后0.5s 算正式切换完成，这里主要是考虑硬件响应时间 */
			if ((USB_CHECK_RESPONSE_LATENCY_TIME/IO_THREAD_USLEEP) == g_stIoCtlInfo.stUsbSwitchInfo.unLoopCnts)
			{
				DEBUGPRINT(DEBUG_INFO, "to mfi ok \n");
				g_stIoCtlInfo.stUsbSwitchInfo.unLoopCnts = 0;
				g_stIoCtlInfo.stUsbSwitchInfo.unUsbCurLinker = USB_LINK_TO_MFI;
				g_stIoCtlInfo.stUsbSwitchInfo.unUsbSwitchState = SWITCH_USB_STATE_INVALID;

				/* 切换到mif 成功 向子进程发送消息  */
				//printf("powron on usb end\n");
				acMsgDataBuf[0] = CHECK_USB_SENDER_USB_LINK_TO_MFI;
				acMsgDataBuf[1] = USB_SWITCH_RETURN_SUCCESS;

				gettimeofday(&g_stTimeVal, NULL);
				DEBUGPRINT(DEBUG_INFO, "TO MFI OK g_stTimeVal.tv_sec = %ld,g_stTimeVal.tv_usec = %ld\n",
								    		g_stTimeVal.tv_sec, g_stTimeVal.tv_usec);
				g_stIoCtlInfo.stUsbPortMonitorInfo.unUsbMonitorMode = USB_MONITOR_MODE_FILE;

				/* 在这里打上一个补丁 */
				if (PORT_STATE_CONNECT == (acMsgDataBuf[2] = get_usb_port_state_by_io()))
				{
					DEBUGPRINT(DEBUG_INFO, "get_usb_port_state_by_io() is state connect \n");
					/* 首先检测文件是否存在 若不存在 每次sleep 500ms 最多6次 */
					while(PORT_STATE_NOT_CONNECT == get_usb_port_state_by_file())
					{
						DEBUGPRINT(DEBUG_INFO, "get_usb_port_stateget_usb_port_state  NOT CONNECT    %d\n", i);
						thread_usleep(500000); /* 指示灯在这里会被停滞 算是一种牺牲 */;
						i++;
						if (7 == i)
						{
							/* 当六次均未出现我们认为处于拔出状态 */
							/* 但有可能是android设备 */
							int jj = 0;
							sleep(1);
							while(jj < 10) {
								if(0 == android_usb_identity()) {
									break;
								}
								sleep(1);
								++jj;
							}
							if(jj < 10) {

							} else {
								acMsgDataBuf[2] = PORT_STATE_NOT_CONNECT;
							}
                        	DEBUGPRINT(DEBUG_INFO, "something must be wrong here already to seven,  WE GIVE IT NOT CONNECT\n");
                        	break;
						}
					}
				}
				else
				{
					/*  add 20130808 by fb.w  */
					while(PORT_STATE_CONNECT != (acMsgDataBuf[2] = get_usb_port_state()))
					{
						DEBUGPRINT(DEBUG_INFO, "get_usb_port_stateget_usb_port_state  NOT CONNECT    %d\n", i);
						/* 把指示灯制成常量状态  TODO */
						thread_usleep(500000); /* 指示灯在这里会被停滞 算是一种牺牲 */
						i++;
                        if (i == 6)
                        {
                        	DEBUGPRINT(DEBUG_INFO, "already to six  NOT CONNECT    \n");
                        	break;
                        }

					}
				}
				//thread_usleep(10000);
				/* 赋值 防止重复上报 add by wfb 在测试中会出现这样的问题  我们程序在跑的过程中 一定要和监测的一致 */
				g_stIoCtlInfo.stUsbPortMonitorInfo.unPortState = (unsigned int)acMsgDataBuf[2];
				thread_usleep(100000);
				send_msg_to_child_process(IO_CTL_MSG_TYPE, MSG_IOCTL_P_USB_SWITCH, acMsgDataBuf, USB_SWITCH_RETURN_MSG_DATA_LEN);
				DEBUGPRINT(DEBUG_INFO, "already send msg .......................................... acMsgDataBuf[2]=%d\n",acMsgDataBuf[2]);
				//system("echo to mfi ok >> /etc/log.txt");
				write_log_info_to_file("to mfi ok\n");
				common_write_log("USB: to mfi ok~~");

			}
		}

	}
	/* 切换到cam */
	else if((SWITCH_TO_CAM_START  <= g_stIoCtlInfo.stUsbSwitchInfo.unUsbSwitchState) &&
				(SWITCH_TO_CAM_STATE_POWERON_USB  >= g_stIoCtlInfo.stUsbSwitchInfo.unUsbSwitchState))
	{
		g_stIoCtlInfo.stUsbSwitchInfo.unLoopCnts++;

		/* 闭合usb电源 */
		if (SWITCH_TO_CAM_START  == g_stIoCtlInfo.stUsbSwitchInfo.unUsbSwitchState)
		{
			if (EXIT_FAILURE == poweroff_usb())
			{
				thread_usleep(10000);
				return;
			}
			DEBUGPRINT(DEBUG_INFO, "to cam poweroff usb\n");
			//system("echo to cam poweroff usb >> /etc/log.txt");
			write_log_info_to_file("to cam poweroff usb \n");
			common_write_log("USB: to cam poweroff usb~~");
			thread_usleep(10000);
	    	g_stIoCtlInfo.stUsbSwitchInfo.unLoopCnts = 0;
	    	g_stIoCtlInfo.stUsbSwitchInfo.unUsbSwitchState = SWITCH_TO_CAM_STATE_POWEROFF_USB;
		}
		/* 当usb电源关闭后1秒钟左右再切换usb，切换后大约0.1秒后在打开电源开关 */
		else if (SWITCH_TO_CAM_STATE_POWEROFF_USB  == g_stIoCtlInfo.stUsbSwitchInfo.unUsbSwitchState)
		{
			/* 开关打开 1s */
			if ((USB_POWEROFF_TO_SWITCH_TIME/IO_THREAD_USLEEP) == g_stIoCtlInfo.stUsbSwitchInfo.unLoopCnts)
			{
				if (EXIT_FAILURE == usb_switch_to_cam())
				{
					g_stIoCtlInfo.stUsbSwitchInfo.unLoopCnts =
							USB_POWEROFF_TO_SWITCH_TIME/IO_THREAD_USLEEP - 1;
					thread_usleep(10000);
					return;
				}
				DEBUGPRINT(DEBUG_INFO, "to cam switch to cam\n");
				//system("echo to cam switch to cam >> /etc/log.txt");
				write_log_info_to_file("to cam switch to cam\n");
				common_write_log("USB: to cam switch to cam~~");
				thread_usleep(10000);
				g_stIoCtlInfo.stUsbSwitchInfo.unLoopCnts = 0;
				g_stIoCtlInfo.stUsbSwitchInfo.unUsbSwitchState = SWITCH_TO_CAM_STATE_SWITCH_USB;
			}
		}
		else if (SWITCH_TO_CAM_STATE_SWITCH_USB  == g_stIoCtlInfo.stUsbSwitchInfo.unUsbSwitchState)
		{
			/* 5代表开关打开后 0.1s */
			if ((USB_SWITCH_TO_POWERON_TIME/IO_THREAD_USLEEP) == g_stIoCtlInfo.stUsbSwitchInfo.unLoopCnts)
			{
				if (EXIT_FAILURE == poweron_usb())
				{
					g_stIoCtlInfo.stUsbSwitchInfo.unLoopCnts =
							USB_SWITCH_TO_POWERON_TIME/IO_THREAD_USLEEP - 1;
					thread_usleep(10000);
					return;
				}
				thread_usleep(10000);
				DEBUGPRINT(DEBUG_INFO, "to cam poweron usb\n");
				//system("echo to cam poweron usb >> /etc/log.txt");
				write_log_info_to_file("to cam poweron usb \n");
				common_write_log("USB: to cam poweron usb~~");
				g_stIoCtlInfo.stUsbSwitchInfo.unLoopCnts = 0;
				g_stIoCtlInfo.stUsbSwitchInfo.unUsbSwitchState = SWITCH_TO_CAM_STATE_POWERON_USB;
			}
		}
		else if (SWITCH_TO_CAM_STATE_POWERON_USB  == g_stIoCtlInfo.stUsbSwitchInfo.unUsbSwitchState)
		{
			/* 切换后0.5s 算正式切换完成，这里主要是考虑硬件响应时间 */
			if ((USB_CHECK_RESPONSE_LATENCY_TIME/IO_THREAD_USLEEP) == g_stIoCtlInfo.stUsbSwitchInfo.unLoopCnts)
			{
				g_stIoCtlInfo.stUsbSwitchInfo.unLoopCnts = 0;
				g_stIoCtlInfo.stUsbSwitchInfo.unUsbCurLinker = USB_LINK_TO_CAM;
				g_stIoCtlInfo.stUsbSwitchInfo.unUsbSwitchState = SWITCH_USB_STATE_INVALID;

				/* 切换到mif 成功 向子进程发送消息  */
				DEBUGPRINT(DEBUG_INFO, "to cam ok\n");
				acMsgDataBuf[0] = CHECK_USB_SENDER_USB_LINK_TO_CAM;
				acMsgDataBuf[1] = USB_SWITCH_RETURN_SUCCESS;
				g_stIoCtlInfo.stUsbPortMonitorInfo.unUsbMonitorMode = USB_MONITOR_MODE_IO;
				while(PORT_STATE_INVALID == (acMsgDataBuf[2] = get_usb_port_state()))
				{
					acMsgDataBuf[2] = PORT_STATE_NOT_CONNECT;
					break;
                    //DEBUGPRINT(DEBUG_INFO, "SWITCH I can not get port state, this I will wait until I GET state \n");
				}
				thread_usleep(10000);
				g_stIoCtlInfo.stUsbPortMonitorInfo.unPortState = (unsigned int)acMsgDataBuf[2];
				send_msg_to_child_process(IO_CTL_MSG_TYPE, MSG_IOCTL_P_USB_SWITCH, acMsgDataBuf, USB_SWITCH_RETURN_MSG_DATA_LEN);
				//system("echo to cam ok >> /etc/log.txt");
				write_log_info_to_file("to cam ok \n");
				common_write_log("USB: to cam ok~~");
			}
		}
	}
	else
	{
		DEBUGPRINT(DEBUG_INFO, "come into the wrong area\n");
	}

}

/**
  *  @brief      通过文件是否存在来探测usb是否仍然插入
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <PORT_STATE_CONNECT PORT_STATE_NOT_CONNECT>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int get_usb_port_state_by_file(void)
{
//	scan_android_usb_bus();

    if (0 == access("/dev/mfiusb0", 0) || USB_DEVICE_ANDROID == androidUsb_t.device)
    {
//    	|| USB_DEVICE_ANDROID == androidUsb_t.device
//    	if(USB_DEVICE_ANDROID == androidUsb_t.device) {
//    		if(PORT_STATE_CONNECT == get_usb_port_state_by_io()){
//    			if(androidUsb_t.io_status != ANDROID_USB_IO_ON) {
//    				androidUsb_t.io_status = ANDROID_USB_IO_ON;
//    				printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
//    			}
//    			return PORT_STATE_CONNECT;
//    		} else {
//    			if(androidUsb_t.io_status != ANDROID_USB_IO_OFF) {
//					androidUsb_t.io_status = ANDROID_USB_IO_OFF;
//					printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
//				}
//    			return PORT_STATE_NOT_CONNECT;
//    		}
//    	}
    	return PORT_STATE_CONNECT;
    }
    else
    {
        return PORT_STATE_NOT_CONNECT;
    }
}

/**
  *  @brief      获取usb口状态
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <PORT_STATE_CONNECT PORT_STATE_NOT_CONNECT>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int get_usb_port_state(void)
{
	if (USB_MONITOR_MODE_FILE == g_stIoCtlInfo.stUsbPortMonitorInfo.unUsbMonitorMode)
	{
		return get_usb_port_state_by_file();
	}
	else
	{
        return get_usb_port_state_by_io();
	}
}

/**
  *  @brief      检测usb口状态
  *  @author     <wfb> 
  *  @param[in]  <pstIoCtl: io线程>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void ioctl_check_usb_port(io_ctl_t *pstIoCtl)
{
    unsigned char aucSndBuf[3] = {0};
    char cTmp = 0;

    if (FALSE == g_stIoCtlInfo.stCheckUsbState.unCheckUsbFlag)
    {
         return;
    }
    else
    {
    	g_stIoCtlInfo.stCheckUsbState.unCheckUsbFlag = FALSE;
    }

    /* 流够时间  */
    thread_usleep(10000);

    while(PORT_STATE_INVALID == (cTmp = get_usb_port_state()))
    {
    	thread_usleep(10000);
    	DEBUGPRINT(DEBUG_INFO, "I CAN NOT GET USB PORT TRY AGAIN\n");
    }

    aucSndBuf[0] = cTmp;


    aucSndBuf[1] = g_stIoCtlInfo.stCheckUsbState.unSenderType;
    DEBUGPRINT(DEBUG_INFO, "msghandler: ioctl_check_usb_port aucSndBuf[0] = %d\n", aucSndBuf[0]);
    /* 赋值 防止重复上报 add by wfb 在测试中会出现这样的问题  我们程序在跑的过程中 一定要和监测的一致 */
    g_stIoCtlInfo.stUsbPortMonitorInfo.unPortState = aucSndBuf[0];

    /* 向子进程发送消息 */
    send_msg_to_child_process(IO_CTL_MSG_TYPE,
    		MSG_IOCTL_P_CHECK_USB_RESULT, aucSndBuf, CHECK_USB_RESULT_MSG_DATA_LEN);
}


/**
  *  @brief      监测usb口状态
  *  @author     <wfb> 
  *  @param[in]  <pstIoCtl: io线程>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void ioctl_usb_port_monitor(io_ctl_t *pstIoCtl)
{
	unsigned int unCurPortState = PORT_STATE_INVALID;
	char acMsgData[8] = {0};

    /* 抗快速插拔 */
	if (USB_PORT_STOP_MONITOR == g_stIoCtlInfo.stUsbPortMonitorInfo.unStopMonitorFlag )
	{
         return;
	}
#if 0
	/* 若是在usb 切换期间那么不上报 */
	if (SWITCH_USB_STATE_INVALID  != g_stIoCtlInfo.stUsbSwitchInfo.unUsbSwitchState)
	{
		//printf("cur is switch usb,do not report usb\n");
		return;
	}
#endif
	/* 时间间隔到则一次 */
    if ((MONITOR_TIME_INTERVAL/IO_THREAD_USLEEP) == g_stIoCtlInfo.stUsbPortMonitorInfo.unLoopCnts)
    {
    	if (USB_POWER_STATE_ON != g_stIoCtlInfo.stUsbSwitchInfo.unUsbPowerState )
    	{
             poweron_usb();
    	}

		unCurPortState = get_usb_port_state();

		/* 如果是无效的 比如打开失败 那么则 不更新 */
		if (PORT_STATE_INVALID == unCurPortState)
		{
			return;
		}

        //printf("cur usb state is: %ld\n",unCurPortState);
		/* 状态不一致 要判断是否状态有改变 */
		if (unCurPortState != g_stIoCtlInfo.stUsbPortMonitorInfo.unPortState)
		{
			//printf(" state diff\n");
			g_stIoCtlInfo.stUsbPortMonitorInfo.unContinuCnts++;

            //printf("usb continue time is %ld\n", stIoCtlInfo.stUsbPortMonitorInfo.unContinuCnts);
			if (MONITOR_STATE_CONTINUE_TIMES == g_stIoCtlInfo.stUsbPortMonitorInfo.unContinuCnts)
			{
				/* 向子进程发送usb切换的消息 */
				acMsgData[0] = (unCurPortState == PORT_STATE_CONNECT) ? DEVICE_ACTION_IN:DEVICE_ACTION_OUT;
				DEBUGPRINT(DEBUG_INFO, "******************************************\n");
				DEBUGPRINT(DEBUG_INFO, "*************usb switch  %d****************\n", (int)acMsgData[0]);
				DEBUGPRINT(DEBUG_INFO, "******************************************\n");

				if (PORT_STATE_CONNECT == unCurPortState)
				{
                    //system("echo usb in >> /etc/log.txt");
					write_log_info_to_file("usb in ~~~\n");
					common_write_log("USB: usb in~~");
				}
				else
				{
                    //system("echo usb out >> /etc/log.txt");
					write_log_info_to_file("usb out ~~~\n");
					common_write_log("USB: usb out~~");
				}

				g_stIoCtlInfo.stUsbPortMonitorInfo.unContinuCnts = 0;
				g_stIoCtlInfo.stUsbPortMonitorInfo.unPortState = unCurPortState;
				g_stIoCtlInfo.stUsbPortMonitorInfo.unLoopCnts = 0;

                /* 若检测到的是usb插入 则先暂停上报usb切换状态 */
				//if (unCurPortState == PORT_STATE_CONNECT) //检测到拔出后 也不上报
				{
                     g_stIoCtlInfo.stUsbPortMonitorInfo.unStopMonitorFlag = USB_PORT_STOP_MONITOR;
                     g_stIoCtlInfo.stUsbPortMonitorInfo.unLoopCnts = 0;
				}

			    send_msg_to_child_process(IO_CTL_MSG_TYPE,
			    		MSG_IOCTL_P_USB_MONITOR, acMsgData, STATE_MONITOR_MSG_DATA_LEN);
			}
		}
		else
		{
			g_stIoCtlInfo.stUsbPortMonitorInfo.unContinuCnts = 0;
		}
	}
	else
	{
		g_stIoCtlInfo.stUsbPortMonitorInfo.unLoopCnts++;
	}
}

/**
  *  @brief      监测net口状态
  *  @author     <wfb> 
  *  @param[in]  <pstIoCtl: io线程>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void ioctl_net_port_monitor(io_ctl_t *pstIoCtl)
{
	unsigned int unCurPortState = PORT_STATE_INVALID;
	char acMsgData[8] = {0};

	/* 时间间隔到则检测一次 */
	if ((MONITOR_TIME_INTERVAL/IO_THREAD_USLEEP) == g_stIoCtlInfo.stNetPortMonitorInfo.unLoopCnts)
	{
		unCurPortState = get_net_port_state();

		if (PORT_STATE_INVALID == unCurPortState)
		{
             return;
		}
		//printf("ioctl comeinto  get net state is : %ld\n", unCurPortState);

		/* 状态不一致 要判断是否状态有改变 */
	    if (unCurPortState != g_stIoCtlInfo.stNetPortMonitorInfo.unPortState)
		{
	    	g_stIoCtlInfo.stNetPortMonitorInfo.unContinuCnts++;
		    //printf("ioctl comeinto diff stat  Continue cnt is %ld\n", stIoCtlInfo.stNetPortMonitorInfo.unContinuCnts);

		    if (MONITOR_STATE_CONTINUE_TIMES == g_stIoCtlInfo.stNetPortMonitorInfo.unContinuCnts)
		    {

		    	acMsgData[0] = (unCurPortState == PORT_STATE_CONNECT) ? DEVICE_ACTION_IN : DEVICE_ACTION_OUT;
		    	send_msg_to_child_process(IO_CTL_MSG_TYPE,
		    			MSG_IOCTL_P_NET_MONITOR, acMsgData, STATE_MONITOR_MSG_DATA_LEN);

		    	/* 向子进程发送usb切换的消息 */
		    	DEBUGPRINT(DEBUG_INFO, "******************************************\n");
		    	DEBUGPRINT(DEBUG_INFO, "*************net switch  %d****************\n", (int)acMsgData[0]);
		    	DEBUGPRINT(DEBUG_INFO, "******************************************\n");
		    	g_stIoCtlInfo.stNetPortMonitorInfo.unContinuCnts = 0;
		    	g_stIoCtlInfo.stNetPortMonitorInfo.unPortState = unCurPortState;
		    	g_stIoCtlInfo.stNetPortMonitorInfo.unLoopCnts = 0;
			}
		}
	    else
	    {
	    	g_stIoCtlInfo.stNetPortMonitorInfo.unContinuCnts = 0;
	    }
	}
	else
	{
		g_stIoCtlInfo.stNetPortMonitorInfo.unLoopCnts++;
	}

	/* 判断是否要发送设置wifi */
	if (TRUE == g_stIoCtlInfo.stNetPortMonitorInfo.unSetWifiFlag)
	{
		DEBUGPRINT(DEBUG_INFO, "hello come into unSetWifiFlag\n");
		if (10 == g_stIoCtlInfo.stNetPortMonitorInfo.unWifiLoopCnts)
		{
			g_stIoCtlInfo.stNetPortMonitorInfo.unSetWifiFlag = FALSE;
			g_stIoCtlInfo.stNetPortMonitorInfo.unWifiLoopCnts = 0;
			unCurPortState = get_net_port_state();
			if (PORT_STATE_INVALID == unCurPortState)
			{
				/* 切换100ms */
				thread_usleep(100000);
				unCurPortState = get_net_port_state();
			}

			if (PORT_STATE_NOT_CONNECT == unCurPortState)
			{
				DEBUGPRINT(DEBUG_INFO, "ready to set wifi \n");
				/* 发送设置wifi */
				send_msg_to_child_process(IO_CTL_MSG_TYPE,
						    MSG_IOCTL_P_INFORM_SET_WIRELESS, NULL,0);
			}

		}
		else
		{
			g_stIoCtlInfo.stNetPortMonitorInfo.unWifiLoopCnts++;
		}
	}
}

/**
  *  @brief      i2c接口
  *  @author     <wfb> 
  *  @param[in]  <pstData: 数据 >
  *  @param[in]  <unLen:   长度>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int ioctl_i2c(char *pstData, unsigned int unLen)
{
	int lFd = -1;
	int lCmd = 0;
	int lRet = EXIT_FAILURE;
	char acTmpbuf[1024] = {0};

	/* 参数判断 */
	if (NULL == pstData)
	{
		DEBUGPRINT(DEBUG_INFO, "func:ioctl_i2c  invalid pstData\n");
	}

	lFd = open(RT5350_I2C_DEVICE_PATH, O_RDWR);

    if (-1 == lFd)
    {
    	DEBUGPRINT(DEBUG_INFO, "open rt_dev failed9\n");
    	return lRet;
    }

    lCmd = (int)*pstData;

    switch (lCmd)
    {
    case I2C_GET_CP_DEVICE_ID:
    	//printf("receive type = %d\n");
    	DEBUGPRINT(DEBUG_INFO, "receive I2C_GET_CP_DEVICE_ID\n");
    	acTmpbuf[0] = 4;
    	acTmpbuf[1] = 4;

    	lRet = read(lFd, acTmpbuf, acTmpbuf[1]);

    	//printf("acTmpbuf[0] = %d\n ", acTmpbuf[0]);
    	//printf("acTmpbuf[1] = %d\n ", (unsigned char)(acTmpbuf[1]));

        send_msg_to_child_process(IO_CTL_MSG_TYPE,
        		MSG_IOCTL_P_I2C_DEVICE_ID, acTmpbuf, CP_DEVICE_ID_LEN);
    	break;
    case I2C_GET_CP_AUTHENTICATION:
    	DEBUGPRINT(DEBUG_INFO, "receive I2C_GET_CP_AUTHENTICATION\n");
    	/* 第一个字节代表类型 所以 data 加1  Len 减1 */
        lRet = get_cp_authentication_info(lFd, pstData+1, unLen - 1);
        break;

    default:
    	DEBUGPRINT(DEBUG_INFO, "lcmd = %d\n", lCmd);
    	break;
    }

    //ioctl(lFd, CMD_GET_USB_STATE, &lData);
    //lRet = read(lFd, buf, len);
    //write

    close(lFd);

	/* 切换出去20ms add by wfb */
    thread_usleep(10000);

    return lRet;

    //return (DEVICE_CONNECT == lData) ? PORT_STATE_CONNECT : PORT_STATE_NOT_CONNECT;
}

/**
  *  @brief      获得cp认证信息
  *  @author     <wfb> 
  *  @param[in]  <lFd:   文件句柄 >
  *  @param[in]  <pcBuf: 数据 >
  *  @param[in]  <ucDataLen:   长度>
  *  @param[out] <无>
  *  @return     <EXIT_FAILURE EXIT_SUCCESS>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int get_cp_authentication_info(int lFd, char *pcBuf, int ucDataLen)
{
    int lRet = EXIT_FAILURE;
    int lRetVal = 0;
    char acBufIn[128] = {0};
    char acDataBuf[128] = {0};
    char acChallengeData[128] = {0};

    if (NULL == pcBuf)
    {
    	DEBUGPRINT(DEBUG_INFO, "func:get_cp_authentication_info, invalid pointer\n");
        return lRet;
    }

    DEBUGPRINT(DEBUG_INFO, "ucDataLen = %d\n", ucDataLen);
    memcpy(&acChallengeData[1], pcBuf, ucDataLen);

    //printf("come into 11111 ok\n");
    acBufIn[0] = 0x20;
    acBufIn[1] = 0x00;
    acBufIn[2] = 0x14;
    lRetVal = write(lFd, acBufIn, 2);
	if(lRetVal < 0)
	{
		DEBUGPRINT(DEBUG_INFO, "error_at_write_i2c_address_0x20\n");
		return lRet;
	}
	//printf("come into 22222 ok\n");
	acChallengeData[0] = 0x21;
	lRetVal = write(lFd, acChallengeData, 0x14);
	if(lRetVal < 0)
	{
		DEBUGPRINT(DEBUG_INFO, "error_at_write_i2c_address_0x21\n");
		return lRet;
	}
	//printf("come into 33333 ok\n");
	acBufIn[0] = 0x10;
	acBufIn[1] = 0x01;
	lRetVal = write(lFd, acBufIn, 0x01);
	if(lRetVal < 0)
	{
		DEBUGPRINT(DEBUG_INFO, "error_at_write_i2c_address_0x10\n");
		return -1;
	}
	//printf("come into 44444 ok\n");
	acBufIn[0] = 0x00;
	while(0x00 == acBufIn[0])
	{
		acBufIn[0] = 0x10;
		lRetVal = read(lFd, acBufIn, 0x01);
	}

	//printf("come into 55555 ok\n");
	acBufIn[0] = 0x11;
	lRetVal = read(lFd, acBufIn, 0x02);
	while(lRetVal < 0)
	{
		DEBUGPRINT(DEBUG_INFO, "come into 55555~~~~ ok\n");
		lRetVal = read(lFd, acBufIn, 0x02);
	}

	//printf("come into 66666 ok\n");
	acDataBuf[0] = 0x12;
	lRetVal = read(lFd, acDataBuf, acBufIn[1]);
	while(lRetVal < 0)
	{
		DEBUGPRINT(DEBUG_INFO, "come into 66666~~~~~ ok\n");
		lRetVal = read(lFd, acDataBuf, acBufIn[1]);
	}

	//printf("len is %d\n", (int)acBufIn[1]);

	send_msg_to_child_process(IO_CTL_MSG_TYPE,
			MSG_IOCTL_P_I2C_GET_AUTHENTICATION, acDataBuf, acBufIn[1]);
	DEBUGPRINT(DEBUG_INFO, "get auth send to ioctl process ok\n");

    return EXIT_SUCCESS;

}

/**
  *  @brief      获取该闪等状态的周期时间
  *  @author     <wfb> 
  *  @param[in]  <nLedCtlType:   LED控制类型>
  *  @param[out] <pnLedOnTimes： on的次数>
  *  @param[out] <pnLedOffTimes：off的次数>
  *  @return     <EXIT_FAILURE EXIT_SUCCESS>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void get_blink_time(int nLedCtlType, int *pnLedOnTimes, int *pnLedOffTimes)
{
    switch(nLedCtlType)
    {
    //指示灯快闪
    case LED_CTL_TYPE_FAST_FLK:
    	*pnLedOnTimes = 4;
    	*pnLedOffTimes = 4;
    	break;
    //指示灯慢闪
    case LED_CTL_TYPE_SLOW_FLK:
    	*pnLedOnTimes = 16;
    	*pnLedOffTimes = 16;
    	break;
    //眨眼
    case LED_CTL_TYPE_BLINK:
    	*pnLedOnTimes = 40;
    	*pnLedOffTimes = 4;
    	break;
    //瞌睡灯
    case LED_CTL_TYPE_DOZE:
    	*pnLedOnTimes = 4;
    	*pnLedOffTimes = 40;
    	break;
    case LED_CTL_TYPE_ON:
    	break;
    case LED_CTL_TYPE_OFF:
    	break;
    default:
    	DEBUGPRINT(DEBUG_INFO, "not exist led type, nLedCtlType=%d\n", nLedCtlType);
    	break;
    }

    return;
}

/**
  *  @brief      判断是否需要闪烁
  *  @author     <wfb> 
  *  @param[in]  <nLedCtlType:   LED控制类型>
  *  @param[out] <无>
  *  @return     <0: 是 1：否>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int led_ctl_type_if_need_flk(int nLedCtlType)
{
	return (LED_CTL_TYPE_FAST_FLK == nLedCtlType ||
			LED_CTL_TYPE_SLOW_FLK == nLedCtlType ||
			LED_CTL_TYPE_BLINK    == nLedCtlType ||
			LED_CTL_TYPE_DOZE     == nLedCtlType );
}
