/*
 * VideoAdapter.h
 *
 *  Created on: Mar 22, 2013
 *      Author: root
 */

#ifndef VIDEOADAPTER_H_
#define VIDEOADAPTER_H_

enum {
	MSG_VIDEO_T_START_CAPTURE = 1,
	MSG_VIDEO_T_STOP_CAPTURE,
	MSG_VIDEO_T_SETTING_PARAMS,
	MSG_VIDEO_T_RESWITCH_USB,
	MSG_VIDEO_T_CHECK_ALIVE,
	MSG_VIDEO_T_NEED_REBOOT = 0xff,
};

extern volatile int iReswitchTimes;


/*
 * @brief 视频采集线程发送出的消息在子进程的处理函数
 * @author:张历
 * @param[in]:<self><接受到的消息>
 * @param[out]:<没有参数>
 * @Return <NULL>
 */
extern void VideoAdapter_cmd_table_fun(msg_container *self);


#endif /* VIDEOADAPTER_H_ */
