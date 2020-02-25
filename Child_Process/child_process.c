/**	
   @file child_process.c
   @brief 应用程序主文件
   @note
   author: yangguozheng
   date: 2013-09-16
   mcdify record: creat this file.
   author: yangguozheng
   date: 2013-09-17
   modify record: add a function
 */
#include "common/time_func.h"
#include "common/thread.h"
#include "common/utils.h"
#include "common/logfile.h"
#include "common/thread.h"
#include "common/common.h"
#include "common/msg_queue.h"
#include "common/logfile.h"
#include "common/common_config.h"

#include "child_process.h"

#include "MFI/MFI.h"
#include "MFI/MFIAdapter.h"
#include "IOCtl/ioctl.h"
#include "IOCtl/ioctl_adapter.h"
#include "NetWorkSet/networkset.h"
#include "NetWorkSet/network_adapter.h"
#include "SerialPorts/SerialPorts.h"
#include "SerialPorts/SerialAdapter.h"
#include "Audio/Audio.h"
#include "Audio/AudioAdapter.h"
#include "Video/video.h"
#include "Video/VideoAdapter.h"
#include "NewsChannel/NewsChannel.h"
#include "NewsChannel/NewsChannelAdapter.h"
#include "NewsChannel/NewsUtils.h"
#include "HTTPServer/adapter_httpserver.h"
#include "HTTPServer/httpsserver.h"
#include "NTP/NTP.h"
#include "VideoAlarm/VideoAlarm.h"
#include "VideoAlarm/VideoAlarmAdapter.h"
#include "UPNP/upnp_thread.h"
#include "UPNP/upnp_adapter.h"
#include "Tooling/ToolingAdapter.h"

#include "AudioAlarm/audioalarm.h"
#include "VideoSend/VideoSend.h"
#include "AudioSend/AudioSend.h"
#include "Udpserver/udpServer.h"
#include "playback/speaker.h"
#include "playback/speakerAdapter.h"

#include "FlashRW/FlashRW.h"
#include "p2p/tutk_rdt_video_server.h"
#include "HTTPServer/tools.h"
#include "kernel_code/iwconfig.h"
#include "kernel_code/iwlib.h"
#include "kernel_code/iwpriv.h"
#include "NewsChannel/BellService.h"

volatile ThreadStatus_t threadStatus[] = {
		{CHILD_PROCESS_MSG_TYPE  , 0},
		{AUDIO_MSG_TYPE          , 0},
		{AUDIO_ALARM_MSG_TYPE    , 0},
		{AUDIO_SEND_MSG_TYPE     , 0},
		{DDNS_SERVER_MSG_TYPE    , 0},
		{FLASH_RW_MSG_TYPE       , 0},
		{FORWARDING_MSG_TYPE     , 0},
		{HTTP_SERVER_MSG_TYPE    , 0},
		{MFI_MSG_TYPE            , 0},
		{NETWORK_SET_MSG_TYPE    , 0},
		{NEWS_CHANNEL_MSG_TYPE   , 0},
		{NTP_MSG_TYPE            , 0},
		{PLAY_BACK_MSG_TYPE      , 0},
		{RECV_PLAY_BACK_MSG_TYPE , 0},
		{SERIAL_PORTS_MSG_TYPE   , 0},
		{TOOLING_MSG_TYPE        , 0},
		{UDP_SERVER_MSG_TYPE     , 0},
		{UPGRADE_MSG_TYPE        , 0},
		{UPNP_MSG_TYPE           , 0},
		{VIDEO_MSG_TYPE          , 0},
		{VIDEO_ALARM_MSG_TYPE    , 0},
		{VIDEO_SEND_MSG_TYPE     , 0},
		{IO_CTL_MSG_TYPE         , 0},
		{CONTROL_MSG_TYPE        , 0},
		{P2P_MSG_TYPE            , 0},
		{LAST_TYPE               , 0},
		{HTTPS_SERVER_DO_TYPE    , 0}
};

#if 1
#define TEST_PRINT(format, arg...)\
	DEBUGPRINT(DEBUG_INFO, format, ##arg)
#else
#define TEST_PRINT(format, arg...)
#endif

/** @brief 模块属性定义 */
typedef struct child_process_t {
	msg_container container; //extends class message container
	char processRunning;
	char msgHandlerSwitch;
	char adapterSwitch;
	time_t tm;
}child_process_t;

/** @brief 适配器列表 */
type_item type_table[] = {
		{CHILD_PROCESS_MSG_TYPE, 0},
		{AUDIO_MSG_TYPE, AudioAdapter_cmd_table_fun},
		{AUDIO_ALARM_MSG_TYPE, audioalarm_cmd_table_fun},
		{AUDIO_SEND_MSG_TYPE, 0},
		{DDNS_SERVER_MSG_TYPE, 0},
		{FLASH_RW_MSG_TYPE, 0},
		{FORWARDING_MSG_TYPE, 0},
		{HTTP_SERVER_MSG_TYPE, HttpServer_cmd_table_fun},
		{MFI_MSG_TYPE, MFI_cmd_table_fun},
		{NETWORK_SET_MSG_TYPE, net_cmd_table_fun},
		{NEWS_CHANNEL_MSG_TYPE, NewsChannelAdapter_cmd_table_fun},
		{NTP_MSG_TYPE, 0},
		{PLAY_BACK_MSG_TYPE, speakerAdapter_cmd_table_fun},
		{RECV_PLAY_BACK_MSG_TYPE, 0},
		{SERIAL_PORTS_MSG_TYPE, serialports_cmd_table_fun},
		{TOOLING_MSG_TYPE, 0},
		{UDP_SERVER_MSG_TYPE, UdpServerCmdTableFun},
		{UPGRADE_MSG_TYPE, 0},
		{UPNP_MSG_TYPE, 0},
		{VIDEO_MSG_TYPE, VideoAdapter_cmd_table_fun},
		{VIDEO_ALARM_MSG_TYPE, videoalarm_cmd_table_fun},
		{VIDEO_SEND_MSG_TYPE, 0},
		{IO_CTL_MSG_TYPE, ioctl_cmd_table_fun},
};

int child_process_init(child_process_t *self);
int child_process_exit(child_process_t *self);
int child_process_start_threads(child_process_t *self);
int child_process_msg_handler(child_process_t *self);
int child_process_start_adapters(child_process_t *self);
int child_process_synchronization(child_process_t *self);
int child_getThreadStatus(int id, void *arg);
int child_setThreadStatus(int id, void *arg);

/**
   @brief 子进程入口函数
   @author yangguozheng
   @return 0
   @note
 */
int child_process_main() {
//	msg_container *self = (msg_container *)malloc(sizeof(msg_container));
	TEST_PRINT("child process start\n");
	child_process_t *self = (child_process_t *)malloc(sizeof(child_process_t));
	child_process_init(self);
	TEST_PRINT("child process synchronization start\n");
	init_Camera_info();
	child_process_synchronization(self);

	TEST_PRINT("child process synchronization stop\n");
	time_t tm;

	while(self->processRunning == PROCESS_RUNNING) {
		child_process_msg_handler(self);
		chile_process_timer_handler();
		time(&tm);
		if(tm - self->tm > 60) {
			time(&(self->tm));
			child_getThreadStatus(0, 0);
		}
		thread_usleep(100000);
	}
	child_process_exit(self);
	return 0;
}

/**
   @brief 子进程模块初始化函数
   @author yangguozheng
   @param[in|out] self 子进程环境变量
   @return 0
   @note
 */
int child_process_init(child_process_t *self) {
	msg_container *container = (msg_container *)self;
	self->processRunning = PROCESS_STOP;
	msg_queue_remove_by_key(CHILD_PROCESS_MSG_KEY);
	msg_queue_remove_by_key(CHILD_THREAD_MSG_KEY);
	msg_queue_remove_by_key(HTTP_SERVER_MSG_KEY);
	msg_queue_remove_by_key(DO_HTTP_SERVER_MSG_KEY);

	container->lmsgid = msg_queue_create(CHILD_PROCESS_MSG_KEY);
	if(container->lmsgid == -1){
		return -1;
	}

	container->rmsgid = msg_queue_create(CHILD_THREAD_MSG_KEY);
	if(container->rmsgid == -1){
		return -1;
	}

	/* 初始化flash配置信息到内存 add by wfb */
	init_flash_config_parameters();
	logfile_init(); //initialize log module
	initAppInfo();
	SetStableFlag(); //
	hardware_init();
	runtimeinfo_init();
	time(&(self->tm));
#ifndef _DISABLE_SERIALPORT
	if(-1 == serialport_open("/dev/ttyS1")) {
		DEBUGPRINT(DEBUG_ERROR, "serialport_open: %s", errstr);
	}
	else {
		DEBUGPRINT(DEBUG_ERROR, "serialport_open: success\n");
	}
#endif
	TEST_PRINT("%s: container->lmsgid: %d, container->rmsgid: %d\n", __FUNCTION__, container->lmsgid, container->rmsgid);
	child_process_start_threads(self); //start threads
	self->msgHandlerSwitch = MSG_HANDLER_ON;
	self->adapterSwitch = ADAPTER_ON;
	self->processRunning = PROCESS_RUNNING;
	TEST_PRINT("%s: success\n", __FUNCTION__);
	return 0;
}

/**
   @brief 子进程模块销毁函数
   @author yangguozheng
   @param[in|out] self 子进程环境变量
   @return 0
   @note
 */
int child_process_exit(child_process_t *self) {
	free(self);
	return 0;
}

/**
   @brief 开启各模块
   @author yangguozheng
   @param[in|out] self 子进程环境变量
   @return 0
   @note
 */
int child_process_start_threads(child_process_t *self) {
	CreateVideoCaptureThread();
	DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++++++++++1\n");
	CreateAudioCaptureThread();
	DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++++++++++2\n");
#ifndef _DISABLE_AUDIO_ALARM
	CreateAudioAlarmThread();
	DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++++++++++3\n");
#endif
#ifndef _DISABLE_VIDEO_ALARM
	VideoAlarm_Thread_Creat();
	DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++++++++++4\n");
#endif
#ifndef _DISABLE_AUDIO_SEND
	Creataudio_send_thread();
	DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++++++++++5\n");
#endif
#ifndef _DISABLE_VIDEO_SEND
	Creatvideo_send_thread();
	DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++++++++++6\n");
#endif
	createHttpsServerThread();
	DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++++++++++7\n");
#ifndef _DISABLE_NETSET
	NetworkSet_Thread_Creat();
	DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++++++++++8\n");
#endif
    //CreateTestThread();
	//p2p_threadStart();
	//udt_send_threadStart();
	NewsChannel_threadStart();
	DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++++++++++9\n");
#ifndef _DISABLE_SPEAKER
    CreateSpeakerThread();
    DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++++++++++10\n");
#endif
#ifndef _DISABLE_SERIALPORT
	SerialPorts_Thread_Creat();
	DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++++++++++11\n");
#endif
	udpServer_threadStart();
	DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++++++++++12\n");
	//test_threadStart();
#ifndef _DISABLE_UPNP
    CreateUpnpThread();
    DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++++++++++13\n");
#endif
	flash_wr_thread_creat();
	DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++++++++++14\n");
	Create_MFI_Thread();
	DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++++++++++15\n");
	ioctl_thread_creat();
	DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++++++++++16\n");
	Create_NTP_Thread();
	DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++++++++++18\n");
//	thread_start(udt_server);
#ifdef _IMG_TYPE_BELL
	BellServiceThreadStart();
#endif

#if _TUTK_VERSION == 1
    create_tutk_server_thread();
    DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++++++++++17\n");
#endif
	return 0;
}

/**
   @brief 内部消息处理
   @author yangguozheng
   @param[in|out] self 子进程环境变量
   @return 0
   @note
 */
int child_process_msg_handler(child_process_t *self) {
	msg_container *container = (msg_container *)self;
	MsgData *msg = container->msgdata_rcv;
	if(self->msgHandlerSwitch == MSG_HANDLER_OFF) {
		return 0;
	}
	while(1) {
		if(msg_queue_rcv(container->lmsgid, container->msgdata_rcv, 1024, CHILD_PROCESS_MSG_TYPE) == -1) {
			break;
		}
		else {
//			DEBUGPRINT(DEBUG_INFO, "%s\n", __FUNCTION__);
			//DEBUGPRINT(DEBUG_INFO, "!!!!!!!msg->type: %d, msg->cmd: %d, msg->len: %d\n", msg->type, msg->cmd, msg->len);
//			log_m(msg->type, msg->cmd); //add log function, trace child process flow
			child_process_start_adapters(self);
		}
		usleep(0);
	}
	return 0;
}

/**
   @brief 适配器查询函数
   @author yangguozheng
   @param[in|out] self 子进程环境变量
   @return 0
   @note
 */
int child_process_start_adapters(child_process_t *self) {
	if(self->adapterSwitch == ADAPTER_OFF) {
		return 0;
	}
	msg_container *container = (msg_container *)self;
	MsgData *msgrcv = (MsgData *)container->msgdata_rcv;
//	msgrcv->type = 1;
//	msgrcv->cmd = 2;
	type_item itype;
	itype.type = msgrcv->type;
	msg_fun fun = utils_type_bsearch(&itype, type_table, sizeof(type_table)/sizeof(type_item));
	if(fun != NULL) {
		fun(self);
	}
	else {
		DEBUGPRINT(DEBUG_INFO, "cannot find module\n");
	}
	return 0;
}

/**
   @brief 子进程同步函数
   @author yangguozheng
   @param[in|out] self 子进程环境变量
   @return 0
   @note
 */
int child_process_synchronization(child_process_t *self) {
	while(1) {
		if(
			(rVideo == 1)&& //1
			(rAudio == 1)&& //2
#ifndef _DISABLE_AUDIO_ALARM
			(rAudioAlarm == 1)&& //3
#endif
#ifndef _DISABLE_VIDEO_ALARM
			(rVideoAlarm == 1)&& //4
#endif
#ifndef _DISABLE_AUDIO_SEND
			(rAudioSend == 1)&& //5
#endif
#ifndef _DISABLE_VIDEO_SEND
			(rVideoSend == 1)&& //6
#endif
//		    (rDdnsServer == 1)&&
//			(rForwarding == 1)
			(rHTTPServer == 1)&& //7
#ifndef _DISABLE_NETSET
			(rNetWorkSet == 1)&& //8
#endif
			(rNewsChannel == 1)&& //9
			(rNTP == 1)&&
#ifndef _DISABLE_SPEAKER
			(rplayback == 1)&& //10
#endif
//			(rrevplayback == 1)&&
#ifndef _DISABLE_SERIALPORT
			(rSerialPorts == 1)&& //11
#endif
//			(rTooling == 1)&&
			(rUdpserver == 1)&& //12
//			(rUpgrade == 1)&&
#ifndef _DISABLE_UPNP
			(rUPNP == 1)&& //13
#endif
			(rFlashRW == 1)&& //14
			(rMFI == 1)&& //15
			(rIoCtl== 1)&& //16
#if _TUTK_VERSION == 1
			(rP2p == 1)&& //17
#endif
			(1)
			) {
			rChildProcess = 1;
			break;
		}
		thread_usleep(THREAD_CUT_OUT_TIME);
		DEBUGPRINT(DEBUG_INFO, "1=======%d\n", rVideo);
		DEBUGPRINT(DEBUG_INFO, "2=======%d\n", rAudio);
		DEBUGPRINT(DEBUG_INFO, "3=======%d\n", rAudioAlarm);
		DEBUGPRINT(DEBUG_INFO, "4=======%d\n", rVideoAlarm);
		DEBUGPRINT(DEBUG_INFO, "5=======%d\n", rAudioSend);
		DEBUGPRINT(DEBUG_INFO, "6=======%d\n", rVideoSend);
		DEBUGPRINT(DEBUG_INFO, "7=======%d\n", rHTTPServer);
		DEBUGPRINT(DEBUG_INFO, "8=======%d\n", rNetWorkSet);
		DEBUGPRINT(DEBUG_INFO, "9=======%d\n", rNewsChannel);
		DEBUGPRINT(DEBUG_INFO, "10=======%d\n", rplayback);
		DEBUGPRINT(DEBUG_INFO, "11=======%d\n", rSerialPorts);
		DEBUGPRINT(DEBUG_INFO, "12=======%d\n", rUdpserver);
		DEBUGPRINT(DEBUG_INFO, "13=======%d\n", rUPNP);
		DEBUGPRINT(DEBUG_INFO, "14=======%d\n", rFlashRW);
		DEBUGPRINT(DEBUG_INFO, "15=======%d\n", rMFI);
		DEBUGPRINT(DEBUG_INFO, "16=======%d\n", rIoCtl);
		DEBUGPRINT(DEBUG_INFO, "17=======%d\n", rP2p);
		DEBUGPRINT(DEBUG_INFO, "18=======%d\n", rChildProcess);
	}
	thread_usleep(THREAD_CUT_OUT_TIME);

	DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++++++++++++child_process_synchronization ok++++++++++++++++++++++++\n");
	return 0;
}

//int child_getThreadStatus(int id, void *arg) {
////	if(NULL == arg) {
////		time(&(threadStatus[id].tm));
////	}
//	static long count = 0;
//	int i = 0;
//	++count;
//	ThreadStatus_t *head = threadStatus;
//	ThreadStatus_t *tail = head + sizeof(threadStatus)/sizeof(ThreadStatus_t);
//	time_t tm;
//	time(&tm);
//	FILE *file = fopen("/usr/share/status.txt", "w");
//	if(NULL == file) {
//		return 0;
//	}
//	fprintf(file, "times: %ld\nid, time\n", count);
//	for(;head != tail;++head, ++i) {
//		fprintf(file, "%02d, %ld\n", head->id, tm - head->tm);
//		if(head->id == NEWS_CHANNEL_MSG_TYPE && (tm - head->tm > 15 * 60)) {
//			NewsChannel_usrInfoCloseAll(NewsChannelSelf->usrChannel, MAX_USER_NUM * 3); //error handle
//		}
//	}
//	fclose(file);
//	return 0;
//}

extern int ioctl_getLedStatus(int *status);

int child_getThreadStatus(int id, void *arg) {
//	if(NULL == arg) {
//		time(&(threadStatus[id].tm));
//	}
	static long count = 0;
	int i = 0;
	++count;
	ThreadStatus_t *head = threadStatus;
	ThreadStatus_t *tail = head + sizeof(threadStatus)/sizeof(ThreadStatus_t);
	time_t tm;
	time(&tm);
	FILE *file = fopen("/usr/share/status.txt", "w");
	if(NULL == file) {
		return 0;
	}
	fprintf(file, "times: %ld\nid, time\n", count);
	char buf[256] = {0};
	int len = 0;
	for(;head != tail;++head, ++i) {
		fprintf(file, "%02d, %ld\n", head->id, tm - head->tm);
		len += sprintf(buf + len, "%d,", tm - head->tm);
		if(head->id == NEWS_CHANNEL_MSG_TYPE && (tm - head->tm > 15 * 60)) {
			NewsChannel_usrInfoCloseAll(NewsChannelSelf->usrChannel, MAX_USER_NUM * 3); //error handle
		}
//		log_printf();
	}
	int ledStatus[2];
	log_printf("thread info: %s\n", buf);
	ioctl_getLedStatus(ledStatus);

	char UID[30];
	int n;

	get_config_item_value(CONFIG_CAMERA_ENCRYTIONID,buf, &n);

	get_config_item_value(CONFIG_P2P_UID,UID, &n);

	log_printf("led: %d, %d, uid: %c%c %c%c r: %c%c%c%c\n", ledStatus[0], ledStatus[1], UID[0], UID[1], UID[18], UID[19], buf[0], buf[1], buf[2], buf[3]);

	get_config_item_value(CONFIG_ALARM_MOTION,buf, &n);

	log_printf("m alarm: %c\n", buf[0]);
	get_config_item_value(CONFIG_ALARM_AUDIO,buf, &n);

	log_printf("a alarm: %c\n", buf[0]);

	char serverAddr[256];
	get_config_item_value(CONFIG_SERVER_URL,serverAddr, &n);
	log_printf("server address: %s\n", serverAddr);

	fclose(file);
	return 0;
}

int child_setThreadStatus(int id, void *arg) {
	if(id < 0 || id > (sizeof(threadStatus)/sizeof(ThreadStatus_t))) {
		return -1;
	}
	if(NULL == arg) {
		time(&(threadStatus[id].tm));
	}
	return 0;
}

