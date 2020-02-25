#ifndef COMMON_H_
#define COMMON_H_

#include <pthread.h>
#include "pdatatype.h"

#define SUCCESS  0
#define FAIL	-1

#define SYSTME_VERSION_LEN 20

#ifndef NULL
#define NULL 0
#endif

#define ICAMERA_NETWORK_HEAD		"IBBM"
#define ICAMERA_NETWORK_VERSION		1 //iBaby Monitor M3s开始使用的版本号

#define PROCESS_SLEEP_TIME 100000 //10ms
#define THREAD_SLEEP_TIME 100000 //10ms
#define THREAD_CUT_OUT_TIME 0 //0ms
#define MSG_MAX_LEN 1024
/*message queue key definition: process and thread*/
enum {
	CHILD_PROCESS_MSG_KEY = 1000,
	CHILD_THREAD_MSG_KEY,
	UPGEADE_REV_DOUPGRADE_MSG_KEY,
	UPGEADE_SEND_DOUPGRADE_MSG_KEY,
	WAN_HTTPS_SEND_HTTPS_MSG_KEY,
	WAN_HTTPS_RETURNSERVER_MSG_KEY,
	WANGSU_THREAD_MSG_KEY,
	NETWORKSET_MSG_KEY,
	P2P_MSG_KEY,
};

enum {
	CHILD_PROCESS_MSG_TYPE  = 0,
	AUDIO_MSG_TYPE          = 1,
	AUDIO_ALARM_MSG_TYPE    = 2,
	AUDIO_SEND_MSG_TYPE     = 3,
	DDNS_SERVER_MSG_TYPE    = 4,
	FLASH_RW_MSG_TYPE       = 5,
	FORWARDING_MSG_TYPE     = 6,
	HTTP_SERVER_MSG_TYPE    = 7,
	MFI_MSG_TYPE            = 8,
	NETWORK_SET_MSG_TYPE    = 9,
	NEWS_CHANNEL_MSG_TYPE   = 10,
	NTP_MSG_TYPE            = 11,
	PLAY_BACK_MSG_TYPE      = 12,
	RECV_PLAY_BACK_MSG_TYPE = 13,
	SERIAL_PORTS_MSG_TYPE   = 14,
	TOOLING_MSG_TYPE        = 15,
	UDP_SERVER_MSG_TYPE     = 16,
	UPGRADE_MSG_TYPE        = 17,
	UPNP_MSG_TYPE           = 18,
	VIDEO_MSG_TYPE          = 19,
	VIDEO_ALARM_MSG_TYPE    = 20,
	VIDEO_SEND_MSG_TYPE     = 21,
	IO_CTL_MSG_TYPE         = 22,
	CONTROL_MSG_TYPE        = 23,
	P2P_MSG_TYPE            = 24,
	LAST_TYPE               = 25,
	HTTPS_SERVER_DO_TYPE	= 26,
};

/** 音视频 开始或停止采集 */
enum
{
	CMD_START_CAPTURE = 0x01,      ///> 音视频停止采集
    CMD_STOP_GATHER_AUDIO = 0x2,   ///> 音频停止采集
};

/** Broadcast Msg Type */
enum
{
	CMD_AV_STARTED = 0x88,         ///> Audio and Video Started Msg
	CMD_STOP_ALL_USE_FRAME = 0xf0, ///> Stop Frame
	CMD_STOP_SEND_DATA = 0xf1,     ///> 网络断连后 TUTK TCP停止发送数据
};

/** 设备类型 */
enum
{
	DEVICE_TYPE_INVALID = -1,
	DEVICE_TYPE_M2 = 0,              ///<正常运行
	DEVICE_TYPE_M3S,                 ///<线程挂起
};

/** 串口上报的电池状态 */
enum
{
	BATTERY_POWER_STATE_NORMAL = 4,
	BATTERY_POWER_STATE_CHARGE = 5,
	BATTERY_POWER_STATE_LOW = 6,
};

/** wifi登录消息 这个宏的值不允许变动  */
enum
{
	WIFI_LOGIN_FAILURE = 0,         ///<WIFI登录失败
	WIFI_LOGIN_SUCCESS = 1,         ///<WIFI登录成功
};

/** dhcp活的成功 这个宏的值不允许变动 */
enum
{
	GET_IP_FAILURE = 0,             ///<WIFI登录失败
	GET_IP_SUCCESS = 1,             ///<WIFI登录失败

};


/** wifi断连 */
enum
{
	WIFI_DISCONNECT_STOP_SEND_DATA = 0,  ///<停止发送数据 重连
	WIFI_DISCONNECT_RECONNECT = 1,       ///<重连
};

/** message type which identify what kind of message can receive*/
#define MAX_MSG_LEN (512-16)

/** 定义消息队列的最大消息长度 */
#define MAX_MSG_QUEUE_LEN          1024

/** 定义网络设置相关 内容 ip地址 子网掩码  长度*/
#define IPV4_ATTRIBUTE_LEN          16
/** 定义网络设置相关 内容 ssid长度 */
#define WIFI_SSID_LEN              128
/** 定义网络设置相关 内容 密码长度 */
#define WIFI_CODE_LEN              128

/** definition of MsgData type, used for message transmit */
struct MsgData {
	long int type;
	int cmd;
	char sign[4];
	int len;
//	char data[MAX_MSG_LEN];
}__attribute__ ((packed));
typedef struct MsgData MsgData;

typedef struct childProcessMsgStruct{
	long int msg_type;
	int Thread_cmd;
	char E_xin[4];
	int length;
	char data[MAX_MSG_LEN];
}Child_Process_Msg;


/** 定义网络设置信息结构体 若有需求再增加  但总长度不要超过1024 */
typedef struct _tag_netset_info_
{
	int  lNetType;                         ///> 网络类型 有线  或是 无线
	int  lIpType;                          ///> ip类型 动态或是静态
	int  lWifiType;                        ///> wifi 类型
	char acIpAddr[IPV4_ATTRIBUTE_LEN];     ///> ip地址
    char acSubNetMask[IPV4_ATTRIBUTE_LEN]; ///> 子网掩码
    char acGateWay[IPV4_ATTRIBUTE_LEN];    ///> 网关
    char acWiredDNS[IPV4_ATTRIBUTE_LEN];   ///> 有线网 DNS
    char acWifiDNS[IPV4_ATTRIBUTE_LEN];    ///> 无线网 DNS
    char acSSID[WIFI_SSID_LEN];            ///> ssid
    char acCode[WIFI_CODE_LEN];            ///> 密码
}netset_info_t;

/**定义网络状态信息 这个结构体用来查询 当前网络状态  */
typedef struct _tag_netset_state_
{
	int lNetType;
	int lIpType;
    int lNetConnectState;
}netset_state_t;

/**wifi信号强度[0-5]: wifi频率 [6]:   wifi质量[7]:wifi信号强度 [8]:wifi噪声强度*/
typedef struct _tag_wifi_signal_level
{
	char acWifiInfo[10];

}wifi_signal_level_t;

/**定义无线类型 */
enum
{
    WIFI_TYPE_OPEN_NONE = 11,
    WIFI_TYPE_WEP_OPEN,
    WIFI_TYPE_WEP_SHARED,
    WIFI_TYPE_WPAPSK_TKIP,
    WIFI_TYPE_WPAPSK_AES,
    WIFI_TYPE_WPA2PSK_TKIP,
    WIFI_TYPE_WPA2PSK_AES,
};

/**网络类型  类型值：0 < x <256 */
enum
{
	NET_TYPE_WIRED = 1,        /*!> 有线网  */
	NET_TYPE_WIRELESS,         /*!> 无线网  */
};

/**ip类型  类型值不能大约256 */
enum
{
	IP_TYPE_DYNAMIC = 1,       /*!> 动态ip */
	IP_TYPE_STATIC,            /*!> 静态ip */
};

/**设置ip结果 静态的应该设置后就会成功 但dhcp不一定会成功 类型值不能大约256 */
enum
{
	SET_IP_SUCCESS = 1,
	SET_IP_FAILURE,
};

/**当前网络连接状态   类型值不能大约256 */
enum
{
	NET_CONNECT_SUCCESS = 1,
	NET_CONNECT_FAILURE,
	NET_CONNECT_DISCONNECT,
};

enum
{
	STEP_SET_START = 1,
	STEP_SET_END,
};

/**设置网络的状态  网络设置 状态机的状态 */
enum
{
    SET_NET_STATE_INVALID = 0,              ///< 当前无效网络状态 初始化的时候会用这个值
    SET_NET_STATE_READY_SET,                ///< 准备设置
    SET_NET_STATE_SET_TYPE,                 ///< 设置网络 有线 或是 无线
    SET_NET_STATE_SET_WIRED_TYPE_OK,        ///< 设置有限网ok
    SET_NET_STATE_SET_WIFI_OK,              ///< 设置wifi成功
    SET_NET_STATE_WIFI_LOGIN_OK,            ///< WIFI加入成功
    SET_NET_STATE_SET_STATIC_IP,            ///< 当前设置静态ip地址
    SET_NET_STATE_SET_DHCP,                 ///< 当前正在设置dhcp
    SET_NET_STATE_SET_IP_SUCCESS,           ///< 设置ip成功
    SET_NET_STATE_SET_IP_FAILURE,           ///< 设置ip失败
    SET_NET_STATE_CHECK_NET_CONNECT,        ///< 检测网络连接
    SET_NET_STATE_NET_CONNECT_SUCCESS,      ///< 网络连接已经ok
    SET_NET_STATE_NET_CONNECT_FAILURE,      ///< 网络连接已经失败
    SET_NET_STATE_NET_INTERVAL_CHECK,       ///< 网络定期检测
};

/*CHILD_PROCESS_MSG_TYPE = 0,
AUDIO_MSG_TYPE,
AUDIO_ALARM_MSG_TYPE,
AUDIO_SEND_MSG_TYPE,
DDNS_SERVER_MSG_TYPE,
FLASH_RW_MSG_TYPE,
FORWARDING_MSG_TYPE,
HTTP_SERVER_MSG_TYPE,
MFI_MSG_TYPE,
NETWORKSET_MSG_TYPE,
NEWSCHANNEL_MSG_TYPE,
NTP_MSG_TYPE,
PLAY_BACK_MSG_TYPE,
RECV_PLAY_BACK_MSG_TYPE,
SERIAL_PORTS_MSG_TYPE,
TOOLING_MSG_TYPE,
UDP_SERVER_MSG_TYPE,
UPGRADE_MSG_TYPE,
UPNP_MSG_TYPE,
VIDEO_MSG_TYPE,
VIDEO_ALARM_MSG_TYPE,
VIDEO_SEND_MSG_TYPE
*/
extern volatile int rChildProcess;
extern volatile int rAudio;
extern volatile int rAudioAlarm;
extern volatile int rAudioSend;
extern volatile int rDdnsServer;
extern volatile int rFlashRW;
extern volatile int rForwarding;
extern volatile int rHTTPServer;
extern volatile int rMFI;
extern volatile int rNetWorkSet;
extern volatile int rNetWorkCheck;
extern volatile int rNewsChannel;
extern volatile int rNTP;
extern volatile int rplayback;
extern volatile int rrevplayback;
extern volatile int rSerialPorts;
extern volatile int rTooling;
extern volatile int rUdpserver;
extern volatile int rUpgrade;
extern volatile int rUPNP;
extern volatile int rVideo;
extern volatile int rVideoAlarm;
extern volatile int rVideoSend;
extern volatile int rIoCtl;
extern volatile int rP2p;

/**定义线程已停止使用音视频祯 （zhen，这个字在linux下的拼音输入法中没找到）*/
extern volatile int g_lAudioFlag;
extern volatile int g_lAudio;
extern volatile int g_lAudioAlarm;
extern volatile int g_lAudioSend;
extern volatile int g_lDdnsServer;
extern volatile int g_lFlashRW;
extern volatile int g_lForwarding;
extern volatile int g_lHTTPServer;
extern volatile int g_lMFI;
extern volatile int g_lNetWorkSet;
extern volatile int g_lNewsChannel;
extern volatile int g_lNTP;
extern volatile int g_lplayback;
extern volatile int g_lRevplayback;
extern volatile int g_lSerialPorts;
extern volatile int g_lTooling;
extern volatile int g_lUdpserver;
extern volatile int g_lUpgrade;
extern volatile int g_lUPNP;
extern volatile int g_lVideo;
extern volatile int g_lVideoAlarm;
extern volatile int g_lVideoSend;
extern volatile int g_lIoCtl;
extern volatile int g_lP2p;

/**定义音频。视频停止采集数据 */
extern volatile int g_lVideoStopFlag;
extern volatile int g_lAudioStopFlag;

/**定义音频。视频停止采集数据 */
extern volatile struct timeval g_stTimeVal;

/**音视频出问题标志 */
extern volatile int g_nAvErrorFlag;

/**设备类型 */
extern volatile int g_nDeviceType;

/**信号强度 */
extern volatile char g_acWifiSignalLevel[10];

/**wifi 检测间隔 */
extern volatile int g_nWifiCheckInterval;    ///>单位是微秒

/**tutk登录成功并发送快闪标志 */
extern volatile int g_nTutkLoginOkFlag;

/** 夜视状态控制标志 打开夜视或关闭夜视 */
extern volatile int g_nLightNightStateCtlFlag;

/** 是否存flash */
extern volatile int g_nIfWriteFlashFlag;

/** 固件版本号*/
extern volatile char Cam_firmware[SYSTME_VERSION_LEN];

/** 摄像头类型 */
extern volatile char Cam_camera_type[SYSTME_VERSION_LEN];

/** 硬件版本号 */
extern volatile char Cam_hardware[SYSTME_VERSION_LEN];

/** 摄像头外型 */
extern volatile char Cam_camera_model[SYSTME_VERSION_LEN];

/** 固件版本号*/
extern volatile char Cam_firmware_HTTP[SYSTME_VERSION_LEN];

/** 摄像头类型 */
extern volatile char Cam_camera_type_HTTP[SYSTME_VERSION_LEN];

/** 硬件版本号 */
extern volatile char Cam_hardware_HTTP[SYSTME_VERSION_LEN];

/** 摄像头外型 */
extern volatile char Cam_camera_model_HTTP[SYSTME_VERSION_LEN];

/** tutk发送标志 */
extern volatile int g_nRcvTutkLoginOKFlag;

/** HTTP发送标志 */
extern volatile int g_nRcvHttpAuthenOKFlag;

/** 电量标志1 */
extern volatile char energyStatus_BATTERY ;

/** 电量标志2 */
extern volatile char chargeStatus_BATTERY ;

/** 夜间模式 */
extern volatile int g_nNightModeFlag;

/** 夜视灯状态 */
extern volatile int g_nNightLedCtl;

/** add these para to test speanker */
volatile int g_nSpeakerFlag1;
volatile int g_nSpeakerFlag2;
volatile int g_nSpeakerFlag3;

/** add audio test flag */
volatile int g_nAudioFlag1;
volatile int g_nAudioFlag2;
volatile int g_nAudioFlag3;

typedef struct ThreadStatus_t {
	int id;
	time_t tm;
}ThreadStatus_t;

/** 记录当前的ip地址 */
extern volatile char g_acWifiIpInfo[16];

extern volatile ThreadStatus_t threadStatus[];
extern int child_getThreadStatus(int id, void *arg);
extern int child_setThreadStatus(int id, void *arg);

enum {
	NET_INFO_NO_SSID_PASSWD = 0,
	NET_INFO_CONN_WIFI = 1,
	NET_INFO_DHCP = 2,
	NET_INFO_DHCP_FAIL = 3,
	NET_INFO_REQ_RL = 4,
	NET_INFO_REQ_UID = 5,
	NET_INFO_UID_OK = 6,
	NET_INFO_TUTK_LOGIN_OK = 7,
	NET_INFO_TUTK_LOGIN_FAIL = 8,
};

extern volatile int network_info;

#define setNetStatusInfo(info) log_printf("%s setNetStatusInfo %d %d\n", __FUNCTION__, __LINE__, info); (network_info = info)

#define getNetStatusInfo() (network_info)

#endif
