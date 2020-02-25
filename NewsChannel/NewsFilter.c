/**	
   @file NewsFilter.c
   @brief 应用程序主文件
   @note
   author: yangguozheng
   date: 2013-09-16
   mcdify record: creat this file.
   author: yangguozheng
   date: 2013-09-17
   modify record: add a function
 */
/** 主要功能: tcp协议过滤 */
#include <stdlib.h>
#include "NewsAdapter.h"
#include "common/msg_queue.h"
#include "common/common.h"
#include "VideoSend/VideoSend.h"
#include "SerialPorts/SerialAdapter.h"
#include "AudioAlarm/audioalarm.h"
#include "common/common_config.h"
#include "VideoAlarm/VideoAlarm.h"
#include "AudioSend/AudioSend.h"
#include "Video/video.h"
#include "NewsChannel.h"
#include "NewsChannelAdapter.h"
#include "NewsFilter.h"
#include "NewsWork.h"
#include "common/common.h"
#include "common/common_func.h"
#include "NewsUtils.h"

/** @brief 过滤函数类型 */
typedef int (*filterFun)(void *self);

int NewsFilter_newsUserJoinReq(void *self);
int NewsFilter_newsVerifyReq(void *self);
int NewsFilter_newsChannelDeclaration(void *self);
int NewsFilter_newsChannelTouchReq(void *self);
int NewsFilter_newsChannelTouchReqV2(void *self);

int NewsFilter_newsSysUpgradeReq(void *self);
int NewsFilter_newsSysUpgradeRep(void *self);
int NewsFilter_newsSysUpgradeProcessIn(void *self);
int NewsFilter_newsSysUpgradeProcessOut(void *self);
int NewsFilter_newsSysUpgradeOver(void *self);

int NewsFilter_newsSysRebootReq(void *self);


int NewsFilter_newsSysUpgradeReq(void *self);
int NewsFilter_newsChannelHitPig(void *self);
int NewsFilter_newsChannelAudioTransmitIn(void *self);
int NewsFilter_newsChannelAudioReq(void *self);

int NewsFilter_nightModeSetReq(void *self);
int NewsFilter_nightModeRecordReq(void *self);
int NewsFilter_wifiQualityReq(void *self);
int NewsFilter_channelSwitchReq(void *self);

/** @brief 协议表，用于过滤 */
NewsCmdItem NewsFilter_cmd_table[] = {
	{NEWS_USER_JOIN_REQ, NewsFilter_newsUserJoinReq},
	{NEWS_USER_VERIFY_REP, NewsFilter_newsVerifyReq},
    {NEWS_CHANNEL_DECLARATION_REQ, NewsFilter_newsChannelDeclaration},
    {NEWS_CHANNEL_TOUCH_REQ, NewsFilter_newsChannelTouchReq},
#if _TCP_VERSION == 2
    {NEWS_CHANNEL_TOUCH_REQ_V2, NewsFilter_newsChannelTouchReqV2},
#endif
	{NEWS_SYS_REBOOT_REQ, NewsFilter_newsSysRebootReq},
//	{NEWS_SYS_REBOOT_REP, 0},
	{NEWS_SYS_UPGRADE_REQ, NewsFilter_newsSysUpgradeReq},
//	{NEWS_SYS_UPGRADE_REP, 0},
//	{NEWS_SYS_UPGRADE_PROCESS_IN, 0},
//	{NEWS_SYS_UPGRADE_PROCESS_OUT, 0},
//	{NEWS_SYS_UPGRADE_OVER, 0},
//	{NEWS_SYS_UPGRADE_PROCESS_CANCEL_REQ, 0},
	{NEWS_SYS_HIT_PIG, NewsFilter_newsChannelHitPig},
    {NEWS_AUDIO_TRANSMIT_IN, NewsFilter_newsChannelAudioTransmitIn},
    {NEWS_AUDIO_TRANSMIT_REQ, NewsFilter_newsChannelAudioReq},
//    {NEWS_NIGHT_MODE_SET_REQ, NewsFilter_nightModeSetReq},
//    {NEWS_NIGHT_MODE_RECORD_REQ, NewsFilter_nightModeRecordReq},
    {NEWS_WIFI_QUALITY_REQ, NewsFilter_wifiQualityReq},
    {NEWS_CHANNEL_SWITCH_REQ, NewsFilter_channelSwitchReq},
};

/**
   @brief 协议查询入口
   @author yangguozheng
   @param[in|out] self 过滤器运行环境
   @return FILTER_FALSE. FILTER_TRUE
   @note
 */
int NewsFilter_cmd_table_fun(void *self) {
	NewsChannel_t *context = (NewsChannel_t *)self;
	RequestHead *head = (RequestHead *)(context->rcvBuf);
	NewsCmdItem icmd;
	icmd.cmd = head->cmd;
	//DEBUGPRINT(DEBUG_INFO, "%s, cmd: %d\n", __FUNCTION__, icmd.cmd);
	filterFun fun = NewsAdapter_cmd_bsearch(&icmd, NewsFilter_cmd_table, sizeof(NewsFilter_cmd_table)/sizeof(NewsCmdItem));
	if(fun != NULL) {
		return fun(self);
	}
	else {
		DEBUGPRINT(DEBUG_INFO, "%s: cannot find command\n", __FUNCTION__);
	}
	return FILTER_FALSE;
}

/**
   @brief 用户加入
   @author yangguozheng
   @param[in|out] self 过滤器运行环境
   @return FILTER_FALSE. FILTER_TRUE
   @note
 */
int NewsFilter_newsUserJoinReq(void *self) {
	NewsChannel_t *context = (NewsChannel_t *)self;
	RequestHead *head = (RequestHead *)(context->rcvBuf);
	UsrChannel *usr = context->currentChannel;
	head->cmd = NEWS_USER_VERIFY_REQ;
	usr->verifyFlow = VERIFY_NULL_FLOW;
	if(usr->verifyFlow == VERIFY_NULL_FLOW) {
		srand((unsigned) time(NULL));
		uint32_t a[4];
		int i = 0;
		for(i = 0;i < 4;++i) {
			a[i] = rand();
			DEBUGPRINT(DEBUG_INFO, "a[i]: %d \n", a[i]);
		}

		memcpy(usr->verifyNumber, a, sizeof(a));

		uint32_t b[4];
		if(appInfo.cameraMode == INFO_FACTORY_MODE || (INFO_NO_R == hasRl())) {
			memset(b, 'F', sizeof(b));
			printf("FFFFFFFFFFFFFFFFFFFFFFFFF\n");
		}
		else {
			memcpy((char *)b, appInfo.rl, 16);
			printf("UUUUUUUUUUUUUUUUUUUUUUUUU\n");
		}
		for(i = 0;i < 4;++i) {
			DEBUGPRINT(DEBUG_INFO, "b[i]: %d \n", b[i]);
		}
		DEBUGPRINT(DEBUG_INFO, "r  r r r%s\n", appInfo.rl);
		utils_btea(a, 4, b);
		for(i = 0;i < 4;++i) {
			DEBUGPRINT(DEBUG_INFO, "-a[i]: %d \n", a[i]);
		}
		head->len = sizeof(a) + 1;
		*(char *)(head + 1) = 1;
		memcpy((char *)(head + 1) + 1, a, sizeof(a));
		DEBUGPRINT(DEBUG_INFO, "head->len1: %d \n", head->len);
		DEBUGPRINT(DEBUG_INFO, "head->len2: %d \n", head->len);
		NewsUtils_sendToNet(usr->fd, head, sizeof(RequestHead) + head->len, 0);
		usr->verifyFlow = VERIFY_JOIN_FLOW;

		utils_btea(a, -4, b);
		for(i = 0;i < 4;++i) {
			DEBUGPRINT(DEBUG_INFO, "--a[i]: %d \n", a[i]);
		}
	}
	else {
		*(char *)(head + 1) = 2;
		head->len = 17;
		NewsUtils_sendToNet(usr->fd, head, sizeof(RequestHead) + head->len, 0);
		usr->verifyFlow = VERIFY_NULL_FLOW;
	}
	return FILTER_TRUE;
}

/**
   @brief 用户验证
   @author yangguozheng
   @param[in|out] self 过滤器运行环境
   @return FILTER_FALSE. FILTER_TRUE
   @note
 */
int NewsFilter_newsVerifyReq(void *self) {
	NewsChannel_t *context = (NewsChannel_t *)self;
	RequestHead *head = (RequestHead *)(context->rcvBuf);
	UsrChannel *usr = context->currentChannel;
	DEBUGPRINT(DEBUG_INFO, "_---------++++-------------------------------____ \n");
	DEBUGPRINT(DEBUG_INFO, "NewsFilter_newsVerifyReq %d\n", usr->verifyFlow);
	DEBUGPRINT(DEBUG_INFO, "_-----------------------++++-----------------____ \n");
	head->cmd = NEWS_USER_JOIN_REP;
	if(usr->verifyFlow == VERIFY_JOIN_FLOW) {
		if(0 == memcmp(head + 1, usr->verifyNumber, sizeof(usr->verifyNumber))) {
			*(char *)(head + 1) = 1;
			usr->verifyFlow = VERIFY_VERIFY_FLOW;
		}
		else {
			*(char *)(head + 1) = 0;
			usr->verifyFlow = VERIFY_JOIN_FLOW;
		}
		head->len = 1;
		NewsUtils_sendToNet(usr->fd, head, sizeof(RequestHead) + head->len, 0);
	}
	else {
		*(char *)(head + 1) = 0;
		head->len = 1;
		NewsUtils_sendToNet(usr->fd, head, sizeof(RequestHead) + head->len, 0);
	}
	return FILTER_TRUE;
}

/**
   @brief 通道声明
   @author yangguozheng
   @param[in|out] self 过滤器运行环境
   @return FILTER_FALSE. FILTER_TRUE
   @note
 */
int NewsFilter_newsChannelDeclaration(void *self) {
    NewsChannel_t *context = (NewsChannel_t *)self;
	RequestHead *head = (RequestHead *)context->rcvBuf;
	UsrChannel *usr = context->currentChannel;
    char *text = (char *)(head + 1);
#if _JOIN_VERIFY == 1
    DEBUGPRINT(DEBUG_INFO, "usr->verifyFlow: %d\n", usr->verifyFlow);
    if(usr->verifyFlow != VERIFY_VERIFY_FLOW) {
    	*(text + 4) = 1;
		*(text + 5) = 5;
		*(int *)(text + 6) = 0;
		head->len = 10;
		if(-1 == NewsUtils_sendToNet(usr->fd, head, sizeof(RequestHead) + head->len, 0)) {
			DEBUGPRINT(DEBUG_INFO, "send error \n");
		}
		return FILTER_TRUE;
	}

    if(appInfo.cameraMode == INFO_FACTORY_MODE || (INFO_HAS_R == hasRl())) {

	}
	else {
		if(*(text+8) != CONTROL_CHANNEL) {
			DEBUGPRINT(DEBUG_INFO, "usr->verifyFlow: %d\n", usr->verifyFlow);
			*(text + 4) = 1;
			*(text + 5) = 5;
			*(int *)(text + 6) = 0;
			head->len = 10;
			if(-1 == NewsUtils_sendToNet(usr->fd, head, sizeof(RequestHead) + head->len, 0)) {
				DEBUGPRINT(DEBUG_INFO, "send error \n");
			}
			return FILTER_TRUE;
		}
	}
#endif
    UsrChannel usr0;
    usr0.name = *(int *)(text+4);
    usr0.type = *(text+8);
    NewsChannel_channelAdd(context->usrChannel, 12, &usr0);
    context->currentChannel->name = *(int *)(text+4);
    context->currentChannel->type = *(text+8);

    char msgData[128];
	RequestHead *sHead = (RequestHead *)msgData;
	memcpy(sHead, head, sizeof(RequestHead));
	char *sText = (char *)(sHead + 1);

	sHead->cmd = NEWS_CHANNEL_DECLARATION_REP;
	*(sText + 4) = context->currentChannel->type;
	*(sText + 5) = 1;
	*(int *)(sText + 6) = 0;
	sHead->len = 10;
	if(-1 == NewsUtils_sendToNet(usr->fd, sHead, sizeof(RequestHead) + sHead->len, 0)) {
		DEBUGPRINT(DEBUG_INFO, "send error \n");
	}

    if(context->currentChannel->type == CONTROL_CHANNEL) {
        context->currentChannel->direction = SOCK_DIRECTION_IN_OUT;
    }
    else if(context->currentChannel->type == VIDEO_CHANNEL) {
        context->currentChannel->direction = SOCK_DIRECTION_OUT;
    }
    else if(context->currentChannel->type == AUDIO_CHANNEL) {
        context->currentChannel->direction = SOCK_DIRECTION_OUT;
    }

    DEBUGPRINT(DEBUG_INFO, "%d, %d, %d\n", *(int *)text, *(text + 4), *(text + 8));
	switch(*(text + 8)) {
		case CONTROL_CHANNEL:
			DEBUGPRINT(DEBUG_INFO, "%s, CONTROL_CHANNEL\n", __FUNCTION__);
			break;
		case VIDEO_CHANNEL:
			DEBUGPRINT(DEBUG_INFO, "%s, VIDEO_CHANNEL\n", __FUNCTION__);
			break;
		case AUDIO_CHANNEL:
			DEBUGPRINT(DEBUG_INFO, "%s, AUDIO_CHANNEL\n", __FUNCTION__);
			break;
	}
    return FILTER_NEXT;
}

/**
   @brief 心跳包请求
   @author yangguozheng
   @param[in|out] self 过滤器运行环境
   @return FILTER_FALSE. FILTER_TRUE
   @note
 */
int NewsFilter_newsChannelTouchReq(void *self) {
	NewsChannel_t *context = (NewsChannel_t *)self;
	RequestHead *head = (RequestHead *)context->rcvBuf;
	char *text = (char *)(head + 1);
    
    head->cmd = NEWS_CHANNEL_WRIGGLE_REP;
    memcpy(head->head, ICAMERA_NETWORK_HEAD, strlen(ICAMERA_NETWORK_HEAD));
    *(text + 4) = 1;
    *(text + 5) = 1;
    int retval =  NewsChannel_isChannelIn(context->usrChannel, 12, context->currentChannel->name, context->currentChannel->type);
    if(retval == -1) {
    	*(text + 5) = 2;
    }
    head->len = 6;
    get_wifi_signal_level(text + 6);
    head->len += 9;
    if(-1 == NewsUtils_sendToNet(context->currentChannel->fd, head, sizeof(RequestHead) + head->len, 0)) {
        DEBUGPRINT(DEBUG_ERROR, "%s\n", errstr);
    }
    DEBUGPRINT(DEBUG_INFO, "NEWS_CHANNEL_WRIGGLE_REP\n");

    time(&(context->currentChannel->mltime));
	return FILTER_TRUE;
}

/**
   @brief 心跳包请求2
   @author yangguozheng
   @param[in|out] self 过滤器运行环境
   @return FILTER_FALSE. FILTER_TRUE
   @note
 */
int NewsFilter_newsChannelTouchReqV2(void *self) {
	NewsChannel_t *context = (NewsChannel_t *)self;
	RequestHead *head = (RequestHead *)context->rcvBuf;
	char *text = (char *)(head + 1);

    head->cmd = NEWS_CHANNEL_WRIGGLE_REP_V2;
    memcpy(head->head, ICAMERA_NETWORK_HEAD, strlen(ICAMERA_NETWORK_HEAD));
    *(text + 4) = 1;
    *(text + 5) = 1;
    int retval =  NewsChannel_isChannelIn(context->usrChannel, 12, context->currentChannel->name, context->currentChannel->type);
    if(retval == -1) {
    	*(text + 5) = 2;
    }
    head->len = 6;
    get_wifi_signal_level(text + 6);
	head->len += 9;
    if(-1 == NewsUtils_sendToNet(context->currentChannel->fd, head, sizeof(RequestHead) + head->len, 0)) {
        DEBUGPRINT(DEBUG_ERROR, "%s\n", errstr);
    }
    DEBUGPRINT(DEBUG_INFO, "NEWS_CHANNEL_WRIGGLE_REP_V2\n");

    time(&(context->currentChannel->mltime));
	return FILTER_TRUE;
}

/**
   @brief 升级过程
   @author yangguozheng
   @param[in|out] self 过滤器运行环境
   @return FILTER_FALSE. FILTER_TRUE
   @note
 */
int NewsFilter_newsSysUpgradeProcessIn(void *self) {
	return FILTER_TRUE;
}

/**
   @brief 接收音频
   @author yangguozheng
   @param[in|out] self 过滤器运行环境
   @return FILTER_FALSE. FILTER_TRUE
   @note
 */
int NewsFilter_newsChannelAudioTransmitIn(void *self) {
    NewsChannel_t *context = (NewsChannel_t *)self;
	RequestHead *head = (RequestHead *)context->rcvBuf;
	char *text = (char *)(head + 1);
    if(context->currentChannel->direction != SOCK_DIRECTION_IN) {
        return FILTER_TRUE;
    }
    static struct timeval tm0, tm1;
    static int t = 0;
    if(t == 0) {
    	gettimeofday(&tm0, NULL);
    	t = 1;
    }
    gettimeofday(&tm1, NULL);

    if(tm1.tv_sec * 1000000 + tm1.tv_usec - tm0.tv_sec * 1000000 - tm0.tv_usec > 6000000) {
		gettimeofday(&tm0, NULL);
		//system("date > /tmp/speakerInfo");
		DEBUGPRINT(DEBUG_INFO, "print time to file\n");
	}
//    DEBUGPRINT(DEBUG_INFO, "PlaySpeaker pack index %d\n", *(int *)(text + 8));
    if(-1 == PlaySpeaker(text + 17)) {
//			DEBUGPRINT(DEBUG_INFO, "PlaySpeaker error\n");
    }
//    DEBUGPRINT(DEBUG_INFO, "PlaySpeaker\n");
	return FILTER_TRUE;
}

/**
   @brief 升级确认
   @author yangguozheng
   @param[in|out] self 过滤器运行环境
   @return FILTER_FALSE. FILTER_TRUE
   @note
 */
int NewsFilter_newsChannelHitPig(void *self) {
	NewsChannel_t *context = (NewsChannel_t *)self;
	RequestHead *head = (RequestHead *)context->rcvBuf;
	char *text = (char *)(head + 1);

	head->cmd = NEWS_SYS_BOMB_BY_PIG;
	memcpy(head->head, ICAMERA_NETWORK_HEAD, strlen(ICAMERA_NETWORK_HEAD));
//	*(text + 4) = 1;
//	*(text + 5) = 1;
//	int retval =  NewsChannel_isChannelIn(context->usrChannel, 12, context->currentChannel->name, context->currentChannel->type);
//	if(retval == -1) {
//		*(text + 5) = 2;
//	}
	head->len = 0;
	if(-1 == NewsUtils_sendToNet(context->currentChannel->fd, head, sizeof(RequestHead) + head->len, 0)) {
		DEBUGPRINT(DEBUG_ERROR, "%s\n", errstr);
	}
#if 10
	utils_bombByPig(1);
#endif
	DEBUGPRINT(DEBUG_INFO, "NEWS_SYS_BOMB_BY_PIG\n");
	return FILTER_TRUE;
}

/**
   @brief 音频请求
   @author yangguozheng
   @param[in|out] self 过滤器运行环境
   @return FILTER_FALSE. FILTER_TRUE
   @note
 */
int NewsFilter_newsChannelAudioReq(void *self) {
	DEBUGPRINT(DEBUG_INFO, "NewsFilter_newsChannelAudioReq\n");
    NewsChannel_t *context = (NewsChannel_t *)self;
    msg_container *container = (msg_container *)&context->container;
	MsgData *msgSnd = (MsgData *)container->msgdata_snd;
    
	RequestHead *head = (RequestHead *)context->rcvBuf;
    char *text = (char *)(head + 1);
    int audioFd;
    char dir;
    int On = 1, Off = 2;
    
    DEBUGPRINT(DEBUG_INFO, "---------audio: %d, ----------speak: %d\n", *(text + 4), *(text + 5));
    printf("---------audio: %d, ----------speak: %d\n", *(text + 4), *(text + 5));
    
    if(*(text + 5) == 1) {
//    	system("date > /tmp/speakerSwitch");
    	DEBUGPRINT(DEBUG_INFO, "speakerSwitch\n");
		if(NULL_FD != NewsChannel_usrInfoGetSpeakerByName(context->usrChannel, MAX_USER_NUM * 3, context->currentChannel)) {
			head->cmd = NEWS_AUDIO_TRANSMIT_REP;
			head->version = 1;
			head->len = 6;
			//*(int *)text = 0;
			*(text + 4) = Off; //audio
			*(text + 5) = On;  //speaker
			//*(int *)(text + 6) = 0;
			//DEBUGPRINT(DEBUG_INFO, "head->cmd: %d\n", head->cmd);
			if(-1 == NewsUtils_sendToNet(context->currentChannel->fd, head, sizeof(RequestHead) + head->len, 0)) {
				DEBUGPRINT(DEBUG_INFO, "send error \n");
			}
			return FILTER_TRUE;
		}
    	else if(INFO_SPEAKER_ON == NewsUtils_getAppInfo(INFO_SPEAKER, 0, 0)) {
    		head->cmd = NEWS_AUDIO_TRANSMIT_REP;
			head->version = 1;
			head->len = 6;
			//*(int *)text = 0;
			*(text + 4) = On; //audio
			*(text + 5) = Off;  //speaker
			//*(int *)(text + 6) = 0;
			//DEBUGPRINT(DEBUG_INFO, "head->cmd: %d\n", head->cmd);
			if(-1 == NewsUtils_sendToNet(context->currentChannel->fd, head, sizeof(RequestHead) + head->len, 0)) {
				DEBUGPRINT(DEBUG_INFO, "send error \n");
			}
			return FILTER_TRUE;
    	}
#if 0
        audioFd = NewsChannel_usrInfoAudioPlayerIn(context->usrChannel, MAX_USER_NUM * 3, context->currentChannel);
        if(audioFd != NULL_FD) {
        	head->cmd = NEWS_AUDIO_TRANSMIT_REP;
			head->version = 1;
			head->len = 6;
			//*(int *)text = 0;
			*(text + 4) = 2;
			*(text + 5) = 2;
			//*(int *)(text + 6) = 0;
			//DEBUGPRINT(DEBUG_INFO, "head->cmd: %d\n", head->cmd);
			if(-1 == NewsUtils_sendToNet(context->currentChannel->fd, head, sizeof(RequestHead) + head->len, 0)) {
				DEBUGPRINT(DEBUG_INFO, "send error \n");
			}
            return FILTER_TRUE;
        }
#endif

        //system("date > /tmp/speakerSwitchin");
        NewsUtils_setAppInfo(INFO_SPEAKER, INFO_SPEAKER_ON , container->lmsgid);
        dir = SOCK_DIRECTION_IN;
        DEBUGPRINT(DEBUG_INFO, "SOCK_DIRECTION_IN\n");
    }
    else {
    	//system("date > /tmp/audioSwitch");
    	DEBUGPRINT(DEBUG_INFO, "audioSwitch\n");
        dir = SOCK_DIRECTION_OUT;
        NewsUtils_setAppInfo(INFO_SPEAKER, INFO_SPEAKER_OFF, container->lmsgid);
    }
    
    
    msgSnd->type = NEWS_CHANNEL_MSG_TYPE;
    msgSnd->cmd = MSG_NEWS_CHANNEL_P_CHANNEL_REQ;
    msgSnd->len = sizeof(RequestHead) + head->len + 2 * sizeof(int);
    memcpy(msgSnd + 1, &context->currentChannel->fd, sizeof(int));
    memcpy((char *)(msgSnd + 1) + sizeof(int), head, sizeof(RequestHead) + head->len);
    audioFd = NewsChannel_usrInfoAudioIn(context->usrChannel, MAX_USER_NUM * 3, context->currentChannel, dir);
    if(audioFd == -1) {
        DEBUGPRINT(DEBUG_ERROR, "+++>audioFd == -1\n");
        memcpy(head->head, ICAMERA_NETWORK_HEAD, strlen(ICAMERA_NETWORK_HEAD));
        head->cmd = NEWS_AUDIO_TRANSMIT_REP;
        head->version = 1;
        head->len = 6;
		//*(int *)text = 0;
        *(text + 4) = On;  //audio
        *(text + 5) = Off; //speaker
		//*(int *)(text + 6) = 0;
        //DEBUGPRINT(DEBUG_INFO, "head->cmd: %d\n", head->cmd);
        if(-1 == NewsUtils_sendToNet(context->currentChannel->fd, head, sizeof(RequestHead) + head->len, 0)) {
            DEBUGPRINT(DEBUG_INFO, "send error \n");
        }
        return 0;
    }
    memcpy((char *)(msgSnd + 1) + sizeof(int) + sizeof(RequestHead) + head->len, &audioFd, sizeof(int));
    msg_queue_snd(container->rmsgid, msgSnd, sizeof(MsgData) - sizeof(long) + msgSnd->len);
    DEBUGPRINT(DEBUG_INFO, "audioFd: %d\n", audioFd);

#if 0
    if(*(text + 5) == 1) {
    	if(audioFd != -1 && context->speakerFd == NULL_FD && context->speakerThread == NULL_THREAD) {
    		context->speakerFd = audioFd;
			context->speakerThread = ithread_start(NewsWork_run, context->speakerFd);
			DEBUGPRINT(DEBUG_INFO, "ithread_start(NewsWork_run, context->speakerFd)\n");
    	}
    }
    else {
    	if(audioFd != -1 && context->speakerFd != NULL_FD && context->speakerThread != NULL_THREAD) {
			pthread_cancel(context->speakerThread);
			context->speakerThread = NULL_THREAD;
			context->speakerFd = NULL_FD;
			DEBUGPRINT(DEBUG_INFO, "cancel ithread_start(NewsWork_run, context->speakerFd)\n");
		}
    }
#endif
    return FILTER_TRUE;
}

/** 升级每包大小 */
#define IMG_PACK_SIZE (23 + 64 * 1024 + 8)
/**
   @brief 升级请求以及整个升级过程
   @author yangguozheng
   @param[in|out] self 过滤器运行环境
   @return FILTER_FALSE. FILTER_TRUE
   @note
 */
int NewsFilter_newsSysUpgradeReq(void *self) {
	DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	DEBUGPRINT(DEBUG_INFO, "++++++++++++++NewsFilter_newsSysUpgradeReq++++++++++++++\n");
	DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

	NewsChannel_t *context = (NewsChannel_t *)self;
    msg_container *container = (msg_container *)&context->container;

	RequestHead *rHead = (RequestHead *)context->rcvBuf;
    char *text = (char *)(rHead + 1);

	UsrChannel *head = context->currentChannel;
	int retval = 0;
	int imgSize = 0;
	int preImgSize = 0;
	int lastPackSize = 0;
	int lastPackNum = 0;
	int cIndex = 0;
	int downLoadFlag = 1;
	int cancelUpgrade = 0;
	char *imgBuf = (char *)malloc(IMG_PACK_SIZE);
	int reqSuccess = 2;
	DEBUGPRINT(DEBUG_INFO, "------------VERIFY_VERIFY_FLOW %d\n " , head->verifyFlow);
#if _JOIN_VERIFY == 1
	if(head->verifyFlow != VERIFY_VERIFY_FLOW) {
		reqSuccess = 1;
	}
#endif
	if(imgBuf == NULL) {
		reqSuccess = 1;
//		return 0;
	}

	FILE *file = fopen("/usr/share/img", "w");

	if(file == NULL) {
		reqSuccess = 1;
//		return 0;
	}

	memcpy(rHead->head, ICAMERA_NETWORK_HEAD, strlen(ICAMERA_NETWORK_HEAD));
	rHead->cmd = NEWS_SYS_UPGRADE_REP;
	rHead->version = 1;
	rHead->len = 5;
	*(text + 4) = reqSuccess;
	if(-1 == NewsUtils_sendToNet(context->currentChannel->fd, rHead, sizeof(RequestHead) + rHead->len, 0)) {
		DEBUGPRINT(DEBUG_INFO, "send error \n");
	}

	if(reqSuccess == 1) {
		if(file != NULL) {
			fclose(file);
		}
		return 0;
	}

	while(downLoadFlag) {
		imgSize = 0;
		rHead = (RequestHead *)imgBuf;
		text = (char *)(rHead + 1);

		while(imgSize < sizeof(RequestHead)) {
			retval = recv(head->fd, imgBuf + imgSize, sizeof(RequestHead) - imgSize, 0);
			if(-1 == NewsChannel_sockError(retval)) {
				NewsChannel_channelClose(head);
				downLoadFlag = 0;
				goto downloadLabel;
			}
			else if(-2 == NewsChannel_sockError(retval)) {
				continue;
			}
			imgSize += retval;
		}

		if(rHead->cmd == 248) {
			log_b(rHead, 100);
			cancelUpgrade = 1;
			break;
		}

		log_b(rHead, 100);

		if(rHead->cmd == 242) {
//			log_b(rHead, 100);
			memcpy(rHead->head, ICAMERA_NETWORK_HEAD, strlen(ICAMERA_NETWORK_HEAD));
			rHead->cmd = 243;
			rHead->version = 1;
			rHead->len = 5;
			*(text + 4) = 2;
			if(-1 == NewsUtils_sendToNet(head->fd, rHead, sizeof(RequestHead) + rHead->len, 0)) {
				DEBUGPRINT(DEBUG_INFO, "send error \n");
			}
			retval = 0;
			imgSize = 0;
			preImgSize = 0;
			lastPackSize = 0;
			lastPackNum = 0;
			cIndex = 0;
			cancelUpgrade = 0;
			continue;
		}

		while(imgSize < sizeof(RequestHead) + 8) {
			retval = recv(head->fd, imgBuf + imgSize, sizeof(RequestHead) + 8 - imgSize, 0);
			if(-1 == NewsChannel_sockError(retval)) {
				NewsChannel_channelClose(head);
				downLoadFlag = 0;
				goto downloadLabel;
			}
			else if(-2 == NewsChannel_sockError(retval)) {
				continue;
			}
			imgSize += retval;
		}

		DEBUGPRINT(DEBUG_INFO, "-----%d\n", *(int *)(imgBuf + 15));
		lastPackSize = rHead->len + 23;
		lastPackNum = *(int *)(text + 4);
		log_printf("upgrade lastPackNum: %d\n", lastPackNum);


		while(imgSize < lastPackSize) {
			retval = recv(head->fd, imgBuf + imgSize, lastPackSize - imgSize, 0);
			if(-1 == NewsChannel_sockError(retval)) {
				NewsChannel_channelClose(head);
				downLoadFlag = 0;
				goto downloadLabel;
			}
			else if(-2 == NewsChannel_sockError(retval)) {
				continue;
			}
			imgSize += retval;
			DEBUGPRINT(DEBUG_INFO, "----------------imgSize < lastPackSize--------------%d--%d\n", imgSize , lastPackSize);
		}

		log_b(rHead, 100);

		if(1 != fwrite(imgBuf + 8  + 23, lastPackSize - 23 - 8, 1, file)) {
			memcpy(rHead->head, ICAMERA_NETWORK_HEAD, strlen(ICAMERA_NETWORK_HEAD));
		    rHead->cmd = NEWS_SYS_UPGRADE_PROCESS_OUT;
		    rHead->version = 1;
		    rHead->len = 5;
		    *(text + 4) = 2;
		    if(-1 == NewsUtils_sendToNet(head->fd, rHead, sizeof(RequestHead) + rHead->len, 0)) {
		        DEBUGPRINT(DEBUG_INFO, "send error \n");
		    }
			continue;
		}
		DEBUGPRINT(DEBUG_INFO, "----------------after write %d--------------\n", cIndex);

		memcpy(rHead->head, ICAMERA_NETWORK_HEAD, strlen(ICAMERA_NETWORK_HEAD));
	    rHead->cmd = NEWS_SYS_UPGRADE_PROCESS_OUT;
	    rHead->version = 1;
	    rHead->len = 5;
	    *(text + 4) = 1;
	    if(-1 == NewsUtils_sendToNet(head->fd, rHead, sizeof(RequestHead) + rHead->len, 0)) {
	        DEBUGPRINT(DEBUG_INFO, "send error \n");
	    }

		if(lastPackNum == 0) {
			DEBUGPRINT(DEBUG_INFO, "----------------lastPackNum 0 break--------------\n");
			cancelUpgrade = 2;
			break;
		}
		DEBUGPRINT(DEBUG_INFO, "----------------lastPackNum: %d--------------\n", lastPackNum);

		++cIndex;
	}
	downloadLabel:
	fclose(file);
	if(cancelUpgrade == 2) {
		//system("ls -l /usr/share/img");
#if 1
		system("cp /usr/share/img /dev/mtdblock4;nvram_set uboot Image1Stable 0;nvram_set uboot Image1Try 0");
#else
		FILE *sysFile = popen("cp /usr/share/img /dev/mtdblock4;nvram_set uboot Image1Stable 0;nvram_set uboot Image1Try 0", "r");
		if(sysFile != NULL) {
			fclose(sysFile);
		}
#endif
		
		memcpy(rHead->head, ICAMERA_NETWORK_HEAD, strlen(ICAMERA_NETWORK_HEAD));
		rHead->cmd = NEWS_SYS_UPGRADE_OVER;
		rHead->version = 1;
		rHead->len = 5;
		*(text + 4) = 1;
		if(-1 == NewsUtils_sendToNet(head->fd, rHead, sizeof(RequestHead) + rHead->len, 0)) {
			DEBUGPRINT(DEBUG_INFO, "send error \n");
		}
		DEBUGPRINT(DEBUG_INFO, "----------------NewsChannel_systemUpgrade success--------------\n");
	}
	else if(cancelUpgrade == 1){
		memcpy(rHead->head, ICAMERA_NETWORK_HEAD, strlen(ICAMERA_NETWORK_HEAD));
		rHead->cmd = 249;
		rHead->version = 1;
		rHead->len = 0;
		if(-1 == NewsUtils_sendToNet(head->fd, rHead, sizeof(RequestHead) + rHead->len, 0)) {
			DEBUGPRINT(DEBUG_INFO, "send error \n");
		}
		DEBUGPRINT(DEBUG_INFO, "----------------NewsChannel_systemUpgrade canceled--------------\n");
	}
	free(imgBuf);
//	head->rBufSize = 0;
	time(&(head->mltime));
	DEBUGPRINT(DEBUG_INFO, "----------------NewsChannel_systemUpgrade over--------------\n");
	return FILTER_TRUE;
}

/**
   @brief 通道声明未使用
   @author yangguozheng
   @param[in|out] self 过滤器运行环境
   @return FILTER_FALSE. FILTER_TRUE
   @note
 */
int NewsFilter_newsChannelDeclarationReq(void *self) {
	NewsChannel_t *context = (NewsChannel_t *)self;
	msg_container *container = (msg_container *)&context->container;

	RequestHead *rHead = (RequestHead *)context->rcvBuf;
	char *text = (char *)(rHead + 1);
	UsrChannel *head = context->currentChannel;
//	DEBUGPRINT(DEBUG_INFO, "NewsAdapter_newsChannelDeclarationReq %d\n", *(text + sizeof(int) + 1 + 1));
	DEBUGPRINT(DEBUG_INFO, "%d, %d, %d\n", *(int *)text, *(text + 4), *(text + 8));
	switch(*(text + 8)) {
		case CONTROL_CHANNEL:
			DEBUGPRINT(DEBUG_INFO, "CONTROL_CHANNEL\n");
			break;
		case VIDEO_CHANNEL:
			DEBUGPRINT(DEBUG_INFO, "VIDEO_CHANNEL\n");
			break;
		case AUDIO_CHANNEL:
			DEBUGPRINT(DEBUG_INFO, "AUDIO_CHANNEL\n");
			break;
	}
	return FILTER_NEXT;
}

/**
   @brief 系统重启
   @author yangguozheng
   @param[in|out] self 过滤器运行环境
   @return FILTER_FALSE. FILTER_TRUE
   @note
 */
int NewsFilter_newsSysRebootReq(void *self) {
	NewsChannel_t *context = (NewsChannel_t *)self;
	msg_container *container = (msg_container *)&context->container;

	RequestHead *rHead = (RequestHead *)context->rcvBuf;
	UsrChannel *head = context->currentChannel;
	char *text = (char *)(rHead + 1);

	memcpy(rHead->head, ICAMERA_NETWORK_HEAD, strlen(ICAMERA_NETWORK_HEAD));
	rHead->cmd = NEWS_SYS_REBOOT_REP;
	rHead->version = 1;
	rHead->len = 5;
	*(text + 4) = 1;
	if(-1 == NewsUtils_sendToNet(head->fd, rHead, sizeof(RequestHead) + rHead->len, 0)) {
		DEBUGPRINT(DEBUG_INFO, "send error \n");
	}
	DEBUGPRINT(DEBUG_INFO, "reboot \n");
	if(appInfo.cameraMode == INFO_FACTORY_MODE && appInfo.cameraType == INFO_M3S) {
		DEBUGPRINT(DEBUG_INFO, "serialport_user_mode\n");
//		serialport_user_mode();
		serialport_foctory_mode(1);
	}
	serialport_sendAuthenticationFailure();
	system("reboot");

	return FILTER_TRUE;
}

/**
   @brief 夜间模式请求
   @author yangguozheng
   @param[in|out] self 过滤器运行环境
   @return FILTER_FALSE. FILTER_TRUE
   @note
 */
int NewsFilter_nightModeSetReq(void *self) {
	NewsChannel_t *context = (NewsChannel_t *)self;
    msg_container *container = (msg_container *)&context->container;
	RequestHead *rHead = (RequestHead *)context->rcvBuf;
    UsrChannel *channel = context->currentChannel;
    char *text = (char *)(rHead + 1);

    DEBUGPRINT(DEBUG_INFO, "NewsFilter_nightModeSetReq ---------------------- %d 1100\n",*((int *)(text+5)));

    if (*((char *)(text + 4)) == NIGHT_MODE_ON) {
    	if(runtimeinfo_get_nm_state() == NIGHT_MODE_OFF){
			runtimeinfo_set_nm_time(*((int *)(text+5)));
			if(g_nDeviceType == DEVICE_TYPE_M2){
				DEBUGPRINT(DEBUG_INFO, "NewsFilter_nightModeSetReq --------M2ON-------------- 1100\n");
				led_if_work_control(LED_TYPE_NET_POWER, LED_FLAG_NOT_WORK, FALSE);
				led_if_work_control(LED_TYPE_NIGHT, LED_FLAG_NOT_WORK, FALSE);
			}else{
				led_if_work_control(LED_TYPE_NET, LED_FLAG_NOT_WORK, FALSE);
				led_if_work_control(LED_TYPE_POWER, LED_FLAG_NOT_WORK, FALSE);
				led_if_work_control(LED_TYPE_NIGHT, LED_FLAG_NOT_WORK, FALSE);
			}
    	}
    }
    else if (*((char *)(text + 4)) == NIGHT_MODE_OFF)  {
    	runtimeinfo_set_nm_time(0);
    	if(g_nDeviceType == DEVICE_TYPE_M2){
    		DEBUGPRINT(DEBUG_INFO, "NewsFilter_nightModeSetReq --------M2OFF-------------- 1100\n");
			led_if_work_control(LED_TYPE_NET_POWER, LED_FLAG_WORK, FALSE);
			led_if_work_control(LED_TYPE_NIGHT, LED_FLAG_WORK, FALSE);
		}else{
			led_if_work_control(LED_TYPE_NET, LED_FLAG_WORK, FALSE);
			led_if_work_control(LED_TYPE_POWER, LED_FLAG_WORK, FALSE);
			led_if_work_control(LED_TYPE_NIGHT, LED_FLAG_WORK, FALSE);
		}
    }
    runtimeinfo_set_nm_state(*(text+4));
	NewsChannel_nightModeAction(runtimeinfo_get_nm_state());
	tutk_send_set_nightmode_command(runtimeinfo_get_nm_state(), runtimeinfo_get_nm_time());

    return FILTER_TRUE;
}

/**
   @brief 夜间模式记录请求
   @author yangguozheng
   @param[in|out] self 过滤器运行环境
   @return FILTER_FALSE. FILTER_TRUE
   @note
 */
int NewsFilter_nightModeRecordReq(void *self) {
	DEBUGPRINT(DEBUG_INFO, "NewsFilter_nightModeRecordReq ---------------------- 1102\n");
    NewsChannel_t *context = (NewsChannel_t *)self;
    msg_container *container = (msg_container *)&context->container;
	RequestHead *rHead = (RequestHead *)context->rcvBuf;
    UsrChannel *channel = context->currentChannel;
    char *text = (char *)(rHead + 1);
    int retval;
    int nightStartTime;
    //retval = NewsUtils_getRunTimeState(0, NULL);
    //NewsUtils_getRunTimeState(INFO_NIGHT_MODE_TIME, &nightStartTime);
    rHead->cmd = NEWS_NIGHT_MODE_RECORD_REP;
    *(text + 4) = runtimeinfo_get_nm_state();
    *(int *)(text + 5) = runtimeinfo_get_nm_time();
    rHead->len = 9;
    NewsUtils_sendToNet(channel->fd, rHead, sizeof(RequestHead) + rHead->len, 0);
    return FILTER_TRUE;
}

/**
   @brief wifi信号质量请求
   @author yangguozheng
   @param[in|out] self 过滤器运行环境 
   @return FILTER_FALSE. FILTER_TRUE
   @note
 */
int NewsFilter_wifiQualityReq(void *self) {
	DEBUGPRINT(DEBUG_INFO, "NewsFilter_wifiQualityReq ---------------------- 1200\n");
    NewsChannel_t *context = (NewsChannel_t *)self;
    msg_container *container = (msg_container *)&context->container;
	RequestHead *rHead = (RequestHead *)context->rcvBuf;
    UsrChannel *channel = context->currentChannel;
    char *text = (char *)(rHead + 1);
    channel->wifiTime.state = *(text + 4);
    //time(&(channel->wifiTime.sTime));
    channel->wifiTime.sTime = 0;
    channel->wifiTime.step = *(int *)(text + 5);
    return FILTER_TRUE;
}

/**
   @brief 通道开关请求
   @author yangguozheng
   @param[in|out] self 过滤器运行环境
   @return FILTER_FALSE. FILTER_TRUE
   @note
 */
int NewsFilter_channelSwitchReq(void *self) {
    NewsChannel_t *context = (NewsChannel_t *)self;
    msg_container *container = (msg_container *)&context->container;
	MsgData *msgSnd = (MsgData *)container->msgdata_snd;
    
	RequestHead *rHead = (RequestHead *)context->rcvBuf;
	rHead->cmd = NEWS_CHANNEL_SWITCH_REP;
    char *text = (char *)(rHead + 1);
    int On = 1, Off = 2, Ok = 1;
    if(*(text + 4 + 1) == On) {
    	NewsChannel_upgradeVideoChannel(context->usrChannel, MAX_USER_NUM * 3, context->currentChannel, SOCK_DIRECTION_OUT);
    }
    else if(*(text + 4 + 1) == Off) {
    	NewsChannel_upgradeVideoChannel(context->usrChannel, MAX_USER_NUM * 3, context->currentChannel, SOCK_DIRECTION_NULL);
    }
    *(text + 5) = Ok;
    rHead->len = 6;
    NewsUtils_sendToNet(context->currentChannel->fd, rHead, sizeof(RequestHead) + rHead->len, 0);
    return FILTER_TRUE;
}

