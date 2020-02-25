/*
 * NewsChannelAdapter.h
 *
 *  Created on: Mar 20, 2013
 *      Author: yangguozheng wangzhiqiang
 */

#ifndef NEWSCHANNELADAPTER_H_
#define NEWSCHANNELADAPTER_H_
#include "common/utils.h"
#include "common/common.h"

/*内部消息协议*/
enum {
	MSG_NEWS_CHANNEL_P_CHANNEL_REQ      = 0,                      //通道命令
	MSG_NEWS_CHANNEL_T_CHANNEL_REP      = 1,                      //通道命令回应
	MSG_NEWS_CHANNEL_P_NEWS_BROADCAST   = 2,                      //未使用
	MSG_NEWS_CHANNEL_T_NEWS_BROADCAST   = 3,                      //广播
	MSG_NEWS_CHANNEL_T_START            = CMD_AV_STARTED,         //开始
	MSG_NEWS_CHANNEL_T_STOP             = CMD_STOP_ALL_USE_FRAME, //停止
};

/*协议查询入口*/
extern void NewsChannelAdapter_cmd_table_fun(msg_container *self);

#endif /* NEWSCHANNELADAPTER_H_ */
