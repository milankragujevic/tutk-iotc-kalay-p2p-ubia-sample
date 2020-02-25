/*
 * speakerAdapter.h
 *
 *  Created on: Apr 2, 2013
 *      Author: root
 */

#ifndef SPEAKERADAPTER_H_
#define SPEAKERADAPTER_H_
#include "common/utils.h"

enum{
	MSG_SPEAKER_T_CHECK_ALIVE = 0x55,
	MSG_SPEAKER_T_NEED_REBOOT = 0xff,
};


/*
 * @brief 放音线程发送出的消息在子进程的处理函数
 * @author:张历
 * @param[in]:<self><接受到的消息>
 * @param[out]:<没有参数>
 * @Return <NULL>
 */
extern void speakerAdapter_cmd_table_fun (msg_container *self);

#endif /* SPEAKERADAPTER_H_ */
