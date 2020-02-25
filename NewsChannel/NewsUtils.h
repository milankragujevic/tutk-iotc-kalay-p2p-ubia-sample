/*
 * NewsUtils.h
 *
 *  Created on: Apr 17, 2013
 *      Author: yangguozheng
 */

#ifndef NEWSUTILS_H_
#define NEWSUTILS_H_

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
#include <pthread.h>

/*向描述列表中添加描述*/
extern int NewsUtils_addSockFd(int *list, int len, int fd);
/*删除描述列表中是否存在特定的描述*/
extern int NewsUtils_rmSockFd(int *list, int len, int fd);

extern int NewsUtils_sendToNet(int fd, void *buf, int len, int flag);

/*摄像头状态*/
enum {
	INFO_FACTORY_MODE, //工厂态
	INFO_USER_MODE, //用户态
};

#define R_LEN 				172
#define CAMERA_MODE_OFFSET  (304 + 176)

enum {
	INFO_NO_R,
	INFO_HAS_R,
};

/*enum {
    INFO_NIGHT_MODE_ON = 1,
    INFO_NIGHT_MODE_OFF,
    INFO_NIGHT_MODE_ON_TIME,
};*/

enum {
	INFO_NIGHT_MODE_NULL,
    INFO_NIGHT_MODE_TIME,
};

enum {
    INFO_NIGHT_MODE_TCP_NOTIFY = 1,
};

enum {
    INFO_ALARM_AUDIO,
    INFO_SPEAKER,
};

enum {
    INFO_ALARM_AUDIO_ON,
    INFO_ALARM_AUDIO_OFF,
    INFO_SPEAKER_ON,
    INFO_SPEAKER_OFF,
};

enum {
	INFO_M2,
	INFO_M3S,
};

typedef struct AppInfo {
	char cameraMode;
	char cameraType;
    char rl[256];
    char alarmStatus;
    char speakerInfo;
    pthread_mutex_t mutex;
}AppInfo;

volatile AppInfo appInfo;

extern int setRl(char *rl, int size);
extern int setCameraMode(char mode);
extern int initAppInfo();
extern int appInitMotor();
extern int utils_getWifiItem(char *wifiItem, int size);
extern int utils_bombByPig(int flag);

extern int NewsUtils_getAppInfo(int key, int value, int args);
extern int NewsUtils_setAppInfo(int key, int value, int args);

//extern int NewsUtils_getRunTimeState(int opt, void *arg);

#endif /* NEWSUTILS_H_ */
