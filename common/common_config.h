/**   @file []
   *  @brief    公用flash配置获取
   *  @note
   *  @author   wangfengbo
   *  @date     2013-9-2
   *  @remarks  增加注释
*/
#ifndef __COMMON_FLASH_H
#define __COMMON_FLASH_H

#include "common.h"

#define FILENOTFOUND -4
#define ITEMISSEXIST -2
#define ITEMNOTFOUND -3
#define NOTFOUND -3

#define DEFAULT_CONFIG        "/usr/share/default_config"
#define DEFAULT_ADD_CONFIG    "/usr/share/default_add_config"
#define FLASH_CONFIG_PATH     "/usr/share/flash_config_path"
#define DEFAULT_TMP_CONFIG    "/usr/share/tmp_config"
#define USER_CONFIG           "/etc/Wireless/RT2860/RT2860.dat"
#define CONFIG_TEMP_PATTERN   "/etc/Wireless/RT2860/tmpcfgXXXXXX"

/** 配置项内容长度 */
#define ITEM_VALUE_MAX_LEN          255


#define CONFIG_TEMP_PATTERN_SIZE    sizeof(CONFIG_TEMP_PATTERN)


/** 定义以下变量是为了能快速查找  注意每个宏的值不能改变 */
#define CONFIGADRESSBASE                          0
#define CONFIG_WIFI_MODE                         (CONFIGADRESSBASE+0)
#define CONFIG_WIFI_ACTIVE                       (CONFIGADRESSBASE+1)
#define CONFIG_WIFI_IP                           (CONFIGADRESSBASE+2)
#define CONFIG_WIFI_SUBNET                       (CONFIGADRESSBASE+3)
#define CONFIG_WIFI_GATEWAY                      (CONFIGADRESSBASE+4)
#define CONFIG_WIFI_DNS                          (CONFIGADRESSBASE+5)
#define CONFIG_NETTRANSMIT_PORT                  (CONFIGADRESSBASE+6)
#define CONFIG_HTTP_PORT                         (CONFIGADRESSBASE+7)
#define CONFIG_AUTOCHANNELSELECT                 (CONFIGADRESSBASE+8)
#define CONFIG_CHANNEL                           (CONFIGADRESSBASE+9)
#define CONFIG_UPNP_USE_IGD                      (CONFIGADRESSBASE+10)
#define CONFIG_WIRED_MODE                        (CONFIGADRESSBASE+11)
#define CONFIG_WIRED_IP                          (CONFIGADRESSBASE+12)
#define CONFIG_WIRED_SUBNET                      (CONFIGADRESSBASE+13)
#define CONFIG_WIRED_GATEWAY                     (CONFIGADRESSBASE+14)
#define CONFIG_WIRED_DNS                         (CONFIGADRESSBASE+15)
#define CONFIG_ALARM_MOTION                      (CONFIGADRESSBASE+16)
#define CONFIG_ALARM_MOTION_SENSITIVITY          (CONFIGADRESSBASE+17)
#define CONFIG_ALARM_MOTION_REGION               (CONFIGADRESSBASE+18)
#define CONFIG_ALARM_AUDIO                       (CONFIGADRESSBASE+19)
#define CONFIG_ALARM_AUDIO_SENSITIVITY           (CONFIGADRESSBASE+20)
#define CONFIG_ALARM_BATTERY                     (CONFIGADRESSBASE+21)
#define CONFIG_ALARM_IMAGE_UPLOAD                (CONFIGADRESSBASE+22)
#define CONFIG_ALARM_VIDEO_UPLOAD                (CONFIGADRESSBASE+23)
#define CONFIG_ALARM_START_TIME                  (CONFIGADRESSBASE+24)
#define CONFIG_ALARM_END_TIME                    (CONFIGADRESSBASE+25)
#define CONFIG_NTPSYNC                           (CONFIGADRESSBASE+26)
#define CONFIG_NTPSERVERIP                       (CONFIGADRESSBASE+27)
#define CONFIG_DEVICE_SERIAL                     (CONFIGADRESSBASE+28)
#define CONFIG_SYSTEM_FIRMWARE_VERSION           (CONFIGADRESSBASE+29)
#define CONFIG_APP_FIRMWARE_VERSION              (CONFIGADRESSBASE+30)
#define CONFIG_LIGHT_NET                         (CONFIGADRESSBASE+31)
#define CONFIG_LIGHT_POWER                       (CONFIGADRESSBASE+32)
#define CONFIG_LIGHT_NIGHT                       (CONFIGADRESSBASE+33)
#define CONFIG_WIFI_ENRTYPE                      (CONFIGADRESSBASE+34)
#define CONFIG_WIFI_SSID                         (CONFIGADRESSBASE+35)
#define CONFIG_WIFI_PASSWD                       (CONFIGADRESSBASE+36)
#define CONFIG_WIFI_KEYID                        (CONFIGADRESSBASE+37)
#define CONFIG_CONF_VERSION                      (CONFIGADRESSBASE+38)
#define CONFIG_CAMERA_TYPE                       (CONFIGADRESSBASE+39)
#define CONFIG_CAMERA_ENCRYTIONID                (CONFIGADRESSBASE+40)
#define CONFIG_SERVER_URL                        (CONFIGADRESSBASE+41)
#define CONFIG_VIDEO_SIZE                        (CONFIGADRESSBASE+42)
#define CONFIG_VIDEO_BRIGHT                      (CONFIGADRESSBASE+43)
#define CONFIG_VIDEO_CONSTRACT                   (CONFIGADRESSBASE+44)
#define CONFIG_VIDEO_HFLIP                       (CONFIGADRESSBASE+45)
#define CONFIG_VIDEO_VFLIP                       (CONFIGADRESSBASE+46)
#define CONFIG_VIDEO_RATE                        (CONFIGADRESSBASE+47)
#define CONFIG_VIDEO_SIMPLE                      (CONFIGADRESSBASE+48)
#define CONFIG_AUDIO_CHANNEL                     (CONFIGADRESSBASE+49)
#define CONFIG_AUDIO_SIMPLE                      (CONFIGADRESSBASE+50)
#define CONFIG_AUDIO_VOLUME                      (CONFIGADRESSBASE+51)
#define CONFIG_P2P_UID                           (CONFIGADRESSBASE+52)

/** 目前没有增加UID 若增加将下边的宏加一  TODO */
#define TABLE_ITEM_NUM                           53

/**
  *  @brief      初始化flash信息到内存
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int init_flash_config_parameters(void);

 /**
   *  @brief      设置配置信息到内存,但不直接写到flash，当flash读写线程收到更新flash的消息后，会把内存
   *              中的值写入到flash，与此同时，flash每隔5分钟，会自动将内存上与flash不同步的配置信息写入到flash。
   *  @author     <wfb> 
   *  @param[in]  <lItemIndex : 配置项的索引  这个在 common_config.h中被定义>
   *  @param[in]  <pcItemValue：配置项的值，这里只支持字符型，若是其他非字符型数据，需将其转换成字符型。>
   *  @param[in]  <lItemLen：配置项的字符串长度(strlen(pcItemValue))，最大支持ITEM_VALUE_MAX_LEN（255）>
   *  @param[out] <无>
   *  @return     <EXIT_FAILURE EXIT_SUCCESS>
   *  @remark     2013-09-02 增加该注释
   *  @note
 */
int set_config_value_to_ram(int lItemIndex,char *pcItemValue, int lItemLen);

/**
  *  @brief       调用该函数，将直接把配置信息写入到内存和flash
  *  @author     <wfb> 
  *  @param[in]  <lItemIndex : 配置项的索引  这个在 common_config.h中被定义>
  *  @param[in]  <pcItemValue：配置项的值，这里只支持字符型，若是其他非字符型数据，需将其转换成字符型。>
  *  @param[in]  <lItemLen：配置项的字符串长度(strlen(pcItemValue))，最大支持ITEM_VALUE_MAX_LEN（255）>
  *  @param[out] <无>
  *  @return     <EXIT_FAILURE EXIT_SUCCESS>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int set_config_value_to_flash(int lItemIndex,char *pcItemValue, int lItemLen);

/**
  *  @brief       获取配置信息
  *  @author     <wfb> 
  *  @param[in]  <lItemIndex : 配置项的索引>
  *  @param[in]  <pcItemValue：配置项的值>
  *  @param[in]  <lItemLen：配置项的字符串长度>
  *  @param[out] <无>
  *  @return     <EXIT_FAILURE EXIT_SUCCESS>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int get_config_item_value(int lItemIndex,char *pcItemValue, int *plItemLen);

/**
  *  @brief      获取mac地址
  *  @author     <wfb> 
  *  @param[in]  <pcMacAddrValue: 获取mac地址指针>
  *  @param[in]  <plLen: mac地址长度>
  *  @param[out] <无>
  *  @return     <EXIT_FAILURE EXIT_SUCCESS>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int get_mac_addr_value(char *pcMacAddrValue, int *plLen);

/**
  *  @brief      设置mac地址
  *  @author     <wfb> 
  *  @param[in]  <pcMacAddrValue:  mac地址指针>
  *  @param[out] <无>
  *  @return     <EXIT_FAILURE EXIT_SUCCESS>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int set_mac_addr_value(char *pcMacAddrValue);

/**
  *  @brief     获取pb_key
  *  @author     <wfb> 
  *  @param[in]  <pcPbKeyValue： 获取pb_key指针>
  *  @param[in]  <plLen： pb_key 长度>
  *  @param[out] <无>
  *  @return     <EXIT_FAILURE EXIT_SUCCESS>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int get_pb_key_value(char *pcPbKeyValue, int *plLen);

/**
  *  @brief     设置pbkey
  *  @author     <wfb> 
  *  @param[in]  <pcMacAddrValue： mac地址指针>
  *  @param[out] <无>
  *  @return     <EXIT_FAILURE EXIT_SUCCESS>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int set_pb_key_value(const char *pcPbKeyValue, int lPbKeySize);

/**
  *  @brief      将ram中的值更新到flash，这个函数只有在flash读写线程中才可以被调用
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <EXIT_FAILURE EXIT_SUCCESS>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void updata_ram_config_to_flash(void);

/**
  *  @brief      快速更新根据ram更新
  *  @author     <wfb> 
  *  @param[in]  <nParaFlag TRUE:更新,FALSE: 若ram中的新旧值均相同则不更新，否则 则更新>
  *  @param[out] <无>
  *  @return     <EXIT_FAILURE EXIT_SUCCESS>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int fast_updata_config_by_flag(int nParaFlag);

#endif//__COMMON_FLASH_H

