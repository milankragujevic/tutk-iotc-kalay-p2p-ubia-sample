/**
   @file logserver.c
   @brief 应用程序主文件
   @note
   author: yangguozheng
   date: 2013-09-16
   mcdify record: creat this file.
   author: yangguozheng
   date: 2014-01-26
   modify record: add a function
 */

#include <time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <stdarg.h>
#include <regex.h>
#include <openssl/ssl.h>

#define MAX_MSG_LOG_LEN          (64 * 1024)    ///< 子进程消息最大条数
#define MAX_B_LOG_LEN            4096           ///< 二进制日志最大条数
#define MAX_STR_LOG_LEN          1024           ///< 字符串日志理论最大条数

#define MAX_MSG_LOG_ITEM_SIZE    6              ///< 子进程日志每条大小
#define MAX_B_LOG_ITEM_SIZE      128            ///< 二进制日志每条大小
#define MAX_STR_LOG_ITEM_SIZE    (128 + 1024)   ///< 字符串日志每条大小

#define MAX_HEAD_BUF_LEN         1024           ///< 通信头部大小

//#define LOG_SERVER_ADDR   "www.baidu.com"
#define LOG_HTTPS_SERVER_ADDR   "127.0.0.1"     ///< https测试地址
//#define LOG_HTTPS_SERVER_PORT   80
#define LOG_HTTPS_SERVER_PORT   443             ///< https测试端口
#define LOG_HTTPS_SERVER_PAGE   "/index.php"    ///< https测试api
#define LOG_MAC   				"?mac=AAAAAAAA" ///< https额外参数

#define LOG_TCP_LISTEN_ADDR     "0.0.0.0"       ///< tcp默认监听地址
#define LOG_TCP_LISTEN_PORT     22306           ///< tcp监听端口
#define LOG_TCP_BLOCK           5               ///< tcp最大监听缓冲池

#define LOG_FILE_FLAG_START     "star"          ///< 文件开始标识
#define LOG_STR_FLAG_START      "$@"            ///< 字符串开始标识
#define LOG_B_FLAG_START        "$@"            ///< 二进制开始标识

#define LOG_UPLOAD_TIME_OUT     (1 * 60)        ///< 上传超时时间,一小时

#define LOG_SEND_BUF_HEAD_LEN   32              ///< 日志文件头部预留大小

//#define TCP_REQUEST_HEAD        "IBBM"        ///< 通信协议头部
#define TCP_REQUEST_HEAD        "LOGT"          ///< 通信协议头部
#define PROTOCOL_VER            1               ///< TCP协议版本

//#define LOG_BUG_FILE_NAME       "/usr/share/bugfile"
#define LOG_BUF_FILE_NAME       "/usr/share/logfile"
#ifndef _TEST
#define printf(format,...)
#endif

//#define printf_i(format,arg...) fprintf(stdout, format, ##arg)
#define printf_i(format,arg...)

/** @brief iBaby网络协议头部结构 */
typedef struct RequestHead {
	char head[4];  ///< 协议头, IBBM
	short cmd;     ///< 命令号
	char version;  ///< 协议版本
	char crsv[8];  ///< 保留字
	int len;       ///< 数据体长度,不包含头部
	int irsv;      ///< 保留字段
}__attribute__ ((packed)) RequestHead;

/** @brief 日志网络协议头部结构,采用iSmart协议头部 */
typedef struct LogReqHead {
	char head[4];  ///< 协议头, LOGT
	int cmd;       ///< 命令号
	int ver;       ///< 协议版本
	int len;       ///< 数据体长度,不包含头部
}__attribute__ ((packed)) LogReqHead;

/** @brief 通道结构,引入M5通道结构 */
typedef struct Channel_t {
	int name;      ///< 通道名称
	int fd;        ///< 通道描述符
	char type;     ///< 通道类型
	time_t tm;     ///< 通道时间
	int status;    ///< 通道状态
}Channel_t;

/** @brief 线程状态 */
enum {
	THREAD_OFF,    ///< 线程关闭
	THREAD_ON,     ///< 线程运行
};

/** @brief 日志区域状态 */
enum {
	LOG_OFF,      ///< 日志关闭
	LOG_ON,       ///< 日志打开
};

/** @brief 日志服务运行状态 */
enum {
	SERVER_OFF,   ///< 日志服务关闭
	SERVER_ON,    ///< 日志服务打开
};

/** @brief 日志上传状态 */
enum {
	LOG_UP_OFF,   ///< 未在上传状态
	LOG_UP_ON,    ///< 上传状态
};

/** @brief 日志区域索引 */
enum {
	LOG_MSG = 0,      ///< 消息日志索引
	LOG_B = 1,        ///< 二进制日志索引
	LOG_STR = 2,      ///< 字符串日志索引
};

enum {
	LOG_PRO_FILE = 1, ///< 日志文件标记
};

enum {
	LOG_PRO_FILE_CONF_REQ = 0x42,     ///< 日志参数设置
	LOG_PRO_FILE_CONF_RES = 0x43,     ///< 日志参数设置回应
	LOG_PRO_FILE_GET_CONF_REQ = 0x44, ///< 日志参数获取
	LOG_PRO_FILE_GET_CONF_RES = 0x45, ///< 日志参数获取回应
	LOG_PRO_FILE_GET_FILE_REQ = 0x46, ///< 日志文件获取
	LOG_PRO_FILE_GET_FILE_RES = 0x47, ///< 日志文件获取回应
	LOG_PRO_FILE_VERIFY_REQ = 0x48,   ///< 日志认证
	LOG_PRO_FILE_VERIFY_RES = 0x49,   ///< 日志认证回应
};

/** @brief 日志启动参数 */
enum {
	LOG_INIT_OFF, ///< 默认不启动参数配置
	LOG_INIT_ON,  ///< 默认启动参数配置
};

/** @brief 通用开关 */
enum {
	OFF,
	ON,
};

/** @brief 日志基本信息 */
typedef struct LogFileInfo_t {
	int itemSize;  ///< 日志条目大小
	int itemNum;   ///< 日志条数
	char *start;   ///< 日志区域开始
	char *head;    ///< 日志区域头指针
	char *stop;    ///< 日志区域尾部
	int logFlag;   ///< 日志区域状态
}LogFileInfo_t;

/** @brief 日志服务基本参数 */
typedef struct ServerInfo_t {
	char httpServerHostName[64];   ///< 服务器地址
	char httpServerApi[64];        ///< 服务器接口
	int httpServerPort;            ///< 服务器端口
	int tcpServerPort;             ///< tcp服务监听端口
	char logfileStartFlag[5];      ///< 日志文件头部
	char strLogStartFlag[3];       ///< 字符日志条目头部
	char bLogStartFlag[2 + 4 + 4]; ///< 二进制日志条目头部
	char httpsUploadSwitch;        ///< 日志上传开关
} ServerInfo_t;

/** @brief 日志服务属性 */
typedef struct Server_t {
	pthread_mutex_t mutex;           ///< 互斥锁
	time_t tm;                       ///< 模块时间
	char rHead[MAX_HEAD_BUF_LEN];    ///< 发送头部缓冲
	char rTcpHead[MAX_HEAD_BUF_LEN]; ///< tcp发送头部缓冲
	char upSwitch;                   ///< 上传开关
	int httpsSockFd;                 ///< https套接字
	int tcpServerFd;                 ///< tcp套接字
	Channel_t clientChannel;         ///< 用户通道
	fd_set fdset;                    ///< 套接字描述集合
	int maxfd;                       ///< 描述集合最大描述符号
	struct timeval timeout;          ///< 超时
	char httpsServerIp[20];          ///< https服务器地址
	ServerInfo_t info;               ///< 可配置参数区属性
	LogFileInfo_t logfileInfo[3];    ///< 日志区域属性
	char serverStatus;               ///< 服务器状态
	char *logBuf;                    ///< 日志区域
	char serviceFlags[4];            ///< 线程状态
	volatile char linkKey[16];       ///< 认证密钥
	int httpsTimeout;                ///< https上传超时
//	FILE *bugFile;
	char *shareData;
} Server_t;

Server_t server_t = {0};             ///< 日志服务器初始化

int log_start(int opt);
void *log_run(void *arg);
int log_init(Server_t *arg);
void log_monitor(Server_t *self);
void log_upload(Server_t *self);
void *log_https_run(void *arg);
void *log_tcp_run(void *arg);
void *log_tcpServer_run(void *arg);
void *log_test_run(void *arg);
void *log_test_run2(void *arg);

char *log_getVersion();
char *log_getVersion() {
	return "1.2"; //log_s->log_printf
}

void log_b(void *data, int len);
void log_s(const char *data, int len);
int log_printf(const char* format, ...);
void log_m(char type, char cmd);
int LogTcpServer_init(Server_t *self, const char *addr, int port);

//char log_upSwitch(char opt, int flag);
int log_httpsRequest(char *request, int reqlen, char *body, int blen, int sock, int timeout);
int packHead(RequestHead *rHead, const char *head, short cmd, int len);
int packLogReqHead(LogReqHead *rHead, const char *head, int cmd, int len);

int logServerConf(int cMaxLen, int bMaxSize, int bMaxLen, const char *bStartFlag, int strBufMaxSize,
		const char *strStartFlag, const char *fileStartFlag, const char *httpsServerAddr,
		int httpsServerPort, const char *httpsServerUrl, int tcpPort, int timeout);

int logServerGetConf(int *cMaxLen, int *bMaxSize, int *bMaxLen, char *bStartFlag, int *strBufMaxSize,
		char *strStartFlag, char *fileStartFlag, char *httpsServerAddr,
		int *httpsServerPort, char *httpsServerUrl, int *tcpPort, int *timeout);

int logServerSetLinkKey(char *key);

int conn_nonb(int sockfd, const struct sockaddr_in *saptr, socklen_t salen, int nsec);

int logChannelHandleAction(Channel_t *head, Channel_t *channel);

/**
 @brief 发送数据
 @author yangguozheng
 @param[in] sock 套接字描述
 @param[out] buf 发送缓冲区
 @param[in] len 发送数据长度
 @param[in] flag 操作选项
 @return 0
 @note
 */
int sendSockData(int sock, void *buf, int len, int flag) {
	int retval = send(sock, buf, len, flag);
	if(-1 == retval) {
		perror("sendSockData: send -1");
		return -1;
	}
	return 0;
}

/**
 @brief 开启线程
 @author yangguozheng
 @param[in] run 线程入口
 @param[in] arg 线程参数
 @return 0
 @note
 */
int threadStart(void *(*run)(void *), void *arg) {
	pthread_t threadId;
	pthread_attr_t threadAttr;
	memset(&threadAttr,0,sizeof(pthread_attr_t));
	pthread_attr_init(&threadAttr);
	pthread_attr_setdetachstate(&threadAttr,PTHREAD_CREATE_DETACHED);
	int result = pthread_create(&threadId, &threadAttr, run, arg);
	pthread_attr_destroy(&threadAttr);
	printf("threadStart: threadId %ld\n", threadId);
	if(result != 0) {
		perror("threadStart: pthread_create");
		return -1;
	}
	return threadId;
}

/**
 @brief 地址初始化
 @author yangguozheng
 @param[out] clientAddr 地址结构
 @param[in] addr 地址
 @param[in] port 端口
 @return 0
 @note
 */
int sockAddrInit(struct sockaddr_in *clientAddr, const char *addr, int port) {
	bzero(clientAddr,sizeof(struct sockaddr_in));
	clientAddr->sin_family = AF_INET;
	clientAddr->sin_addr.s_addr = inet_addr(addr);
	clientAddr->sin_port = htons(port);
	return 0;
}

/**
 @brief 服务模块初始化
 @author yangguozheng
 @param[in|out] self 服务属性
 @param[in] addr 地址
 @param[in] port 监听端口
 @return 0
 @note
 */
int LogTcpServer_init(Server_t *self, const char *addr, int port) {
	self->tcpServerFd = socket(AF_INET, SOCK_STREAM, 0);
	if(self->tcpServerFd == -1) {
		perror("TcpServer_init: socket");
		return -1;
	}
	printf("TcpServer_init: socket ok\n");
	struct sockaddr_in sAddr;
	sockAddrInit(&sAddr, addr, port);
	int sockOpt = 1; /*turn on*/

	if(setsockopt(self->tcpServerFd, SOL_SOCKET, SO_REUSEADDR, &sockOpt, sizeof(int)) != 0) {
		perror("TcpServer_init: setsockopt");
		return -1;
	}
	printf("TcpServer_init: setsockopt ok\n");
	if( -1 == bind(self->tcpServerFd, (struct sockaddr *)&sAddr, sizeof(struct sockaddr_in))) {
		perror("TcpServer_init: bind");
		close(self->tcpServerFd);
		return -1;
	}
	printf("TcpServer_init: bind ok\n");
	if(-1 == listen(self->tcpServerFd, LOG_TCP_BLOCK)) {
		perror("TcpServer_init: listen");
		close(self->tcpServerFd);
		return -1;
	}
	printf("TcpServer_init: listen ok\n");
	return 0;
}

#ifdef _TEST
/** @brief 测试程序 */
void signal_handle(int opt) {

}

int main(int argc, char *argv[]) {
	printf("++++++++++++++++++++++++++++++++++++++++\n");
	printf("main process %d\n", getpid());
	printf("++++++++++++++++++++++++++++++++++++++++\n");
	if(SIG_ERR == signal(SIGPIPE, signal_handle)) {
		perror("SIGPIPE register error");
	}
	if(-1 == log_start(1)) {
		return -1;
	}
	while(1) {
		sleep(100);
	}
	return 0;
}
#endif

#include <sys/shm.h>
#include <sys/stat.h>

/**
 @brief 开启服务
 @author yangguozheng
 @param[in] opt 可选参数
 @return 0
 @note
 */
int log_start(int opt) {
	key_t key;
	key = ftok(".", 10086);
	int segment_id = shmget(key, getpagesize(), IPC_CREAT);

	Server_t *self = &server_t;
	self->shareData = (char*)shmat(segment_id, 0, 0);

	self->serverStatus = SERVER_OFF;
	if(0 != pthread_mutex_init(&self->mutex, NULL)) {
		perror("log_start: 0 != pthread_mutex_init");
		return -1;
	}
	self->serverStatus = SERVER_ON;
//	self->bugFile = fopen(LOG_BUG_FILE_NAME, "w");
//	if(NULL != self->bugFile) {
//		fprintf(self->bugFile, "log_start %d\n", getpid());
//	}
	printf("log_start %d\n", getpid());
	/////
	self->logfileInfo[LOG_STR].itemNum = MAX_STR_LOG_LEN * MAX_STR_LOG_ITEM_SIZE;
	self->logfileInfo[LOG_STR].itemSize = 1;
	self->info.httpServerPort = LOG_HTTPS_SERVER_PORT;
	strcpy(self->info.httpServerHostName, LOG_HTTPS_SERVER_ADDR);
	self->info.tcpServerPort = LOG_TCP_LISTEN_PORT;
	strcpy(self->info.httpServerApi, LOG_HTTPS_SERVER_PAGE);
	strcat(self->info.httpServerApi, LOG_MAC);

	self->logfileInfo[LOG_B].itemNum = MAX_B_LOG_LEN;
	self->logfileInfo[LOG_B].itemSize = MAX_B_LOG_ITEM_SIZE;
	self->logfileInfo[LOG_MSG].itemNum = MAX_MSG_LOG_LEN;
	self->logfileInfo[LOG_MSG].itemSize = MAX_MSG_LOG_ITEM_SIZE;
	strcpy(self->info.strLogStartFlag, LOG_STR_FLAG_START);
	strcpy(self->info.bLogStartFlag, LOG_B_FLAG_START);
	strcpy(self->info.logfileStartFlag, LOG_FILE_FLAG_START);
	self->httpsTimeout = 0;
	memset(self->linkKey, 'F', sizeof(self->linkKey));
	self->info.httpsUploadSwitch = ON;
//	self->info.tcpServerPort;
	if(LOG_INIT_ON == opt) {
		if(-1 == log_init(self)) {
			perror("log_run: -1 == log_init");
			return -1;
		}
	}
	threadStart(log_run, self);
#ifdef _TEST
	threadStart(log_test_run, self);
#endif
	threadStart(log_tcpServer_run, self);
	return 0;
}

/**
 @brief 日志监测服务
 @author yangguozheng
 @param[in] arg 环境变量
 @return 0
 @note
 */
void *log_run(void *arg) {
	printf("log_run %d\n", getpid());
	Server_t *self = (Server_t *)arg;
//	if(NULL != self->bugFile) {
//		fprintf(self->bugFile, "log_run %d\n", getpid());
//	}
	printf("++++++++++++++++++++++++++++++++++++++++\n");
	printf("log_run %d\n", getpid());
	printf("++++++++++++++++++++++++++++++++++++++++\n");
	while(1) {
		if(SERVER_OFF == self->serverStatus) {
			self->serviceFlags[0] = THREAD_OFF;
			continue;
		}
		self->serviceFlags[0] = THREAD_ON;
		log_monitor(self);
		sleep(1);
	}
	return 0;
}

/**
 @brief 日志项参数初始化
 @author yangguozheng
 @param[info] arg 日志指针
 @param[itemSize] 单条大小
 @param[itemNum] 日志条数
 @return 0
 @note
 */
int logFileBufInit(LogFileInfo_t *info, int itemSize, int itemNum) {
	int retval = info->itemNum * info->itemSize;
	if(retval <= 0 || retval > itemSize * itemNum) {
		info->itemNum = 0;
		info->itemSize = 0;
		retval = 0;
		info->logFlag = LOG_OFF;
		printf("OFF OFF \n");
	}
	else {
		if(retval > itemSize * itemNum) {
			info->itemNum = itemNum;
			info->itemSize = itemSize;
			retval = itemSize * itemNum;
		}
		info->logFlag = LOG_ON;
		printf("ON ON \n");
	}
	return retval;
}

/**
 @brief 日志项参数初始化
 @author yangguozheng
 @param[self] 环境变量
 @return 0
 @note
 */
int log_init(Server_t *self) {
//	pthread_mutex_lock(&self->mutex);
	self->serverStatus = SERVER_OFF;
	self->upSwitch = LOG_UP_OFF;
	int retval = 0;
	retval += logFileBufInit(self->logfileInfo + LOG_MSG, MAX_MSG_LOG_ITEM_SIZE, MAX_MSG_LOG_LEN);
	retval += logFileBufInit(self->logfileInfo + LOG_B, MAX_B_LOG_ITEM_SIZE, MAX_B_LOG_LEN);
	retval += logFileBufInit(self->logfileInfo + LOG_STR, MAX_STR_LOG_ITEM_SIZE,  MAX_STR_LOG_LEN);
	self->httpsTimeout = 24 * 60;
	if(0 == retval) {
//		pthread_mutex_unlock(&self->mutex);
		self->httpsTimeout = 0;
		return -1;
	}
	if(NULL != self->logBuf) {
		free(self->logBuf);
	}
	self->logBuf = malloc(retval + 32);
	if(NULL == self->logBuf) {
		pthread_mutex_unlock(&self->mutex);
		return -1;
	}
	self->logfileInfo[LOG_MSG].start = self->logBuf + 32;
	self->logfileInfo[LOG_MSG].head = self->logfileInfo[LOG_MSG].start;
	self->logfileInfo[LOG_MSG].stop = self->logfileInfo[LOG_MSG].start + self->logfileInfo[LOG_MSG].itemSize * self->logfileInfo[LOG_MSG].itemNum;
	self->logfileInfo[LOG_B].start = self->logfileInfo[LOG_MSG].stop;
	self->logfileInfo[LOG_B].head = self->logfileInfo[LOG_B].start;
	self->logfileInfo[LOG_B].stop = self->logfileInfo[LOG_B].start + self->logfileInfo[LOG_B].itemSize * self->logfileInfo[LOG_B].itemNum;
	self->logfileInfo[LOG_STR].start = self->logfileInfo[LOG_B].stop;
	self->logfileInfo[LOG_STR].head = self->logfileInfo[LOG_STR].start;
	self->logfileInfo[LOG_STR].stop = self->logfileInfo[LOG_STR].start + self->logfileInfo[LOG_STR].itemSize * self->logfileInfo[LOG_STR].itemNum;
	strcpy(self->logBuf, self->info.logfileStartFlag);
	*(int *)(self->logBuf + strlen(self->info.logfileStartFlag)) = time(NULL);
	*(int *)(self->logBuf + sizeof(int) + strlen(self->info.logfileStartFlag)) = self->logfileInfo[LOG_MSG].itemNum;
	*(int *)(self->logBuf + 2 * sizeof(int) + strlen(self->info.logfileStartFlag)) = self->logfileInfo[LOG_B].itemNum;
	*(int *)(self->logBuf + 3 * sizeof(int) + strlen(self->info.logfileStartFlag)) = self->logfileInfo[LOG_STR].itemSize * self->logfileInfo[LOG_STR].itemNum;
	*(int *)(self->logBuf + 4 * sizeof(int) + strlen(self->info.logfileStartFlag)) = self->logfileInfo[LOG_B].itemSize;
	self->serverStatus = SERVER_ON;
	time(&self->tm);
//	pthread_mutex_unlock(&self->mutex);
	return 0;
}

/**
 @brief 日志监测
 @author yangguozheng
 @param[self] 环境变量
 @return 0
 @note
 */
void log_monitor(Server_t *self) {
	static char upState = 0;
	static int upIndex = 0;
	int logfileSize = 0;
	printf("%d, %d, %d\n", self->logfileInfo[LOG_MSG].stop - self->logfileInfo[LOG_MSG].head,
			self->logfileInfo[LOG_B].stop - self->logfileInfo[LOG_B].head,
			self->logfileInfo[LOG_STR].stop - self->logfileInfo[LOG_STR].head);

	upState = self->upSwitch;
//		printf("upState %d\n", upState);
	if(LOG_UP_ON == upState) {
		++upIndex;
		printf("LOG_UP_ONLOG_UP_ONLOG_UP_ONLOG_UP_ONLOG_UP_ON\n");
		logfileSize = self->logfileInfo[LOG_MSG].itemNum * self->logfileInfo[LOG_MSG].itemSize
					+ self->logfileInfo[LOG_B].itemNum * self->logfileInfo[LOG_B].itemSize
					+ self->logfileInfo[LOG_STR].itemNum * self->logfileInfo[LOG_STR].itemSize + 32;
		FILE *file = fopen(LOG_BUF_FILE_NAME, "w");
//		fwrite(&upIndex, sizeof(int), 1, file);
		fwrite(self->logBuf, logfileSize, 1, file);
//		fflush(file);
		fclose(file);
		*(int *)self->shareData = upIndex;
//		*(int *)(self->shareData + 1) = upIndex;
		self->upSwitch = LOG_UP_OFF;
	}
	if(0 == self->httpsTimeout) {
		return;
	}
	if(time(NULL) - self->tm > self->httpsTimeout * 60) {
		time(&self->tm);
		self->upSwitch = LOG_UP_ON;
	}
	return;
//	if(upState != self->upSwitch) {
		upState = self->upSwitch;
//		printf("upState %d\n", upState);
		if(LOG_UP_ON == upState) {
			printf("LOG_UP_ONLOG_UP_ONLOG_UP_ONLOG_UP_ONLOG_UP_ON\n");
			log_upload(self);
			logfileSize = self->logfileInfo[LOG_MSG].itemNum * self->logfileInfo[LOG_MSG].itemSize
						+ self->logfileInfo[LOG_B].itemNum * self->logfileInfo[LOG_B].itemSize
						+ self->logfileInfo[LOG_STR].itemNum * self->logfileInfo[LOG_STR].itemSize + 32;
			FILE *file = fopen(LOG_BUF_FILE_NAME, "w");
			fwrite(&upIndex, sizeof(int), 1, file);
			fwrite(self->logBuf, logfileSize, 1, file);
			fclose(file);
			self->upSwitch = LOG_UP_OFF;
		}
//	}
}

/**
 @brief 连接套接字
 @author yangguozheng
 @param[sockfd] 套接字描述
 @param[saptr] 地址信息
 @param[salen] 地址信息长度
 @param[nsec] 超时时间
 @return 0
 @note
 */
int conn_nonb(int sockfd, const struct sockaddr_in *saptr, socklen_t salen, int nsec)
{
    int flags, n, error, code;
    socklen_t len;
    fd_set wset;
    struct timeval tval;

    flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    error = 0;
    if ((n == connect(sockfd, saptr, salen)) == 0) {
        goto done;
    } else if (n < 0 && errno != EINPROGRESS){
        return (-1);
    }

    /* Do whatever we want while the connect is taking place */

    FD_ZERO(&wset);
    FD_SET(sockfd, &wset);
    tval.tv_sec = nsec;
    tval.tv_usec = 0;

    if ((n = select(sockfd+1, NULL, &wset,
                    NULL, nsec ? &tval : NULL)) == 0) {
        close(sockfd);  /* timeout */
        errno = ETIMEDOUT;
        return (-1);
    }

    if (FD_ISSET(sockfd, &wset)) {
        len = sizeof(error);
        code = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len);
        /* 如果发生错误，Solaris实现的getsockopt返回-1，
         * 把pending error设置给errno. Berkeley实现的
         * getsockopt返回0, pending error返回给error.
         * 我们需要处理这两种情况 */
        if (code < 0 || error) {
            close(sockfd);
            if (error)
                errno = error;
            return (-1);
        }
    } else {
        fprintf(stderr, "select error: sockfd not set");
        return -1;
    }

done:
    fcntl(sockfd, F_SETFL, flags);  /* restore file status flags */
    return (0);
}

/**
 @brief 根据主机名获得ip
 @author yangguozheng
 @param[in] ip 得到的ip
 @param[in] host主机名
 @return 小于0代表失败 大于0代表成功
 @note
 */
int getIpByHost(char *ip, const char *host)
{

    int nRet = 0;
    char **ppList;
    struct hostent *pHostent;
    char strIP[40+1];

    if ((NULL==ip) || (NULL==host))
    {
        //DEBUGPRINT(DEBUG_ERROR, "Pointer is NULL! @:%s @:%d\n", __FILE__, __LINE__);
        return -1;
    }

    if((pHostent = gethostbyname(host)) == NULL)
    {
        //DEBUGPRINT(DEBUG_ERROR, "Get host IP by name error! @:%s @:%d\n", __FILE__, __LINE__);
        return -1;
    }

    switch(pHostent->h_addrtype)
    {
       case AF_INET:
       case AF_INET6:
            ppList = pHostent->h_addr_list;
            for (; *ppList!=NULL; ppList++)
            {
                sprintf(ip, "%s", inet_ntop(pHostent->h_addrtype, *ppList, strIP, sizeof(strIP)));
                if (strlen(ip) > 0)
                {
                    break;
                }
            }
            break;
       default:
            //DEBUGPRINT(DEBUG_ERROR, "Unknown address type! @:%s @:%d\n", __FILE__, __LINE__);
            nRet = -3;
            break;
    }

    if (strlen(ip)>0)
    {
        if (ip[strlen(ip)-1] == '\n')
        {
            ip[strlen(ip)-1] = '\0';
        }
    }

    return nRet;
}

/**
 @brief 连接判断
 @author yangguozheng
 @param[sock] 套接字描述
 @return 0
 @note
 */
int LogSocketConnected(int sock)
{
	if(sock<=0) {
		return 0;
	}
	struct tcp_info info;
	int len=sizeof(info);
	getsockopt(sock, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len);
	if((info.tcpi_state == TCP_ESTABLISHED))
	{
		//myprintf("socket connected\n");
		return 1;
	}
	else
	{
	//myprintf("socket disconnected\n");
		return 0;
	}
}

/**
 @brief 日志上传
 @author yangguozheng
 @param[self] 环境变量
 @return 0
 @note
 */
void log_upload(Server_t *self) {
	if(self->info.httpsUploadSwitch == OFF) {
		return;
	}
	if(!(self->serverStatus == SERVER_ON)) {
		return;
	}
	if(self->serviceFlags[2] == THREAD_ON) {
		return;
	}
//	if(0 != self->tcpClientFd) {
//		if(1 == LogSocketConnected(self->tcpClientFd)) {
//			threadStart(log_tcp_run, self);
//			return;
//		}
//		else {
//			shutdown(self->tcpClientFd, SHUT_RDWR);
//			close(self->tcpClientFd);
//			self->tcpClientFd = 0;
//			sleep(2);
//		}
//	}
	printf("log_upload %d\n", self->upSwitch);

	self->httpsSockFd = socket(AF_INET, SOCK_STREAM, 0);
	if(self->httpsSockFd == -1) {
		perror("TcpServer_init: socket");
//		log_upSwitch(LOG_UP_OFF, LOG_UP_SET);
		return;
	}
	printf("TcpServer_init: socket ok\n");
	struct sockaddr_in sAddr;
	if(0 != getIpByHost(self->httpsServerIp, self->info.httpServerHostName)) {
		perror("log_upload: getIpByHost");
//		log_upSwitch(LOG_UP_OFF, LOG_UP_SET);
		return;
	}
	sockAddrInit(&sAddr, self->httpsServerIp, self->info.httpServerPort);
	int sockOpt = 1; /*turn on*/

	if(setsockopt(self->httpsSockFd, SOL_SOCKET, SO_REUSEADDR, &sockOpt, sizeof(int)) != 0) {
		perror("TcpServer_init: setsockopt");
//		log_upSwitch(LOG_UP_OFF, LOG_UP_SET);
		return;
	}
	printf("TcpServer_init: setsockopt ok\n");
	if(0 != conn_nonb(self->httpsSockFd, &sAddr, sizeof(struct sockaddr_in), 3)) {
		printf("log_upload: conn_nonb error\n");
//		log_upSwitch(LOG_UP_OFF, LOG_UP_SET);
		return;
	}
//	if(0 != connect(self->httpsSockFd, &sAddr, sizeof(struct sockaddr_in))) {
//		perror("log_upload: connect");
//		return;
//	}
	threadStart(log_https_run, self);
}

/**
 @brief 接收数据
 @author yangguozheng
 @param[in] sock 套接字描述
 @param[out] buf 接收缓冲区
 @param[in] len 接收数据长度
 @param[in] flag 操作选项
 @return 0
 @note
 */
int recvSockData(int sock, void *buf, int len, int flag) {
	int retval = recv(sock, buf, len, flag);
	if(retval > 0) {

	} else if((retval < 0)&&(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)) {
		perror("recvSockData: -2 error but not disconnect");
		return -2;
	} else {
		return -1;
	}
	return retval;
}

/**
 @brief 读取http信息并分析
 @author yangguozheng
 @param[in] sock 套接字描述
 @param[out] buf 接收缓冲区
 @param[in] len 缓冲区最大数据长度
 @param[in] flag 操作选项
 @return 0
 @note
 */
int recvHttpSockData(int sock, char *buf, int len, int flag) {
	int ret = recvSockData(sock, buf, len, flag);
	if(-1 == ret) {
		return -1;
	}
	regex_t re;
	regmatch_t match[2];
	regex_t headEndRe;
	int rLen = ret;
	regcomp(&headEndRe, "\r\n\r\n", REG_EXTENDED);
	regcomp(&re, "Content-Length:[ ]*([0-9]{1,})\r\n", REG_EXTENDED);
	while(1) {
		if(0 == regexec(&headEndRe, buf, 1, match, REG_EXTENDED)) {
			break;
		}
		if(rLen == len) {
			break;
		}
		ret = recvSockData(sock, buf + rLen, len - rLen, flag);
		if(ret > 0) {
			rLen += ret;
		} else if(-1 == ret) {
			return -1;
		}
	}
	int rLenBody = 0;
	int rLenHead = match[0].rm_eo;
	if(0 == regexec(&re, buf, 2, match, REG_EXTENDED)) {
//		printf("%c %c\n", *(char *)(buf + match->rm_so), *(char *)(buf + match[1].rm_so));
		sscanf(buf + match[1].rm_so, "%d", &rLenBody);
		printf("%d\n", rLenBody);
		if(rLenBody <= 0 || rLenBody + rLenHead >= len) {
			return rLen;
		}
		while(1) {
			if(rLen == rLenBody + rLenHead) {
				break;
			}
			ret = recvSockData(sock, buf + rLen, rLenBody + rLenHead - rLen, flag);
			if(ret > 0) {
				rLen += ret;
			} else if(-1 == ret) {
				return -1;
			}
		}
//		printf("recvHttpSockData\n");
	}
	printf("recvHttpSockData\n");
	return ret;
}

/**
 @brief 接收数据
 @author yangguozheng
 @param[in] arg 环境变量
 @return 0
 @note
 */
void *log_https_run(void *arg) {
	printf("log_tcpServer_run %d\n", getpid());
	Server_t *self = (Server_t *)arg;
//	if(NULL != self->bugFile) {
//		fprintf(self->bugFile, "log_https_run %d\n", getpid());
//	}

	printf("++++++++++++++++++++++++++++++++++++++++\n");
	printf("log_https_run %d\n", getpid());
	printf("++++++++++++++++++++++++++++++++++++++++\n");
	if(SERVER_OFF == self->serverStatus) {
		self->serviceFlags[2] = THREAD_OFF;
		goto done;
	}
	self->serviceFlags[2] = THREAD_ON;
	struct timeval timeOut = {0, 5000000};
	int ret = 0;
	setsockopt(self->httpsSockFd,SOL_SOCKET,SO_SNDTIMEO,&timeOut,sizeof(struct timeval));
	setsockopt(self->httpsSockFd,SOL_SOCKET,SO_RCVTIMEO,&timeOut,sizeof(struct timeval));
//	ChildProcessLog_t *head = self->childProcessLog_t;
	memset(self->rHead, '\0', sizeof(self->rHead));
//	sprintf(self->rHead, "POST %s HTTP/1.1\r\n\
//Host: %s\r\n\
//Content-Type: application/octet-stream\r\n\
//Content-Length: %d\r\n\r\n",
//			LOG_SERVER_PAGE, LOG_SERVER_ADDR,
//			sizeof(self->childProcessLog_t) + sizeof(self->httpsLog_t) + sizeof(self->tcpLog_t)
//			+ sizeof(self->logHead) + sizeof(self->logTail));
	sprintf(self->rHead, "POST %s&time=%08X HTTP/1.1\r\n\
Host: %s\r\n\
Content-Type: application/octet-stream\r\n\
Content-Length: %d\r\n\r\n",
			self->info.httpServerApi, time(NULL), self->info.httpServerHostName,
			self->logfileInfo[LOG_MSG].itemNum * self->logfileInfo[LOG_MSG].itemSize
			+ self->logfileInfo[LOG_B].itemNum * self->logfileInfo[LOG_B].itemSize
			+ self->logfileInfo[LOG_STR].itemNum * self->logfileInfo[LOG_STR].itemSize + 32);
	printf("http request head %s, head len %d\n", self->rHead, strlen(self->rHead));
	ret = log_httpsRequest(self->rHead, sizeof(self->rHead), self->logBuf,
			self->logfileInfo[LOG_MSG].itemNum * self->logfileInfo[LOG_MSG].itemSize
			+ self->logfileInfo[LOG_B].itemNum * self->logfileInfo[LOG_B].itemSize
			+ self->logfileInfo[LOG_STR].itemNum * self->logfileInfo[LOG_STR].itemSize + 32,
			self->httpsSockFd, 0);
	if(-1 == ret) {
		perror("log_https_run: log_httpsRequest");
		goto done;
	}
	//*(long *)(self->logHead + 8) = time(NULL);
#if 0
	if(-1 == sendSockData(self->httpsSockFd, self->rHead, strlen(self->rHead), MSG_WAITALL)) {
		goto done;
	}
//	if(-1 == sendSockData(self->httpsSockFd, self->logHead,
//			sizeof(self->childProcessLog_t) + sizeof(self->tcpLog_t)
//			+ sizeof(self->httpsLog_t) + sizeof(self->logHead) + sizeof(self->logTail), MSG_WAITALL)) {
//		goto done;
//	}
	*(int *)(self->logBuf) = time(NULL);
	if(-1 == sendSockData(self->httpsSockFd, self->logBuf,
			self->logfileInfo[LOG_MSG].itemNum * self->logfileInfo[LOG_MSG].itemSize
			+ self->logfileInfo[LOG_B].itemNum * self->logfileInfo[LOG_B].itemSize
			+ self->logfileInfo[LOG_STR].itemNum * self->logfileInfo[LOG_STR].itemSize + 32, MSG_WAITALL)) {
		goto done;
	}
//	if(-1 == sendSockData(self->httpsSockFd, self->httpsLog_t, sizeof(self->httpsLog_t), MSG_WAITALL)) {
//		goto done;
//	}
//	if(-1 == sendSockData(self->httpsSockFd, self->tcpLog_t, sizeof(self->tcpLog_t), MSG_WAITALL)) {
//		goto done;
//	}
	memset(self->rHead, '\0', sizeof(self->rHead));
	if(-1 == recvHttpSockData(self->httpsSockFd, self->rHead, sizeof(self->rHead), MSG_WAITALL)) {
		goto done;
	}
#endif
	printf("http response head %s\n", self->rHead);
	done: {
		shutdown(self->httpsSockFd, SHUT_RDWR);
		close(self->httpsSockFd);
//		log_upSwitch(LOG_UP_OFF, LOG_UP_SET);
		printf("log_https_run: done\n");
	}
	sleep(1);
//	printf("%s\n", self->logfileInfo[LOG_STR].start);
	printf("log_https_run ******************************************************* %d\n", self->upSwitch);
	sleep(1);
	self->serviceFlags[2] = THREAD_OFF;
	return 0;
}

/**
 @brief 接受用户连接，并处理通道声明
 @author yangguozheng
 @param[in|out] self 服务属性
 @return 0
 @note
 */
int LogTcpServer_accept(Server_t *self) {
	int cfd = 0;
	if(FD_ISSET(self->tcpServerFd, &(self->fdset))) {
		cfd = accept(self->tcpServerFd, NULL, NULL);
		if(-1 == cfd) {
			perror("TcpServer_accept: accept\n");
			return -1;
		}
		printf("TcpServer_accept: accept ok\n");
		if(0 != self->clientChannel.fd) {
			shutdown(self->clientChannel.fd, SHUT_RDWR);
			close(self->clientChannel.fd);
			self->clientChannel.fd = 0;
			sleep(2);
		}
		self->clientChannel.fd = cfd;
		struct timeval timeOut = {12000, 0};
		setsockopt(self->clientChannel.fd, SOL_SOCKET, SO_SNDTIMEO, &timeOut, sizeof(struct timeval));
		setsockopt(self->clientChannel.fd, SOL_SOCKET, SO_RCVTIMEO, &timeOut, sizeof(struct timeval));
		if(-1 == logChannelHandleAction(&self->clientChannel, NULL)) {
			shutdown(self->clientChannel.fd, SHUT_RDWR);
			close(self->clientChannel.fd);
			self->clientChannel.fd = 0;
			sleep(1);
		}
		LogReqHead *rHead = (LogReqHead *)self->rTcpHead;
		char *text = (char *)(rHead + 1);
		if(LOG_PRO_FILE_VERIFY_REQ == rHead->cmd) {
			printf_i("key r:%08X, %08X, %08X, %08X\n"
					, *(int *)text
					, *(int *)(text + sizeof(int))
					, *(int *)(text + 2 * sizeof(int))
					, *(int *)(text + 3 * sizeof(int)));
			printf_i("key l:%08X, %08X, %08X, %08X\n"
					, *(int *)(self->linkKey)
					, *(int *)(self->linkKey + sizeof(int))
					, *(int *)(self->linkKey + 2 * sizeof(int))
					, *(int *)(self->linkKey + 3 * sizeof(int)));
			if(0 == memcmp(text, self->linkKey, sizeof(self->linkKey))) {
				rHead->cmd = LOG_PRO_FILE_VERIFY_RES;
				rHead->len = 2;
				strcpy(text, "00");
				if(-1 == sendSockData(self->clientChannel.fd, rHead, sizeof(LogReqHead) + rHead->len, MSG_WAITALL)) {
					perror("LogTcpServer_accept: sendSockData LOG_PRO_FILE_VERIFY_REQ");
				}
				printf("LogTcpServer_accept: sendSockData LOG_PRO_FILE_VERIFY_REQ can access\n");
				printf_i("key ok\n");
			}else {
				rHead->cmd = LOG_PRO_FILE_VERIFY_RES;
				rHead->len = 2;
				strcpy(text, "01");
				if(-1 == sendSockData(self->clientChannel.fd, rHead, sizeof(LogReqHead) + rHead->len, MSG_WAITALL)) {
					perror("LogTcpServer_accept: sendSockData LOG_PRO_FILE_VERIFY_REQ cannot access");
				}
				printf("LogTcpServer_accept: sendSockData LOG_PRO_FILE_VERIFY_REQ cannot access\n");
				printf_i("key error\n");
				shutdown(self->clientChannel.fd, SHUT_RDWR);
				close(self->clientChannel.fd);
				self->clientChannel.fd = 0;
				sleep(1);
				return -1;
			}
		}
		if(-1 == logChannelHandleAction(&self->clientChannel, NULL)) {
			shutdown(self->clientChannel.fd, SHUT_RDWR);
			close(self->clientChannel.fd);
			self->clientChannel.fd = 0;
			sleep(1);
		}
		if(LOG_PRO_FILE_CONF_REQ == rHead->cmd) {
			self->serviceFlags[1] = THREAD_OFF;
			self->serviceFlags[3] = THREAD_OFF;
			printf("LOG_PRO_FILE_CONF_REQ %d, %d, %d, %d, %d\n",
					*(int *)text
					,*(int *)(text + sizeof(int))
					,*(int *)(text + 2 * sizeof(int))
					,*(int *)(text + 3 * sizeof(int))
					,*(int *)(text + 4 * sizeof(int)));
			logServerConf(*(int *)text, *(int *)(text + sizeof(int)), *(int *)(text + 2 * sizeof(int)),
					NULL, *(int *)(text + 3 * sizeof(int)), NULL, NULL, NULL, -1, NULL, -1,
					*(int *)(text + 4 * sizeof(int)));
			rHead->cmd = LOG_PRO_FILE_CONF_RES;
			rHead->len = 2;
			strcpy(text, "00");
			if(-1 == sendSockData(self->clientChannel.fd, rHead, sizeof(LogReqHead) + rHead->len, MSG_WAITALL)) {
				perror("LogTcpServer_accept: sendSockData LOG_PRO_FILE_CONF_REQ");
			}
		} else if(LOG_PRO_FILE_GET_CONF_REQ == rHead->cmd) {
			rHead->cmd = LOG_PRO_FILE_GET_CONF_RES;
			rHead->len = 20;
			logServerGetConf((int *)text, (int *)(text + sizeof(int)), (int *)(text + 2 * sizeof(int)),
					NULL, (int *)(text + 3 * sizeof(int)), NULL, NULL, NULL, NULL, NULL, NULL,
					(int *)(text + 4 * sizeof(int)));
			if(-1 == sendSockData(self->clientChannel.fd, rHead, sizeof(LogReqHead) + rHead->len, MSG_WAITALL)) {
				perror("LogTcpServer_accept: sendSockData LOG_PRO_FILE_CONF_REQ");
			}
		} else if(LOG_PRO_FILE_GET_FILE_REQ == rHead->cmd) {
//			int totalSize = self->logfileInfo[LOG_MSG].itemNum * self->logfileInfo[LOG_MSG].itemSize
//					+ self->logfileInfo[LOG_B].itemNum * self->logfileInfo[LOG_B].itemSize
//					+ self->logfileInfo[LOG_STR].itemNum * self->logfileInfo[LOG_STR].itemSize + 32;
//			int start = *(int *)(text);
//			int stop = *(int *)(text + sizeof(int));
//			if(start >= 0 && start <= stop && stop <= totalSize) {
			printf_i("key get file\n");
			threadStart(log_tcp_run, self);
//			}
		}
	}
	return 0;
}

/**
 @brief 网络数据输入监测
 @author yangguozheng
 @param[in|out] self 服务属性
 @return 0
 @note
 */
int LogTcpServer_select(Server_t *self) {
	int retval = select(self->maxfd + 1, &(self->fdset), NULL, NULL, &(self->timeout));
	switch(retval) {
	case 0:
		printf("TcpServer_select: select 0\n");
		break;
	case -1:
		perror("TcpServer_select: select -1");
		break;
	default:
		printf("TcpServer_select: select > 0\n");
		LogTcpServer_accept(self);
		break;
	}
	return 0;
}

/**
 @brief 开启tcp服务
 @author yangguozheng
 @param[in|out] arg 服务属性
 @return 0
 @note
 */
void *log_tcpServer_run(void *arg) {
	printf("log_tcpServer_run %d\n", getpid());
	Server_t *self = (Server_t *)arg;
//	if(NULL != self->bugFile) {
//		fprintf(self->bugFile, "log_tcpServer_run %d\n", getpid());
//	}
	printf("++++++++++++++++++++++++++++++++++++++++\n");
	printf("log_tcpServer_run %d\n", getpid());
	printf("++++++++++++++++++++++++++++++++++++++++\n");
	if(-1 == LogTcpServer_init(self, LOG_TCP_LISTEN_ADDR, LOG_TCP_LISTEN_PORT)) {
		return 0;
	}
	while(1) {
		if(SERVER_OFF == self->serverStatus) {
			self->serviceFlags[1] = THREAD_OFF;
			continue;
		}
		self->serviceFlags[1] = THREAD_ON;
		self->timeout.tv_sec = 1;
		self->timeout.tv_usec = 0;
		FD_ZERO(&self->fdset);
		FD_SET(self->tcpServerFd, &self->fdset);
		self->maxfd = self->tcpServerFd;
		LogTcpServer_select(self);
	}
	return 0;
}

#include <sys/stat.h>
/**
 @brief 获取文件大小
 @author yangguozheng
 @param[in] path 文件位置
 @return 0
 @note
 */
unsigned long get_file_size(const char *path)
{
	unsigned long filesize = -1;
	struct stat statbuff;
	if(stat(path, &statbuff) < 0){
		return filesize;
	}else{
		filesize = statbuff.st_size;
	}
	return filesize;
}

/**
 @brief tcp传输线程
 @author yangguozheng
 @param[in] arg 环境变量
 @return 0
 @note
 */
void *log_tcp_run(void *arg) {
	printf("log_tcp_run %d\n", getpid());
	Server_t *self = (Server_t *)arg;
//	if(NULL != self->bugFile) {
//		fprintf(self->bugFile, "log_tcp_run %d\n", getpid());
//	}
	printf("++++++++++++++++++++++++++++++++++++++++\n");
	printf("log_tcp_run %d\n", getpid());
	printf("++++++++++++++++++++++++++++++++++++++++\n");
	if(SERVER_OFF == self->serverStatus) {
		self->serviceFlags[3] = THREAD_OFF;
		goto done;
	}
	self->serviceFlags[3] = THREAD_ON;
	int totalSize = self->logfileInfo[LOG_MSG].itemNum * self->logfileInfo[LOG_MSG].itemSize
			+ self->logfileInfo[LOG_B].itemNum * self->logfileInfo[LOG_B].itemSize
			+ self->logfileInfo[LOG_STR].itemNum * self->logfileInfo[LOG_STR].itemSize + 32;
	LogReqHead *rHead = (LogReqHead *)(self->rTcpHead);
	char *text = (char *)(rHead + 1);
	int start = 0;
	*(int *)(text) = start;
	int stop = totalSize;
	*(int *)(text + sizeof(int)) = totalSize;
	*(int *)(text + 2 * sizeof(int)) = totalSize;
	packLogReqHead(rHead, TCP_REQUEST_HEAD, LOG_PRO_FILE_GET_FILE_RES, 12 + stop - start);
	printf_i("key send head\n");
	if(-1 == sendSockData(self->clientChannel.fd, rHead,
			sizeof(LogReqHead) + 12, MSG_WAITALL)) {
		goto done;
	}
	printf_i("key send body\n");
	if(-1 == sendSockData(self->clientChannel.fd, self->logBuf + start,
			stop - start, MSG_WAITALL)) {
		goto done;
	}
	done: {
//		log_upSwitch(LOG_UP_OFF, LOG_UP_SET);
//		shutdown(self->clientChannel.fd, SHUT_RDWR);
//		close(self->clientChannel.fd);
//		self->clientChannel.fd = 0;
		sleep(2);
		printf("log_tcp_run: done\n");
	}
	return 0;
}

//char log_upSwitch(char opt, int flag) {
//	if(0 != pthread_mutex_lock(&server_t.mutex)) {
//		perror("log_m: pthread_mutex_lock");
//		return 0;
//	}
//	if(LOG_UP_GET & flag) {
//		opt = server_t.upSwitch;
//	} else if(LOG_UP_SET & flag) {
//		server_t.upSwitch = opt;
//	}
//	pthread_mutex_unlock(&server_t.mutex);
//	return opt;
//}


/**
 @brief log测试线程
 @author yangguozheng
 @param[in] arg 环境变量
 @return 0
 @note
 */
void *log_test_run(void *arg) {
	int i = 10;
	char aa[1024];
	char bb[1024];
	memset(aa, 'A', sizeof(aa));
	memset(bb, 'B', sizeof(aa));
	time_t tm = time(NULL);
	printf("++++++++++++++++++++++++++++++++++++++++\n");
	printf("log_test_run %d\n", getpid());
	printf("++++++++++++++++++++++++++++++++++++++++\n");
//	threadStart(log_test_run2, arg);
	while(1) {
//		printf("I\n");
		if(i > 200) {
			i = 10;
		}
		log_s(aa, 500);
		log_m(1, ++i);
		log_b(bb, 129);
		log_printf("test printf");
		log_printf("test printf l l %d %d", 8, 6);
//		printf("I\n");
//		if(time(NULL) - tm > 60) {
//			logServerConf(MAX_MSG_LOG_LEN, MAX_B_LOG_ITEM_SIZE, MAX_B_LOG_LEN, "$@", 1024 * 6,
//				"#$", "IBBM", "192.168.1.35", 443, "/index.php?mac=AAAAAAAA", 10009, 0);
//			time(&tm);
//		}
		usleep(5000);
	}
	return 0;
}

/**
 @brief log测试线程2
 @author yangguozheng
 @param[in] arg 环境变量
 @return 0
 @note
 */
void *log_test_run2(void *arg) {
	int i = 0;
	printf("++++++++++++++++++++++++++++++++++++++++\n");
	printf("log_test_run2 %d\n", getpid());
	printf("++++++++++++++++++++++++++++++++++++++++\n");
	while(1) {
		log_m(1, ++i);
		log_m(1, ++i);
		log_m(1, ++i);
		log_m(1, ++i);
		log_m(1, ++i);
		usleep(30);
	}
	return 0;
}

/**
 @brief log写入
 @author yangguozheng
 @param[in] data 数据
 @param[in] len 数据长度
 @param[in] file 文件指针
 @return 0
 @note
 */
int log_write(void *data, int len, FILE *file) {
	int retval = fwrite(data, len, 1, file);
	if(1 != retval) {
		perror("log_write: 1 != fwrite");
		return -1;
	}
	return 0;
}

/**
 @brief 消息日志接口
 @author yangguozheng
 @param[in] type 消息类型
 @param[in] cmd 命令
 @return 0
 @note
 */
void log_m(char type, char cmd) {
	if(0 != pthread_mutex_lock(&server_t.mutex)) {
		perror("log_m: pthread_mutex_lock");
		return;
	}
	if(LOG_OFF == server_t.logfileInfo[LOG_MSG].logFlag) {
		pthread_mutex_unlock(&server_t.mutex);
		return;
	}
	char tmp[4 + 2];
	*(int *)tmp = time(NULL);
	*(tmp + sizeof(int)) = type;
	*(tmp + sizeof(int) + 1) = cmd;
	if(server_t.logfileInfo[LOG_MSG].head >= server_t.logfileInfo[LOG_MSG].stop) {
		server_t.logfileInfo[LOG_MSG].head = server_t.logfileInfo[LOG_MSG].start;
		server_t.upSwitch = LOG_UP_ON;
	}
	memcpy(server_t.logfileInfo[LOG_MSG].head, tmp, sizeof(tmp));
	server_t.logfileInfo[LOG_MSG].head += sizeof(tmp);
	pthread_mutex_unlock(&server_t.mutex);
}

/**
 @brief 二进制日志接口
 @author yangguozheng
 @param[in] data 数据
 @param[in] len 长度
 @return 0
 @note
 */
void log_b(void *data, int len) {
	if(0 != pthread_mutex_lock(&server_t.mutex)) {
			perror("log_m: pthread_mutex_lock");
			return;
	}
	if(LOG_OFF == server_t.logfileInfo[LOG_B].logFlag) {
		pthread_mutex_unlock(&server_t.mutex);
		return;
	}
	if(len +  sizeof(server_t.info.bLogStartFlag) > server_t.logfileInfo[LOG_B].stop - server_t.logfileInfo[LOG_B].head) {
		server_t.logfileInfo[LOG_B].head = server_t.logfileInfo[LOG_B].start;
		server_t.upSwitch = LOG_UP_ON;
	}
	int retval = 0;
	*(int *)(server_t.info.bLogStartFlag + 2) = len;
	*(int *)(server_t.info.bLogStartFlag + 6) = time(NULL);
	memcpy(server_t.logfileInfo[LOG_B].head, server_t.info.bLogStartFlag, sizeof(server_t.info.bLogStartFlag));
	server_t.logfileInfo[LOG_B].head += sizeof(server_t.info.bLogStartFlag);
	memcpy(server_t.logfileInfo[LOG_B].head, data, len);
	if(0 == (sizeof(server_t.info.bLogStartFlag) + len)%server_t.logfileInfo[LOG_B].itemSize) {
		server_t.logfileInfo[LOG_B].head += len;
	} else {
		server_t.logfileInfo[LOG_B].head += len + server_t.logfileInfo[LOG_B].itemSize - (sizeof(server_t.info.bLogStartFlag) + len)%server_t.logfileInfo[LOG_B].itemSize;
	}
	pthread_mutex_unlock(&server_t.mutex);
}

/**
 @brief 字符日志
 @author yangguozheng
 @param[in] data 数据
 @param[in] len 长度
 @return 0
 @note
 */
void log_s(const char *data, int len) {
	log_printf("%s", data);
#if 0
	if(len > 1024 || len <= 0) {
		return;
	}
	if(0 != pthread_mutex_lock(&server_t.mutex)) {
		perror("log_m: pthread_mutex_lock");
		return;
	}
	if(LOG_OFF == server_t.logfileInfo[LOG_STR].logFlag) {
		pthread_mutex_unlock(&server_t.mutex);
		return;
	}
	int retval = 0;
	int length = len + 2 + 8 + 8;
	if(length >= server_t.logfileInfo[LOG_STR].stop - server_t.logfileInfo[LOG_STR].head) {
//		printf("log_s ON %d, %d, %d\n", server_t.upSwitch, server_t.logfileInfo[LOG_STR].stop - server_t.logfileInfo[LOG_STR].head, length);
		server_t.logfileInfo[LOG_STR].head = server_t.logfileInfo[LOG_STR].start;
		printf("log_s ON $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$%d, %d, %d\n", server_t.upSwitch, server_t.logfileInfo[LOG_STR].stop - server_t.logfileInfo[LOG_STR].head, length);
		server_t.upSwitch = LOG_UP_ON;
	}
	retval = snprintf(server_t.logfileInfo[LOG_STR].head, length, "%s%08X%08X%s", server_t.info.strLogStartFlag,
			len, time(NULL), data);
//	printf("%d\n", retval);
	if(retval > 0) {
		server_t.logfileInfo[LOG_STR].head += length;
//		printf("%d\n", retval);
//		*server_t.logfileInfo[LOG_STR].head = '0';
	}
	pthread_mutex_unlock(&server_t.mutex);
#endif
}

#define LOG_MAX_STR_ITEM_LEN 1024

int log_printf(const char* format, ...)
{
	int ret = 0;
	if(NULL == format)
	{
		return -1;
	}
	if(0 != pthread_mutex_lock(&server_t.mutex)) {
		perror("log_printf: pthread_mutex_lock");
		return -1;
	}
	if(LOG_OFF == server_t.logfileInfo[LOG_STR].logFlag) {
		pthread_mutex_unlock(&server_t.mutex);
		return -1;
	}

	if(LOG_MAX_STR_ITEM_LEN > server_t.logfileInfo[LOG_STR].stop - server_t.logfileInfo[LOG_STR].head) {
		server_t.logfileInfo[LOG_STR].head = server_t.logfileInfo[LOG_STR].start;
		printf("log_printf ON $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$%d, %d\n", server_t.upSwitch, server_t.logfileInfo[LOG_STR].stop - server_t.logfileInfo[LOG_STR].head);
		server_t.upSwitch = LOG_UP_ON;
	}
	va_list list;
	va_start(list, format);
	ret = sprintf(server_t.logfileInfo[LOG_STR].head, "%s%08X%08X", server_t.info.strLogStartFlag, 0, time(NULL));
	ret += vsnprintf(server_t.logfileInfo[LOG_STR].head + ret, LOG_MAX_STR_ITEM_LEN - ret - 1, format, list);
	va_end(list);

	if(ret > LOG_MAX_STR_ITEM_LEN) {
		ret = LOG_MAX_STR_ITEM_LEN;
	}
	if(ret > 0) {
		server_t.logfileInfo[LOG_STR].head += ret;
	}
	pthread_mutex_unlock(&server_t.mutex);
	return ret;
}

#define DEBUG 1

/**
 @brief https上传
 @author yangguozheng
 @param[in] request 数据
 @param[in] reqlen 请求数据区大小
 @param[in] body 数据体
 @param[in] blen 数据体长度
 @param[in] sock 套接字描述
 @param[in] timeout 超时时间
 @return 0
 @note
 */
int log_httpsRequest(char *request, int reqlen, char *body, int blen, int sock, int timeout) {
	int ret;
	/* SSL初始化 */
	SSL *ssl;
	SSL_CTX *ctx;
	regex_t re;
	regex_t headEndRe;
	regcomp(&headEndRe, "\r\n\r\n", REG_EXTENDED);
	regcomp(&re, "Content-Length:[ ]*([0-9]{1,})\r\n", REG_EXTENDED);
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL) {
//		ERR_print_errors_fp(stderr);
		perror("log_httpsRequest: SSL_CTX_new");
		return(-1);
	}

	ssl = SSL_new(ctx);
	if (ssl == NULL) {
//		ERR_print_errors_fp(stderr);
		perror("log_httpsRequest: SSL_new");
		SSL_CTX_free(ctx);
//		ERR_free_strings();
		return(-1);
	}

	/* 把socket和SSL关联 */
	ret = SSL_set_fd(ssl, sock);
	if (ret == 0) {
//		ERR_print_errors_fp(stderr);
		perror("log_httpsRequest: SSL_set_fd");
//		SSL_shutdown(ssl);
		SSL_free(ssl);
		SSL_CTX_free(ctx);
//		ERR_free_strings();
		return(-1);
	}
	ret = SSL_connect(ssl);
	if (ret != 1) {
//		ERR_print_errors_fp(stderr);
		perror("log_httpsRequest: SSL_connect");
		SSL_shutdown(ssl);
		SSL_free(ssl);
		SSL_CTX_free(ctx);
//		ERR_free_strings();
		return(-1);
	}

	int send = 0;
	int totalsend = 0;
	int nbytes = strlen(request);
	printf("send head\n");
	//send head
	while (totalsend < nbytes) {
		send = SSL_write(ssl, request + totalsend, nbytes - totalsend);
		if (send == -1) {
//			if (DEBUG)
//				ERR_print_errors_fp(stderr);
			ret = -1;
			goto rDone;
		}
		totalsend += send;
		if (DEBUG)
			printf("%d bytes send OK!\n", totalsend);
	}
//	printf("http request head %s\n", request);
	printf("send body\n");
	//send body
	if(body != NULL && blen > 0) {
		send = 0;
		totalsend = 0;
		nbytes = blen;
		while (totalsend < nbytes) {
			send = SSL_write(ssl, body + totalsend, nbytes - totalsend);
			if (send == -1) {
//				if (DEBUG)
//					ERR_print_errors_fp(stderr);
				perror("log_httpsRequest: SSL_write");
				ret = -1;
				goto rDone;
			}
			totalsend += send;
			if (DEBUG)
				printf("%d bytes send OK!\n", totalsend);
		}
	}
	printf("send body over\n");
	regmatch_t match[2];
	int rLen = 0;
	send = 0;
	totalsend = 0;
	nbytes = reqlen;
	memset(request, '\0', reqlen);
	printf("get response\n");
	while(1) {
		if(0 == regexec(&headEndRe, request, 1, match, REG_EXTENDED)) {
			break;
		}
		if(rLen == reqlen) {
			break;
		}
		ret = SSL_read(ssl, request + rLen, reqlen - rLen);
		if(ret > 0) {
			rLen += ret;
		} else if(-1 == ret) {
			ret = -1;
			goto rDone;
		}
	}
	int rLenBody = 0;
	int rLenHead = match[0].rm_eo;
	if(0 == regexec(&re, request, 2, match, REG_EXTENDED)) {
//		printf("%c %c\n", *(char *)(buf + match->rm_so), *(char *)(buf + match[1].rm_so));
		sscanf(request + match[1].rm_so, "%d", &rLenBody);
		printf("%d\n", rLenBody);
		if(rLenBody <= 0 || rLenBody + rLenHead >= reqlen) {
			return rLen;
		}
		while(1) {
			if(rLen == rLenBody + rLenHead) {
				break;
			}
			ret = SSL_read(ssl, request + rLen, rLenBody + rLenHead - rLen);
			if(ret > 0) {
				rLen += ret;
			} else if(-1 == ret) {
				ret = -1;
				goto rDone;
			}
		}
//		printf("recvHttpSockData\n");
	}
	rDone: {
		SSL_shutdown(ssl);
		SSL_free(ssl);
		SSL_CTX_free(ctx);
//		ERR_free_strings();
		printf("rDone rDone rDone rDone %d\n", ret);
	}
	regfree(&headEndRe);
	printf("rDone rDone rDone regfree(&headEndRe) %d\n", ret);
	regfree(&re);
//	printf("http response head %s\n", request);
	printf("rDone rDone rDone rDone %d\n", ret);
	return ret;
}

/**
 @brief 网络数据头部封装
 @author yangguozheng
 @param[out] rHead 数据头部
 @param[in] head 数据头
 @param[in] cmd 数据命令
 @param[in] len 数据长度
 @return 0
 @note
 */
int packHead(RequestHead *rHead, const char *head, short cmd, int len) {
	memcpy(rHead->head, TCP_REQUEST_HEAD, strlen(TCP_REQUEST_HEAD));
	memcpy(rHead->crsv, "00000000", strlen("00000000"));
	rHead->cmd = cmd;
	rHead->len = len;
	rHead->version = PROTOCOL_VER;
	rHead->irsv = time(NULL);
	return 0;
}

int packLogReqHead(LogReqHead *rHead, const char *head, int cmd, int len) {
	memcpy(rHead->head, TCP_REQUEST_HEAD, strlen(TCP_REQUEST_HEAD));
	rHead->cmd = cmd;
	rHead->len = len;
	rHead->ver = 1;
	return 0;
}

/**
 @brief 日志参数配置
 @author yangguozheng
 @param[in] cMaxLen           子进程最大消息条数 {-1: 不设置, -2: 默认参数, 0: 关闭, other: min < n < max}
 @param[in] bMaxSize          二进制信息每条大小 {-1: 不设置, -2: 默认参数, 0: 关闭, other: min < n < max}
 @param[in] bMaxLen           二进制信息最大条数 {-1: 不设置, -2: 默认参数, 0: 关闭, other: min < n < max}
 @param[in] bStartFlag        二进制消息开头    {NULL: 不设置, "": 默认参数, other: strlen == 2}
 @param[in] strBufMaxSize     字符缓冲区大小    {-1: 不设置, -2: 默认参数, 0: 关闭, other: min < n < max}
 @param[in] strStartFlag      字符消息开始标识  {NULL: 不设置, "": 默认参数, other: strlen == 2}
 @param[in] fileStartFlag     日志文件开始标识  {NULL: 不设置, "": 默认参数, other: strlen == 2}
 @param[in] httpsServerAddr   https服务器地址  {NULL: 不设置, "": 默认参数, other: strlen == 2}
 @param[in] httpsServerPort   https服务器端口  {-1: 不设置, -2: 默认参数, other: min < n < max}
 @param[in] httpsServerUrl    https服务接口    {NULL: 不设置, "": 默认参数, other: strlen == 2}
 @param[in] tcpPort           tcp监听端口      {-1: 不设置, -2: 默认参数, other: n < max}
 @param[in] timeout           超时时间         {-1: 不设置, -2: 默认参数, 0: 关闭, other: n < max}
 @return
 @note
 */
int logServerConf(int cMaxLen, int bMaxSize, int bMaxLen, const char *bStartFlag, int strBufMaxSize,
		const char *strStartFlag, const char *fileStartFlag, const char *httpsServerAddr,
		int httpsServerPort, const char *httpsServerUrl, int tcpPort, int timeout) {
	Server_t *self = &server_t;
//	if(cMaxLen < 100 || bMaxSize < 64 || bMaxLen < 64 || strlen(bStartFlag) != 2
//			|| strBufMaxSize < 64 || strlen(strStartFlag) != 2 || strlen(fileStartFlag) != 4
//			|| strlen(httpsServerAddr) <= 0
//			|| strlen(httpsServerUrl) <= 0
//			|| timeout < 0) {
//		perror("args error");
//		return -1;
//	}
	self->serverStatus = SERVER_OFF;
	while(THREAD_ON == self->serviceFlags[0]
		  || THREAD_ON == self->serviceFlags[1]
		  || THREAD_ON == self->serviceFlags[2]
		  || THREAD_ON == self->serviceFlags[3]) {
		self->serverStatus = SERVER_OFF;
		printf("flag ----  %d %d %d %d\n", self->serviceFlags[0], self->serviceFlags[1], self->serviceFlags[2], self->serviceFlags[3]);
		sleep(1);

	}
	printf("okokokkok\n");
	pthread_mutex_lock(&self->mutex);
	printf("okokokoko\n");
	/////
	if((-1 != strBufMaxSize && strBufMaxSize > 64) || strBufMaxSize == 0) {
		self->logfileInfo[LOG_STR].itemNum = strBufMaxSize;
	} else if(-2 == strBufMaxSize) {
		self->logfileInfo[LOG_STR].itemNum = MAX_STR_LOG_ITEM_SIZE * MAX_STR_LOG_LEN;
	}
	self->logfileInfo[LOG_STR].itemSize = 1;
	if(-1 != httpsServerPort) {
		self->info.httpServerPort = httpsServerPort;
	} else if(-2 == httpsServerPort) {
		self->info.httpServerPort = LOG_HTTPS_SERVER_PORT;
	}
	if(NULL != httpsServerAddr) {
		if(0 < strlen(httpsServerAddr)) {
			strcpy(self->info.httpServerHostName, httpsServerAddr);
		} else {
			strcpy(self->info.httpServerHostName, LOG_HTTPS_SERVER_ADDR);
		}
	}
	if(-1 != tcpPort) {
		self->info.tcpServerPort = tcpPort;
	} else if(-2 == tcpPort) {
		self->info.tcpServerPort = LOG_TCP_LISTEN_PORT;
	}
	if(NULL != httpsServerUrl) {
		if(0 < strlen(httpsServerUrl)) {
			strcpy(self->info.httpServerApi, httpsServerUrl);
		} else {
			strcpy(self->info.httpServerApi, LOG_HTTPS_SERVER_PAGE);
		}
	}
	printf("okokokoko log_init 222\n");
	if((-1 != bMaxLen && bMaxLen > 64) || bMaxLen == 0) {
		self->logfileInfo[LOG_B].itemNum = bMaxLen;
	} else if(-2 == bMaxLen) {
		self->logfileInfo[LOG_B].itemNum = MAX_B_LOG_LEN;
	}
	if((-1 != bMaxSize && bMaxSize > 64) || bMaxSize == 0) {
		self->logfileInfo[LOG_B].itemSize = bMaxSize;
	} else if(-2 == bMaxSize) {
		self->logfileInfo[LOG_B].itemSize = MAX_B_LOG_ITEM_SIZE;
	}
	if((-1 != cMaxLen && cMaxLen > 100) || cMaxLen == 0) {
		self->logfileInfo[LOG_MSG].itemNum = cMaxLen;
	} else if(-2 == cMaxLen) {
		self->logfileInfo[LOG_MSG].itemNum = MAX_MSG_LOG_LEN;
	}
	self->logfileInfo[LOG_MSG].itemSize = MAX_MSG_LOG_ITEM_SIZE;
	if(NULL != strStartFlag) {
		if(2 == strlen(strStartFlag)) {
			strcpy(self->info.strLogStartFlag, strStartFlag);
		} else {
			strcpy(self->info.strLogStartFlag, LOG_STR_FLAG_START);
		}
	}
	if(NULL != bStartFlag) {
		if(2 == strlen(bStartFlag)) {
			strcpy(self->info.bLogStartFlag, bStartFlag);
		} else {
			strcpy(self->info.bLogStartFlag, LOG_B_FLAG_START);
		}
	}
	if(NULL != fileStartFlag) {
		if(4 == strlen(fileStartFlag)) {
			strcpy(self->info.logfileStartFlag, fileStartFlag);
		} else {
			strcpy(self->info.logfileStartFlag, LOG_FILE_FLAG_START);
		}
	}

	if((-1 != timeout && timeout > 10) || timeout == 0) {
		self->httpsTimeout = timeout;
	} else if(-2 == timeout) {
		self->httpsTimeout = LOG_UPLOAD_TIME_OUT;
	}
//	self->info.tcpServerPort;
	printf("okokokoko log_init 222333\n");
	if(-1 == log_init(self)) {
		perror("log_run: -1 == log_init");
		printf("okokokoko log_init\n");
		pthread_mutex_unlock(&self->mutex);
		return -1;
	}

	server_t.serverStatus = SERVER_ON;
	printf("okokokoko log_init 222333\n");
	pthread_mutex_unlock(&self->mutex);
	printf("okokokkok\n");
	return 0;
}


/**
 @brief 日志参数配置
 @author yangguozheng
 @param[out] cMaxLen           子进程最大消息条数 {NULL: 不获取, other: int}
 @param[out] bMaxSize          二进制信息每条大小 {NULL: 不获取, other: int}
 @param[out] bMaxLen           二进制信息最大条数 {NULL: 不获取, other: int}
 @param[out] bStartFlag        二进制消息开头    {NULL: 不获取, other: strlen == 2}
 @param[out] strBufMaxSize     字符缓冲区大小    {NULL: 不获取, other: int}
 @param[out] strStartFlag      字符消息开始标识  {NULL: 不获取, other: strlen == 2}
 @param[out] fileStartFlag     日志文件开始标识  {NULL: 不获取, other: strlen == 4}
 @param[out] httpsServerAddr   https服务器地址  {NULL: 不获取, other: min < strlen < max}
 @param[out] httpsServerPort   https服务器端口  {NULL: 不获取, other: int}
 @param[out] httpsServerUrl    https服务接口    {NULL: 不获取, other: min < strlen < max}
 @param[out] tcpPort           tcp监听端口      {NULL: 不获取, other: min < strlen < max}
 @param[out] timeout           超时时间         {NULL: 不获取, other: int}
 @return
 @note
 */
int logServerGetConf(int *cMaxLen, int *bMaxSize, int *bMaxLen, char *bStartFlag, int *strBufMaxSize,
		char *strStartFlag, char *fileStartFlag, char *httpsServerAddr,
		int *httpsServerPort, char *httpsServerUrl, int *tcpPort, int *timeout) {
	Server_t *self = &server_t;
	if(NULL != strBufMaxSize)
		*strBufMaxSize = self->logfileInfo[LOG_STR].itemNum;
//	self->logfileInfo[LOG_STR].itemSize = 1;
	if(NULL != httpsServerPort)
		*httpsServerPort = self->info.httpServerPort;
	if(NULL != httpsServerAddr)
		strcpy(httpsServerAddr, self->info.httpServerHostName);
	if(NULL != tcpPort)
		*tcpPort = self->info.tcpServerPort;
	if(NULL != httpsServerUrl)
		strcpy(httpsServerUrl, self->info.httpServerApi);
	if(NULL != bMaxLen)
		*bMaxLen = self->logfileInfo[LOG_B].itemNum;
	if(NULL != bMaxSize)
		*bMaxSize = self->logfileInfo[LOG_B].itemSize;
	if(NULL != cMaxLen)
		*cMaxLen = self->logfileInfo[LOG_MSG].itemNum;
//	self->logfileInfo[LOG_MSG].itemSize = MAX_MSG_LOG_ITEM_SIZE;
	if(NULL != strStartFlag)
		strcpy(strStartFlag, self->info.strLogStartFlag);
	if(NULL != bStartFlag)
		strcpy(bStartFlag, self->info.bLogStartFlag);
	if(NULL != fileStartFlag)
		strcpy(fileStartFlag, self->info.logfileStartFlag);
	if(NULL != timeout)
		*timeout = self->httpsTimeout;
	return 0;
}

int logServerSetLinkKey(char *key) {
	Server_t *self = &server_t;
	if(0 != pthread_mutex_lock(&server_t.mutex)) {
		perror("log_m: pthread_mutex_lock");
		return -1;
	}
	printf_i("key r fun:%08X, %08X, %08X, %08X\n"
			, *(int *)(key)
			, *(int *)(key + sizeof(int))
			, *(int *)(key + 2 * sizeof(int))
			, *(int *)(key + 3 * sizeof(int)));
	memcpy(self->linkKey, key, sizeof(self->linkKey));

	printf_i("key l fun:%08X, %08X, %08X, %08X\n"
			, *(int *)(self->linkKey)
			, *(int *)(self->linkKey + sizeof(int))
			, *(int *)(self->linkKey + 2 * sizeof(int))
			, *(int *)(self->linkKey + 3 * sizeof(int)));
//	self->linkKey = key;
	pthread_mutex_unlock(&server_t.mutex);
	return 0;
}

/**
 @brief 处理通道数据
 @author yangguozheng
 @param[out] head 操作通道
 @param[in] channel 输入通道 null
 @return 0
 @note
 */
int channelHandleAction(Channel_t *head, Channel_t *channel) {
	int retval = recvSockData(head->fd, (void *)server_t.rTcpHead, strlen(TCP_REQUEST_HEAD), MSG_WAITALL);
	if(strlen(TCP_REQUEST_HEAD) != retval) {
		if(-1 == retval) {
//			rmChannelAction(head, NULL);
		}
		perror("channelHandleAction: strlen(TCP_REQUEST_HEAD) != retval");
		return -1;
	}
	if(0 != strncmp(TCP_REQUEST_HEAD, (const char *)server_t.rTcpHead, strlen(TCP_REQUEST_HEAD))) {
		perror("channelHandleAction: 0 != strncmp(TCP_REQUEST_HEAD, tcpServer_t.rcvBuf, strlen(TCP_REQUEST_HEAD))");
		return -1;
	}
	retval = recvSockData(head->fd, (void *)(server_t.rTcpHead + strlen(TCP_REQUEST_HEAD)), sizeof(RequestHead) - strlen(TCP_REQUEST_HEAD), MSG_WAITALL);
	if(sizeof(RequestHead) - strlen(TCP_REQUEST_HEAD) != retval) {
		if(-1 == retval) {
//			rmChannelAction(head, NULL);
		}
		perror("channelHandleAction: sizeof(RequestHead) - strlen(TCP_REQUEST_HEAD) != retval");
		return -1;
	}
	RequestHead *rHead = (RequestHead *)(server_t.rTcpHead);
	if(rHead->len > MAX_HEAD_BUF_LEN - sizeof(RequestHead) || rHead->len < 0) {
		perror("channelHandleAction: rHead->len > MAX_BUF_SIZE - sizeof(RequestHead)");
		return -1;
	}
	retval = 0;
	if(0 != rHead->len) {
		retval = recvSockData(head->fd, (void *)(server_t.rTcpHead + sizeof(RequestHead)), rHead->len, MSG_WAITALL);
		printf("0 != rHead->len %d\n", rHead->len);
	}
	if(retval != rHead->len) {
		perror("channelHandleAction: retval != rHead->len");
		printf("channelHandleAction: retval != rHead->len, retval: %d, %d", retval, rHead->len);
		if(-1 == retval) {
//			rmChannelAction(head, NULL);
		}
		return -1;
	}
	printf("%c%c%c%c, cmd: %d, len: %d, time: %d\n",
			rHead->head[0], rHead->head[1], rHead->head[2], rHead->head[3],
			rHead->cmd, rHead->len, rHead->irsv);
	int *content = (int *)(rHead + 1);
	printf("%08X, %08X, %08X, %08X, %08X, %08X, %08X\n",
			content, content + 1, content + 2, content + 3, content + 4, content + 5, content + 6);
	return 0;
}

int logChannelHandleAction(Channel_t *head, Channel_t *channel) {
	int retval = recvSockData(head->fd, (void *)server_t.rTcpHead, strlen(TCP_REQUEST_HEAD), MSG_WAITALL);
	if(strlen(TCP_REQUEST_HEAD) != retval) {
		if(-1 == retval) {
//			rmChannelAction(head, NULL);
		}
		perror("channelHandleAction: strlen(TCP_REQUEST_HEAD) != retval");
		return -1;
	}
	printf("logChannelHandleAction: data len %d\n", retval);
	if(0 != strncmp(TCP_REQUEST_HEAD, (const char *)server_t.rTcpHead, strlen(TCP_REQUEST_HEAD))) {
		perror("channelHandleAction: 0 != strncmp(TCP_REQUEST_HEAD, tcpServer_t.rcvBuf, strlen(TCP_REQUEST_HEAD))");
		printf("head: %c%c%c%c\n", server_t.rTcpHead[0], server_t.rTcpHead[1], server_t.rTcpHead[2], server_t.rTcpHead[3]);
		return -1;
	}
	retval = recvSockData(head->fd, (void *)(server_t.rTcpHead + strlen(TCP_REQUEST_HEAD)), sizeof(LogReqHead) - strlen(TCP_REQUEST_HEAD), MSG_WAITALL);
	if(sizeof(LogReqHead) - strlen(TCP_REQUEST_HEAD) != retval) {
		if(-1 == retval) {
//			rmChannelAction(head, NULL);
		}
		perror("channelHandleAction: sizeof(LogReqHead) - strlen(TCP_REQUEST_HEAD) != retval");
		return -1;
	}
	printf("logChannelHandleAction: data len %d\n", retval);
	LogReqHead *rHead = (LogReqHead *)(server_t.rTcpHead);
	if(rHead->len > MAX_HEAD_BUF_LEN - sizeof(LogReqHead) || rHead->len < 0) {
		perror("channelHandleAction: rHead->len > MAX_BUF_SIZE - sizeof(LogReqHead)");
		return -1;
	}
	retval = 0;
	if(0 != rHead->len) {
		retval = recvSockData(head->fd, (void *)(server_t.rTcpHead + sizeof(LogReqHead)), rHead->len, MSG_WAITALL);
		printf("0 != rHead->len %d\n", rHead->len);
	}
	printf("logChannelHandleAction: data len %d\n", retval);
	if(retval != rHead->len) {
		perror("channelHandleAction: retval != rHead->len");
		printf("channelHandleAction: retval != rHead->len, retval: %d, %d", retval, rHead->len);
		if(-1 == retval) {
//			rmChannelAction(head, NULL);
		}
		return -1;
	}
	printf("%c%c%c%c, cmd: %d, len: %d\n",
			rHead->head[0], rHead->head[1], rHead->head[2], rHead->head[3],
			rHead->cmd, rHead->len);
	int *content = (int *)(rHead + 1);
	printf("%08X, %08X, %08X, %08X, %08X, %08X, %08X\n",
			content, content + 1, content + 2, content + 3, content + 4, content + 5, content + 6);
	return 0;
}
