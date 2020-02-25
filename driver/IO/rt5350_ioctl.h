#ifndef __RT5350_IOCTL_H__
#define __RT5350_IOCTL_H__



/****************************************************/
/*                       头文件                         */
/****************************************************/

#include <asm/ioctl.h>


/* IOCTL模数 */
#define RT5350_IOCTL_MAGIC   (125)

/* 错误的IOCTL值 */
#define IOCTL_CMD_INVALID       (0xFFFFFFFF)

/* IOCTL读写 */
#define RT5350_IOCTL_R(x)    _IOR(RT5350_IOCTL_MAGIC, (x), int)
#define RT5350_IOCTL_W(x)    _IOW(RT5350_IOCTL_MAGIC, (x), int)



/* 获取usb状态  */
#define CMD_GET_USB_STATE         RT5350_IOCTL_R(1)
/* 获取net状态  */
#define CMD_GET_NET_STATE         RT5350_IOCTL_R(2)


/* poweron  usb电源 */
#define CMD_POWERON_USB           RT5350_IOCTL_W(3)
/* poweroff usb电源 */
#define CMD_POWEROFF_USB          RT5350_IOCTL_W(4)
/* usb切换到mfi */
#define CMD_SWITCH_TO_MFI         RT5350_IOCTL_W(5)
/* usb切换到cam */
#define CMD_SWITCH_TO_CAM         RT5350_IOCTL_W(6)
/* 网口灯亮（蓝灯） */
#define CMD_NET_LIGHT_ON          RT5350_IOCTL_W(7)
/* 网口灯灭（蓝灯） */
#define CMD_NET_LIGHT_OFF         RT5350_IOCTL_W(8)
/* 电源灯亮（红灯） */
#define CMD_POWER_LIGHT_ON        RT5350_IOCTL_W(9)
/* 电源灯灭（红灯） */
#define CMD_POWER_LIGHT_OFF       RT5350_IOCTL_W(10)

#define CMD_NIGHT_LIGHT_ON        RT5350_IOCTL_W(11)

#define CMD_NIGHT_LIGHT_OFF       RT5350_IOCTL_W(12)


/* ADD THE KEY WORD ,FIX ME  TO DO */
enum
{
	DEVICE_CONNECT = 0,       /*! 设备有连接   */
	DEVICE_NOT_CONNECT = 1,   /*! 设备没有连接 */
};


#endif

