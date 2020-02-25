/** @file [VideoAlarmAdapter.h]
 *  @brief 视频报警线程的外部交互h文件
 *	@author: Summer.Wang
 *	@data: 2013-9-24
 *	@modify record: 即日创建了这套文件注释系统
 */
#ifndef VIDEOALARMADAPTER_H_
#define VIDEOALARMADAPTER_H_
#include "common/utils.h"

/*msg commmad 宏定义*/
enum {
	MSG_VA_P_ALARM = 6,			///< 报警动作上传
	MSG_VA_P_FINISH,			///< 报警图像路径上传
	MSG_VA_P_REPLY,				///< 停止取帧后的反馈信息
	MSG_VA_P_CONTROL,			///< 预留接口，为控制通道的控制信息作反馈，暂未使用
	MSG_VA_P_FRAME,				///< 更改视频图像大小
	MSG_VA_P_INFORM,			///< 通知音频报警现存30秒内不报警
	MSG_VA_P_AUDIO_START,		///< 为音频报警线程通知控制通道和p2p的音频报警开始
	MSG_VA_P_AUDIO_STOP,		///< 为音频报警线程通知控制通道和p2p的音频报警结束
	MSG_VA_P_ALIVE = 36,		///< 预留接口，为线程点名使用
};

extern void videoalarm_test(msg_container *self);
extern void videoalarm_cmd_table_fun(msg_container *self);
#endif
