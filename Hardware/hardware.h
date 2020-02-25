/**   @file []
   *  @brief    (1)usb以及网口灯插拔监测  (2)状态灯设置 (3)usb切换
   *  @note
   *  @author   wangfengbo
   *  @date     2013-9-2
   *  @remarks  增加注释
*/

#ifndef _HARDWARE_H_
#define _HARDWARE_H_


/**
  @brief 硬件初始化,主义该函数无比在子进程调用,初始化后不deinit
  @author <wfb> 
  @param[in] <无>
  @param[out] <无>
  @return <成功：EXIT_SUCCESS   失败：EXIT_FAILURE>
  @remark 2014-02-28
  @note
*/
void hardware_init();

/**
  *  @brief      usb切换处到MFI
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <成功：EXIT_SUCCESS   失败：EXIT_FAILURE>
  *  @remark     2014-02-28 增加该注释
  *  @note
*/
int usb_switch_to_mfi(void);

/**
  *  @brief      usb切换处到cam
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <成功：EXIT_SUCCESS   失败：EXIT_FAILURE>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int usb_switch_to_cam(void);

/**
  *  @brief      关闭电源开关（切断）
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <成功：EXIT_SUCCESS   失败：EXIT_FAILURE>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int poweroff_usb(void);

/**
  *  @brief      闭合电源开关
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <成功：EXIT_SUCCESS   失败：EXIT_FAILURE>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int poweron_usb(void);

/**
  *  @brief      通过Io口获取usb口状态
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <DEVICE_CONNECT  DEVICE_NOT_CONNECT>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int get_usb_port_state_by_io(void);

/**
  @brief      获取网口状态
  @author     <wfb> 
  @param[in]  <无>
  @param[out] <无>
  @return     <PORT_STATE_CONNECT PORT_STATE_NOT_CONNECT>
  @remark     2013-09-02 增加该注释
  @note
*/
int get_net_port_state(void);

/**
  @brief 设置网络灯开
  @author <wfb> 
  @param[in] <无>
  @param[out]<无>
  @return
  @remark 2014-02-28 增加该函数
  @note
*/
int ioctl_set_net_led_on();

/**
  @brief 设置网络灯关
  @author <wfb> 
  @param[in] <无>
  @param[out]<无>
  @return
  @remark 2014-02-28 增加该函数
  @note
*/
int ioctl_set_net_led_off();

/**
  @brief 设置电源灯开
  @author <wfb> 
  @param[in] <无>
  @param[out]<无>
  @return
  @remark 2014-02-28 增加该函数
  @note
*/
int ioctl_set_power_led_on();

/**
  @brief 设置电源灯关
  @author <wfb> 
  @param[in] <无>
  @param[out]<无>
  @return
  @remark 2014-02-28 增加该函数
  @note
*/
int ioctl_set_power_led_off();

/**
  @brief 设置夜视灯开
  @author <wfb> 
  @param[in] <无>
  @param[out]<无>
  @return
  @remark 2014-02-28 增加该函数
  @note
*/
int ioctl_set_night_led_on();

/**
  @brief 设置夜视灯关
  @author <wfb> 
  @param[in] <无>
  @param[out]<无>
  @return
  @remark 2014-02-28 增加该函数
  @note
*/
int ioctl_set_night_led_off();

/**
  @brief 设置网络电源灯 绿色
  @author <wfb> 
  @param[in] <无>
  @param[out]<无>
  @return
  @remark 2014-02-28 增加该函数
  @note
*/
int ioctl_set_net_power_led_green();

/**
  @brief 设置网络电源灯 橘色
  @author <wfb> 
  @param[in] <无>
  @param[out]<无>
  @return
  @remark 2014-02-28 增加该函数
  @note
*/
int ioctl_set_net_power_led_orange();

/**
  @brief 设置网络电源灯 红色
  @author <wfb> 
  @param[in] <无>
  @param[out]<无>
  @return
  @remark 2014-02-28 增加该函数
  @note
*/
int ioctl_set_net_power_led_red();

/**
  @brief 设置网络电源灯关
  @author <wfb> 
  @param[in] <无>
  @param[out]<无>
  @return
  @remark 2014-02-28 增加该函数
  @note
*/
int ioctl_set_net_power_led_off();


#endif /* _HARDWARE_H_ */
