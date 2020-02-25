/** @file [MFI.h]
 *  @brief 与苹果设备USB的认证过程和与APPS的USB通讯协议的主要功能函数h文件
 *	@author: Summer.Wang
 *	@data: 2013-9-24
 *	@modify record: 即日创建了这套文件注释系统
 */

#ifndef MFI_H_
#define MFI_H_

/*msg commmad 宏定义*/
enum {
	MSG_MFI_P_TEST_ALIVE = 3,					///< IIC
	MSG_MFI_P_GET_CP = 4,						///< 获取CP信息
	MSG_MFI_P_INFORM_SWITCH_USB_TO_CAM = 5,		///< 切换IO
	MSG_MFI_P_NO_R,								///< 没有R‘，预留接口
	MSG_MFI_P_SEND_SSID = 21,           		///< 发送SSID
	MSG_MFI_P_SUSPEND_IOCTL = 50,				///< stop IOctl
	MSG_MFI_P_START_IOCTL = 51,					///< start IOctl
};

#define BUF_SIZE    512

enum {
	ANDROID_USB_IO_OFF,
	ANDROID_USB_IO_ON,
};

typedef struct AndroidUsb_t {
	volatile int device;              //<设备类型
	volatile int io_status;              //<设备类型
	int status;              //<状态
	int communicate_status;  //<通信状态
	volatile struct usb_dev_handle *sDevice;  //<android操作句柄
	int io_fd;                       //<io描述符
	volatile int endpoint[2];        //<写入及写出端点
	char bufsnd[BUF_SIZE];           //<发送缓冲区
	char bufrcv[BUF_SIZE];           //<接收缓冲区
	char debugLever;  //lever
}AndroidUsb_t;

/** @brief USB设备类型 */
enum {
	USB_DEVICE_ANDROID = 1, //<Android设备
	USB_DEVICE_APPLE = 2,   //<ios设备
};

extern AndroidUsb_t androidUsb_t;

extern int Create_MFI_Thread();

extern int android_usb_read_data_and_deal();

extern int android_usb_identity();

extern int android_usb_open (void *data); //null

extern int android_usb_close (void *data); //null

extern int scan_android_usb_bus();

#endif /* MFI_H_ */
