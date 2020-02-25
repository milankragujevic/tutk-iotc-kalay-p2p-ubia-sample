#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>

#include "tutk_rdt_video_server.h"

#if 0
struct VideoSendThreadMsg{
	int videoSendThreadMsgID;
	int childProcessMsgID;
//	int iBufferSize;
	char VideoMsgRecv[MAX_VIDEO_MSG_LEN];
	char VideoMsgSend[MAX_VIDEO_MSG_LEN];
	//char *settingError;
};

struct HandleVideoSendMsg{
	long int type;
	int cmd;
	char unUsed[4];
	int len;
	int videosocketfd;
//	unsigned char data[AUDIO_CAPTURE_MSG_LEN-16];
};
VideoData videoData;
#endif

void tutk_log_s(const char *fmt, ...)
{
#define BUF_SIZE        1024
        char buf[BUF_SIZE];
        int n;
        va_list ap;

        memset(buf, 0, sizeof(buf));
        strcpy(buf, "TUTK:");
        n = strlen(buf);
        va_start(ap, fmt);
        vsnprintf(&buf[n], 1024, fmt, ap);
        n = strlen(buf);
        if(n > BUF_SIZE-2){
                fprintf(stderr, "%s: log big\n", __FUNCTION__);
                return;
        }
        buf[n] = '\n';
        n++;
        buf[n] = '\0';
        n++;
        va_end(ap);
        log_s(buf, n);
}

unsigned int getTimeStamp()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec*1000 + tv.tv_usec/1000);
}

volatile struct tutk_iotc_session tutk_session[TUTK_SESSION_NUM];
volatile struct tutk_thread_msg  tutk_msg;

void tutk_error_write(int ret)
{
	tutk_msg.login_fp = fopen(LOGIN_ERROR_PATH, "w");
	if(tutk_msg.login_fp == NULL){
		DEBUGPRINT(DEBUG_INFO, "open %s fail\n", LOGIN_ERROR_PATH);
		return;
	}

	fprintf(tutk_msg.login_fp, "%d\n", ret);
	fclose(tutk_msg.login_fp);
}

/**
 * @brief tutk发送数据函数
 * @author <王志强>
 * ＠param[in]<rdt_id><rdt的id>
 * ＠param[in]<rdt_sendbuf><发送的数据首地址>
 * ＠param[in]<len><发送数据的长度>

 * @return <小于0: 失败, 1: 成功>
 */
int tutk_send_vast(int rdt_id, char *rdt_sendbuf, int len)
{
	int ret;
	int have_send = 0;
	RequestHead *head = (RequestHead *)rdt_sendbuf;

	if(head->cmd != 304 && head->cmd != 400){
//		tutk_log_s("tutk_send_vast, cmd:", head->cmd);
		log_printf("tutk_send_vast, cmd: %d", head->cmd);
	}
	while(len != 0){
		if(len > 500){
			ret = RDT_Write(rdt_id, rdt_sendbuf + have_send, 500);
			if(ret < 0){
				return ret;	
			}	
		}else{
			ret = RDT_Write(rdt_id, rdt_sendbuf + have_send, len);
			if(ret < 0){
				return ret;	
			}
		}
		//printf("tutk_send_vast %d ret = %d\n", rdt_id, ret);
		len -=ret;
		have_send += ret;
	}
	return 1;
}

/**
 * @brief tutk发送告警数据函数
 * @author <王志强>
 * ＠param[in]<msg><发送的数据首地址>

 * @return <0: 成功>
 */
int tutk_send_alarm_event(char *msg)
{
	struct MsgData *msgq = (struct MsgData *)msg;
	RequestHead *head = (RequestHead *)tutk_msg.msg_data_snd;
	int i, j;
#if 1
	for(i = 0; i < TUTK_SESSION_NUM; i++){
		for(j = 0; j < TUTK_CHANNEL_NUM; j++){
			if(tutk_session[i].ch[j].rdt_id > -1){
				memset(&tutk_msg.msg_data_snd, 0, sizeof(tutk_msg.msg_data_snd));
				memcpy(head->head, ICAMERA_NETWORK_HEAD, sizeof(ICAMERA_NETWORK_HEAD));
				head->version = ICAMERA_NETWORK_VERSION;
				head->cmd = NEWS_ALARM_EVENT_NOTIFY;
				head->len = msgq->len;
				memcpy(head+1, msgq+1, msgq->len);
				tutk_send_vast(tutk_session[i].ch[j].rdt_id, head, msgq->len+23);
			}
		}
	}
#endif
	return 0;
}

/**
 * @brief 清除tutk缓冲区的数据
 * @author <王志强>
 * ＠param[in]<rdt_id><rdt的id>
 * ＠param[in]<rdt_sendbuf><接收数据首地址>
 * ＠param[in]<len><接收数据的长度>

 * @return <0: 成功>
 */
int tutk_recv_data_throw(int rdt_id, char *rdt_sendbuf, int sum)
{
	RDT_Read(rdt_id, rdt_sendbuf, sum, 30000);
	return 0;
}

/**
 * @brief 清除tutk缓冲区的数据，带超时
 * @author <王志强>
 * ＠param[in]<rdt_id><rdt的id>
 * ＠param[in]<rdt_sendbuf><接收数据首地址>
 * ＠param[in]<len><接收数据的长度>
 * ＠param[in]<timeout><超时时间 ms>

 * @return <接收数据长度: 成功  小于0：失败>
 */
int tutk_recv_data_timeout(int rdt_id, char *rdt_sendbuf, int sum, int timeout)
{
	int len = 0;
	int have_recv = 0;

	if(rdt_sendbuf == NULL){
		return 0;
	}
	while(sum){
		len = RDT_Read(rdt_id, rdt_sendbuf+have_recv, sum, timeout);
		if(len < 0){
			return len;
		}
		have_recv += len;
		sum -= len;
	}

	return have_recv;
}

/**
 * @brief 接收数据函数
 * @author <王志强>
 * ＠param[in]<rdt_id><rdt的id>
 * ＠param[in]<rdt_sendbuf><接收数据首地址>
 * ＠param[in]<len><接收数据的长度>

 * @return <接收数据长度: 成功  小于0：失败>
 */
int tutk_recv_data(int rdt_id, char *rdt_sendbuf, int sum)
{
	int len = 0;
	int have_recv = 0;

	if(rdt_sendbuf == NULL){
		return 0;
	}
	while(sum){
		len = RDT_Read(rdt_id, rdt_sendbuf+have_recv, sum, 30000);
		if(len < 0){
			return len;
		}
		have_recv += len;
		sum -= len;		
	}

	return have_recv;
}

/**
 * @brief 接收协议头数据函数
 * @author <王志强>
 * ＠param[in]<rdt_id><rdt的id>
 * ＠param[in]<rdt_sendbuf><接收数据首地址>

 * @return <接收数据长度: 成功  小于0：失败>
 */
int tutk_recv_head_data(int rdt_id, char *rdt_sendbuf)
{
	int len = 0;
	int have_recv = 0;
	int sum = sizeof(RequestHead);

	if (rdt_sendbuf == NULL ) {
		return 0;
	}
	while (sum) {
		len = RDT_Read(rdt_id, rdt_sendbuf + have_recv, sum, 30000);
		if (len < 0) {
			return len;
		}
		have_recv += len;
		sum -= len;
	}

	return have_recv;
}

/**
 * @brief 重新登录函数
 * @author <王志强>

 * @return <0: 成功>
 */
int tutk_relogint(void)
{
	int iRet;

	RDT_DeInitialize();
	IOTC_DeInitialize();

//	DEBUGPRINT(DEBUG_INFO,"tutk_relogint**********************************\n");
//	iRet = IOTC_Initialize(0, "m4.iotcplatform.com", "m2.iotcplatform.com",
//			"m3.iotcplatform.com", "m1.iotcplatform.com"); //set master information
	iRet = IOTC_Initialize2(0);
	if (0 != iRet) {
		printf("JZF:iotc init error iRet = %d\n", iRet);
		return -1;
	}
	iRet = RDT_Initialize();
	if (iRet < 0) {
		printf("JZF:RDT_Initialize error!!\n");
		return -1;
	}
	tutk_msg.client_cnt = 0;
	return 0;
}

/**
 * @brief tutk登录线程
 * 	调用IOTC_Device_Login函数，根据返回值判断登录是否成功。
 * 	返回0登录成功，此时需要设置灯的状态
 * 	返回－10为无效的UID，应该给https线程发送消息
 * 	其他的返回认为登录不成功，每5s登录一次
 *
 * @author <王志强>
 * ＠param[in]<arg><未用>

 * @return <接收数据长度: 成功  小于0：失败>
 */
void *tutk_login_thread(void *arg)
{
	int ret;
	//char Camera_Type[8];
	int n;
	int cnt = 0;
//	unsigned long info;

	while (1) {
		//IOTC_Get_Login_Info(&info);
		//if (info != 7) {

	//	UID_is_Err();
		ret = IOTC_Device_Login(tutk_msg.tutk_id, DEVICENAME, DEVICEPWD); //login the server with UID
		if(ret == IOTC_ER_UNLICENSE ){
			DEBUGPRINT(DEBUG_INFO, "%s: login error number is:%d\n", tutk_msg.tutk_id, ret);
			UID_is_Err();
		}else if (ret == IOTC_ER_NETWORK_UNREACHABLE){
			if(cnt % 3 == 0){
				//发消息到networkset
				//TUTK_DEBUG("%s: login error number is:%d, start check net\n", tutk_msg.tutk_id, ret);
				DEBUGPRINT(DEBUG_INFO, "%s: login error number is:%d, start check net\n", tutk_msg.tutk_id, ret);
				if(appInfo.cameraMode != INFO_FACTORY_MODE){
					start_check_net_connect();
				}
				cnt = 0;
			}
			cnt++;
			usleep(5000000);

			tutk_error_write(ret);
//			android_usb_send_msg(tutk_msg.upnp_rev_process_id, MFI_MSG_TYPE, 11);

			continue;
		} else if(ret == IOTC_ER_NoERROR || ret == IOTC_ER_LOGIN_ALREADY_CALLED){
			DEBUGPRINT(DEBUG_INFO, "JZF:%s ---------------------------> login ok!\n", tutk_msg.tutk_id);
			tutk_log_s("login login: %d, %d", g_nDeviceType, appInfo.cameraMode);
			android_usb_send_msg(tutk_msg.upnp_rev_process_id, MFI_MSG_TYPE, 10);
			//tutk_msg.tutk_login = 0;
			usleep(20000);
			if(DEVICE_TYPE_M3S == g_nDeviceType){
				usleep(20000);
				tutk_log_s("login login M3S: %d, %d", g_nDeviceType, appInfo.cameraMode);
				if(appInfo.cameraMode != INFO_FACTORY_MODE){
					tutk_and_http_set_led(NET_STATE_CONTROL_TUTK);
				}
				//tutk_login_ok_set_led(LED_M3S_TYPE_NET, LED_CTL_TYPE_ON);
			}
			else{
				tutk_log_s("login login M2: %d, %d", g_nDeviceType, appInfo.cameraMode);
				if(appInfo.cameraMode != INFO_FACTORY_MODE){
					tutk_and_http_set_led(NET_STATE_CONTROL_TUTK);
				}
				//tutk_login_ok_set_led(LED_M2_TYPE_MIXED, LED_CTL_TYPE_ON);
			}
		}else if(ret < 0){
			DEBUGPRINT(DEBUG_INFO, "%s: login error number is:%d\n", tutk_msg.tutk_id, ret);
			usleep(5000000);

			tutk_error_write(ret);
//			android_usb_send_msg(tutk_msg.upnp_rev_process_id, MFI_MSG_TYPE, 11);

			continue;
		}
		if(ret < 0 && ret != IOTC_ER_LOGIN_ALREADY_CALLED){
			tutk_log_s("login errno %d", ret);
//			android_usb_send_msg(tutk_msg.upnp_rev_process_id, MFI_MSG_TYPE, 11);
		}
		tutk_error_write(ret);

		usleep(30000000);
	}
	return 0;		
}

/**
 * @brief tutk初始化函数
 * 初始化tutk用户相关的全局变量
 * 等待UID
 * 创建tutk登录线程
 *
 * @author <王志强>
 */
void tutk_init(void)
{
	int i;
	pthread_t tid;

	for (i = 0; i < TUTK_SESSION_NUM; i++) {
		tutk_session_init(&tutk_session[i]);
	}
	//等待tutk uid
	while (tutk_msg.tutk_uid == 2) {
		usleep(10000);
	}

	if (pthread_create(&tid, NULL, tutk_login_thread, NULL) != 0) {
		printf("login_RDT_thread,\n");
		return;
	}

}

/**
 * @brief tutk监听线程
 * @author <王志强>
 * ＠param[in]<arg><未用>

 */
void *tutk_listen_thread(void *arg)
{
	pthread_t tid;
	pthread_attr_t thread_attr;
	int sid;
	int ret;
	struct tutk_iotc_session session_tmp;
	//struct tutk_iotc_session *ssp = (struct iotc_session *)&tutk_session[0];

	tutk_msg.thread_syn = 2;

	tutk_init();
	do {
		if(tutk_msg.tutk_start == TUTK_SERVER_STOP){
			//DEBUGPRINT(DEBUG_INFO,"TUTK_SERVER_STOP...#################\n");
			goto tutk_sleep;
		}

		if(tutk_msg.tutk_netch == 2){
			goto tutk_sleep;
		}

		sid = IOTC_Listen(6000);
		DEBUGPRINT(DEBUG_INFO, "TUTK UID:%s........%d..\n", tutk_msg.tutk_id, tutk_msg.client_cnt);
		if (sid > -1) { //success
#if 0
			tutk_session_init(&session_tmp);
			ret = IOTC_Session_Check(sid, &sinfo);
			if (ret < 0) {
				DEBUGPRINT(DEBUG_ERROR, "IOTC_Session_Check: %d", ret);
				continue;
			}

			char *mode[3] = { "P2P", "RLY", "LAN" };
			DEBUGPRINT(DEBUG_INFO, "Client from %s:%d Mode=%s %d\n", sinfo.RemoteIP,
					sinfo.RemotePort, mode[(int) sinfo.Mode], sid);
			switch (sinfo.Mode) {
				case 0:
					session_tmp.session_mode = TUTK_P2P;
					break;
				case 1:
					session_tmp.session_mode = TUTK_RLY;
					break;
				case 2:
					session_tmp.session_mode = TUTK_LAN;
					break;
			}
#endif
			session_tmp.session_id = sid;
			session_tmp.send_picture_time = getTimeStamp();
			//session_tmp.session_used = SESSION_USED_USING;
			tutk_msg.client_cnt++;

			pthread_attr_init(&thread_attr);
			pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
			pthread_create(&tid, &thread_attr, tutk_session_thread, sid);
			
//			printf("listen: wait\n");
//			pthread_mutex_lock(&session_tmp.tutk_mutex);
//			pthread_cond_wait(&session_tmp.tutk_cond, &session_tmp.tutk_mutex);
//			pthread_mutex_unlock(&session_tmp.tutk_mutex);
//			printf("listen: wait ...\n");

		}
tutk_sleep:
		usleep(100000);
	}while(1);

	pthread_exit(NULL);
}

/**
 * @brief tutk消息有关的全局变量初始化
 * @author <王志强>

 */
void tutk_global_var_init(void)
{
	memset(&tutk_msg, 0, sizeof(tutk_msg));
	tutk_msg.client_cnt = 0; //用户数量为0
	tutk_msg.thread_syn = 1; //同步标志
	tutk_msg.tutk_uid = 2;
	tutk_msg.tutk_start = TUTK_SERVER_USED;
	tutk_msg.tutk_netch = 1;
	tutk_msg.audio_val = 10 * 4096;
	tutk_msg.tutk_video_user_num = 0;
	tutk_msg.tutk_audio_user_num = 0;
	tutk_msg.upnp_rev_process_id = msg_queue_get((key_t) CHILD_THREAD_MSG_KEY);
	if (tutk_msg.upnp_rev_process_id == -1) {
		return;
	}
	tutk_msg.upnp_snd_process_id = msg_queue_get((key_t) CHILD_PROCESS_MSG_KEY);
	if (tutk_msg.upnp_snd_process_id == -1) {
		return;
	}
}

/**
 * @brief tutk消息初始化
 * 消息有关的全局变量初始化
 * 创建监听线程
 * 等待内部线程同步
 * @author <王志强>
 * @return <0: 成功>
 */
int tutk_msg_init(void)
{
    tutk_global_var_init();
    thread_start(tutk_listen_thread);

	while(tutk_msg.thread_syn == 1){
		usleep(10000);
	}

	return 0;
}

/**
 * @brief tutk处理消息线程
 * @author <王志强>
 * @return <0: 成功>

 */
int *tutk_server(void *arg)
{
	struct MsgData *msg = (struct MsgData *)tutk_msg.msg_data_rcv;

	tutk_msg_init();
	thread_synchronization(&rP2p, &rChildProcess);

	while (1) {
		if (msg_queue_rcv(tutk_msg.upnp_rev_process_id, tutk_msg.msg_data_rcv,
				sizeof(tutk_msg.msg_data_rcv), P2P_MSG_TYPE) == -1) {
		} else {
			switch (msg->cmd) {
			case MSG_P2P_T_TUTKID:
				DEBUGPRINT(DEBUG_INFO, "============================tutk_recv_msgq_thread: %d %s\n", msg->len, msg+1);
				memcpy(tutk_msg.tutk_id, msg + 1, sizeof(tutk_msg.tutk_id)-1);
				tutk_msg.tutk_uid = 1;
				break;

			case MSG_P2P_T_ALARMEVENT:
				tutk_send_alarm_event(tutk_msg.msg_data_rcv);
				break;

			case MSG_P2P_T_NETCHANGE:
				tutk_msg.tutk_netch = 2;
//				tutk_all_user_stop();
//				tutk_check_user_stop();
				tutk_relogint();
				tutk_msg.tutk_netch = 1;
				DEBUGPRINT(DEBUG_INFO,"============================= MSG_P2P_T_NETCHANGE\n");
				break;

			case CMD_STOP_ALL_USE_FRAME:
				tutk_msg.tutk_start = TUTK_SERVER_STOP;
				tutk_all_avchannel_stop();
				tutk_check_avchannel_stop();
				g_lP2p = 1;
				DEBUGPRINT(DEBUG_INFO,"============================= TUTK_SERVER_STOP\n");
				break;

			case CMD_AV_STARTED:
				tutk_all_avchannel_start();
				tutk_msg.tutk_start = TUTK_SERVER_START;
				DEBUGPRINT(DEBUG_INFO,"===========================TUTK_SERVER_START\n");
				break;

			default:
				break;
			}
		}
		usleep(100000);
	}

	return 0;
}

/**
 * @brief tutk的入口函数
 * @author <王志强>
 * @return <0: 成功  －1：失败>

 */
int create_tutk_server_thread(void)
{
	int ret;

	ret = thread_start(&tutk_server);
	if (ret < 0) {
		DEBUGPRINT(DEBUG_ERROR,"creat  udt send thread Faile\n");
		return -1;
	} else {
		DEBUGPRINT(DEBUG_INFO,"creat  udt send thread  Start  success\n");
	}

	return 0;
}


