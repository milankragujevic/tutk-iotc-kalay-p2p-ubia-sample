#ifndef UDPSERVER_H_
#define UDPSERVER_H_
#include "common/utils.h"
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

#define UDP_MSG_MAX_LEN 1024

#define LOCAL_PORT		10000
#define BROADCAST_PORT	10000

#define UDP_DISCOVER_REQ			100
#define UDP_DISCOVER_RES			101
#define UDP_DISCOVER_VERSION_REQ	102
#define UDP_DISCOVER_VERSION_RES	103
enum{
	MSG_UDPSERVER_P_REQUEST,
	MSG_UDPSERVER_T_RESPONSE,
};

typedef struct UDPMsg {
	char head[4];
	int cmd;
	int version;
	int length;
//	char text[UDP_MSG_MAX_LEN];
}UDPMsg;

extern int udpServer_threadStart();
extern void UdpServerCmdTableFun(msg_container *msg);

int UdpServerSndMsg(int sockfd, struct sockaddr_in *clientAddr, int cmd, char *data, int len);
int udpServer_sndMsg(int sockfd, struct sockaddr_in *clientAddr, char *data, int len);
extern int udpServer_recvMsg(int sockfd, struct sockaddr_in *clientAddr, char *data, int len);
extern int udpServer_clientSockInit(int *sockfd);
extern int udpServer_serverSockInit(int *sockfd, struct sockaddr_in *serverAddr);
extern int udpServer_clientAddrInit(struct sockaddr_in *clientAddr, const char *addr, int port);
extern int udpServer_serverAddrInit(struct sockaddr_in *serverAddr, int port);
extern int udpServer_sockAddrInit(struct sockaddr_in *clientAddr, const char *addr, int port);


#endif /* UDPSERVER_H_ */
