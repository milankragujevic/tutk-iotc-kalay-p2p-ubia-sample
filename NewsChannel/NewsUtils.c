/**	
   @file NewsUtils.c
   @brief 应用程序主文件
   @note
   author: yangguozheng
   date: 2013-09-16
   mcdify record: creat this file.
   author: yangguozheng
   date: 2013-09-17
   modify record: add a function
 */
#include "NewsUtils.h"
#include "common/logfile.h"
#include "common/common.h"
#include "common/common_func.h"
#include <regex.h>
#include <signal.h>
#include <string.h>
#include "HTTPServer/httpsserver.h"
#include "common/common_config.h"
#include "AudioAlarm/audioalarm.h"
#include "NewsChannel.h"
#define NULL_FD 0


int NewsUtils_addSockFd(int *list, int len, int fd);
int NewsUtils_rmSockFd(int *list, int len, int fd);
int NewsUtils_sockFdIn(int *list, int len, int fd);

int setRl(char *rl, int size);
int setCameraMode(char mode);
int initAppInfo();
int getCameraType();

int NewsUtils_getAppInfo(int key, int value, int args);
int NewsUtils_setAppInfo(int key, int value, int args);

int NewsUtils_sendToNet(int fd, void *buf, int len, int flag);

/**
   @brief 向描述列表中添加描述
   @author yangguozheng
   @param[in|out] list 通道列表
   @param[in] len 类表长度
   @param[in] fd 套接字描述符
   @return 0
   @note
 */
int NewsUtils_addSockFd(int *list, int len, int fd) {
	DEBUGPRINT(DEBUG_INFO, "+++++>News Utils add fd %d", fd);
	if(-1 != NewsUtils_sockFdIn(list, len, fd)) {
        return -1;
    }
    int *head = list;
	int *tail = list + len;
	int i = 0;
	for(;head != tail;++head, ++i) {
		if(*head == NULL_FD) {
			*head = fd;
			return i;
		}
	}
	return -1;
}

/**
   @brief 查找描述列表中是否存在特定的描述
   @author yangguozheng
   @param[in|out] list 通道列表
   @param[in] len 类表长度
   @param[in] fd 套接字描述符
   @return 0
   @note
 */
int NewsUtils_sockFdIn(int *list, int len, int fd) {
	int *head = list;
	int *tail = list + len;
	int i = 0;
	for(;head != tail;++head, ++i) {
		if(*head == fd) {
			return i;
		}
	}
	return -1;
}

/**
   @brief 删除描述列表中是否存在特定的描述
   @author yangguozheng
   @param[in|out] list 通道列表
   @param[in] len 类表长度
   @param[in] fd 套接字描述符
   @return 0
   @note
 */
int NewsUtils_rmSockFd(int *list, int len, int fd) {
	int *head = list;
	int *tail = list + len;
	int i = 0;
	for(;head != tail;++head, ++i) {
		if(*head == fd) {
			*head = NULL_FD;
			return i;
		}
	}
	return -1;
}

/**
   @brief 删除描述列表中是否存在特定的描述
   @author yangguozheng
   @param[in] fd 套接字描述符
   @param[in] buf 缓冲区指针
   @param[in] flag 可选参数
   @return write
   @note
 */
int NewsUtils_sendToNet(int fd, void *buf, int len, int flag) {
    //struct sockaddr addr;
	//socklen_t addrlength;
	int ret = 0;
#if 0
	if(-1 == getpeername(fd, &addr, &addrlength)) {
		DEBUGPRINT(DEBUG_ERROR, "error fd %s\n", strerror(errno));
		return -1;
	}
#endif
	RequestHead *head = (RequestHead *)buf;
//	char *text = (char *)(head + 1);
	int *tcpData = (int *)(head + 1);
	log_b(head, 100);
	LOG_TEST("NewsChannel time: %08X, tcp send [cmd: %d, len: %d, %08X, %08X, %08X, %08X, %08X, %08X, %08X]\n", time(NULL), head->cmd, head->len, *tcpData, *(tcpData + 1), *(tcpData + 2), *(tcpData + 3), *(tcpData + 4), *(tcpData + 5), *(tcpData + 6));
    ret = write(fd, buf, len);
//    log_b(head, 100);
    log_printf("tcp write ret: %d cmd: %d\n", ret, head->cmd);
    return ret;
}

/**
   @brief 设置rl
   @author yangguozheng
   @param[in] rl rl指针
   @param[in] size rl长度
   @return 0: 成功, -1: 失败
   @note
 */
int setRl(char *rl, int size) {
	memset(appInfo.rl, '\0', sizeof(appInfo.rl));
	if(size > 256) {
		return -1;
	}
    memcpy(appInfo.rl, rl, size);
    return 0;
}

/** @brief tcp认证项 */
#define INFO_MODE_TCP       '1'
/** @brief tcp认证模式 */
#define INFO_MODE_TCP_AUTH  '1'
/** @brief 认证key长度 */
#define INFO_KEY_LEN         16

/**
   @brief 检测是否有Rl
   @author yangguozheng
   @return INFO_HAS_R, INFO_NO_R
   @note
 */
int hasRl() {
	conncheck *head = user_pm;
	conncheck *tail = user_pm + 3;
	for(;head != tail;++head) {
		if(head->Cam_type[0] == INFO_MODE_TCP && head->Cam_mode == INFO_MODE_TCP_AUTH) {
			bzero(appInfo.rl, sizeof(appInfo.rl));
			memcpy(appInfo.rl, head->Cam_key, INFO_KEY_LEN);
			return INFO_HAS_R;
		}
	}
	tutk_send_https_check_msg();
	return INFO_NO_R;
#if 0
	if(strnlen(appInfo.rl, sizeof(appInfo.rl) - 1) == R_LEN) {
		printf("y y y y r r r r r r r\n");
		return INFO_HAS_R;
	}
	printf("nonono rr rr  rr %d\n", strnlen(appInfo.rl, sizeof(appInfo.rl) - 1));
	return INFO_NO_R;
#endif
}

/**
   @brief 设置摄像头状态
   @author yangguozheng
   @param[in] mode 摄像头进入的模式 INFO_FACTORY_MODE, INFO_USER_MODE
   @return 0
   @note
 */
int setCameraMode(char mode) {
	FILE *pFile = NULL;
	appInfo.cameraMode = mode;
//	FILE *file = fopen("/dev/mtdblock3", "wb");
//	if(file == NULL) {
//		return -1;
//	}
//	if(fseek(file, CAMERA_MODE_OFFSET, SEEK_SET) != 0) {
//		fclose(file);
//		return -1;
//	}
////	if(fread(&(appInfo.cameraMode), 1, 1, file) != 1) {
////		fclose(file);
////		return -1;
////	}
//	if(fwrite(&(mode), 1, 1, file) != 1) {
//		fclose(file);
//		return -1;
//	}
//	fclose(file);
	if(appInfo.cameraMode == INFO_FACTORY_MODE) {
		//system("nvram_set deviceState f");
        pFile = popen("nvram_set deviceState f", "r");
        if (NULL != pFile)
        {
        	char acTmpBuf[128] = {0};

        	fread(acTmpBuf, 1, 128, pFile);
        	pclose(pFile);
        	pFile = NULL;
        }
		printf("fffffffffffffffffffff\n");
	}
	else {
		//system("nvram_set deviceState u");
		pFile = popen("nvram_set deviceState u;ralink_init renew 2860 /usr/share/default_config", "r");
	    if (NULL != pFile)
	    {
	    	char acTmpBuf[128] = {0};

	    	fread(acTmpBuf, 1, 128, pFile);
	    	pclose(pFile);
	    	pFile = NULL;
	    }
		printf("uuuuuuuuuuuuuuuuuuuuu\n");
        
	}

	appInitLed(1);
	appInitMotor(1);
    return 0;
}

/**
   @brief 信号处理函数
   @author yangguozheng
   @param[in] signo 信号标志
   @return 0
   @note
 */
int signal_handle(int signo) {
//    printf("hug hug signal_handle: %d\n", signo);
    return 0;
}

/**
   @brief 初始化应用信息
   @author yangguozheng
   @return 0
   @note
 */
int initAppInfo() {
//	FILE *file = fopen("/dev/mtdblock3", "rb");
//	if(file == NULL) {
//		return -1;
//	}
//	if(fseek(file, CAMERA_MODE_OFFSET, SEEK_SET) != 0) {
//		fclose(file);
//		return -1;
//	}
//	if(fread(&(appInfo.cameraMode), 1, 1, file) != 1) {
//		fclose(file);
//		return -1;
//	}
////	if(fwrite(keyNE, sizeNE, 1, file) != 1) {
////		fclose(file);
////		return -1;
////	}
//	fclose(file);
	char buf[2] = {'\0'};
	FILE *file = popen("nvram_get deviceState", "r");

	if(file != NULL) {
		fgets(buf, 2, file);
		pclose(file);
	}

	if(buf[0] == 'f') {
		appInfo.cameraMode = INFO_FACTORY_MODE;
		printf("ffffffffffffffffff000\n");
	}
	else {
		appInfo.cameraMode = INFO_USER_MODE;
		printf("uuuuuuuuuuuuuuuuuuuuu000\n");
	}

//	file = fopen("/usr/share/info.txt", "r");
//	char buf2[512] = {'\0'};
//	if(file != NULL) {
//		while(NULL != fgets(buf2, sizeof(buf2), file)) {
//			strncmp(buf2);
//		}
//	}

	getCameraType();

    if(SIG_ERR == signal(SIGPIPE, signal_handle)) {
    	DEBUGPRINT(DEBUG_ERROR, "SIGPIPE register error\n");
    }
    if(SIG_ERR == signal(SIGUSR1, signal_handle)) {
		DEBUGPRINT(DEBUG_ERROR, "SIGUSR1 register error\n");
	}
    if(SIG_ERR == signal(SIGTERM, signal_handle)) {
		DEBUGPRINT(DEBUG_ERROR, "SIGTERM register error\n");
	}
    if(SIG_ERR == signal(SIGINT, signal_handle)) {
		DEBUGPRINT(DEBUG_ERROR, "SIGINT register error\n");
	}
    if(SIG_ERR == signal(SIGCHLD, SIG_IGN)) {
		DEBUGPRINT(DEBUG_ERROR, "SIGCHLD register error\n");
	}
    
    pthread_mutex_init(&appInfo.mutex, NULL);
    appInfo.speakerInfo = INFO_SPEAKER_OFF;
	return 0;
}

/**
   @brief 初始化灯的状态
   @author yangguozheng
   @param[in] flag 可选参数
   @return 0
   @note
 */
int appInitLed(int flag) {
#if 10
	if(appInfo.cameraMode == INFO_FACTORY_MODE && appInfo.cameraType == INFO_M2) {
		led_control(LED_TYPE_NET_POWER, LED_CTL_TYPE_BLINK);
	}
	else if(appInfo.cameraMode == INFO_FACTORY_MODE && appInfo.cameraType == INFO_M3S){
		led_control(LED_TYPE_NET, LED_CTL_TYPE_BLINK);
	}
#endif
	return 0;
}

/**
   @brief 初始化电机的状态
   @author yangguozheng
   @param[in] flag 运行模式 1: 进入用户模式, 0： 正常使用
   @return 0
   @note
 */
int appInitMotor(int flag) {
#if 10
	static i = 0;
	if(appInfo.cameraMode == INFO_FACTORY_MODE && appInfo.cameraType == INFO_M3S) {
		serialport_foctory_mode(0);
	}
	else if(flag == 1 && appInfo.cameraType == INFO_M3S){
		serialport_user_mode();
	}
#endif
	return 0;
}

void *utils_bomb_run(void *arg);

void *utils_bomb_run(void *arg) {
	while(1) {
		if(appInfo.cameraMode == INFO_FACTORY_MODE && appInfo.cameraType == INFO_M3S) {
			DEBUGPRINT(DEBUG_INFO, "serialport_user_mode\n");
	//		serialport_user_mode();
			led_control(LED_TYPE_NET, LED_CTL_TYPE_FAST_FLK);
			serialport_foctory_mode(1);
		}
		else if(appInfo.cameraType == INFO_M2){
			led_control(LED_TYPE_NET_POWER, LED_CTL_TYPE_FAST_FLK);
		}
		sleep(3);
	}
	return 0;
}

/**
   @brief 升级确认
   @author yangguozheng
   @param[in] flag 可选参数
   @return 0
   @note
 */
int utils_bombByPig(int flag) {
	static char my_life = 0;
	DEBUGPRINT(DEBUG_INFO, "utils_bombByPig\n");
	DEBUGPRINT(DEBUG_INFO, "+++++++++++++++++++++++++++++\n");
	DEBUGPRINT(DEBUG_INFO, "++++++++++++kiss kiss+++++++++\n");
	DEBUGPRINT(DEBUG_INFO, "+++++++++++++++++++++++++++++\n");
	if(appInfo.cameraMode == INFO_FACTORY_MODE && appInfo.cameraType == INFO_M3S) {
		DEBUGPRINT(DEBUG_INFO, "serialport_user_mode\n");
//		serialport_user_mode();
//		led_control(LED_M3S_TYPE_NET, LED_CTL_TYPE_FAST_FLK);
        tutk_login_ok_set_led(LED_TYPE_NET, LED_CTL_TYPE_FAST_FLK);
		serialport_foctory_mode(1);
	}
	else if(appInfo.cameraType == INFO_M2){
//		led_control(LED_M2_TYPE_MIXED, LED_CTL_TYPE_FAST_FLK);
        tutk_login_ok_set_led(LED_TYPE_NET_POWER, LED_CTL_TYPE_FAST_FLK);
	}
#if _FAST_BLINK_SWITCH == 1
	if(0 == my_life) {
		my_life = 1;
		ithread_start(utils_bomb_run, NULL);
	}
#endif

	return 0;
}

/**
   @brief 获取摄像头类型
   @author yangguozheng
   @return -1: 失败, 0: 成功
   @note
 */
int getCameraType() {
	appInfo.cameraType = INFO_M2;
	FILE *file = fopen("/usr/share/info.txt", "r");
	regex_t re;
	regmatch_t match[1];
	regcomp(&re, "camera_model=", REG_EXTENDED);
	char buf[512];
	if(file == NULL) {
		printf("%s\n", strerror(errno));
		return -1;
	}
	while(NULL != fgets(buf, sizeof(buf) - 1, file)) {
		if(0 == regexec(&re, buf, 1, match, REG_EXTENDED)) {
			if(NULL != strstr(buf, "M3S")) {
				appInfo.cameraType = INFO_M3S;
				printf("M3S\n");
			}
			else if(NULL != strstr(buf, "M2")){
				appInfo.cameraType = INFO_M2;
				printf("M2\n");
			}
			break;
		}
	}
	fclose(file);
	return 0;
}

/**
   @brief 获取wifi列表
   @author yangguozheng
   @param[out] wifiItem 用于输出wifi信息项
   @param[in] size wifi信息项长度
   @return -1: 失败, 0: 成功
   @note
 */
int utils_getWifiItem(char *wifiItem, int size) {
	FILE *pFile = NULL;

    if(size > 512) {
        return 0;
    }
    static int itemIndex = 0;
    static FILE *file;
    static char buf[512];
	static regex_t re, re_qu, re_channel;
	static regmatch_t match[1], match_qu[1], match_channel[1];

    if(itemIndex == 0) {
        //system("iwlist ra0 scanning > /tmp/wifilist");
    	pFile = popen("iwlist ra0 scanning > /tmp/wifilist", "r");
        if (NULL != pFile)
        {
        	char acTmpBuf[128] = {0};

        	fread(acTmpBuf, 1, 128, pFile);
        	pclose(pFile);
        	pFile = NULL;
        }
        file = fopen("/tmp/wifilist", "r");
        if(file == NULL) {
            itemIndex = 0;
            return -1;
        }
        regcomp(&re, "ESSID:\"[^\"]*\"", REG_EXTENDED);
        regcomp(&re_qu, "Quality=[0-9]{1,3}", REG_EXTENDED);
        regcomp(&re_channel, "Channel [0-9]{1,3}", REG_EXTENDED);
    }

    int aCount = 0;
    *wifiItem = 0xFF;
    *(wifiItem + 1) = 0xFF;
    int len = 0;
    while(aCount != 3) {
        ++itemIndex;
        if(NULL != fgets(buf, sizeof(buf), file)) {
            //            printf("%s", buf);
            if(0 == regexec(&re, buf, 1, match, 0)) {
                //DEBUGPRINT(DEBUG_INFO, "y :%s\n", buf + match[0].rm_so);
                //                strncat(wifiList, buf + match[0].rm_so, match[0].rm_eo - match[0].rm_so);
                //                strcat(wifiList, " ");
                memcpy(wifiItem + 3, buf + match[0].rm_so + 7, match[0].rm_eo - match[0].rm_so - 7 -1);
                *(wifiItem + 2) = match[0].rm_eo - match[0].rm_so - 7 -2 + 1;
                //                printf("1\n");
                ++aCount;
            }
            else if(0 == regexec(&re_qu, buf, 1, match_qu, 0)){
                //DEBUGPRINT(DEBUG_INFO, "y :%s\n", buf + match_qu[0].rm_so);
                //                    strncat(wifiList, buf + match_qu[0].rm_so, match_qu[0].rm_eo - match_qu[0].rm_so);
                //                    strcat(wifiList, "\n");
                int quality = 0;
            	sscanf(buf + match_qu[0].rm_so + 8, "%d", &quality);
                *(wifiItem + 1) = quality;
                //                printf("2\n");
                ++aCount;
            }
            else if(0 == regexec(&re_channel, buf, 1, match_channel, 0)){
                //DEBUGPRINT(DEBUG_INFO, "y :%s\n", buf + match_qu[0].rm_so);
                //                    strncat(wifiList, buf + match_qu[0].rm_so, match_qu[0].rm_eo - match_qu[0].rm_so);
                //                    strcat(wifiList, "\n");
                int channelNum = 0;
            	sscanf(buf + match_channel[0].rm_so + 8, "%d", &channelNum);
                *wifiItem = channelNum;
                //                printf("3\n");
                ++aCount;
            }
        }
        else {
            if(aCount != 3) {

            }
            *wifiItem = 0xFF;
			*(wifiItem + 1) = 0xFF;
            itemIndex = 0;
            fclose(file);
            //system("rm /tmp/wifilist");
            pFile = popen("rm /tmp/wifilist", "r");
            if (NULL != pFile)
			{
				char acTmpBuf[128] = {0};

				fread(acTmpBuf, 1, 128, pFile);
				pclose(pFile);
				pFile = NULL;
			}

            return -1;
        }
    }
	return 0;
}

/*int NewsUtils_getRunTimeState(int opt, void *arg) {
    if (INFO_NIGHT_MODE_ON_TIME == opt && arg != NULL) {
        *(int *)arg = runtimeinfo_get_nm_time();
        return 0;
    }
    if(NIGHT_MODE_ON == runtimeinfo_get_nm_state()) {
    	return INFO_NIGHT_MODE_ON;
    }
    return INFO_NIGHT_MODE_OFF;
}*/

/**
 * keys
 *  INFO_ALARM_AUDIO,
 *  INFO_SPEAKER,
 * values
 *  INFO_ALARM_AUDIO_ON,
 *  INFO_ALARM_AUDIO_OFF,
 *  INFO_SPEAKER_ON,
 *  INFO_SPEAKER_OFF,
 * args
 *  thread message queue id
 */
/**
   @brief 获取应用信息
   @author yangguozheng
   @param[in] key 查询key
   @param[in] value 查询值
   @param[in] args 可选参数
   @return -1: 失败, ＊: 对应的返回值
   @note
 */
int NewsUtils_getAppInfo(int key, int value, int args) {
    int retval = 0;
    pthread_mutex_lock(&(appInfo.mutex));
    if(INFO_ALARM_AUDIO == key) {
        retval = appInfo.alarmStatus;
    }
    else if(INFO_SPEAKER == key) {
        retval = appInfo.speakerInfo;
    }
    pthread_mutex_unlock(&(appInfo.mutex));
    return retval;
}

/**
   @brief 设置应用信息
   @author yangguozheng
   @param[in] key 设置项的key
   @param[in] valu 设置项的值
   @param[in] args 可选参数
   @return -1: 失败, ＊: 对应的返回值
   @note
 */
int NewsUtils_setAppInfo(int key, int value, int args) {
    int retval = 0;
    char msgBuf[100];
	MsgData *qMsg = (MsgData *)msgBuf;
    pthread_mutex_lock(&(appInfo.mutex));
    if(INFO_SPEAKER == key) {
        appInfo.speakerInfo = value;
        if(INFO_SPEAKER_OFF == value) {
            //如果声音报警设置为打开，则打开声音报警
        	char audioAlarm[10];
        	int n;
        	get_config_item_value(CONFIG_ALARM_AUDIO, audioAlarm, &n);
        	if('y' == audioAlarm[0]) {
//        		//sadasda
//        		if(send_msg_to_otherthread(self, AUDIO_ALARM_MSG_TYPE, MSG_AUDIOALARM_T_START_AUDIO_CAPTURE, NULL, 0) < 0){
//					DEBUGPRINT(DEBUG_INFO, "send AUDIOALARM start failed\n");
//				}
        		qMsg->len = 0;
        		qMsg->cmd = MSG_AUDIOALARM_T_START_AUDIO_CAPTURE;
        		qMsg->type = AUDIO_ALARM_MSG_TYPE;
        		msg_queue_snd(args, qMsg, sizeof(MsgData) - sizeof(long) + qMsg->len);

        	}
//            if(0);
        }
        else if(INFO_SPEAKER_ON == value) {
            //
        	qMsg->len = 0;
			qMsg->cmd = MSG_AUDIOALARM_T_STOP_AUDIO_CAPTURE;
			qMsg->type = AUDIO_ALARM_MSG_TYPE;
			msg_queue_snd(args, qMsg, sizeof(MsgData) - sizeof(long) + qMsg->len);
        }
    }
    pthread_mutex_unlock(&(appInfo.mutex));
    return retval;
}

