/*
 * speaker.h
 *
 *  Created on: Mar 18, 2013
 *      Author: root
 */

#ifndef SPEAKER_H_
#define SPEAKER_H_
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#define		SPEAKER_BUF_SIZE	100
#define		AUDIO_DATA_LEN 		160

enum{
	MSG_SPEAKER_P_REBOOT = 0xff,
	MSG_SPEAKER_P_CHECK_ALIVE = 0x55,
};


/*
 * @brief 创建声音播放线程
 * @author:张历
 * @param[in]:<没有参数>
 * @param[out]:<没有参数>
 * @Return <成功：线程ID ；失败：-1>
 */
int CreateSpeakerThread();

/*
 * @brief 播放声音
 * @author:张历
 * @param[in]:<pszAudioDecode><要播放数据的指针 160字节的ADPCM>
 * @param[out]:<没有参数>
 * @Return <0：成功 ； -1：失败；>
 */
int PlaySpeaker(char* pszAudioDecode);


/*
 * @brief 播放声音
 * @author:张历
 * @param[in]:<buffer><要播放数据的指针 格式为PCM>
 * @param[out]:<没有参数>
 * @Return <0：成功 ； -1：失败；>
 */
int PlayMusic(char* buffer);

/*
 * @brief 启动测试线程
 * @author:张历
 * @param[in]:<无参数>
 * @param[out]:<没有参数>
 * @Return <0：成功 ； -1：失败；>
 */
int CreateTestThread();

#endif /* SPEAKER_H_ */
