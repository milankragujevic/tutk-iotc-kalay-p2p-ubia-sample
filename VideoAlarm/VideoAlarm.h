/** @file [VideoAlarm.h]
 *  @brief 视频报警线程的主要功能函数h文件
 *	@author: Summer.Wang
 *	@data: 2013-9-24
 *	@modify record: 即日创建了这套文件注释系统
 */

#ifndef VIDEOALARM_H_
#define VIDEOALARM_H_
#include "common/common.h"
#include "common/utils.h"

#define VA_BITMAP_SIZE	640*480   					///< 位图图像存储大小
#define VA_PIC_STORE	"/usr/share/alarmpic/"		///< 文件夹存储路径

enum {
	MSG_VA_T_START = CMD_AV_STARTED,				///< 视频线程通知可以开始取帧
	MSG_VA_T_STOP = CMD_STOP_ALL_USE_FRAME,			///< 视频线程通知必须停止取帧
	MSG_VA_T_CONTROL = 3,							///< 控制通道通知的视频报警线程的参数
	MSG_VA_T_ALRAM,									///< 音频报警线程通知的音频报警的开始和结束
	MSG_VA_T_MOTOR,									///< 串口控制线程通知的电机转动状态
	MSG_VA_T_TEST = 35,								///< 预留接口，为线程点名使用
};

typedef struct
{
	long int type;
	int cmd;
	char sign[4];
	int len;
	char data[MAX_MSG_LEN];
}VA_MsgData;

typedef struct
{
	int iIndex;
	int upIndex;
	int iLen;
	char *srcdata;
}VA_SRCIMAGE;

typedef struct
{
	int width;
	int height;
	unsigned char *buf;
}VA_BITMAP;

/*---------全局变量结构体---------*/
struct VideoAlarm_t{
	msg_container MsgBowl;							///< extends class msg_container
	char fetch,old_fetch;							///< 允许取帧标志和前一次标志状态
	char holdback,holdfore;							///< 是否持有背景/前景帧标志位
	char noalarm,alarm;								///< 不再报警模块,进入报警模块
	char getback,getfore;							///< 进入背景模块,进入前景模块
	char store;										///< 进入存储模块
	char alarm_type;								///< 报警类型(video or audio)
	char alarm_state;								///< 报警状态(开始和结束)
	char autio_start;								///< 音频报警是否处于开始状态
	int Pixels,oldPixels;							///< 图像分辨率，上一次的分辨率记录
	int count_back;									///< 用来定时改变背景
	time_t count_noalarm;							///< 用来定时不再报警
	int count_store;								///< 用来记录存入的图片数目
	int count_fetch;								///< 用来在开始取帧后定时完成才能取帧
	int MotionSense;								///< 报警级别
	int alarmswitch,old_alarmswitch;				///< 报警开关，上一次报警开关的状态
	int Motormove;									///< 电机转动状态
	int timestamp;									///< 报警时刻的时间戳
	int noise;										///< 去噪参考值
	int light;										///< 强光比参考值
	int framefail;									///< 用于花屏多久后不再检测花屏
	int count_framfail;								///< 用于花屏后多久不再报警
	int count_noalarm_bug;							///< 用于修复对比图像长期为0时连续采集到几次后采取措施
	char pathname[60];								///< 图像存储路径
//	char timestamp[20];
};

extern struct VideoAlarm_t *VideoAlarm_list;
extern int VideoAlarm_Thread_Creat();
extern int VideoAlarm_Thread_Stop();
extern int ReadJpegBuf(char * dataSrc,int iSize, VA_BITMAP *bitmap);
extern void videoalarm_write_log_info_to_file(char *pStrInfo,char *Oper);


#endif
