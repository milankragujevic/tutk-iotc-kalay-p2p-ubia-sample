#ifndef TUTK_SESSION_H_
#define TUTK_SESSION_H_

#include "common/msg_queue.h"
#include "playback/speaker.h"
#include "Audio/Audio.h"
#include "Video/video.h"

#include "IOTCAPIs.h"
#include "RDTAPIs.h"
#include "tutk_rdt_video_server.h"

/********4个用户******/
#define TUTK_SESSION_NUM	4
/********3个通道******/
#define TUTK_CHANNEL_NUM	3

struct tutk_rdt_channel{
#define RDT_ID_INVALID	-1
	int rdt_id;

#define RDT_USED_INVALID	-1
#define RDT_USED_EXIT		0
#define RDT_USED_USING		1
#define RDT_USED_WAIT_EXIT	2
	int rdt_used; //0:not connect(default) 1:connect 2:exit

	//int tid;
	pthread_mutex_t rdt_mutex;
};

struct tutk_iotc_session{
#define SESSION_ID_INVALID	-1
	int session_id;

#define SESSION_USED_INVALID	-1
#define SESSION_USED_EXIT		0
#define SESSION_USED_USING		1
	int session_used; // -1：会话可用(default) 0：会话退出 1:正在使用

	struct tutk_rdt_channel ch[TUTK_CHANNEL_NUM];

#define USER_NAME_INVALID	0
	int user_name;

#define TUTK_P2P	1
#define TUTK_RLY	2
#define TUTK_LAN	3
        int session_mode; //1:P2P  2:RLY  3:LAN(default)

#define TUTK_CAMERA_TO_MOBILE	1
#define TUTK_MOBILE_TO_CAMERA	2
        int audio_direction; // 1:monitor--->mobile(default)   2:mobile--->monitor

#define TUTK_AUDIO_DIR_CH	1
#define TUTK_AUDIO_DIR_UNCH	2
        int audio_dir_change; //1:dir change 2:dir no change(default)

#define TUTK_CAM_MODE_STATUS_OK		1
#define TUTK_CAM_MODE_STATUS_FAIL	2
        int cam_mode; // 1: ok 2: fail(default)
        char cam_key[16];//key
        int verify[4];

        /*
         *  2013-08-23 增加查询wifi信号命令
         *
         * */
#define TUTK_WIFI_STATUS_ON		1
#define TUTK_WIFI_STATUS_OFF	2
        int wifi_st; //1:on 2:off(default)
        int wifi_time_interval; // [5, 60]s, default value is 0
        time_t wifi_time_stamp;

#define TUTK_VIDEO_ON	1
#define TUTK_VIDEO_OFF	2
	/*视频通道开关*/
        int close_video_ch; //1:on(default), 2:off
#define TUTK_AUDIO_ON	1
#define TUTK_AUDIO_OFF	2
	/*音频通道开关*/
        int close_audio_ch; //1:on(default), 2:off

#define USB_INSERT		1
#define AV_CHANNEL_STOP	1
	int audio_usb_insert;
	int audio_stop;

	int video_usb_insert;
	int video_stop;

    pthread_mutex_t tutk_mutex;
	pthread_cond_t	tutk_cond;

#define TUTK_DEBUG_LOG 1
#ifdef TUTK_DEBUG_LOG
	unsigned int send_picture_time;//单位：毫秒
#endif
};
extern volatile struct tutk_iotc_session tutk_session[TUTK_SESSION_NUM];

/***********************************************用户操作函数****************/
void tutk_session_init(struct tutk_iotc_session *s);
void set_session_exit_state(int n);
void tutk_check_session_exit(int n);
void tutk_all_user_stop(void);
void tutk_check_user_stop(void);
void tutk_all_avchannel_start(void);
void tutk_all_avchannel_stop(void);
void tutk_check_avchannel_stop(void);

int tutk_check_v_buf_size(int n);
int tutk_check_av_buf_size(int n);
int tutk_check_a_buf_size(int n);

int set_audio_channel_dir(int n, int val);

//视频数据包协议结构体
typedef struct{
	RequestHead head;
	int     iDatetime;
	char    byteFormat;
	int     iDataLen;
	char buf[100 * 1024];
}__attribute__ ((packed)) VideoData;

typedef struct {
	RequestHead head; /*protocol  head*/
	int iCheckNum;
	int iDatetime;
	int iSerialNum;
	char byteFormat;
	int iDataLen;
	char buf[160];
}__attribute__ ((packed)) AudioData;

int create_tutk_server_thread(void);
void *tutk_recv_msgq_thread(void *arg);
void *tutk_listen_thread(void *arg);
void *tutk_login_thread(void *arg);
void *tutk_session_thread(void *arg);
void *ctrlchannel_rdt_thread(void *arg);
void *videochannel_rdt_thread(void *arg);
void *audiochannel_rdt_thread(void *arg);
//int *tutk_RDT_server(void *arg);

int tutk_send_vast(int rdt_id, char *rdt_sendbuf, int len);
int tutk_send_alarm_event(char *msg);
int tutk_recv_data_throw(int rdt_id, char *rdt_sendbuf, int sum);
int tutk_recv_data(int rdt_id, char *rdt_sendbuf, int sum);
int tutk_recv_data_timeout(int rdt_id, char *rdt_sendbuf, int sum, int timeout);
int tutk_recv_head_data(int rdt_id, char *rdt_sendbuf);
void set_session_stop_state(int n);

int set_audio_channel_dir(int n, int val);

int tutk_send_set_nightmode_command(int on, int time);
#endif
