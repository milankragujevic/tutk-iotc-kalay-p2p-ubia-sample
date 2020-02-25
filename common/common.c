/**   @file []
   *  @brief    这里存储进程全局变量
   *  @note
   *  @author   wangfengbo
   *  @date     2013-9-2
   *  @remarks  增加注释
*/
#include <stdlib.h>
#include "common.h"

volatile int rChildProcess = 0;
volatile int rAudio = 0;
volatile int rAudioAlarm = 0;
volatile int rAudioSend = 0;
volatile int rDdnsServer = 0;
volatile int rFlashRW = 0;
volatile int rForwarding = 0;
volatile int rHTTPServer = 0;
volatile int rMFI = 0;
volatile int rNetWorkSet = 0;
volatile int rNetWorkCheck = 0;
volatile int rNewsChannel = 0;
volatile int rNTP = 0;
volatile int rplayback = 0;
volatile int rrevplayback = 0;
volatile int rSerialPorts = 0;
volatile int rTooling = 0;
volatile int rUdpserver = 0;
volatile int rUpgrade = 0;
volatile int rUPNP = 0;
volatile int rVideo = 0;
volatile int rVideoAlarm = 0;
volatile int rVideoSend = 0;
volatile int rIoCtl = 0;
volatile int rP2p = 0;

/** 定义线程已停止使用音视频祯 （zhen，这个字在linux下的拼音输入法中没找到） 1代表停止  0开始启用  初始化时开启 */
volatile int g_lAudioFlag = 0;
volatile int g_lAudio = 0;
volatile int g_lAudioAlarm = 0;
volatile int g_lAudioSend = 0;
volatile int g_lDdnsServer = 0;
volatile int g_lFlashRW = 0;
volatile int g_lForwarding = 0;
volatile int g_lHTTPServer = 0;
volatile int g_lMFI = 0;
volatile int g_lNetWorkSet = 0;
volatile int g_lNewsChannel = 0;
volatile int g_lNTP = 0;
volatile int g_lplayback = 0;
volatile int g_lRevplayback = 0;
volatile int g_lSerialPorts = 0;
volatile int g_lTooling = 0;
volatile int g_lUdpserver = 0;
volatile int g_lUpgrade = 0;
volatile int g_lUPNP = 0;
volatile int g_lVideo = 0;
volatile int g_lVideoAlarm = 0;
volatile int g_lVideoSend = 0;
volatile int g_lIoCtl = 0;
volatile int g_lP2p = 0;

/** 定义音频。视频停止采集数据 */
volatile int g_lVideoStopFlag = 0;
volatile int g_lAudioStopFlag = 0;

/** 时间结构体 getimofday测试时使用 */
volatile struct timeval g_stTimeVal;

/** 音视频出问题标志 */
volatile int g_nAvErrorFlag = FALSE;

/** 设备类型 */
volatile int g_nDeviceType = DEVICE_TYPE_INVALID;

/** 信号强度信息 */
volatile char g_acWifiSignalLevel[10] = {0};

/** wifi 检测间隔 */
volatile int g_nWifiCheckInterval = 5000000;    ///> 单位是微妙

/** tutk登录成功并发送快闪标志 */
volatile int g_nTutkLoginOkFlag = FALSE;

/** 夜视状态控制标志 打开夜视或关闭夜视 */
volatile int g_nLightNightStateCtlFlag = FALSE;

/** 夜视状态控制标志 打开夜视或关闭夜视 */
volatile int g_nIfWriteFlashFlag = FALSE;

/** 固件版本号*/
volatile char Cam_firmware[SYSTME_VERSION_LEN];

/** 摄像头类型 */
volatile char Cam_camera_type[SYSTME_VERSION_LEN];

/** 硬件版本号 */
volatile char Cam_hardware[SYSTME_VERSION_LEN];

/** 摄像头外型 */
volatile char Cam_camera_model[SYSTME_VERSION_LEN];

/** 固件版本号*/
volatile char Cam_firmware_HTTP[SYSTME_VERSION_LEN];

/** 摄像头类型 */
volatile char Cam_camera_type_HTTP[SYSTME_VERSION_LEN];

/** 硬件版本号 */
volatile char Cam_hardware_HTTP[SYSTME_VERSION_LEN];

/** 摄像头外型 */
volatile char Cam_camera_model_HTTP[SYSTME_VERSION_LEN];

/** tutk发送标志 */
volatile int g_nRcvTutkLoginOKFlag = FALSE;

/** HTTP发送标志 */
volatile int g_nRcvHttpAuthenOKFlag = FALSE;

/** 电量标志1 */
volatile char energyStatus_BATTERY ;

/** 电量标志2 */
volatile char chargeStatus_BATTERY ;

/** add these para to test speanker */
volatile int g_nSpeakerFlag1 = 1;
volatile int g_nSpeakerFlag2 = 1;
volatile int g_nSpeakerFlag3 = 1;

/** add audio test flag */
volatile int g_nAudioFlag1 = -1;
volatile int g_nAudioFlag2 = -1;
volatile int g_nAudioFlag3 = -1;

/** 记录当前的ip地址 */
volatile char g_acWifiIpInfo[16] = {0};

/** 夜间模式 */
volatile int g_nNightModeFlag = 0;

/** 夜视灯状态 */
volatile int g_nNightLedCtl = 0;

volatile int network_info = 0;

