    /**   @file []
   *  @brief    (1)这里提供同硬件的接口
   *  @note
   *  @author   wangfengbo
   *  @date     2014-02-28
   *  @remarks  增加该文件
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
#include "common/logfile.h"
#include "rt5350_ioctl.h"



/*******************************************************************************/
/*                              宏定义 结构体区                                */
/*******************************************************************************/
/** io设备路径 */
#define RT350_IO_DEVICE_PATH         "/dev/rt_dev"

/*******************************************************************************/
/*                                全局变量区                                   */
/*******************************************************************************/
int g_nGPIODevFd = -1;


/*******************************************************************************/
/*                               接口函数声明                                  */
/*******************************************************************************/


/******************************************************************************/
/*                                接口函数                                     */
/******************************************************************************/
void gpio_driver_init()
{
	/* 初始化设备文件 直至成功 */
	while(-1 == (g_nGPIODevFd = open(RT350_IO_DEVICE_PATH, O_RDWR, O_NONBLOCK)))
	{
		DEBUGPRINT(DEBUG_INFO, "open rt5350 device failed\n");
		usleep(20000);
	}

	DEBUGPRINT(DEBUG_INFO, "open rtdev success  g_nGPIODevFd = %d\n", g_nGPIODevFd);
}

/**
  @brief 硬件初始化,主义该函数无比在子进程调用,初始化后不deinit
  @author <wfb> 
  @param[in] <无>
  @param[out] <无>
  @return <成功：EXIT_SUCCESS   失败：EXIT_FAILURE>
  @remark 2014-02-28
  @note
*/
void hardware_init()
{
	gpio_driver_init();
}

/***********************************************************************/
/***********************************************************************/
/************************以下是驱动人员需要维护的平台代码*******************/
/***********************************************************************/
/***********************************************************************/

/**
  *  @brief      usb切换处到MFI
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <成功：EXIT_SUCCESS   失败：EXIT_FAILURE>
  *  @remark     2014-02-28 增加该注释
  *  @note
*/
int usb_switch_to_mfi(void)
{
	int lRet = EXIT_FAILURE;
    int lData = -1;

	if (-1 == ioctl(g_nGPIODevFd, CMD_SWITCH_TO_MFI, &lData))
	{
	    DEBUGPRINT(DEBUG_INFO, "rt_dev ioctl failed33  fd: %d  error is %d, descrip: %s\n",
	    		g_nGPIODevFd, errno, strerror(errno));
        return EXIT_FAILURE;
	}
	else
	{
		return EXIT_SUCCESS;
	}
}

/**
  *  @brief      usb切换处到cam
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <成功：EXIT_SUCCESS   失败：EXIT_FAILURE>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int usb_switch_to_cam(void)
{
	int lData = -1;
	
	if (-1 == ioctl(g_nGPIODevFd, CMD_SWITCH_TO_CAM, &lData))
	{
		DEBUGPRINT(DEBUG_INFO, "rt_dev ioctl failed44 fd:= %d error is %d, descrip: %s\n",
				g_nGPIODevFd, errno, strerror(errno));
		return EXIT_FAILURE;
	}
	else
	{
		return EXIT_SUCCESS;
	}
}


/**
  *  @brief      关闭电源开关（切断）
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <成功：EXIT_SUCCESS   失败：EXIT_FAILURE>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int poweroff_usb(void)
{
	int lData = -1;

   	if (-1 == ioctl(g_nGPIODevFd, CMD_POWEROFF_USB, &lData))
	{
		DEBUGPRINT(DEBUG_INFO, "rt_dev ioctl failed55 fd:= %d error is %d, descrip: %s\n",
				g_nGPIODevFd, errno, strerror(errno));
		return EXIT_FAILURE;
	}
    else
    {	
	    return EXIT_SUCCESS;
    }
}

/**
  *  @brief      闭合电源开关
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <成功：EXIT_SUCCESS   失败：EXIT_FAILURE>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int poweron_usb(void)
{
	int lData = -1;

	
	if (-1 == ioctl(g_nGPIODevFd, CMD_POWERON_USB, &lData))
	{
		DEBUGPRINT(DEBUG_INFO, "rt_dev ioctl failed66 fd:= %d error is %d, descrip: %s\n",
				g_nGPIODevFd, errno, strerror(errno));
		return EXIT_FAILURE;
	}
	else
	{
		return EXIT_SUCCESS;
	}
}


/**
  @brief      通过Io口获取usb口状态
  @author     <wfb> 
  @param[in]  <无>
  @param[out] <无>
  @return     <DEVICE_CONNECT  DEVICE_NOT_CONNECT>
  @remark     2013-09-02 增加该注释
  @note
*/
int get_usb_port_state_by_io(void)
{
	int lData = -1;

    if (-1 == ioctl(g_nGPIODevFd, CMD_GET_USB_STATE, &lData))
    {
    	DEBUGPRINT(DEBUG_INFO, "rt_dev ioctl failed77 fd:= %d error is %d, descrip: %s\n",
    			g_nGPIODevFd, errno, strerror(errno));
        return -1;
    }

    return lData;
}




/**
  @brief      获取网口状态
  @author     <wfb> 
  @param[in]  <无>
  @param[out] <无>
  @return     <PORT_STATE_CONNECT PORT_STATE_NOT_CONNECT>
  @remark     2013-09-02 增加该注释
  @note
*/
int get_net_port_state(void)
{
	int lData = -1;

	if (-1 == ioctl(g_nGPIODevFd, CMD_GET_NET_STATE, &lData))
	{
		DEBUGPRINT(DEBUG_INFO, "rt_dev ioctl failed22 fd:= %d error is %d, descrip: %s\n",
				g_nGPIODevFd, errno, strerror(errno));
        return -1;
	}

	return lData;
}

/**
  @brief 设置网络灯开
  @author <wfb> 
  @param[in] <无>
  @param[out]<无>
  @return
  @remark 2014-02-28 增加该函数
  @note
*/
int ioctl_set_net_led_on()
{	
    int nData = 0;
	
    if (-1 == ioctl(g_nGPIODevFd, CMD_NET_LIGHT_ON, &nData))
	{
        DEBUGPRINT(DEBUG_INFO, "CMD_NET_LIGHT_ON, fd = %d error is %d, descrip: %s\n",
        		g_nGPIODevFd, errno, strerror(errno));
		return EXIT_FAILURE;
	}
	else
	{
	    return EXIT_SUCCESS;
	}		
}

/**
  @brief 设置网络灯关
  @author <wfb> 
  @param[in] <无>
  @param[out]<无>
  @return
  @remark 2014-02-28 增加该函数
  @note
*/
int ioctl_set_net_led_off()
{
    int nData = 0;
	
    if (-1 == ioctl(g_nGPIODevFd, CMD_NET_LIGHT_OFF, &nData))
	{
        DEBUGPRINT(DEBUG_INFO, "CMD_NET_LIGHT_OFF, fd = %d error is %d, descrip: %s\n",
        		g_nGPIODevFd, errno, strerror(errno));
		return EXIT_FAILURE;
	}
	else
	{
	    return EXIT_SUCCESS;
	}		
}


/**
  @brief 设置电源灯开
  @author <wfb> 
  @param[in] <无>
  @param[out]<无>
  @return
  @remark 2014-02-28 增加该函数
  @note
*/
int ioctl_set_power_led_on()
{	
    int nData = 0;
	
    if (-1 == ioctl(g_nGPIODevFd, CMD_POWER_LIGHT_ON, &nData))
	{
        DEBUGPRINT(DEBUG_INFO, "CMD_POWER_LIGHT_ON, fd = %d error is %d, descrip: %s\n",
        		g_nGPIODevFd, errno, strerror(errno));
		return EXIT_FAILURE;
	}
	else
	{
	    return EXIT_SUCCESS;
	}		
}

/**
  @brief 设置电源灯关
  @author <wfb> 
  @param[in] <无>
  @param[out]<无>
  @return
  @remark 2014-02-28 增加该函数
  @note
*/
int ioctl_set_power_led_off()
{
    int nData = 0;
	
    if (-1 == ioctl(g_nGPIODevFd, CMD_POWER_LIGHT_OFF, &nData))
	{
        DEBUGPRINT(DEBUG_INFO, "CMD_POWER_LIGHT_OFF, fd = %d error is %d, descrip: %s\n",
        		g_nGPIODevFd, errno, strerror(errno));
		return EXIT_FAILURE;
	}
	else
	{
	    return EXIT_SUCCESS;
	}		
}


/**
  @brief 设置夜视灯开 这个是M3S的 M2的正好相反
  @author <wfb> 
  @param[in] <无>
  @param[out]<无>
  @return
  @remark 2014-02-28 增加该函数
  @note
*/
int ioctl_set_night_led_on()
{	
    int nData = 0;
	
    if (-1 == ioctl(g_nGPIODevFd, CMD_NIGHT_LIGHT_ON, &nData))
	{
        DEBUGPRINT(DEBUG_INFO, "CMD_NIGHT_LIGHT_ON failed11, fd = %d error is %d, descrip: %s\n",
        		g_nGPIODevFd, errno, strerror(errno));
		return EXIT_FAILURE;
	}
	else
	{
	    return EXIT_SUCCESS;
	}		
}

/**
  @brief 设置夜视灯关 这个是M3S的 M2的正好相反
  @author <wfb> 
  @param[in] <无>
  @param[out]<无>
  @return
  @remark 2014-02-28 增加该函数
  @note
*/
int ioctl_set_night_led_off()
{
    int nData = 0;
	
    if (-1 == ioctl(g_nGPIODevFd, CMD_NIGHT_LIGHT_OFF, &nData))
	{
        DEBUGPRINT(DEBUG_INFO, "CMD_NIGHT_LIGHT_OFF failed11, fd = %d error is %d, descrip: %s\n",
        		g_nGPIODevFd, errno, strerror(errno));
		return EXIT_FAILURE;
	}
	else
	{
	    return EXIT_SUCCESS;
	}		
}

/**
  @brief 设置网络电源灯 绿色
  @author <wfb> 
  @param[in] <无>
  @param[out]<无>
  @return
  @remark 2014-02-28 增加该函数
  @note
*/
int ioctl_set_net_power_led_green()
{	
    int nData = 0;

    if (-1 == ioctl(g_nGPIODevFd, CMD_NET_LIGHT_OFF, &nData))
	{
        DEBUGPRINT(DEBUG_INFO, "CMD_NET_LIGHT_OFF failed11, fd = %d error is %d, descrip: %s\n",
        		g_nGPIODevFd, errno, strerror(errno));
		return EXIT_FAILURE;
	}	
	
	if (-1 == ioctl(g_nGPIODevFd, CMD_POWER_LIGHT_ON, &nData))
	{
        DEBUGPRINT(DEBUG_INFO, "CMD_POWER_LIGHT_ON failed11, fd = %d error is %d, descrip: %s\n",
        		g_nGPIODevFd, errno, strerror(errno));
		return EXIT_FAILURE;
	}
	else
	{
	    return EXIT_SUCCESS;
	}		
}


/**
  @brief 设置网络电源灯 橘色
  @author <wfb> 
  @param[in] <无>
  @param[out]<无>
  @return
  @remark 2014-02-28 增加该函数
  @note
*/
int ioctl_set_net_power_led_orange()
{	
    int nData = 0;
			
    if (-1 == ioctl(g_nGPIODevFd, CMD_NET_LIGHT_ON, &nData))
	{
        DEBUGPRINT(DEBUG_INFO, "CMD_NET_LIGHT_ON failed11, fd = %d error is %d, descrip: %s\n",
        		g_nGPIODevFd, errno, strerror(errno));
		return EXIT_FAILURE;
	}	
	
	if (-1 == ioctl(g_nGPIODevFd, CMD_POWER_LIGHT_ON, &nData))
	{
        DEBUGPRINT(DEBUG_INFO, "CMD_POWER_LIGHT_ON failed11, fd = %d error is %d, descrip: %s\n",
        		g_nGPIODevFd, errno, strerror(errno));
		return EXIT_FAILURE;
	}
	else
	{
	    return EXIT_SUCCESS;
	}		
}


/**
  @brief 设置网络电源灯 红色
  @author <wfb> 
  @param[in] <无>
  @param[out]<无>
  @return
  @remark 2014-02-28 增加该函数
  @note
*/
int ioctl_set_net_power_led_red()
{	
    int nData = 0;
	
    if (-1 == ioctl(g_nGPIODevFd, CMD_NET_LIGHT_ON, &nData))
	{
        DEBUGPRINT(DEBUG_INFO, "CMD_NET_LIGHT_ON failed11, fd = %d error is %d, descrip: %s\n",
        		g_nGPIODevFd, errno, strerror(errno));
		return EXIT_FAILURE;
	}	
	
	if (-1 == ioctl(g_nGPIODevFd, CMD_POWER_LIGHT_OFF, &nData))
	{
        DEBUGPRINT(DEBUG_INFO, "CMD_POWER_LIGHT_OFF failed11, fd = %d error is %d, descrip: %s\n",
        		g_nGPIODevFd, errno, strerror(errno));
		return EXIT_FAILURE;
	}
	else
	{
	    return EXIT_SUCCESS;
	}		
}


/**
  @brief 设置网络电源灯关
  @author <wfb> 
  @param[in] <无>
  @param[out]<无>
  @return
  @remark 2014-02-28 增加该函数
  @note
*/
int ioctl_set_net_power_led_off()
{
    int nData = 0;
	
    if (-1 == ioctl(g_nGPIODevFd, CMD_NET_LIGHT_OFF, &nData))
	{
        DEBUGPRINT(DEBUG_INFO, "CMD_NET_LIGHT_OFF failed11, fd = %d error is %d, descrip: %s\n",
        		g_nGPIODevFd, errno, strerror(errno));
		return EXIT_FAILURE;
	}	
	
	if (-1 == ioctl(g_nGPIODevFd, CMD_POWER_LIGHT_OFF, &nData))
	{
        DEBUGPRINT(DEBUG_INFO, "CMD_POWER_LIGHT_OFF failed11, fd = %d error is %d, descrip: %s\n",
        		g_nGPIODevFd, errno, strerror(errno));
		return EXIT_FAILURE;
	}
	else
	{
	    return EXIT_SUCCESS;
	}
	
}
