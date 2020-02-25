/*
 * Audio.h
 *
 *  Created on: Jan 7, 2013
 *      Author: root
 */

#ifndef AUDIO_H_
#define AUDIO_H_



#define AUDIO_BUFFER_SIZE 750
#define AUDIO_ALARM_BUF_SIZE 5//2
#define AUDIO_ALARM_DATA_LEN 1024//8192
#define AUDIO_CAPTURE_MSG_LEN 128
#define AUDIO_MAX_INDEX 60000

enum{
	MSG_AUDIO_P_START_CAPTURE = 1,
	MSG_AUDIO_P_STOP_CAPTURE,
	MSG_AUDIO_P_START_ENCODE,
	MSG_AUDIO_P_STOP_ENCODE,
	MSG_AUDIO_P_RESWITCH_USB,
	MSG_AUDIO_P_CHECK_ALIVE,
	MSG_AUDIO_P_REBOOT = 0xff,
};

struct HandleAudioMsg{
	long int type;
	int cmd;
	char unUsed[4];
	int len;
	char data[AUDIO_CAPTURE_MSG_LEN-16];
};

/*
 * @brief 创建音频采集线程
 * @author:张历
 * @param[in]:<没有参数>
 * @param[out]:<没有参数>
 * @Return <成功：线程ID ；失败：-1>
 */
int CreateAudioCaptureThread();

/*
 * @brief 获取最新的压缩音频包
 * @author:张历
 * @param[in]:<没有参数>
 * @param[out]:<pData><音频数据>
 * @param[out]:<piIndex><音频缓存包序号>
 * @Return <0：成功 ； -1：失败；>
 * @Deprecated 失效
 */
int GetNewestAudioFrame(char** pData, int* piIndex);

/*
 * @brief 获取指定的压缩音频数据包
 * @author:张历
 * @param[in]:<没有参数>
 * @param[out]:<pData><音频数据>
 * @param[out]:<iWantIndex><需要的音频数据包>
 * @Return <0：成功； -1：失败； -2：没有指定包，等待；-3：索引超出范围，重置索引为0； 其他：当前索引和数据最新索引的距离  >
 */
int GetAudioFrameWithWantedIndex(char** pData, int iWantIndex);

/*
 * @brief 获取没有压缩的原始音频数据
 * @author:张历
 * @param[in]:<没有参数>
 * @param[out]:<pData><音频数据>
 * @param[out]:<piIndex><音频缓存包序号>
 * @Return <0：成功 ； -1：失败；>
 */
int GetAlarmAudioFrame(char** pData, int* piIndex);

#endif /* AUDIO_H_ */
