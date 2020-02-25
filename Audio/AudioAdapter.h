/*
 * AudioAdapter.h
 *
 *  Created on: Mar 22, 2013
 *      Author: root
 */

#ifndef AUDIOADAPTER_H_
#define AUDIOADAPTER_H_

enum {
	MSG_AUDIO_T_START_CAPTURE = 1,
	MSG_AUDIO_T_STOP_CAPTURE,
	MSG_AUDIO_T_START_ENCODE,
	MSG_AUDIO_T_STOP_ENCODE,
	MSG_AUDIO_T_RESWITCH_USB,
	MSG_AUDIO_T_CHECK_ALIVE = 0x55,
	MSG_AUDIO_T_AUDIO_NEED_REBOOT = 0xff,
};

/*
 * @brief 音频采集线程发送出的消息在子进程的处理函数
 * @author:张历
 * @param[in]:<self><接受到的消息>
 * @param[out]:<没有参数>
 * @Return <NULL>
 */
extern void AudioAdapter_cmd_table_fun(msg_container *self);

#endif /* AUDIOADAPTER_H_ */
