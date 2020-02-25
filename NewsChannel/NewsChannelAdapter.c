/**	
   @file NewsChannelAdapter.c
   @brief 应用程序主文件
   @note
   author: yangguozheng
   date: 2013-09-16
   mcdify record: creat this file.
   author: yangguozheng
   date: 2013-09-17
   modify record: add a function
 */
#include "common/utils.h"
#include "NewsChannelAdapter.h"
#include "common/common.h"
#include "common/logfile.h"
#include "common/msg_queue.h"
#include "NewsChannel.h"
#include "NewsAdapter.h"

void NewsChannelAdapter_cmd_table_fun(msg_container *self);
void NewsChannelAdapter_p_channelReq(msg_container *self);
void NewsChannelAdapter_t_channelRep(msg_container *self);
void NewsChannelAdapter_p_newsBroadcast(msg_container *self);
void NewsChannelAdapter_t_newsBroadcast(msg_container *self);

/** 协议实现列表 */
cmd_item NewsChannelAdapter_cmd_table[] = {
		{MSG_NEWS_CHANNEL_P_CHANNEL_REQ, NewsChannelAdapter_p_channelReq},
		{MSG_NEWS_CHANNEL_T_CHANNEL_REP, NewsChannelAdapter_t_channelRep},
		{MSG_NEWS_CHANNEL_P_NEWS_BROADCAST, NewsChannelAdapter_p_newsBroadcast},
		{MSG_NEWS_CHANNEL_T_NEWS_BROADCAST, NewsChannelAdapter_t_newsBroadcast},
};//"T":from process,used by thread;"P":from thread,used by process.

/**
   @brief 协议查询入口
   @author yangguozheng
   @param[in|out] self 适配器运行环境
   @return
   @note
 */
void NewsChannelAdapter_cmd_table_fun(msg_container *self) {
//	DEBUGPRINT(DEBUG_INFO, "%s: find command\n", __FUNCTION__);
	MsgData *msgrcv = (MsgData *)self->msgdata_rcv;
	cmd_item icmd;
	icmd.cmd = msgrcv->cmd;
//	printf("NewsChannelAdapter_cmd_table_fun-------------- %d\n", icmd.cmd);
	msg_fun fun = utils_cmd_bsearch(&icmd, NewsChannelAdapter_cmd_table, sizeof(NewsChannelAdapter_cmd_table)/sizeof(cmd_item));
	if(fun != NULL) {
		fun(self);
	}
	else {
		DEBUGPRINT(DEBUG_INFO, "%s: cannot find command\n", __FUNCTION__);
	}
}

/**
   @brief tcp命令入口
   @author yangguozheng 
   @param[in|out] self 适配器运行环境 
   @return
   @note
 */
void NewsChannelAdapter_p_channelReq(msg_container *self) {
	NewsAdapter_cmd_table_fun(self);
}

void NewsChannelAdapter_t_channelRep(msg_container *self) {

}

/**
   @brief 通知广播
   @author yangguozheng 
   @param[in|out] self 适配器运行环境
   @return
   @note
 */
void NewsChannelAdapter_t_newsBroadcast(msg_container *self) {
	NewsChannel_t *newchan = (NewsChannel_t *)self;
	UsrChannel *currentChannel;
	MsgData *msgrcv = (MsgData *)self->msgdata_rcv;
	int i, n;

	DEBUGPRINT(DEBUG_INFO, "a a a a a a a a a a a a Alarm type: %d\n", *((char *)(msgrcv+1) + 4));

	n = MAX_USER_NUM * 3;
	for(i = 0; i < n; i++){
		currentChannel = &newchan->usrChannel[i];
		if(currentChannel->fd != NULL_FD && currentChannel->type == CONTROL_CHANNEL){
		//if(currentChannel->fd != NULL_FD){
			send_msg_to_network(currentChannel->fd, NEWS_ALARM_EVENT_NOTIFY, msgrcv+1, msgrcv->len, 1);
		}
	}
}

void NewsChannelAdapter_p_newsBroadcast(msg_container *self){

}
