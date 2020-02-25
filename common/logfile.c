/**	
   @file logfile.c
   @brief log打印程序
   @note
   author: yangguozheng
   date: 2013-09-16
   mcdify record: creat this file.
   author: yangguozheng
   date: 2013-09-17
   modify record: add a function
 */
#define LOGFILE_C_

//#include "logfile.h"
#include <string.h>
#include <stdarg.h>
#include "Udpserver/udpServer.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <bits/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <error.h>
#include <errno.h>
#include <semaphore.h>
#include <common.h>
#include "playback/speaker.h"

#define UDP_LEN  1024
#ifdef _DEBUG_TEST
#define UDP_ADDR "224.0.1.255"
#define UDP_PORT 11660
#else
#define UDP_ADDR "255.255.255.255"
#define UDP_PORT 12345
#endif

/** @brief 调试模块属性 */
typedef struct logfile_t {
	int sockfd;  //< udp描述符
	sem_t semid; //< 信号量
	struct sockaddr_in clientAddr;  //< 网络数据结构
	char udpDataSnd[UDP_LEN]; //< 缓存数据
}logfile_t;

static logfile_t *logfile_attr; //< 定义调试模块属性指针

/** @brief 函数声明 */
int printf_udp(const char* pszFormat, ...); //< udp打印接口声明
int logfile_init(); //< 模块初始化函数声明
FILE *popen_redef(const char *command, const char *type);

/**
   @brief udp打印接口
   @author yangguozheng 
   @param[in] pszFormat 打印格式 
   @param[in] ... 可选参数
   @return 成功: 0, 失败: -1
   @note
 */
int printf_udp(const char* pszFormat, ...)
{
	logfile_t *self = logfile_attr;
	if(self == NULL) {
		return -1;
	}
	sem_wait(&self->semid);
	char *szMsg = self->udpDataSnd;
	memset(szMsg, 0, sizeof(self->udpDataSnd));
	if(NULL == pszFormat)
	{
		//printf("pszFormat:%s, in file:%s line:%d\n", bytelevel, __FILE__, __LINE__);
		sem_post(&self->semid);
		return -1;
	}

	va_list list;
	va_start(list, pszFormat);
	vsnprintf(szMsg, sizeof(self->udpDataSnd)-1, pszFormat, list);
	va_end(list);

	udpServer_sndMsg(self->sockfd ,&self->clientAddr, szMsg , strlen(szMsg));
//	printf(szMsg);
	sem_post(&self->semid);
	return 0;
}

/**
   @brief 调试模块初始化入口
   @author yangguozheng 
   @return 成功: 0, 失败: -1
   @note
 */
int logfile_init() {
	log_start(1);
	logfile_attr = (logfile_t *)malloc(sizeof(logfile_t));
	logfile_t *self = logfile_attr;
	if(logfile_attr == NULL) {
		return -1;
	}

	if(sem_init(&self->semid, 0, 1) != 0) {
		return -1;
	}

	udpServer_sockAddrInit(&self->clientAddr, UDP_ADDR, UDP_PORT);
	if(udpServer_clientSockInit(&self->sockfd) == -1) {
		return -1;
	}
	return 0;
}

FILE *popen_redef(const char *command, const char *type) {
	static int a0 = 0;
	FILE *file = popen(command, type);
	int lThreadMsgId = -1;
    char acBuf[32] = {0};
    MsgData *pstMsgSnd = (MsgData *)acBuf;

	if(NULL == file) {
		printf("+++++++++++++++++static a0       =           %d ++++++++++++++++++++++ \n", a0);
		if(++a0 > 20) {
			//reboot
			//system("iCamera &;sleep 2;reboot");
			get_process_msg_queue_id(&lThreadMsgId,  NULL);
			pstMsgSnd->type = PLAY_BACK_MSG_TYPE;
			pstMsgSnd->cmd = MSG_SPEAKER_P_REBOOT;

			msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long));
		}
	}else{
		a0 = 0;
	}
	return file;
}
