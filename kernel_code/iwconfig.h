/**   @file []
   *  @brief    线程公用函数
   *  @note
   *  @author   wangfengbo
   *  @date     2013-9-2
   *  @remarks  增加注释
*/
#ifndef __IWCONFIG_H_
#define __IWCONFIG_H_



/**
  @brief 获取ra0参数
  @author wfb
  @param[in] struct wireless_info *pstWireLessInfo
  @param[out] 无
  @return 无
  @remark 2013-11－14 增加该函数
  @note
*/
int iwconfig_ra0(struct wireless_info *pstWireLessInfo);

/**
  @brief 获取频率字符串
  @author wfb
  @param[in] char *buffer, int buflen, double freq
  @param[out] 无
  @return 无
  @remark 2013-11－14 增加该函数
  @note
*/
void iw_get_freq_str_value(char *buffer, int buflen, double freq);

/**
  @brief 获取ra0参数
  @author wfb
  @param[in] struct wireless_info *pstWireLessInfo
  @param[out] 无
  @return 无
  @remark 2013-11－14 增加该函数
  @note
*/
void iw_get_wifi_signal(char *buffer);

/**
  @brief 实现iwconfig_essid_off
  @author wfb
  @param[in] pacIfName:设备名称
  @param[out] 无
  @return 无
  @remark 2013-11-25 增加该函数
  @note
*/
void iwconfig_essid_off(char *pacIfName);

#endif /* __IWCONFIG_H_ */
