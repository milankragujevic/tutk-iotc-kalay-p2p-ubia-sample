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
#include <time.h>
#include "Udpserver/udpServer.h"
#include "common/msg_queue.h"
#include "common/thread.h"
#include "common/utils.h"
#include "common/common.h"
#include "common/logfile.h"
#include "common/common_config.h"
#include "NewsChannel/NewsChannel.h"

#define UDP_SERVER_BUF_LEN 1024
#define UDP_SERVER_MSG_LEN 128

struct udpServerAttr_t {
	int upnp_rev_process_id;
	int upnp_snd_process_id;
	//int threadMsgId;
	//int processMsgId;
	char msgDataRcv[UDP_SERVER_MSG_LEN];
	char msgDataSnd[UDP_SERVER_MSG_LEN];
	int sockfd;
	struct sockaddr_in serverAddr;           //server address
	struct sockaddr_in clientAddr;           //server address
	struct sockaddr_in broadcastAddr;
	char request[UDP_SERVER_BUF_LEN];    //request buffer
	char response[UDP_SERVER_BUF_LEN];   //response buffer
};

int udpServer_init(struct udpServerAttr_t *self);
int udpServer_exit(struct udpServerAttr_t *self);
int udpServer_threadStart();
void *udpServer_threadRun(void *arg);
int udpServer_msgHandler(struct udpServerAttr_t *self);
int udpServer_UDPHandler(struct udpServerAttr_t *self);

int UdpServerSndMsg(int sockfd, struct sockaddr_in *clientAddr, int cmd, char *data, int len);
int udpServer_sndMsg(int sockfd, struct sockaddr_in *clientAddr, char *data, int len);
int udpServer_recvMsg(int sockfd, struct sockaddr_in *clientAddr, char *data, int len);
int udpServer_clientSockInit(int *sockfd);
int udpServer_serverSockInit(int *sockfd, struct sockaddr_in *serverAddr);
int udpServer_clientAddrInit(struct sockaddr_in *clientAddr, const char *addr, int port);
int udpServer_serverAddrInit(struct sockaddr_in *serverAddr, int port);
int udpServer_sockAddrInit(struct sockaddr_in *clientAddr, const char *addr, int port);
void udpServer_getdata(struct udpServerAttr_t *self);
/**
 * @brief udp初始化函数
 * @author <王志强>
 * ＠param[in]<self><适配器运行环境>

 * @return <-1: 失败, 0: 成功>
 */
int udpServer_init(struct udpServerAttr_t *self) {
	self->upnp_rev_process_id = msg_queue_get((key_t) CHILD_THREAD_MSG_KEY);
	if(self->upnp_rev_process_id == -1) {
		return -1;
	}
	self->upnp_snd_process_id = msg_queue_get((key_t) CHILD_PROCESS_MSG_KEY);
	if(self->upnp_snd_process_id == -1) {
		return -1;
	}

	udpServer_sockAddrInit(&self->broadcastAddr, "255.255.255.255", 10001);
	udpServer_sockAddrInit(&self->serverAddr, "0.0.0.0", BROADCAST_PORT);
	if(udpServer_serverSockInit(&self->sockfd, &self->serverAddr) == -1) {
		return -1;
	}
	return 0;
}

/**
 * @brief udp退出函数
 * @author <王志强>
 * ＠param[in]<self><适配器运行环境>

 * @return <-1: 失败, 0: 成功>
 */
int udpServer_exit(struct udpServerAttr_t *self) {
	free(self);
	return 0;
}

/**
 * @brief udp入口函数
 * @author <王志强>

 * @return <-1: 失败, 0: 成功>
 */
int udpServer_threadStart() {
#if 0
	udpServer_threadRun(NULL);
	return 0;
#else
	return thread_start(udpServer_threadRun);
#endif
}

/**
 * @brief udp主函数，处理udp网络消息和于子进程的消息
 * @author <王志强>
 * ＠param[in]<arg><未用>

 */
void *udpServer_threadRun(void *arg) {
	struct udpServerAttr_t *udpServerAttr = (struct udpServerAttr_t *)malloc(sizeof(struct udpServerAttr_t));

	udpServer_init(udpServerAttr);
	thread_synchronization(&rUdpserver, &rChildProcess);

	DEBUGPRINT(DEBUG_INFO, "Udp Server start success\n");
	while(1) {
		udpServer_UDPHandler(udpServerAttr);
		udpServer_msgHandler(udpServerAttr);
		//UdpServerSndMsg(udpServerAttr->sockfd, &udpServerAttr->clientAddr, UDP_DISCOVER_RES, NULL, 0);

		usleep(100000);
	}
	udpServer_exit(udpServerAttr);
	return NULL;
}

/**
 * @brief udp服务器变量初始化函数
 * @author <王志强>
 * ＠param[in|out]<serverAddr><服务属性>
 * ＠param[in]<port><绑定的端口>

 * @return <0: 成功>

 */
int udpServer_serverAddrInit(struct sockaddr_in *serverAddr, int port) {
	bzero(serverAddr,sizeof(struct sockaddr_in));
	serverAddr->sin_family = AF_INET;
	serverAddr->sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr->sin_port = htons(port);
	return 0;
}

/**
 * @brief udp服务器变量初始化函数
 * @author <王志强>
 * ＠param[in|out]<clientAddr><服务属性>
 * ＠param[in|out]<addr><客户端地址>
 * ＠param[in]<port><发送的端口>

 * @return <0: 成功>
 */
int udpServer_clientAddrInit(struct sockaddr_in *clientAddr, const char *addr, int port) {
	bzero(clientAddr,sizeof(struct sockaddr_in));
	clientAddr->sin_family = AF_INET;
	clientAddr->sin_addr.s_addr = inet_addr(addr);
	clientAddr->sin_port = htons(port);
	return 0;
}

/**
 * @brief udp服务器变量初始化函数
 * @author <王志强>
 * ＠param[in|out]<clientAddr><服务属性>
 * ＠param[in|out]<addr><客户端地址>
 * ＠param[in]<port><发送的端口>

 * @return <0: 成功>
 */
int udpServer_sockAddrInit(struct sockaddr_in *clientAddr, const char *addr, int port) {
	bzero(clientAddr,sizeof(struct sockaddr_in));
	clientAddr->sin_family = AF_INET;
	clientAddr->sin_addr.s_addr = inet_addr(addr);
	clientAddr->sin_port = htons(port);
	return 0;
}

/**
 * @brief udp客户端初始化函数
 * @author <王志强>
 * ＠param[in]<sockfd><描述符>

 * @return <-1: 失败, 0: 成功>
 */
int udpServer_clientSockInit(int *sockfd) {
	*sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(*sockfd == -1) {
		return -1;
	}
	int sockOpt = 1; /*turn on*/
	if(setsockopt(*sockfd, SOL_SOCKET, SO_BROADCAST, &sockOpt, sizeof(int)) != 0) {
		return -1;
	}

	sockOpt = 1;

	if(setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &sockOpt, sizeof(int)) != 0) {
		return -1;
	}
	return 0;
}

/**
 * @brief udp服务器初始化函数
 * @author <王志强>
 * ＠param[in|out]<sockfd><描述符>
 * ＠param[in|out]<serverAddr><服务属性>

 * @return <-1: 失败, 0: 成功>
 */
int udpServer_serverSockInit(int *sockfd, struct sockaddr_in *serverAddr) {
	*sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(*sockfd == -1) {
		return -1;
	}

	int sockOpt = 1; /*turn on*/
	if(setsockopt(*sockfd, SOL_SOCKET, SO_BROADCAST, &sockOpt, sizeof(int)) != 0) {
		return -1;
	}

	sockOpt = 1;
	if(setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &sockOpt, sizeof(int)) != 0) {
		return -1;
	}

	if(bind(*sockfd ,(struct sockaddr *)serverAddr, sizeof(struct sockaddr)) == -1) {
		return -1;
	}

	return 0;
}

/**
 * @brief udp发送函数
 * @author <王志强>
 * ＠param[in]<sockfd><描述符>
 * ＠param[in]<clientAddr><服务属性>
 * ＠param[in]<data><发送的数据>
 * ＠param[in]<len><发送的数据的长度>
 * @return <-1: 失败, 0: 成功>
 */
int udpServer_sndMsg(int sockfd, struct sockaddr_in *clientAddr, char *data, int len)
{
	if(sendto(sockfd, data, len, 0, (struct sockaddr *)clientAddr, sizeof(struct sockaddr)) == -1) {
		return -1;
	}
	return 0;
}

/**
 * @brief 拼接udp数据，再发送到客户端
 * @author <王志强>
 * ＠param[in]<sockfd><描述符>
 * ＠param[in]<clientAddr><服务属性>
 * ＠param[in]<cmd><发送的命令>
 * ＠param[in]<data><发送的数据>
 * ＠param[in]<len><发送的数据的长度>
 * @return <-1: 失败, 0: 成功>
 */
int UdpServerSndMsg(int sockfd, struct sockaddr_in *clientAddr, int cmd, char *data, int len)
{
	RequestHead udpmsg;
	char buf[200];
	//char *p;
	//int tmp_len;

	if(sockfd < 0 || clientAddr == NULL){
		return -2;
	}

	memcpy(udpmsg.head, ICAMERA_NETWORK_HEAD, sizeof(ICAMERA_NETWORK_HEAD));
	udpmsg.version = ICAMERA_NETWORK_VERSION;
	udpmsg.cmd = cmd;
	udpmsg.len = len;

	if (len > 0 && data != NULL) {
		memcpy(&buf[sizeof(udpmsg)], data, len);
	}
	len += sizeof(udpmsg);
#if _UDP_VERSION == 2
	/*if (fp != NULL ) {
		while (fgets(tmp, 100, fp)) {
			p = NULL;
			if ((p = strstr(tmp, "firmware")) != NULL ) {
				tmp_len = strlen("firmware");
			} else if ((p = strstr(tmp, "camera_type")) != NULL ) {
				tmp_len = strlen("camera_type");
			} else if ((p = strstr(tmp, "hardware")) != NULL ) {
				tmp_len = strlen("hardware");
			} else if ((p = strstr(tmp, "camera_model")) != NULL ) {
				tmp_len = strlen("camera_model");
			}

			if (p != NULL ) {
				p += tmp_len;
				p += 1;
				memcpy(&buf[len], p, strlen(p));
				len += strlen(p);
			}
		}
	}*/

	memcpy(&buf[len], Cam_firmware, strlen(Cam_firmware));
	len += strlen(Cam_firmware);
	memcpy(&buf[len], Cam_camera_type, strlen(Cam_camera_type));
	len += strlen(Cam_camera_type);
	memcpy(&buf[len], Cam_hardware, strlen(Cam_hardware));
	len += strlen(Cam_hardware);
	memcpy(&buf[len], Cam_camera_model, strlen(Cam_camera_model));
	len += strlen(Cam_camera_model);



#endif
	time_t t;

	t = time(NULL);
	memcpy(&buf[len], &t, sizeof(int));
	len += 4;

	udpmsg.len = len-sizeof(RequestHead);
	memcpy(buf, &udpmsg, sizeof(udpmsg));
	if(sendto(sockfd, buf, len, 0, (struct sockaddr *)clientAddr, sizeof(struct sockaddr)) == -1) {
			DEBUGPRINT(DEBUG_INFO,"UDP sendto failed...%s\n", errstr);
			//fclose(fp);
			return -1;
	}
	//fclose(fp);
	return 0;
}

/**
 * @brief 接收udp数据函数
 * @author <王志强>
 * ＠param[in]<sockfd><描述符>
 * ＠param[in]<clientAddr><服务属性>
 * ＠param[in]<data><接收数据的缓冲区>
 * ＠param[in]<len><接收数据缓冲区的长度>
 * @return <-1: 失败, 0: 成功>
 */
int udpServer_recvMsg(int sockfd, struct sockaddr_in *clientAddr, char *data, int len) 
{
	fd_set fds;
	struct timeval timeout;

	FD_ZERO(&fds);
	FD_SET(sockfd, &fds);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	socklen_t ipLen =  sizeof(struct sockaddr);
	switch(select(sockfd+1,&fds,NULL,NULL,&timeout)) {
		case -1:return -1;
		case 0:return -1;
		default:
			if(FD_ISSET(sockfd, &fds)) {
				if(recvfrom(sockfd, data, len, 0, (struct sockaddr *)clientAddr, &ipLen) == -1) {
					return -1;
				}
				else {

				}
			}
		break;
	}
	return 0;
}

/**
 * @brief 接收消息数据函数
 * @author <王志强>
 * ＠param[in]<self><适配器运行环境>
 * @return <-1: 失败, 0: 成功>
 */
int udpServer_msgHandler(struct udpServerAttr_t *self)
{
	int ret = 0;
	//RequestHead *udpmsg;
	struct MsgData *msg = (struct MsgData *)self->msgDataRcv;
	int i;

	if(msg_queue_rcv(self->upnp_rev_process_id, self->msgDataRcv, sizeof(self->msgDataRcv), UDP_SERVER_MSG_TYPE) == -1) {
		ret = -1;
	}
	else {
		switch(msg->cmd) {
//			case MSG_UDPSERVER_T_RESPONSE:
//				for(i = 0;i< 5; i++){
//					if(UdpServerSndMsg(self->sockfd, &self->clientAddr, UDP_DISCOVER_RES, &self->msgDataRcv[16], msg->len) == -1) {
//						DEBUGPRINT(DEBUG_ERROR, "<Q>--[error: %s]--[errno: %d]--<Q>\n",strerror(errno),errno);
//					}
//				}
//				for(i = 0; i < 2; i++){
//					if (UdpServerSndMsg(self->sockfd, &self->broadcastAddr, UDP_DISCOVER_RES, &self->msgDataRcv[16], msg->len) == -1) {
//						DEBUGPRINT(DEBUG_ERROR, "<Q>--[error: %s]--[errno: %d]--<Q>\n",strerror(errno),errno);
//					}
//				}
////				DEBUGPRINT(DEBUG_INFO, "hello\n");
//				break;
			case CMD_STOP_ALL_USE_FRAME: 
				g_lUdpserver = 1;
				break;

			default:
				break;
		}
	}
	return ret;
}

/**
 * @brief 处理接收的udp消息
 * @author <王志强>
 * ＠param[in]<self><适配器运行环境>
 * @return <-1: 失败, 0: 成功>
 */
int udpServer_UDPHandler(struct udpServerAttr_t *self)
{
	RequestHead *udpmsg = (RequestHead *)self->request;
	MsgData *msg = (MsgData *)self->msgDataSnd;
	struct sockaddr_in clientAddr;
	int i;

	memset(self->request, '\0', UDP_SERVER_BUF_LEN);
	int res = udpServer_recvMsg(self->sockfd, &self->clientAddr, self->request, UDP_SERVER_BUF_LEN);
	if(res == -1) {
		//DEBUGPRINT(DEBUG_ERROR, "<Q>--[error: %s]--[errno: %d]--<Q>\n",strerror(errno),errno);
	}else if(res == 0 && udpmsg->cmd == UDP_DISCOVER_REQ) {
		DEBUGPRINT(DEBUG_INFO, "------[UDP request: %s]-----\n", self->request);

//		char str_ip[20];
//		inet_ntop(AF_INET, &self->clientAddr.sin_addr.s_addr, str_ip, 16);
//		DEBUGPRINT(DEBUG_INFO, "%s: %d\n", str_ip, self->clientAddr.sin_port);
//		msg->type = UDP_SERVER_MSG_TYPE;
//		msg->cmd = MSG_UDPSERVER_P_REQUEST;
//		msg->len = 0;
//		if (msg_queue_snd(self->upnp_snd_process_id, msg,
//				sizeof(struct UDPMsg) - sizeof(long)) == -1) {
//			DEBUGPRINT(DEBUG_ERROR, "<Q>--[error: %s]--[errno: %d]--<Q>\n",strerror(errno),errno);
//		}
		udpServer_getdata(self);
		for(i = 0;i< 5; i++){
			if(UdpServerSndMsg(self->sockfd, &self->clientAddr, UDP_DISCOVER_RES, self->msgDataRcv, 16) == -1) {
				DEBUGPRINT(DEBUG_ERROR, "<Q>--[error: %s]--[errno: %d]--<Q>\n",strerror(errno),errno);
			}
		}
		for(i = 0; i < 2; i++){
			if (UdpServerSndMsg(self->sockfd, &self->broadcastAddr, UDP_DISCOVER_RES, self->msgDataRcv, 16) == -1) {
				DEBUGPRINT(DEBUG_ERROR, "<Q>--[error: %s]--[errno: %d]--<Q>\n",strerror(errno),errno);
			}
		}
	}else if(res == 0 && udpmsg->cmd == UDP_DISCOVER_VERSION_REQ) {
		if(UdpServerSndMsg(self->sockfd, &self->clientAddr, UDP_DISCOVER_VERSION_RES, NULL, 0) == -1) {
			DEBUGPRINT(DEBUG_ERROR, "<Q>--[error: %s]--[errno: %d]--<Q>\n",strerror(errno),errno);
		}
		if(UdpServerSndMsg(self->sockfd, &self->broadcastAddr, UDP_DISCOVER_VERSION_RES, NULL, 0) == -1) {
			DEBUGPRINT(DEBUG_ERROR, "<Q>--[error: %s]--[errno: %d]--<Q>\n",strerror(errno),errno);
		}
	}

	return 0;
}

/**
 * @brief udp适配器函数
 * @author <王志强>
 * ＠param[in]<msg><适配器运行环境>
 * @return <-1: 失败, 0: 成功>
 */
void UdpServerCmdTableFun(msg_container *msg)
{
	int ret;
	MsgData *pmsg = (MsgData *)msg->msgdata_rcv;
	MsgData *msgself = (MsgData *)msg->msgdata_snd;
	int cmd = pmsg->cmd;
	int len, len_msg;
	char buf_mac[100];
	int port;

	memset(msg->msgdata_snd, 0, MSG_MAX_LEN);
	switch(cmd){
		case MSG_UDPSERVER_T_RESPONSE:
			break;

		case MSG_UDPSERVER_P_REQUEST:
			msgself->type = UDP_SERVER_MSG_TYPE;
			msgself->cmd = MSG_UDPSERVER_T_RESPONSE;
			/*
			 * MAC地址长度12字节，端口号4字节
			 * */
			len = sizeof(MsgData);
			memset(buf_mac, 0, 100);
			get_mac_addr_value(buf_mac, &len_msg);
			memcpy(&msg->msgdata_snd[len], &buf_mac[12], 12);
			len += 12;

			memset(buf_mac, 0, 100);
			get_config_item_value(CONFIG_NETTRANSMIT_PORT, buf_mac, &len_msg);
//			port = atoi(buf_mac);
			port = SERVER_PORT;
			memcpy(&msg->msgdata_snd[len], &port, 4);
			//printf("%d listen port: %s, %d\n", len_msg, buf_mac, port);
			msgself->len = 16;

			ret = msg_queue_snd(msg->rmsgid, msg->msgdata_snd, 16 + msgself->len);
			if (ret != 0) {
				DEBUGPRINT(DEBUG_ERROR, "In UdpServerCmdTableFun: send msg failed\n");
			}
			break;

		default:
			break;
	}
}

/**
 * @brief udp获取IP和端口号
 * @author <王志强>
 * ＠param[in]<msg><适配器运行环境>
 */
void udpServer_getdata(struct udpServerAttr_t *self)
{
	char *pmsg = self->msgDataRcv;
	int len = 0;
	int undefine;
	int port;

	/*
	 * MAC地址长度12字节，端口号4字节
	 * */
	len = 0;
	get_mac_addr_value(&pmsg[len], &undefine);
	memcpy(pmsg, &pmsg[12], 12);
	len += 12;

//	get_config_item_value(CONFIG_NETTRANSMIT_PORT, buf_mac, &len_msg);
//			port = atoi(buf_mac);
	port = SERVER_PORT;
	memcpy(&pmsg[len], &port, 4);
	//printf("%d listen port: %s, %d\n", len_msg, buf_mac, port);
	//len += 4;
}

