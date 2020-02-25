/**   @file []
   *  @brief    (1)处理时间消息功能
   *  @note
   *  @author   wangfengbo
   *  @date     2013-09-17
   *  @remarks  增加注释
*/
/*******************************************************************************/
/*                                    头文件                                         */
/*******************************************************************************/
//#ifndef _TIMEFUNC_H_
//#define _TIMEFUNC_H_
#ifndef __TIME_FUNC_H_
#define __TIME_FUNC_H_


/** timer id 定义区域  */
enum
{
    TIMER_ID_FIRST = 0,
    TIMER_ID_SEND_AV_START,             ///< 发送开启音视频
    TIMER_ID_STOP_USE_FRAME,            ///< 停止使用占用枕
    TIMER_ID_STOP_GATHER,               ///< 停止采集
    TIMER_ID_TEST,                      ///< 停止使用枕 停止采集超时时间
    TIMER_ID_START_GATHER,              ///< 开始采集
    TIMER_ID_SYSTEM_REBOOT,             ///< 系统重启
    TIMER_ID_CTL_NIGHT_MODE_CTL,        ///< 控制夜视模式
    TIMER_ID_CTL_NIGHT_LED_CTL,         ///< 控制夜视灯
    TIMER_ID_CTL_VIDEO_ALARM,           ///< 控制视频报警 根据标记是否打开
    TIMER_ID_LAST,
};

/** timer 状态  */
enum
{
	TIMER_ENABLE = 0,             ///< 时钟使能
	TIMER_DISABLE,                ///< 时钟非使能
};

/**
  *  @brief     时间消息处理函数
  *  @author    <wfb> 
  *  @param[in] <无>
  *  @param[out]<无>
  *  @return    <无>
  *  @remark    2013-09-02 增加该注释
  *  @note
*/
extern void chile_process_timer_handler();

/**
  *  @brief      调用该函数将重新开始该timer，timer的超时为上次设置的值，若从没设置过，则为初始值
  *  @author     <wfb> 
  *  @param[in]  <lTimerId>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark    2013-09-02 增加该注释
  *  @note
*/
void start_timer_by_id(int lTimerId);

/**
  *  @brief      重新设置timer的超时时间，但不会修改正在运行timer的剩余时间
  *  @author     <wfb> 
  *  @param[in]  <lTimerId :   要修改参数的timer id 所有的id都在TimeFunc.h中被定义>
  *  @param[in]  <unTimerExpd: timer的超时时间，若设为0，则默认上次不变，超时时间会在timer下次开始时使用>
  *  @param[in]  <pTimerData:  timer处理函数的参数，若为NULL，则默认使用上次，设置完参数后，当该timer到期时，立即被使用>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark    2013-09-02 增加该注释
  *  @note
*/
void reset_timer_param_by_id(int lTimerId, unsigned int unTimerExpd, char *pTimerData);


/**
  *  @brief     挂起timer
  *  @author    <wfb> 
  *  @param[in] <lTimerId>
  *  @param[out]<无>
  *  @return    <无>
  *  @remark    2013-09-02 增加该注释
  *  @note
*/
void suspend_timer_by_id(int lTimerId);



#endif /* __TIME_FUNC_H_ */
