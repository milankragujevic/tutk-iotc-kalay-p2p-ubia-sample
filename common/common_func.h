/**   @file []
   *  @brief    线程公用函数
   *  @note
   *  @author   wangfengbo
   *  @date     2013-9-2
   *  @remarks  增加注释
*/
#ifndef __COMMON_FUNC_H_
#define __COMMON_FUNC_H_


/** 指示灯颜色类型 注意这个值不能超过0xff */
enum
{
	LED_TYPE_NET = 1,               ///<蓝灯   实际上就是网络灯 一个管教控制  M3S
	LED_TYPE_POWER = 2,             ///<橘黄灯 实际上就是电源灯 一个管教控制  M3S
	LED_TYPE_NIGHT = 3,             ///<夜视灯 一个管教控制                   M2 M3S
	LED_TYPE_NET_POWER = 4,         ///<网络电源灯 1个灯 2个管教控制          M2
};


/** 指示灯控制类别 */
enum
{
	LED_CTL_TYPE_INVALID = 0,     ///<无效控制
	LED_CTL_TYPE_ON,              ///<指示灯开
	LED_CTL_TYPE_OFF,             ///<指示灯关
	LED_CTL_TYPE_FAST_FLK,        ///<指示灯快闪
	LED_CTL_TYPE_SLOW_FLK,        ///<指示灯慢闪
	LED_CTL_TYPE_BLINK,           ///<眨眼灯
	LED_CTL_TYPE_DOZE,            ///<瞌睡灯
};

/* 网络电源灯颜色类型 该值不可改变 已经和电机的确定ok */
enum
{
	LED_COLOR_TYPE_GREEN = 1,   ///<M2绿灯
	LED_COLOR_TYPE_ORANGE = 2,  ///<M2黄灯
	LED_COLOR_TYPE_RED = 3,     ///<M2红灯
};

/** 网路线程上报的网路状态 */
enum
{
	NET_STATE_NO_IP,                      ///<未获得ip
	NET_STATE_HAVE_IP,                    ///<已获得ip
	NET_STATE_OK,                         ///<网络功能ok
};

/** 指示灯使用状态 */
enum
{
	LED_FLAG_NOT_WORK = 1,                ///<led不在工作
	LED_FLAG_WORK,                        ///<led在工作
};


/** 是否是夜视模式 */
enum
{
	LED_NIGHT_MODE = 1,
	LED_NOT_NIGHT_MODE,
};
/** 网络状态控制者 */
enum
{
	NET_STATE_CONTROL_HTTP = 1,           ///<LED控制者 http
	NET_STATE_CONTROL_TUTK,               ///<LED控制者 tutk
};

/** 指示灯控制 */
enum
{
	LED_CONTROL_ITEM_NET_STATE,           ///<网络状态
	LED_CONTROL_ITEM_NIGHT_MODE,          ///<夜间模式
	LED_CONTROL_ITEM_IF_WORK,             ///<指示灯是否工作
	LED_CONTROL_ITEM_BATTERY_STATE,       ///<电池状态
};

/* ping结构体 */
typedef struct _tag_ping_info_
{
   char  acIpAddr[16];  //待ping的ip地址
   int   nTimeOut;      //超时时间，单位是秒
   int   nPackageNum;   //发包数
}stPingInfo;

/**
  *  @brief      向子线程发送消息接口
  *  @author     <wfb> 
  *  @param[in]  <ulThreadId: 发送线程>
  *  @param[in]  <Cmd:        发送命令>
  *  @param[in]  <pMsgData:   数据长度, 这里最大长度MAX_MSG_LEN>
  *  @param[in]  unDataLen:   数据指针>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int send_msg_to_child_process(long int ulThreadId, int Cmd, void *pMsgData, unsigned int unDataLen);

/**
  *  @brief      获取同子线程通信消息队列
  *  @author     <wfb> 
  *  @param[in]  <plThreadMsgId:  发送线程>
  *  @param[in]  <plProcessMsgId: 发送命令>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int get_process_msg_queue_id(int *plThreadMsgId,  int *plProcessMsgId);

/**
  *  @brief      是否所有的线程都已回复停止使用音视频针的回复
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int is_all_thread_return_stop_use_frame_rep(void);

/**
  *  @brief      音视频是否停止采集
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int is_all_video_and_audio_stop_gather(void);

/**
  *  @brief      发送网络设置
  *  @author     <wfb> 
  *  @param[in]  <pstNetSetInfo： pstNetSetInfo：网络设置信息 在common.h中定义>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int send_message_to_set_net(netset_info_t *pstNetSetInfo);


/**
  *  @brief      设置状态灯状态
  *  @author     <wfb> 
  *  @param[in]  <nLedColor：   指示灯名称>
  *  @param[in]  <nLedCtlType： 指示灯控制类型>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void set_led_state(int nLedColor, int nLedCtlType);

/**
  *  @brief      获取指示灯状态
  *  @author     <wfb> 
  *  @param[in]  <nLedName：   指示灯名称>
  *  @param[in]  <nLedCtlType： 指示灯控制类型>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int get_led_state(int nLedName);

/**
  *  @brief      清空标志
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void reset_flag(void);

/**
  *  @brief      获取当前网络状态
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[in]  <无>
  *  @param[out] <pstNetSetState:网络状态结构体>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int get_cur_net_state(netset_state_t *pstNetSetState);

/**
  *  @brief      设置当前网络状态，有线或无线  静态或动态  连接情况，由于是三个变量
  *               若只设置一个变量一定要先读出然后再设置
  *  @author     <wfb> 
  *  @param[out]  <无>
  *  @param[in] <pstNetSetState:网络状态结构体>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int set_cur_net_state(netset_state_t *pstNetSetState);

/**
  *  @brief      led灯控制  若是M2 nLedType: LED_M2_TYPE_MIXED
  *  @author     <wfb> 
  *  @param[in]  <nLedType: 指示灯名称>
  *  @param[in]  <nCtlType: 指示灯控制类型>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void led_control(int nLedType, int nCtlType);

/**
  @brief      电池电量上报
  @author     <wfb> 
  @param[in]  <nBattaryPowerState:电池电量>
  @param[out] <无>
  @return     <无>
  @remark     2013-09-02 增加该注释
  @note
*/
void battary_power_state_report(int nBattaryPowerState);

/**
  *  @brief      通过flash配置设置wifi PARAM: null
  *  @author     <wfb> 
  *  @param[in]  <pstNetSetInfo: NULL>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void set_wifi_by_config(netset_info_t *pstNetSetInfo);

/**
  *  @brief      led是否工作控制
  *  @author     <wfb> 
  *  @param[in]  <nLedType: 指示灯类型>
  *  @param[in]  <nCtlType: 指示灯控制类型>
  *  @param[in]  <nWriteFlashFlag: 是否写Flash标志>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void led_if_work_control(int nLedType, int nCtlType, int nWriteFlashFlag);

/**
  *  @brief      是否没有使用的线程
  *  @author     <wfb> 
  *  @param[in]  <nThreadId: 线程ID>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int if_not_use_thread(int nThreadId);

/**
  *  @brief      发送更新消息
  *  @author     <wfb> 
  *  @param[in]  <nThreadId: 线程ID>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void updata_config_to_flash();

/**
  *  @brief      写文件 存储log信息 /etc/loginfo.txt
  *  @author     <wfb> 
  *  @param[in]  <nThreadId: 线程ID>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void write_log_info_to_file(char *pStrInfo);


/**
  *  @brief      获取wifi信号强度, 注意这个函数是对全局变量赋值
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <pcWifiInfo: 获取wifi信息结构体指针>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void get_wifi_signal_level(char *pcWifiInfo);


/**
  *  @brief      设置wifi信号轻度
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <pcWifiInfo: 设置wifi信息结构体指针>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int set_wifi_signal_level(char *pcWifiInfo);

/**
  *  @brief      wifi断连后，tutk和tcp停止发送数据
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void tutk_and_tcp_stop_send_data();

/**
  *  @brief      tutk登录成功 led闪灯
  *  @author     <wfb> 
  *  @param[in]  <nLedType: 指示灯名称>
  *  @param[in]  <nCtlType： 指示灯控制类型>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void tutk_login_ok_set_led(int nLedType, int nCtlType);

/**
  *  @brief      清空tutk登录成功标志
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void reset_tutk_login_flag();

/**
  *  @brief      向视频报警线程发送停止报警
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void send_msg_to_video_alarm_stop_alarm();

/**
  *  @brief      向视频报警线程发送开始报警
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void send_msg_to_video_alarm_start_alarm();

/**
  *  @brief      夜间模式处理函数
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void night_mode_ctl_msg_handler(unsigned int nNightModeFlag);

/**
  @brief 也是灯处理函数
  @author wfb 
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void night_led_if_work_msg_handler(unsigned int nNightLedCtl);


/**
  @brief led是否工作直接控制，主要是解决夜视灯在开关的时候会报警
  @author wfb 
  @param[in] nLedType: 指示灯类型
  @param[in] nCtlType: 指示灯控制类型
  @param[in] nWriteFlashFlag: 是否写Flash标志
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
          2014-03-05 修改该函数，增加对夜视灯的特殊处理 夜视灯要关闭报警功能
  @note
*/
void led_if_work_direct_control(int nLedType, int nCtlType, int nWriteFlashFlag);

/**
  @brief 清空tutk和http标志位
  @author wfb 
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该函数
  @note
*/
void reset_tutk_https_set_led_flag();

/**
  @brief HTTP或TUTK设置led闪灯
  @author wfb 
  @param[in] nControler： tutk或是http  NET_STATE_CONTROL_TUTK或NET_STATE_CONTROL_HTTP
  @param[out] 无
  @return 无
  @remark 2013-10-16 增加该函数 tutk和http都成功后才置灯
  @note
*/
void tutk_and_http_set_led(int nControler);

/**
  @brief 开始检测网络
  @author wfb 
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-19-25 增加该函数
  @note
*/
void start_check_net_connect(void);

/**
  @brief 直接设置led灯的状态  若是M2 nLedType: LED_M2_TYPE_MIXED
  @author wfb 
  @param[in] nLedType: 指示灯名称
  @param[in] nCtlType: 指示灯控制类型
  @param[out] 无
  @return 无
  @remark 2013-10-31 增加该函数
  @note
*/
void direct_set_led_state(int nLedType, int nCtlType);

/**
  *  @brief      system_stop_speaker_and_rebootr
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void system_stop_speaker_and_reboot();

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
void get_ip_addr(char *pcIpAddr, int nNetType);

/**
  *  @brief      void set_cur_ip_addr(char *pIpAddr)
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2014-02-26 增加该函数
  *  @note
*/
void set_cur_ip_addr(char *pIpAddr);

/**
  *  @brief      void get_cur_ip_addr(char *pIpAddr)
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2014-02-26 增加该函数
  *  @note
*/
void get_cur_ip_addr(char *pIpAddr);

/**
  @brief 正在联网或无网络可用
  @author wfb 
  @param[in] nLedType : 指示灯名称
  @param[in] nCtlType： 指示灯控制类型
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void net_no_ip_set_led(void);

/**
  @brief 获取ip成功设置led闪灯
  @author wfb 
  @param[in] nLedType : 指示灯名称
  @param[in] nCtlType： 指示灯控制类型
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void net_get_ip_ok_set_led(void);

/**
  @brief 网络连接成功设置led闪灯
  @author wfb 
  @param[in] nLedType : 指示灯名称
  @param[in] nCtlType： 指示灯控制类型
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
void net_connect_ok_set_led(void);


void send_msg_to_io_reboot_rt5350();


extern int init_Camera_info();


#define NIGHT_MODE_ON		1
#define NIGHT_MODE_OFF		2
typedef struct run_time_state_info{
	int nightmode_state; //1:on, 2:off
	int nightmode_start_time;

	pthread_mutex_t nm_mutex;
}RunTimeStateInfo;
extern RunTimeStateInfo rtsi;

void runtimeinfo_init(void);
void runtimeinfo_set_nm_state(int state);
int runtimeinfo_get_nm_state(void);
void runtimeinfo_set_nm_time(int time);
int runtimeinfo_get_nm_time(void);

extern int ping_source_func(stPingInfo *pstPingInfo);

/**
  @brief void common_write_log(const char *fmt, ...)
  @author <wfb> 
  @param[in] <fmt>
  @param[out] <无>
  @return <无>
  @remark 2014-02-26 增加该函数
  @note
*/
void common_write_log(const char *fmt, ...);

#endif /* __COMMON_FUNC_H_ */
