
#ifndef TUTK_RDT_VIDEO_SERVER_H_
#define TUTK_RDT_VIDEO_SERVER_H_
#include <stdarg.h>
#include "common/common.h"
#include "NewsChannel/NewsChannel.h"
#include "common/msg_queue.h"
#include "common/common_func.h"
#include "common/common_config.h"
#include "common/logfile.h"
#include "common/thread.h"

#include "NewsChannel/NewsUtils.h"
#include "NewsChannel/NewsAdapter.h"
#include "NewsChannel/NewsChannelAdapter.h"

#include "playback/speaker.h"
#include "Audio/Audio.h"
#include "Video/video.h"

#include "HTTPServer/adapter_httpserver.h"
#include "HTTPServer/httpsserver.h"

#include "VideoSend/VideoSend.h"
#include "common/logserver.h"

#include "tutk_session.h"
#include "control_channel.h"
#include "IOTCAPIs.h"
#include "RDTAPIs.h"

//#define DEVICEUID "DZ4T9NNECYUZSH6PWZYS"
#define DEVICEUID "DRC9ANNYJEARAG6PSZWT"
#define DEVICENAME "TUTK_RDT_VIDEO_SERVER"
#define DEVICEPWD "123456"

enum{
	MSG_P2P_T_TUTKID = 1,
	MSG_P2P_T_ALARMEVENT,
	MSG_P2P_T_NETCHANGE,
};

#define LOGIN_ERROR_PATH	"/usr/share/tutk_login.txt"

#define TUTK_SERVER_START	1
#define TUTK_SERVER_STOP	2
#define TUTK_SERVER_USED	3
struct tutk_thread_msg{
	int upnp_rev_process_id;
	int upnp_snd_process_id;
	char msg_data_rcv[512];
	char msg_data_snd[512];

	int client_cnt;
	int thread_syn;
	int tutk_uid; //1:start 2:stop
	char tutk_id[21];

	int tutk_start; //1: start 2:stop
	int tutk_netch; //1:no change 2: change

	int audio_val;
	int tutk_video_user_num;
	int tutk_audio_user_num;
	pthread_t login_tid;


	FILE *login_fp;
};
extern volatile struct tutk_thread_msg  tutk_msg;

unsigned int getTimeStamp();

#define TUTK_DEBUG_LOG 1
#ifdef TUTK_DEBUG_LOG
#define TUTK_DEBUG(fmt, arg...) LOG_TEST(fmt, arg)
#else
#define TUTK_DEBUG(fmt, arg...)
#endif

void tutk_log_s(const char *fmt, ...);

#endif
