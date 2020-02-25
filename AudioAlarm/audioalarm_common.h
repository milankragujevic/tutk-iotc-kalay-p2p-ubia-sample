#ifndef AUDIOALARM_COMMON_H
#define AUDIOALARM_COMMON_H

#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>

#include "audioalarm.h"
#include "testcommon.h"

//30s=30*1000ms=30000ms
#define AUDIOALARM_USLEEP_TIME		300000 //300ms
#define AUDIOALARM_USLEEP_COUNT		100

#define ALARM 1
#define NOALARM 0

typedef volatile struct {
	int AudioAlarmFlag;//报警开关
	int AudioPush ;//用于30秒以及与视频报警互斥的不检测报警
	int Level;
	int last_index;
	int alarm_on; //1:send  2:stop
	int bcast_comm; //1:start 2:stop
} AUDIOALARM_FLAG;

/**************************audio data*************************************/
struct wav_header
{
    /* RIFF WAVE Chunk */
    uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t riff_fmt;
    /* Format Chunk */
    uint32_t fmt_id;
    uint32_t fmt_sz;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate; /* sample_rate * num_channels * bps / 8 */
    uint16_t block_align; /* num_channels * bps / 8 */
    uint16_t bits_per_sample;
    /* Data Chunk */
    uint32_t data_id;
    uint32_t data_sz;
}__attribute__((packed));

#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT 0x20746d66
#define ID_DATA 0x61746164
#define FORMAT_PCM 1

typedef struct
{
	float *fReal;
	float *fImag;
}compx;   //定义一个复数结构,这个结构体主要是用来存储数据源；

struct compxEE
{
	float fReal;
	float fImag;

};//这个结构体用来做乘法的运算的和复数的临时变量；

typedef struct                        //定义一个统计频谱的结构体类型；
{
	float fAmplitude;
	float fFrequency;
}spectrum;

/*
 * 写wav文件头
 * */
void audio_file_init(char *path);

/*
 * 写音频数据
 * */
int audio_variable_init(void);

void audio_file_write_data(char *path, char *pdata);

int handleAudioAlarm(AUDIOALARM_FLAG *flag);

/************************* Message rev and snd ************************************/
int AudioAlarmSendStartResponse(ArgsStruct *self);
int AudioAlarmSendStopResponse(ArgsStruct *self);
int AudioAlarmSendAlarmEnd(ArgsStruct *self);
int AudioAlarmSendAlarmResponse(ArgsStruct *self);
int AudioAlarmSendVideoAlarm(msg_container *self, int cmd);
int AudioAlarmSendNewsChannel(msg_container *self);
int AudioAlarmSendP2P(msg_container *self);
#endif
