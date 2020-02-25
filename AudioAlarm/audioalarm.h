/*
 * AudioAlarm.h
 *
 *  Created on: Mar 7, 2013
 *      Author: root
 */

#ifndef AUDIOALARM_H_
#define AUDIOALARM_H_

#include "common/utils.h"
#include "common/common.h"

#define FFT_N  512//4096
//#define PI 3.1415926535897932384626433832795028841971         	//定义圆周率值
//#define PO 0.00002     //标准声压
#define SF 8000		   //采样率
#define L_ONE 1	                                                 //报警等级
#define L_TWO 2
#define L_THR 3
#define L_FOU 4
#define L_FIV 5
#define L_SIX 6
#define L_SEV 7
#define L_EIG 8
#define L_NIN 9


enum AudioAlarm_CMD{
	MSG_AUDIOALARM_T_ALARM_LEVEL = 1,
	MSG_AUDIOALARM_P_AUDIO_ALRAM_NOW,
	MSG_AUDIOALARM_P_AUDIO_ALRAM_END,

	MSG_AUDIOALARM_T_START_AUDIO_CAPTURE,
	MSG_AUDIOALARM_P_START_AUDIO_RESPONSE,

	MSG_AUDIOALARM_T_STOP_AUDIO_CAPTURE,
	MSG_AUDIOALARM_P_STOP_AUDIO_RESPONSE,

	MSG_AUDIOALARM_T_IPU_TRIGGER_ALARM,
	MSG_AUDIOALARM_T_IPU_STOP_ALARM,

	MSG_AUDIOALARM_T_STOP_4_VIDEO_CAPTURE,
	MSG_AUDIOALARM_P_STOP_4_VIDEO_RESPONSE,


};

enum AudioAlarmAevel {
	ALARM_LEVEL_0 = 0,	//警报级别 0 无报警
	ALARM_LEVEL_1 = 1,	//警报级别 1   报警
};

extern int CreateAudioAlarmThread();
void audioalarm_cmd_table_fun(msg_container *self);

#endif /* AUDIOALARM_H_ */
