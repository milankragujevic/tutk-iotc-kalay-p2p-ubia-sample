/**   @file []
   *  @brief    (1)usb以及网口灯插拔监测  (2)状态灯设置 (3)usb切换
   *  @note
   *  @author   wangfengbo
   *  @date     2013-9-2
   *  @remarks  增加注释
*/

#ifndef _IOCTL_H_
#define _IOCTL_H_

/** 定义监测望口状态长度 */
#define  STATE_MONITOR_MSG_DATA_LEN      1

/** usb切换长度 */
#define  USB_SWITCH_MSG_DATA_LEN         1

/** usb切换返回结果 */
#define  USB_SWITCH_RETURN_MSG_DATA_LEN  3

/** 指示灯数据长度 */
#define  LED_CTL_MSG_DATA_LEN            2

/** 指示灯是否工作数据长度 */
#define  LED_IF_WORK_MSG_DATA_LEN        4

/** DEVICE ID 长度 */
#define  CP_DEVICE_ID_LEN                4

/** 检测usb端口 数据长度 */
#define CHECK_USB_REQ_MSG_DATA_LEN       1

/** 回复usb端口 数据长度 */
#define  CHECK_USB_RESULT_MSG_DATA_LEN   2

/** 网络状态消息长度*/
#define NET_STATE_REPORT_LEN             2


/** io控制类型 */
enum
{
	MSG_IOCTL_T_LED_CTL = 0,            ///<LED灯控制
	MSG_IOCTL_T_USB_SWITCH,             ///<usb切换
	MSG_IOCTL_P_USB_SWITCH,             ///<usb切换完成
	MSG_IOCTL_P_USB_MONITOR,            ///<usb监测
	MSG_IOCTL_P_NET_MONITOR,            ///<网口监测
	MSG_IOCTL_T_I2C,                    ///<I2C
	MSG_IOCTL_P_I2C_DEVICE_ID,          ///<I2C 回复给子进程
	MSG_IOCTL_P_I2C_GET_AUTHENTICATION, ///<获取认证
	MSG_IOCTL_T_SUSPEND,                ///<stop ioctl
    MSG_IOCTL_T_START,                  ///<start ioctl
    MSG_IOCTL_T_CHECK_USB,              ///<检测usb
    MSG_IOCTL_P_CHECK_USB_RESULT,       ///<usb端口检测结果
    MSG_IOCTL_T_CTL_USB_MONITOR,        ///<控制USB监测是否打开
    MSG_IOCTL_P_INFORM_SET_WIRELESS,    ///<led
    MSG_IOCTL_T_SET_LED_IF_WORK,        ///<控制led灯是否工作
    MSG_IOCTL_T_SET_POWER_STATE,        ///<设置电源状态
    MSG_IOCTL_T_QBUF_SWITCH_USB,        ///<出现Qbuf，切换usb
    MSG_IOCTL_P_QBUF_SWITCH_USB,        ///<出现Qbuf，切换usb
    MSG_IOCTL_T_DIRECT_SET_LED_STATE,   ///<直接设置led状态，这个只是置灯，但不及录灯的状态，即可能出现灯的状态是开，但灯为关
    MSG_IOCTL_T_KERNEL_MUST_REBOOT,     ///<内核马上重启
    MSG_IOCTL_T_REPORT_NET_STATE,       ///<设置电池电量
    MSG_IOCTL_T_NIGHT_MODE_REPORT,      ///<夜间模式上报
    MSG_IOCTL_T_CTL_LED_STATE,          ///<接受来自 电池电量 网络信息 是否工作 以及是否夜视
};

/** 端口状态 */
enum
{
	PORT_STATE_INVALID = -1,
	PORT_STATE_CONNECT,
	PORT_STATE_NOT_CONNECT,
};

/** 监测状态 */
enum
{
	DEVICE_ACTION_INVALID = -1,
	DEVICE_ACTION_IN = 0,          //< USB和网口的状态  设备插入
	DEVICE_ACTION_OUT,             //< USB和网口的状态  设备拔出
};

/** usb电源 */
enum
{
	USB_POWER_STATE_ON = 1,        ///< USB电源打开
	USB_POWER_STATE_OFF,           ///< USB电源关闭
};

/** usb切换方向 */
enum
{
	USB_SWITCH_TO_MFI = 0,         ///< usb切换到mfi
	USB_SWITCH_TO_CAM,             ///< usb切换到cam
};

/** USB切换返回值 */
enum
{
    USB_SWITCH_RETURN_FAILURE = 0, ///< USB切换返回值失败
    USB_SWITCH_RETURN_SUCCESS,     ///< USB切换返回值成功
};

/** I2C */
enum
{
    I2C_GET_CP_DEVICE_ID = 1,      ///< I2C获取设别id
    I2C_GET_CP_AUTHENTICATION = 2,
};

/** 检测usb端口方向 */
enum
{
	CHECK_USB_SENDER_USB_LINK_TO_CAM = 50,
	CHECK_USB_SENDER_USB_LINK_TO_MFI = 51,
};

/** 检测usb端口方向 */
enum
{
	USB_PORT_STOP_MONITOR = 1,         ///< 停止监控
	USB_PORT_START_MONITOR,            ///< 开始监控
};


/**
  *  @brief     IO线程创建函数
  *  @author    <wfb> 
  *  @param[in] <无>
  *  @param[out]<无>
  *  @return    <线程ID号>
  *  @remark    2013-09-02 增加该注释
  *  @note
*/
int ioctl_thread_creat(void);

#endif /* _IOCTL_H_ */
