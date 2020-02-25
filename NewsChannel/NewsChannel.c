/**	
   @file NewsChannel.c
   @brief 应用程序主文件
   @note
   author: yangguozheng
   date: 2013-09-16
   mcdify record: creat this file.
   author: yangguozheng
   date: 2013-09-17
   modify record: add a function
 */
#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <string.h>
#include <error.h>

#include "NewsChannel.h"
#include "common/logfile.h"
#include "common/utils.h"
#include "common/thread.h"
#include "common/common.h"
#include "NewsChannelAdapter.h"
#include "Udpserver/udpServer.h"
#include "NewsChannel.h"
#include "NewsAdapter.h"
#include "common/msg_queue.h"
#include "NewsFilter.h"
#include "common/thread.h"
#include "NewsChannel/NewsUtils.h"

/** @brief 模块全局属性 */
volatile NewsChannel_t *NewsChannelSelf;

NewsChannel_t newsChannel_t = {0};

int NewsChannel_threadStart();
int NewsChannel_threadStop();
void* NewsChannel_threadRun(void* arg);
int NewsChannel_init(NewsChannel_t *self);
int NewsChannel_exit(NewsChannel_t *self);
int NewsChannel_msgHandlers(NewsChannel_t *self);
int NewsChannel_newsHandlers(NewsChannel_t *self);
int NewsChannel_newsMsgHandlers(NewsChannel_t *self);
int NewsChannel_newsTouchWriggleHandlers(NewsChannel_t *self);
int NewsChannel_msgQueueFdInit(int *fd, int key);
int NewsChannel_sockFdCreateAndBind(int *fd, struct sockaddr_in* server);

int NewsChannel_usrInfoClear(UsrChannel *usrList, int len);
int NewsChannel_usrInfoAdd(UsrChannel *usrList, int len, UsrChannel *usr);
int NewsChannel_usrInfoAudioPlayerIn(UsrChannel *list, int len, UsrChannel *usr);
int NewsChannel_usrInfoGetSpeakerByName(UsrChannel *list, int len, UsrChannel *usr);

int NewsChannel_usrInfoAudioIn(UsrChannel *list, int len, UsrChannel *usr, char dir);
int NewsChannel_usrInfoCloseAll(UsrChannel *list, int len);

int NewsChannel_channelAdd(UsrChannel *list, int len, UsrChannel *usr);
UsrChannel *NewsChannel_channelFind(UsrChannel *list, int len, UsrChannel *usr);
int NewsChannel_channelClearButFd(UsrChannel *usr);
int NewsChannel_channelClear(UsrChannel *usr);
int NewsChannel_channelClose(UsrChannel *usr);

int NewsChannel_closeChannelsByUsrName(UsrChannel *list, int len, UsrChannel *usr);

int NewsChannel_isChannelIn(UsrChannel *list, int len, int name, char type);

int NewsChannel_upgradeVideoChannel(UsrChannel *list, int len, UsrChannel *usr, char dir);

int NewsChannel_clientTimeoutHandler(NewsChannel_t *self);
int NewsChannel_recvClientMsgHandler(NewsChannel_t *self);
int NewsChannel_pickPackageOfClient(UsrChannel *head);
int NewsChannel_tcpClientMsgHandler(NewsChannel_t *self);
int NewsChannel_touchWriggleHandler(NewsChannel_t *self);
int NewsChannel_timeOutHandler(NewsChannel_t *self);
int NewsChannel_sysMsgHandler(NewsChannel_t *self);

int NewsChannel_sockError(int retval);

int NewsChannel_nightModeAction(int opt);

/**
   @brief 以线程模式打开本模块
   @author yangguozheng 
   @return 0
   @note
 */
int NewsChannel_threadStart() {
	thread_start(NewsChannel_threadRun);
	return 0;
}

/**
   @brief 停止控制通道模块
   @author yangguozheng 
   @return 0
   @note
 */
int NewsChannel_threadStop() {
	return 0;
}

/**
   @brief 控制模块入口函数
   @author yangguozheng 
   @param[in|out] arg 未使用 
   @return 0
   @note
 */
void* NewsChannel_threadRun(void* arg) {
	TEST_PRINT("NewsChannel thread start success\n");
//	NewsChannel_t *self = (NewsChannel_t *)malloc(sizeof(NewsChannel_t));
	NewsChannel_t *self = &newsChannel_t;
	if(NewsChannel_init(self) == -1) {
		DEBUGPRINT(DEBUG_ERROR, "%s\n", errstr);
	}
	NewsChannelSelf = self;
	TEST_PRINT("NewsChannel thread synchronization start\n");
	thread_synchronization(&rNewsChannel, &rChildProcess);
	TEST_PRINT("NewsChannel thread synchronization stop\n");
	while(self->threadRunning == THREAD_RUNNING) {
        time(&(self->mTimer));
		NewsChannel_msgHandlers(self);
		NewsChannel_newsHandlers(self);
		child_setThreadStatus(NEWS_CHANNEL_MSG_TYPE, NULL);
//		thread_usleep(10000);
		if(self->sleepTime > 0) {
			thread_usleep(self->sleepTime);
		}
	}
	NewsChannel_exit(self);
	return 0;
}

/**
   @brief 控制模块初始化
   @author yangguozheng 
   @param[in|out] self 模块属性 
   @return -1 : 失败, 0 : 成功 
   @note
 */
int NewsChannel_init(NewsChannel_t *self) {
	msg_container *container = (msg_container*)self;
	self->threadRunning = THREAD_STOP;
	self->msgHandlerSwitch = MSG_HANDLER_OFF;
	self->newsHandlerSwitch = NEWS_HANDLER_OFF;
	if(self == NULL) {
		return -1;
	}
	if(NewsChannel_msgQueueFdInit(&container->lmsgid, CHILD_THREAD_MSG_KEY) == -1) {
		return -1;
	}
	if(NewsChannel_msgQueueFdInit(&container->rmsgid, CHILD_PROCESS_MSG_KEY) == -1) {
		return -1;
	}
	TEST_PRINT("%s: container->lmsgid: %d, container->rmsgid: %d\n", __FUNCTION__, container->lmsgid, container->rmsgid);

	udpServer_sockAddrInit(&self->server, SERVER_ADDR, SERVER_PORT);
	if(NewsChannel_sockFdCreateAndBind(&self->sockfd, &self->server) == -1) {
		return -1;
	}
	NewsChannel_usrInfoClear(self->usrChannel, MAX_USER_NUM * 3);
	NewsChannel_fdSetInit(self);

	self->sockTimeOut.tv_usec = 10000000;
	self->threadRunning = THREAD_RUNNING;
	self->msgHandlerSwitch = MSG_HANDLER_ON;
	self->newsHandlerSwitch = NEWS_HANDLER_ON;
	self->speakerFd = NULL_FD;
	self->speakerThread = NULL_THREAD;
	TEST_PRINT("NewsChannel initialize success\n");
	return 0;
}

/**
   @brief 控制模块退出
   @author yangguozheng 
   @param[in|out] self 模块属性 
   @return 0
   @note
 */
int NewsChannel_exit(NewsChannel_t *self) {
	free(self);
	return 0;
}

/**
   @brief 内部消息处理句柄
   @author yangguozheng 
   @param[in|out] self 控制模块属性 
   @return 0
   @note
 */
int NewsChannel_msgHandlers(NewsChannel_t *self) {
	msg_container *container = (msg_container*)self;
	if(msg_queue_rcv(container->lmsgid, container->msgdata_rcv, 1024, NEWS_CHANNEL_MSG_TYPE) == -1) {

	}
	else {
		//TEST_PRINT("NewsChannel msgHandlers receive message\n");
		NewsChannel_sysMsgHandler(self);
		if(self->msgHandlerSwitch == MSG_HANDLER_ON) {
			NewsChannelAdapter_cmd_table_fun(self);
		}
	}
	return 0;
}

/**
   @brief 接收外部消息句柄
   @author yangguozheng 
   @param[in|out] self 控制模块属性 
   @return 0
   @note
 */
int NewsChannel_newsHandlers(NewsChannel_t *self) {
	if(self->newsHandlerSwitch != NEWS_HANDLER_ON) {
		return 0;
	}
	msg_container *container = (msg_container *)self;
	MsgData *msgSnd = (MsgData *)container->msgdata_snd;
	NewsChannel_fdSetInit(self);
	int retval = select(self->maxsockfd + 1, &self->allfds, NULL, NULL, &self->timeout);
	if(retval < 0) {
		DEBUGPRINT(DEBUG_ERROR, "%s\n", errstr);
	}
	else if(retval == 0) {
		//TEST_PRINT("NewsChannel time out %d, %d\n", self->maxsockfd + 1, self->sockfd);
//		LOG_TEST("NewsChannel_newsHandlerstime out %d, %d\n", self->maxsockfd + 1, self->sockfd);
	}
	else {
		NewsChannel_recvClientMsgHandler(self);
	}
//	NewsChannel_clientTimeoutHandler(self);
//	NewsChannel_touchWriggleHandler(self);
    NewsChannel_tcpClientMsgHandler(self);
    NewsChannel_timeOutHandler(self);
	return 0;
}

/**
   @brief 外部消息处理句柄
   @author yangguozheng 
   @param[in|out] self 控制模块属性 
   @return 0
   @note
 */
int NewsChannel_newsMsgHandlers(NewsChannel_t *self) {
	msg_container *context = (msg_container *)&self->container;
	MsgData *msgSnd = (MsgData *)context->msgdata_snd;
    self->rcvBuf = self->currentChannel->rBuf;

	RequestHead *head = (RequestHead *)self->rcvBuf;
    char *text = (char *)(head + 1);
    int *tcpData = (int *)(head + 1);
    if(head->cmd != NEWS_AUDIO_TRANSMIT_IN) {
//    	TEST_PRINT("data from tcp [cmd: %d, len: %d, %08X, %08X, %08X, %08X, %08X, %08X, %08X, %08X, 08%X, 08%X ]\n", head->cmd, head->len, *tcpData, *(tcpData + 1), *(tcpData + 2), *(tcpData + 3), *(tcpData + 4), *(tcpData + 5), *(tcpData + 6), *(tcpData + 7), *(tcpData + 8), *(tcpData + 9));
    	log_b(head, 100);
    	LOG_TEST("NewsChannel time: %08X, tcp recv [cmd: %d, len: %d, %08X, %08X, %08X, %08X, %08X, %08X, %08X]\n", time(NULL), head->cmd, head->len, *tcpData, *(tcpData + 1), *(tcpData + 2), *(tcpData + 3), *(tcpData + 4), *(tcpData + 5), *(tcpData + 6));
    }
    if(FILTER_TRUE == NewsFilter_cmd_table_fun(self)) {
//    	LOG_TEST("NewsChannel time: %08X, tcp recv [cmd: %d, len: %d, %08X, %08X, %08X, %08X, %08X, %08X, %08X]\n", time(NULL), head->cmd, head->len, *tcpData, *(tcpData + 1), *(tcpData + 2), *(tcpData + 3), *(tcpData + 4), *(tcpData + 5), *(tcpData + 6));
        return 0;
    }
//    LOG_TEST("NewsChannel time: %08X, tcp recv [cmd: %d, len: %d, %08X, %08X, %08X, %08X, %08X, %08X, %08X]\n", time(NULL), head->cmd, head->len, *tcpData, *(tcpData + 1), *(tcpData + 2), *(tcpData + 3), *(tcpData + 4), *(tcpData + 5), *(tcpData + 6));

//    TEST_PRINT("-----head->head: %c, %c, %c, %c\n", head->head[0], head->head[1], head->head[2], head->head[3]);
//    TEST_PRINT("-----head->len: %d\n", head->len);
//    TEST_PRINT("------control channel head commond------head->cmd: %d\n", head->cmd);
    TEST_PRINT("data from tcp [cmd: %d, len: %d, %08X, %08X, %08X, %08X, %08X, %08X, %08X, %08X, 08%X, 08%X ]\n", head->cmd, head->len, *tcpData, *(tcpData + 1), *(tcpData + 2), *(tcpData + 3), *(tcpData + 4), *(tcpData + 5), *(tcpData + 6), *(tcpData + 7), *(tcpData + 8), *(tcpData + 9));
#if 0
	if(head->cmd > 1002 || head->cmd < 100) {
    	return 0;
    }
#endif
	//TEST_PRINT("%d, head->len %d\n", head->cmd, head->len);
	msgSnd->type = NEWS_CHANNEL_MSG_TYPE;
	msgSnd->cmd = MSG_NEWS_CHANNEL_P_CHANNEL_REQ;
	msgSnd->len = sizeof(RequestHead) + head->len + sizeof(int);
	memcpy(msgSnd + 1, &self->currentChannel->fd, sizeof(int));
	memcpy((char *)(msgSnd + 1) + sizeof(int), head, sizeof(RequestHead) + head->len);
	msg_queue_snd(context->rmsgid, msgSnd, sizeof(MsgData) - sizeof(long) + msgSnd->len);
	//TEST_PRINT("%d, head->len %d\n", head->cmd, head->len);
	return 0;
}

/**
   @brief 消息队列id初始化
   @author yangguozheng 
   @param[out] fd 消息队列id 
   @param[in] key 消息队列键值 
   @return -1 : "失败", 0 : 成功 
   @note
 */
int NewsChannel_msgQueueFdInit(int *fd, int key) {
	*fd = msg_queue_get(key);
	//TEST_PRINT("%s: %d", __FUNCTION__, *fd);
	if(*fd == -1) {
		return -1;
	}
	return 0;
}

/**
   @brief 套接字创建和帮定服务
   @author yangguozheng 
   @param[out] fd 套接字id 
   @param[in] server 服务属性 
   @return -1 : "失败", 0 : 成功 
   @note
 */
int NewsChannel_sockFdCreateAndBind(int *fd, struct sockaddr_in* server) {
	*fd = socket(AF_INET, SOCK_STREAM, 0);
	if(*fd == -1) {
		return -1;
	}
	int sockOpt = 1; /*turn on*/

	if(setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR, &sockOpt, sizeof(int)) != 0) {
		return -1;
	}
	if(bind(*fd, server, sizeof(struct sockaddr_in)) == -1) {
		return -1;
	}
	listen(*fd, LISTEN_MAX_NUM);
	return 0;
}

/**
   @brief 清空用户通道，通道初始化
   @author yangguozheng 
   @param[out] usrChannel 用户通道列表 
   @param[in] len 用户列表长度 
   @return -1 : "失败", n : 成功 
   @note
 */
int NewsChannel_usrInfoClear(UsrChannel *usrChannel, int len) {
	UsrChannel *head = usrChannel;
	int i = 0;
	for(;i < len;++i, ++head) {
		NewsChannel_channelClear(head);
	}
	TEST_PRINT("%s: success\n", __FUNCTION__);
	return 0;
}

/**
   @brief 添加用户通道
   @author yangguozheng 
   @param[in|out] list 用户通道列表 
   @param[in] len 用户列表长度 
   @param[in] usr 当前用户属性 
   @return  -1 : "失败", n : 成功 
   @note
 */
int NewsChannel_usrInfoAdd(UsrChannel *list, int len, UsrChannel *usr) {
	UsrChannel *head = list;
	int i = 0;
	for(;i < len;++i, ++head) {
        TEST_PRINT("tcp@%s: direction: %d, fd: %d, name: %d, rBufSize: %d, type: %d\n",
        		__FUNCTION__,
        		head->direction,
        		head->fd,
        		head->name,
        		head->rBufSize,
        		head->type);
	}
    
	head = list;

	LOG_TEST("NewsChannel_usrInfoAdd: time: %08X, usr: %d, type: %d, fd: %d\n", time(NULL), usr->name, usr->type, usr->fd);
    for(i = 0;i < len;++i, ++head) {
		if(head->fd == NULL_FD) {
			head->fd = usr->fd;
			NewsChannel_channelClearButFd(head);
			TEST_PRINT("%s: success\n", __FUNCTION__);
			return i;
		}
	}
	return -1;
}

/**
   @brief 检测是否存在speaker输入通道
   @author yangguozheng 
   @param[out] list 用户通道列表 
   @param[in] len 用户列表长度 
   @param[in] usr 当前用户属性 
   @return 0 : "失败", n : 更新的通道fd 
   @note
 */
int NewsChannel_usrInfoAudioPlayerIn(UsrChannel *list, int len, UsrChannel *usr) {
	UsrChannel *head = list;
	int i = 0;
	for(;i < len;++i, ++head) {
		if(head->direction == SOCK_DIRECTION_IN) {
            return head->fd;
        }
	}
	return NULL_FD;
}

/**
   @brief 检测是否存在speaker输入通道
   @author yangguozheng 
   @param[in] list 用户通道列表 
   @param[in] len 用户列表长度 
   @param[in] usr 当前用户属性 
   @return 0 : "失败", n : 更新的通道fd 
   @note
 */
int NewsChannel_usrInfoGetSpeakerByName(UsrChannel *list, int len, UsrChannel *usr) {
	UsrChannel *head = list;
	int i = 0;
	for(;i < len;++i, ++head) {
		if(head->direction == SOCK_DIRECTION_IN && usr->name == head->name) {
            return head->fd;
        }
	}
	return NULL_FD;
}

/**
   @brief 检测是否存在特定输入方式的音频通道
   @author yangguozheng 
   @param[in|out] list 用户通道列表 
   @param[in] len 用户列表长度 
   @param[in] usr 当前用户属性 
   @return  0 : "失败", n : 更新的通道fd 
   @note
 */
int NewsChannel_usrInfoAudioIn(UsrChannel *list, int len, UsrChannel *usr, char dir) {
	UsrChannel *head = list;
	int i = 0;
	for(;i < len;++i, ++head) {
        if(head->name == usr->name && head->type == AUDIO_CHANNEL) {
            head->direction = dir;
			return head->fd;
		}
	}
	return NULL_FD;
}

/**
   @brief 更新视频通道属性
   @author yangguozheng 
   @param[in|out] list 用户通道列表 
   @param[in] len 用户列表长度 
   @param[in] usr 当前用户属性 
   @param[in] usr 当前用户属性 
   @return 0: "失败", n : 更新的通道fd 
   @note
 */
int NewsChannel_upgradeVideoChannel(UsrChannel *list, int len, UsrChannel *usr, char dir) {
	UsrChannel *head = list;
	UsrChannel *tail = head + len;
	for(;head != tail; ++head) {
		if(head->name == usr->name && head->type == VIDEO_CHANNEL) {
			head->direction = dir;
			return head->fd;
		}
	}
	return 0;
}

/**
   @brief 关闭所有活动的通道
   @author yangguozheng 
   @param[in|out] list 用户通道列表 
   @param[in] len 用户列表长度 
   @return 0
   @note
 */
int NewsChannel_usrInfoCloseAll(UsrChannel *list, int len) {
	UsrChannel *head = list;
	int i = 0;
	for(;i < len;++i, ++head) {
		if(head->fd != NULL_FD) {
			NewsChannel_channelClose(head);
		}
	}
	TEST_PRINT("%s: success\n", __FUNCTION__);
	return 0;
}

/**
   @brief 更新视频通道属性
   @author yangguozheng 
   @param[in|out] list 用户通道列表 
   @param[in] len 用户列表长度 
   @param[in] usr 当前用户属性 
   @return 0
   @note
 */
int NewsChannel_channelAdd(UsrChannel *list, int len, UsrChannel *usr) {
	UsrChannel *result = NewsChannel_channelFind(list, len, usr);
	if(result == 0) {

	}
	else {
		NewsChannel_channelClose(result);
//		NewsChannel_closeChannelsByUsrName(list, len, result);
	}
	return 0;
}

/**
   @brief 判断特定类型的通道是否存在
   @author yangguozheng 
   @param[in|out] list 用户通道列表 
   @param[in] len 用户列表长度 
   @param[in] name 用户名 
   @param[in] type 通道类型 
   @return -1 : "失败", n : 成功 
   @note
 */
int NewsChannel_isChannelIn(UsrChannel *list, int len, int name, char type) {
	UsrChannel *head = list;
	int i = 0;
	for(;i < len;++i, ++head) {
		if(head->name == name && head->type == type) {
			return i;
		}
	}
	TEST_PRINT("%s: success\n", __FUNCTION__);
	return -1;
}

/**
   @brief 查找特定的通道
   @author yangguozheng 
   @param[in|out] list 用户通道列表 
   @param[in] len 用户列表长度 
   @param[in] usr 当前用户属性 
   @return 0 : "失败", * : 指向用户通道的指针 
   @note
 */
UsrChannel *NewsChannel_channelFind(UsrChannel *list, int len, UsrChannel *usr) {
	UsrChannel *head = list;
	int i = 0;
	for(;i < len;++i, ++head) {
		if(head->name == usr->name && head->type == usr->type) {
			return head;
		}
	}
	TEST_PRINT("%s: success\n", __FUNCTION__);
	return 0;
}

/**
   @brief 清空用户通道, 但不关闭通信描述
   @author yangguozheng 
   @param[in|out] usr 用户通道列表 
   @return 0
   @note
 */
int NewsChannel_channelClearButFd(UsrChannel *usr) {
	if(usr->type == AUDIO_CHANNEL && usr->direction == SOCK_DIRECTION_IN) {
		NewsUtils_setAppInfo(INFO_SPEAKER, INFO_SPEAKER_OFF, NewsChannelSelf->container.lmsgid);
	}
	memset(&usr->client, 0, sizeof(struct sockaddr_in));
	usr->ltime = LEFT_TIME;
	usr->rtime = RIGHT_TIME;
	time(&(usr->mltime));
	usr->type = NULL_CHANNLE;
	usr->direction = SOCK_DIRECTION_NULL;
	usr->name = 0;
	usr->rBufSize = 0;
	usr->verifyFlow = VERIFY_NULL_FLOW;
    usr->wifiTime.state = WIFI_SEND_SWITCH_OFF;
    usr->isRecvOK = 0;
	memset(usr->verifyNumber, 0, sizeof(usr->verifyNumber));
	return 0;
}

/**
   @brief 通道清理
   @author yangguozheng 
   @param[in|out] usr 用户通道列表 
   @return 0
   @note
 */
int NewsChannel_channelClear(UsrChannel *usr) {
	NewsChannel_channelClearButFd(usr);
	usr->fd = NULL_FD;
	return 0;
}

/**
   @brief 通道关闭
   @author yangguozheng 
   @param[in|out] usr 用户通道列表 
   @return 0
   @note
 */
int NewsChannel_channelClose(UsrChannel *usr) {
//	close(usr->fd);
	int fd = usr->fd;
	shutdown(fd, SHUT_RDWR);
	close(fd);
	log_printf("tcp shut n-t-f-a %d %d %d %d", usr->name, usr->type, usr->fd, usr->mltime);
	DEBUGPRINT(DEBUG_INFO, "=========fd: %d, type: %d socket shut down\n", usr->fd, usr->type);
	LOG_TEST("NewsChannel_channelClose: time: %08X usr: %d, type: %d, fd: %d\n", time(NULL), usr->name, usr->type, usr->fd);
//	log_s("shutdown", strlen("shutdown"));
	NewsChannel_channelClear(usr);
	return 0;
}

/**
   @brief 关闭特定名字的通道
   @author yangguozheng 
   @param[in|out] list 用户通道列表 
   @param[in] len 用户列表长度 
   @param[in] usr 当前用户属性 
   @return 0
   @note
 */
int NewsChannel_closeChannelsByUsrName(UsrChannel *list, int len, UsrChannel *usr) {
	DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++++++++++++++\n");
	DEBUGPRINT(DEBUG_INFO, "+++++++close channels by user+++++\n");
	DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++++++++++++++\n");
	UsrChannel *head = list;
	int userName = usr->name;
	int i = 0;
	LOG_TEST("NewsChannel_closeChannelsByUsrName: time: %08X, usr: %d\n", time(NULL), usr->name);
	for(;i < MAX_USER_NUM * 3;++i, ++head) {
//		DEBUGPRINT(DEBUG_INFO, "+++++>head user name: %d, channel type: %d, user name: %d, user type: %d\n", head->name, head->type, usr->name, usr->type);
		if(head->fd != NULL_FD && head->name == userName) {
			NewsChannel_channelClose(head);
		}
	}
	return 0;
}

/**
   @brief 生成套接字描述集
   @author yangguozheng 
   @param[in|out] self 控制模块属性 
   @return 0
   @note
 */
int NewsChannel_fdSetInit(NewsChannel_t *self){
	FD_ZERO(&self->allfds);
	FD_SET(self->sockfd, &self->allfds);
	self->timeout.tv_sec = TIME_OUT_S;
	self->timeout.tv_usec = TIME_OUT_US;
	int i = 0;
	UsrChannel *head = self->usrChannel;
	self->maxsockfd = self->sockfd;
	for(;i < MAX_USER_NUM * 3;++i, ++head) {
		if(head->fd != NULL_FD) {
			if(head->fd > self->maxsockfd) {
				self->maxsockfd = head->fd;
			}
			FD_SET(head->fd, &self->allfds);
		}
	}
	return 0;
}

/**
   @brief 轮询接收客户数据
   @author yangguozheng 
   @param[in|out] self 控制模块属性 
   @return 0
   @note
 */
int NewsChannel_recvClientMsgHandler(NewsChannel_t *self) {
	int i = 0;
	int n;
	UsrChannel *head = self->usrChannel;
	struct sockaddr client;
	socklen_t addrlen = sizeof(struct sockaddr);
	int retval;
	RequestHead *tcphead = self->rcvBuf;
	if(FD_ISSET(self->sockfd, &self->allfds)) {
		int sockfd = accept(self->sockfd, &client, &addrlen);
		if(-1 == sockfd) {
			DEBUGPRINT(DEBUG_ERROR, "%s\n", errstr);
			return -1;
		}
//		log_s("accept", strlen("accept"));
		log_printf("tcp apt start %d", __LINE__);
		LOG_TEST("NewsChannel_recvClientMsgHandler: time: %08X, accept %d\n", time(NULL), sockfd);
//		setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &self->sockTimeOut, sizeof(struct timeval));
		UsrChannel usrChannel;
		usrChannel.fd = sockfd;
//		memcpy(&usrChannel.client, &client, addrlen);
		RequestHead *rHead = usrChannel.rBuf;
		TEST_PRINT("accept client\n");
		if(-1 == NewsChannel_usrInfoAdd(self->usrChannel, MAX_USER_NUM * 3, &usrChannel)) {
			DEBUGPRINT(DEBUG_ERROR, "%s\n", errstr);
            DEBUGPRINT(DEBUG_ERROR, "yyyyyyyyyyyyyyyyyyyy\n");
            DEBUGPRINT(DEBUG_ERROR, "    add user fail\n");
            DEBUGPRINT(DEBUG_ERROR, "yyyyyyyyyyyyyyyyyyyy\n");
            memcpy(rHead->head, TCP_REQUEST_HEAD, strlen(TCP_REQUEST_HEAD));
            rHead->version = 0;
            rHead->cmd = 141;
            rHead->len = 17;
            NewsUtils_sendToNet(sockfd, rHead, sizeof(RequestHead) + rHead->len, 0);
            shutdown(sockfd, SHUT_RDWR);
			close(sockfd);
			log_printf("tcp apt error %d", __LINE__);
		}
//		log_s("accept ok", strlen("accept ok"));
		log_printf("tcp apt ok %d", __LINE__);
	}
	for(;i < MAX_USER_NUM * 3;++i, ++head) {
		if(head->fd != NULL_FD) {
			if(FD_ISSET(head->fd, &self->allfds)) {
				NewsChannel_pickPackageOfClient(head);
			}
		}
	}
	return 0;
}

/**
   @brief 拼取客户数据
   @author yangguozheng 
   @param[in|out] head 用户通道 
   @return 0
   @note
 */
int NewsChannel_pickPackageOfClient(UsrChannel *head) {
    RequestHead *rHead = (RequestHead *)head->rBuf;
    int retval;
//    TEST_PRINT("NewsChannel_pickPackageOfClient\n");
    if(head->rBufSize < sizeof(RequestHead)) {
        retval = recv(head->fd, head->rBuf + head->rBufSize, sizeof(RequestHead) - head->rBufSize, 0);
        if(-1 == NewsChannel_sockError(retval)) {
        	NewsChannel_channelClose(head);
        }
        else if(-2 == NewsChannel_sockError(retval)){
        	return 0;
        }
        head->rBufSize += retval;
//        RequestHead *rmHead = (RequestHead *)head->rBuf;
//		TEST_PRINT("b b b b b b b b b rmHead->cmd: %d c c c c c c c c %d, eee eee eee %d\n", rmHead->cmd, head->fd, head->rBufSize);
		if(head->rBufSize == sizeof(RequestHead)) {

			RequestHead *rmHead = (RequestHead *)head->rBuf;
			//TEST_PRINT("Guozhixin     rmHead->cmd: %d\n", rmHead->cmd);

			if(rHead->len == 0){

				TEST_PRINT("aaaaaaaaaaaaaaaaaaaarmHead->cmd: %d aaaaaaaaaaaaaaaaaaa\n", rmHead->cmd);
				if(0 != strncmp(ICAMERA_NETWORK_HEAD, rHead->head, strlen(ICAMERA_NETWORK_HEAD))) {
					head->rBufSize = 0;
					return 0;
				}

				head->isRecvOK = 0;

				head->rBufSize = -1;
				TEST_PRINT("upTest --------------------------\n");
				TEST_PRINT("++++++ --------------------------\n");
		//			RequestHead *rmHead = (RequestHead *)head->rBuf;
				TEST_PRINT("rmHead->head: %c, %c, %c, %c, %d, %d\n", rmHead->head[0], rmHead->head[1], rmHead->head[2], rmHead->head[3], rmHead->cmd, rmHead->len);

				return 0;
			}else{
				head->isRecvOK = 1;
			}
		}
    }
#ifdef _TCP_RCV_BODY_NEXT_LOOP
    else {
        if(0 != strncmp(ICAMERA_NETWORK_HEAD, rHead->head, strlen(ICAMERA_NETWORK_HEAD))) {
            head->rBufSize = 0;

            head->isRecvOK = 0;

//            TEST_PRINT("strncmp(ICAMERA_NETWORK_HEAD, rHead->head, strlen(ICAMERA_NETWORK_HEAD))\n");
            return 0;
        }
//        TEST_PRINT("       1 head->rBufSize: %d\n", head->rBufSize);
        if(rHead->len > MAX_TCP_CLIENT_BUF_SIZE - sizeof(RequestHead) || rHead->len < 0) {
            head->rBufSize = 0;

            head->isRecvOK = 0;

//            TEST_PRINT("rHead->len > MAX_TCP_CLIENT_BUF_SIZE - sizeof(RequestHead) || rHead->len < 0\n");
            return 0;
        }
//        TEST_PRINT("       2 head->rBufSize: %d, rHead->len: %d\n", head->rBufSize, rHead->len);
        if(head->rBufSize < rHead->len + sizeof(RequestHead)){
            retval = recv(head->fd, head->rBuf + head->rBufSize, rHead->len + sizeof(RequestHead) - head->rBufSize, 0);
            if(-1 == NewsChannel_sockError(retval)) {
				NewsChannel_channelClose(head);
			}
			else if(-2 == NewsChannel_sockError(retval)){
				return 0;
			}
            head->rBufSize += retval;

            head->isRecvOK = 1;
//            TEST_PRINT("       3 head->rBufSize: %d, rHead->len: %d\n", head->rBufSize, rHead->len);
        }
        if(head->rBufSize >= rHead->len + sizeof(RequestHead)) {

        	head->isRecvOK = 0;

//        	TEST_PRINT("       4 head->rBufSize: %d, rHead->len: %d\n", head->rBufSize, rHead->len);
        	head->rBufSize = -1;
//        	RequestHead *rmHead = (RequestHead *)head->rBuf;
//        	TEST_PRINT("rmHead->cmd: %d\n", rmHead->cmd);
        }
    }
#else
    if(head->rBufSize >= sizeof(RequestHead)) {
        if(0 != strncmp(ICAMERA_NETWORK_HEAD, rHead->head, strlen(ICAMERA_NETWORK_HEAD))) {
            head->rBufSize = 0;
            
            head->isRecvOK = 0;
            
            //            TEST_PRINT("strncmp(ICAMERA_NETWORK_HEAD, rHead->head, strlen(ICAMERA_NETWORK_HEAD))\n");
            return 0;
        }
        //        TEST_PRINT("       1 head->rBufSize: %d\n", head->rBufSize);
        if(rHead->len > MAX_TCP_CLIENT_BUF_SIZE - sizeof(RequestHead) || rHead->len < 0) {
            head->rBufSize = 0;
            
            head->isRecvOK = 0;
            
            //            TEST_PRINT("rHead->len > MAX_TCP_CLIENT_BUF_SIZE - sizeof(RequestHead) || rHead->len < 0\n");
            return 0;
        }
        //        TEST_PRINT("       2 head->rBufSize: %d, rHead->len: %d\n", head->rBufSize, rHead->len);
        if(head->rBufSize < rHead->len + sizeof(RequestHead)){
            retval = recv(head->fd, head->rBuf + head->rBufSize, rHead->len + sizeof(RequestHead) - head->rBufSize, 0);
            if(-1 == NewsChannel_sockError(retval)) {
				NewsChannel_channelClose(head);
			}
			else if(-2 == NewsChannel_sockError(retval)){
				return 0;
			}
            head->rBufSize += retval;
            
            head->isRecvOK = 1;
            //            TEST_PRINT("       3 head->rBufSize: %d, rHead->len: %d\n", head->rBufSize, rHead->len);
        }
        if(head->rBufSize >= rHead->len + sizeof(RequestHead)) {
            
        	head->isRecvOK = 0;
            
            //        	TEST_PRINT("       4 head->rBufSize: %d, rHead->len: %d\n", head->rBufSize, rHead->len);
        	head->rBufSize = -1;
            //        	RequestHead *rmHead = (RequestHead *)head->rBuf;
            //        	TEST_PRINT("rmHead->cmd: %d\n", rmHead->cmd);
        }
    }
#endif
    return 0;
}

/**
   @brief wifi数据发送
   @author yangguozheng 
   @param[in|out] channel 用户通道列表 
   @return 0
   @note
 */
int NewsChannel_sendWifiDbInfo(UsrChannel *channel) {
    RequestHead *rHead;
    char *text;
    if(channel->fd != NULL_FD && channel->type == CONTROL_CHANNEL && WIFI_SEND_SWITCH_ON == channel->wifiTime.state) {
        if (channel->wifiTime.sTime < NewsChannelSelf->mTimer) {
            channel->wifiTime.sTime = NewsChannelSelf->mTimer + channel->wifiTime.step;
            rHead = (RequestHead *)channel->rBuf;
            text = (char *)(rHead + 1);
            rHead->cmd = NEWS_WIFI_QUALITY_REP;
            get_wifi_signal_level(text + 4);
            rHead->len = 13;
            NewsUtils_sendToNet(channel->fd, rHead, sizeof(RequestHead) + rHead->len, 0);
            DEBUGPRINT(DEBUG_INFO, "NewsChannel_sendWifiDbInfo ---------------------- 1201\n");
        }
    }
    return 0;
}

/**
   @brief 夜间模式进入
   @author yangguozheng 
   @param[in] opt 夜间模式开关 1 : 开,2 : 关 
   @return 0
   @note
 */
int NewsChannel_nightModeAction(int opt) {

	DEBUGPRINT(DEBUG_INFO, "NewsChannel_nightModeAction start\n");
    NewsChannel_t *self = NewsChannelSelf;
    UsrChannel *head = self->usrChannel;
    RequestHead *rHead;
    char *text;
    int nightStartTime;
    int i = 0;
    int retval;
    nightStartTime = runtimeinfo_get_nm_time();
    //NewsUtils_getRunTimeState(INFO_NIGHT_MODE_ON_TIME, &nightStartTime);
    //retval = NewsUtils_getRunTimeState(0, NULL);

//    if(INFO_NIGHT_MODE_TCP_NOTIFY == opt) {
    	head = self->usrChannel;
		for(;i < MAX_USER_NUM * 3;++i, ++head) {
			if(head->fd != NULL_FD && head->type == CONTROL_CHANNEL) {
				rHead = (RequestHead *)head->rBuf;
				memcpy(rHead->head, ICAMERA_NETWORK_HEAD, strlen(ICAMERA_NETWORK_HEAD));
				rHead->version = ICAMERA_NETWORK_VERSION;
				text = (char *)(rHead + 1);
				rHead->cmd = NEWS_NIGHT_MODE_SET_REP;
				*(char *)(text + 4) = opt;
				*(int *)(text + 5) = nightStartTime;
				rHead->len = 9;
				NewsUtils_sendToNet(head->fd, rHead, sizeof(RequestHead) + rHead->len, 0);
				DEBUGPRINT(DEBUG_INFO, "send NewsChannel_nightModeAction start\n");
			}
		}
		return 0;
//	}

//    head = self->usrChannel;
//    for(;i < MAX_USER_NUM * 3;++i, ++head) {
//        if(head->fd != NULL_FD && head->type == CONTROL_CHANNEL) {
//            rHead = (RequestHead *)head->rBuf;
//            text = (char *)(rHead + 1);
//            rHead->cmd = NEWS_NIGHT_MODE_SET_REP;
//            *(text + 4) = retval;
//            *(int *)(text + 5) = nightStartTime;
//            rHead->len = 9;
//            NewsUtils_sendToNet(head->fd, rHead, sizeof(RequestHead) + rHead->len, 0);
//        }
//    }
    return 0;
}

/**
   @brief 标识可处理的用户数据
   @author yangguozheng 
   @param[in|out] self 控制模块属性 
   @return 0
   @note
 */
int NewsChannel_tcpClientMsgHandler(NewsChannel_t *self) {
    UsrChannel *head = self->usrChannel;
    int i = 0;
    for(;i < MAX_USER_NUM * 3;++i, ++head) {
        if(head->rBufSize == -1) {
            self->currentChannel = head;
//            TEST_PRINT("       5 head->rBufSize: %d\n", head->rBufSize);
            NewsChannel_newsMsgHandlers(self);
            head->rBufSize = 0;
        }

        if(head->isRecvOK == 0){
        	NewsChannel_sendWifiDbInfo(head);
        }
    }
    return 0;
}

/**
   @brief 处理超时
   @author yangguozheng 
   @param[in|out] self 控制模块属性 
   @return 0
   @note
 */
int NewsChannel_clientTimeoutHandler(NewsChannel_t *self) {
	UsrChannel *head = self->usrChannel;
	int i = 0;
	for(;i < MAX_USER_NUM * 3;++i, ++head) {
		if(head->fd != NULL_FD) {
			head->ltime++;
			if(head->ltime > head->rtime) {
				close(head->fd);
				head->fd = NULL_FD;
				TEST_PRINT("touch time out\n");
			}
		}
	}
	return 0;
}

/**
   @brief 心跳处理
   @author yangguozheng 
   @param[in|out] self 控制模块属性 
   @return 0
   @note
 */
int NewsChannel_touchWriggleHandler(NewsChannel_t *self) {
	UsrChannel *head = self->usrChannel;
	RequestHead *tcphead = (RequestHead *)self->rcvBuf;

	int i = 0;
	for(;i < MAX_USER_NUM * 3;++i, ++head) {
		if(head->fd != NULL_FD && head->type == CONTROL_CHANNEL) {
			head->ltime++;
		}
		if(head->fd != NULL_FD && head->type == CONTROL_CHANNEL && head->ltime == RIGHT_TIME + LEFT_TIME_OFFSET) {
			tcphead->cmd = NEWS_CHANNEL_WRIGGLE_REP;
			//DEBUGPRINT(DEBUG_INFO, "NewsChannel_touchWriggleHandler: %d, %d\n", *(text + 4),  *(text + 5));
			char *text = (char *)(tcphead + 1);
			DEBUGPRINT(DEBUG_INFO, "NewsChannel_touchWriggleHandler: %d, %d\n", *(text + 4),  *(text + 5));
			memcpy(tcphead->head, ICAMERA_NETWORK_HEAD, sizeof(ICAMERA_NETWORK_HEAD));
			*(text + 4) = 1;
			*(text + 5) = 1;
			tcphead->len = 6;
			if(-1 == NewsUtils_sendToNet(head->fd, tcphead, sizeof(RequestHead) + tcphead->len, 0)) {
				DEBUGPRINT(DEBUG_ERROR, "%s\n", errstr);
			}
			head->ltime = LEFT_TIME;
		}
	}
	return 0;
}

/**
   @brief 超时处理
   @author yangguozheng 
   @param[in|out] self 控制模块属性 
   @return 0
   @note 当通道120s没有反应，则关闭通道
 */
int NewsChannel_timeOutHandler(NewsChannel_t *self) {
	time_t curTime;
	time(&curTime);

	UsrChannel *head = self->usrChannel;

	int i = 0;
	for(;i < MAX_USER_NUM * 3;++i, ++head) {
		if(head->fd != NULL_FD && head->type == CONTROL_CHANNEL) {
//			DEBUGPRINT(DEBUG_INFO, "+++++++++++>touch timeout current time: %ld, pre time: %ld", curTime, head->mltime);
//			DEBUGPRINT(DEBUG_INFO, "+++++++++++>touch timeout head name: %d, head type: %d", head->name, head->type);
			if(curTime - head->mltime > HEART_BEAT_TIME_OUT) {
				NewsChannel_closeChannelsByUsrName(self->usrChannel, MAX_USER_NUM * 3, head);
			}
		}
	}
	return 0;
}

/**
   @brief 系统消息处理
   @author yangguozheng 
   @param[in|out] self 控制模块属性 
   @return 0
   @note
 */
int NewsChannel_sysMsgHandler(NewsChannel_t *self) {
	msg_container *container = (msg_container *)self;
	MsgData *msgrcv = (MsgData *)container->msgdata_rcv;
	switch(msgrcv->cmd) {
		case MSG_NEWS_CHANNEL_T_START:
			self->msgHandlerSwitch = MSG_HANDLER_ON;
			self->newsHandlerSwitch = NEWS_HANDLER_ON;
			self->sleepTime = 0;
			DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++++++++++++++++++");
			DEBUGPRINT(DEBUG_INFO, "++++pick a pig into a packet  ++++++++");
			DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++++++++++++++++++");
			FILE *logFile = fopen("/tmp/NewsChannel", "w");
			if(logFile != NULL) {
				fwrite("pug out", strlen("pug out"), 1, logFile);
				fclose(logFile);
			}
			break;
		case MSG_NEWS_CHANNEL_T_STOP:
			NewsChannel_usrInfoCloseAll(self->usrChannel, MAX_USER_NUM * 3);
			DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++++++++++++++++++");
			DEBUGPRINT(DEBUG_INFO, "++++I am not pig he he he he he+++++++");
			DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++++++++++++++++++");
			self->msgHandlerSwitch = MSG_HANDLER_OFF;
			self->newsHandlerSwitch = NEWS_HANDLER_OFF;
			g_lNewsChannel = 1;
			self->sleepTime = 300000;
			FILE *logFile2 = fopen("/tmp/NewsChannel", "w");
			if(logFile2 != NULL) {
				fwrite("pug in", strlen("pug in"), 1, logFile2);
				fclose(logFile2);
			}
			break;
		case CMD_STOP_SEND_DATA:
			NewsChannel_usrInfoCloseAll(self->usrChannel, MAX_USER_NUM * 3);
			break;
	}
	return 0;
}

/**
   @brief 通信错误处理
   @author yangguozheng 
   @param[in] retval socket 错误号 
   @return 0 : 正常, -2 : 异常, -1 : 错误 
   @note
 */
int NewsChannel_sockError(int retval) {
	if(retval > 0) {
		return 0;
	}
	else if((retval < 0)&&(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)) {
		DEBUGPRINT(DEBUG_INFO, "=========socket error but not disconnect: %d\n", errno);
		log_printf("socket error but not disconnect: %d\n", errno);
		return -2;
	}
	DEBUGPRINT(DEBUG_INFO, "=========socket error: %d\n", retval);
	DEBUGPRINT(DEBUG_INFO, "=========socket will shut down\n");
	log_printf("socket error disconnect: %d\n", errno);
	return -1;
}
