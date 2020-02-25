/**	@file NewsChannel.c
 *  @brief brief description
 *  @note
 *  author: yangguozheng
 *  date: 2013-09-16
 *  mcdify record: creat this file.
 *  author: yangguozheng
 *  date: 2013-09-17
 *  modify record: add a function
 */

#ifndef NEWSCHANNEL_H_
#define NEWSCHANNEL_H_

#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include "common/utils.h"
#include "common/logfile.h"

#define SERVER_ADDR             "0.0.0.0"  ///< 本地ip
#define SERVER_PORT             10002      ///< 监听端口
#define LISTEN_MAX_NUM          5          ///< 最大用户量
#define MAX_USER_NUM            4          ///< 最大客户数量
//#define NULL_FD               -1         //空的描述符
#define NULL_THREAD             -1         ///< 空的线程描述
#define HEART_BEAT_TIME_OUT     (60 * 2)   ///< 默认超时时间
#define TCP_REQUEST_HEAD        "IBBM"     ///< 通信协议头部

#define TCP_DYNAMIC_PASSWORD(x)			(x) ///< 密码宏

#if 1
#define TEST_PRINT(format, arg...)\
	DEBUGPRINT(DEBUG_INFO, format, ##arg)
#else
#define TEST_PRINT(format, arg...)
#endif  ///< 测试使用的打印

/*＊ 通道类型 */
enum {
	NULL_CHANNLE,      ///< 空的通道类型
	CONTROL_CHANNEL,   ///< 控制通道
	VIDEO_CHANNEL,     ///< 视频通道
	AUDIO_CHANNEL,     ///< 音频通道
};

/*＊ 通道输出方向 */
enum {
    SOCK_DIRECTION_NULL,
    SOCK_DIRECTION_IN,
    SOCK_DIRECTION_OUT,
    SOCK_DIRECTION_IN_OUT,
};

/** 云台的位置命令 */
enum{
	CAMERA_HORIZONTAL = 1,   ///< 水平
	CAMERA_VERTICALITY = 2,  ///< 垂直
	CAMERA_MIDDLE = 3,       ///< 居中
};

/** 基本协议 */
enum {
    /** 连接认证 */
	NEWS_USER_JOIN_REQ                   = 140, ///< 用户加入
    NEWS_USER_VERIFY_REQ                 = 141, ///< 用户验证
    NEWS_USER_VERIFY_REP                 = 142, ///< 用户验证回应
    NEWS_USER_JOIN_REP                   = 143, ///< 用户加入回应
    /** 通道声明 */
	NEWS_CHANNEL_DECLARATION_REQ         = 150, ///< 通道声明
	NEWS_CHANNEL_DECLARATION_REP         = 151, ///< 通道声明回应
	NEWS_CHANNEL_CLOSE_REQ               = 152, ///< 通道关闭请求
	NEWS_CHANNEL_CLOSE_REP               = 153, ///< 通道关闭回应
	NEWS_CHANNEL_TOUCH_REQ               = 154, ///< 心跳包请求
	NEWS_CHANNEL_WRIGGLE_REP             = 155, ///< 心跳包回应
	NEWS_CHANNEL_TOUCH_REQ_V2            = 156, ///< 心跳包请求
	NEWS_CHANNEL_WRIGGLE_REP_V2          = 157, ///< 心跳包回应
    /** 网络灯控制 */
	NEWS_CTL_CHANNEL_NET_LED_QUERY_REQ   = 160, ///< 网络灯查询请求
	NEWS_CTL_CHANNEL_NET_LED_QUERY_REP   = 161, ///< 网络灯查询回应
	NEWS_CTL_CHANNEL_NET_LED_SET_REQ     = 162, ///< 网络灯设置
	NEWS_CTL_CHANNEL_NET_LED_SET_REP     = 163, ///< 网络灯设置回应
    /** 电源灯控制 */
	NEWS_CTL_CHANNEL_POWER_LED_QUERY_REQ = 170, ///< 电源灯查询请求
	NEWS_CTL_CHANNEL_POWER_LED_QUERY_REP = 171, ///< 电源灯查询回应
	NEWS_CTL_CHANNEL_POWER_LED_SET_REQ   = 172, ///< 电源灯设置请求
	NEWS_CTL_CHANNEL_POWER_LED_SET_REP   = 173, ///< 电源灯设置回应
    /** 夜视灯控制 */
	NEWS_CTL_CHANNEL_NIGHT_LED_QUERY_REQ = 180, ///< 夜视灯查询请求
	NEWS_CTL_CHANNEL_NIGHT_LED_QUERY_REP = 181, ///< 夜视灯查询回应
	NEWS_CTL_CHANNEL_NIGHT_LED_SET_REQ   = 182, ///< 夜视灯设置请求
	NEWS_CTL_CHANNEL_NIGHT_LED_SET_REP   = 183, ///< 夜视灯设置回应

	/** 系统命令 240 -259 */
    NEWS_SYS_REBOOT_REQ                  = 240, ///< 系统重启请求
    NEWS_SYS_REBOOT_REP                  = 241, ///< 系统重启请求回应
    /** 固件升级 */
    NEWS_SYS_UPGRADE_REQ                 = 242, ///< 系统升级请求
    NEWS_SYS_UPGRADE_REP                 = 243, ///< 系统升级请求回应
    NEWS_SYS_UPGRADE_PROCESS_IN          = 244, ///< 系统升级过程中输入包
    NEWS_SYS_UPGRADE_PROCESS_OUT         = 245, ///< 系统升级过程中输出包
    NEWS_SYS_UPGRADE_OVER                = 247, ///< 系统升级完成
    NEWS_SYS_UPGRADE_PROCESS_CANCEL_REQ  = 248, ///< 取消系统升级请求
    NEWS_SYS_UPGRADE_PROCESS_CANCEL_REP  = 249, ///< 取消系统升级回应
    /** 其他命令 */
	NEWS_DISCOVER_VERSION_REQ			 = 251, ///< 请求摄像头版本
	NEWS_DISCOVER_VERSION_REP			 = 252, ///< 版本回应
    NEWS_SYS_HIT_PIG                  	 = 253, ///< 升级确认
    NEWS_SYS_BOMB_BY_PIG               	 = 254, ///< 升级确认回应
	/** 音频设置（未实现） */
    NEWS_AUDIO_SET_REQ                   = 300, ///< 音频设置请求
	NEWS_AUDIO_SET_REP                   = 301, ///< 音频设置回应
	NEWS_AUDIO_QUERY_REQ                 = 302, ///< 音频查询请求
	NEWS_AUDIO_QUERY_REP                 = 303, ///< 音频查询回应
	/** 音频通道数据传输 */
    NEWS_AUDIO_TRANSMIT_OUT              = 304, ///< 音频输出（audio）
	NEWS_AUDIO_TRANSMIT_IN               = 305, ///< 音频输入（speaker）
	NEWS_AUDIO_TRANSMIT_REQ              = 306, ///< 通道切换请求
	NEWS_AUDIO_TRANSMIT_REP              = 307, ///< 通道切换回应
	/** 视频设置 */
	NEWS_VIDEO_SET_REQ                   = 401, ///< 视频设置请求
	NEWS_VIDEO_SET_REP                   = 402, ///< 视频设置回应
	NEWS_VIDEO_QUERY_REQ                 = 403, ///< 视频参数查询请求
	NEWS_VIDEO_QUERY_REP                 = 404, ///< 视频参数查询回应
    /** 云台控制（m3s电机转动） */
	NEWS_CAMERA_MOVE_ABS                 = 500, ///< 摄像头绝对移动
	NEWS_CAMERA_POSITION                 = 501, ///< 摄像头位置
	NEWS_CAMERA_CRUISE                   = 502, ///< 摄像头巡航
	NEWS_CAMERA_MOVE_REL                 = 503, ///< 摄像头相对转动
	NEWS_CAMERA_MOVE_RES                 = 504, ///< 摄像头绝对移动

    /** 报警 */
	NEWS_ALARM_QUERY_REQ                 = 600, ///< 报警查询
	NEWS_ALARM_QUERY_REP                 = 601, ///< 报警查询回应
	NEWS_ALARM_CONFIG_REQ                = 602, ///< 报警设置请求
	NEWS_ARARM_CONFIG_REP                = 603, ///< 报警设置回应
	NEWS_ALARM_EVENT_NOTIFY              = 605, ///< 报警触发
    NEWS_CAMERA_STAMINA_REQ              = 1000,///< 电量查询请求
    NEWS_CAMERA_STAMINA_REP              = 1001,///< 电量查询回应
    /** 夜间模式 */
    NEWS_NIGHT_MODE_SET_REQ              = 1100,///< 设置夜间模式请求
    NEWS_NIGHT_MODE_SET_REP              = 1101,///< 设置夜间模式响应
    NEWS_NIGHT_MODE_RECORD_REQ           = 1102,///< 查询记录时间请求
    NEWS_NIGHT_MODE_RECORD_REP           = 1103,///< 查询记录时间响应
    /** 查询wifi信号强度 */
    NEWS_WIFI_QUALITY_REQ                = 1200,///< 查询wifi信号强度请求
    NEWS_WIFI_QUALITY_REP                = 1201,///< 查询wifi信号强度响应
    
    /** 开关通道命令 */
    NEWS_CHANNEL_SWITCH_REQ              = 2001,///< 通道开关请求
    NEWS_CHANNEL_SWITCH_REP              = 2002,///< 通道开关回应
};

#define NULL_FD                 -1      ///< 空描述符
#define TIME_OUT_S              1       ///< 超时秒数
#define TIME_OUT_US             100000  ///< 超时U秒
#define LEFT_TIME               0       ///< 初始时间
#define RIGHT_TIME              100     ///< 当前时间
#define LEFT_TIME_OFFSET        10      ///< 时间偏移
#define MAX_TCP_CLIENT_BUF_SIZE 512     ///< 客户缓冲数据大小
#define MAX_RCV_LEN             1024    ///< 接收缓冲最大大小

/** 网络协议头部结构 */
typedef struct RequestHead {
	char head[4];
	short cmd;
	char version;
	char crsv[8];
	int len;
	int irsv;
}__attribute__ ((packed)) RequestHead;

/** 网络协议查找字段项定义 */
typedef struct NewsCmdItem {
	short cmd;  ///< 命令字
	void *data; ///< 数据段
}NewsCmdItem;

/** 认证阶段 */
enum {
	VERIFY_NULL_FLOW,   //< 无认证
	VERIFY_JOIN_FLOW,   //< 用户加入
	VERIFY_VERIFY_FLOW, //< 用户认证
};

/** 发送wifi信息开关 */
enum {
    WIFI_SEND_SWITCH_ON = 1, ///< 开发送
    WIFI_SEND_SWITCH_OFF,    ///< 关发送
};

/** wifi发送计时 */
typedef struct wifiSendTime_t {
    char state;   ///< 发送状态
    time_t sTime; ///< 开始时间
    time_t step;  ///< 时间间隔
}wifiSendTime_t;

/** 通道结构 */
typedef struct UsrChannel {
	int fd;                               ///< 描述符
	char type;                            ///< 通道类型
    int name;                             ///< 通道用户名
	struct sockaddr_in client;            ///< 客户端ip信息
	int ltime;                            ///< 未使用
	int rtime;                            ///< 未使用
	time_t mltime;                        ///< 最近了活动时间
	time_t mrtime;                        ///< 未使用
    wifiSendTime_t wifiTime;              ///< wifi发送信息时间记录
	char verifyFlow;                      ///< 认证流程
	uint32_t verifyNumber[4];             ///< 认证随机数
    char direction;                       ///< 通道方向
    int rBufSize;                         ///< 当前缓冲大小
    char rBuf[MAX_TCP_CLIENT_BUF_SIZE];   ///< 缓冲区
    int isRecvOK;                         ///< 处理是否结束
}UsrChannel;

/** 控制通道属性 */
typedef struct NewsChannel_t {
	msg_container container;                   ///< 消息队列容器
	struct sockaddr_in server;                 ///< 服务ip信息
	int sockfd;                                ///< 服务描述
	int maxsockfd;                             ///< 最大描述服
	UsrChannel usrChannel[MAX_USER_NUM * 3];   ///< 用户通道列表
	UsrChannel *currentChannel;                ///< 当前通道
	fd_set allfds;                             ///< 描述集合
	struct timeval timeout;                    ///< select超时
//	char rcvBuf[MAX_RCV_LEN];                  ///<
    char *rcvBuf;                              ///< 当前缓冲区
	char sndBuf[MAX_RCV_LEN];                  ///< 发送缓冲区（未使用）
	char threadRunning;                        ///< 线程运行状态
	char msgHandlerSwitch;                     ///< 消息处理模块
	char newsHandlerSwitch;                    ///< 外部消息处理模块
	int speakerFd;                             ///< speaker 描述
	pthread_t speakerThread;                   ///< speaker 线程（未使用）
	int sleepTime;                             ///< 线程休息时间
	struct timeval sockTimeOut;                ///< 套接字发送超时
    time_t mTimer;                             ///< 全局时间计时
}NewsChannel_t;

extern volatile NewsChannel_t *NewsChannelSelf;

/** 开启控制通道 */
extern int NewsChannel_threadStart();
/** 初始化消息队列, 助手函数 */
extern int NewsChannel_msgQueueFdInit(int *fd, int key);
/** 检测是否存在speaker输入通道 */
extern int NewsChannel_usrInfoAudioPlayerIn(UsrChannel *list, int len, UsrChannel *usr);
/** 关闭通道, 并清理通道 */
extern int NewsChannel_channelClose(UsrChannel *usr);
/** 通信错误处理 */
extern int NewsChannel_sockError(int retval);
/** 添加通道, 携带类型且添加过滤 */
extern int NewsChannel_channelAdd(UsrChannel *list, int len, UsrChannel *usr);
/** 判断特定类型的通道是否存在 */
extern int NewsChannel_isChannelIn(UsrChannel *list, int len, int name, char type);

/** 夜间模式动作 */
extern int NewsChannel_nightModeAction(int opt);
/** 更新食品通道状态 */
extern int NewsChannel_upgradeVideoChannel(UsrChannel *list, int len, UsrChannel *usr, char dir);
/** 根据用户名获取speaker状态 */
extern int NewsChannel_usrInfoGetSpeakerByName(UsrChannel *list, int len, UsrChannel *usr);
#endif /* NEWSCHANNEL_H_ */
