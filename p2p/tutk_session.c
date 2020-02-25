#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include "control_channel.h"
#include "tutk_rdt_video_server.h"
#include "tutk_session.h"

/***********************************************用户操作函数*************************************************/
/**
 * @brief 初始化会话全局变量
 *
 * @author <王志强>
 * ＠param[in]<s><某个会话的指针>

 */
void tutk_session_init(struct tutk_iotc_session *s)
{
	int i;

	s->session_id = SESSION_ID_INVALID;
	s->session_used = SESSION_USED_INVALID;

	for (i = 0; i < TUTK_CHANNEL_NUM; i++) {
		s->ch[i].rdt_id = RDT_ID_INVALID;
		s->ch[i].rdt_used = RDT_USED_INVALID;
		
		pthread_mutex_init(&s->ch[i].rdt_mutex, NULL);
	}

	s->user_name = USER_NAME_INVALID;

	s->session_mode = TUTK_LAN;
	s->audio_direction = TUTK_CAMERA_TO_MOBILE;
	s->audio_dir_change = TUTK_AUDIO_DIR_UNCH;
	s->cam_mode = TUTK_CAM_MODE_STATUS_FAIL;
	memset(s->cam_key, 0, sizeof(s->cam_key));
	memset(s->verify, 0 , sizeof(s->verify));

	/*  2013-08-23 增加查询wifi信号命令
	 *  设置初始值
	 * */
	s->wifi_st = TUTK_WIFI_STATUS_OFF;
	s->wifi_time_interval = 0;
	s->wifi_time_stamp = 0;

	s->close_video_ch = TUTK_VIDEO_ON;
	s->close_audio_ch = TUTK_AUDIO_ON;

	tutk_session[i].audio_usb_insert = 0;
	tutk_session[i].audio_stop = 0;
	tutk_session[i].video_usb_insert = 0;
	tutk_session[i].video_stop = 0;

	pthread_mutex_init(&s->tutk_mutex, NULL);
	pthread_cond_init(&s->tutk_cond, NULL);

#ifdef TUTK_DEBUG_LOG
	tutk_session[i].send_picture_time = 0;
#endif
}

/**
 * @brief 设置会话退出
 *
 * @author 王志强
 * ＠param[in] n : 会话数组的下标

 */
void set_session_exit_state(int n)
{
	pthread_mutex_lock(&tutk_session[n].tutk_mutex);
	tutk_session[n].session_used = SESSION_USED_EXIT;
	pthread_mutex_unlock(&tutk_session[n].tutk_mutex);
}

/**
 * @brief 检测用户退出
 *
 * @author 王志强
 * ＠param[in] n : 会话数组的下标

 */
void tutk_check_session_exit(int n)
{
	while(1){
		if(tutk_session[n].session_id == SESSION_ID_INVALID){
			break;
		}
		usleep(100000);
	}
}

/**
 * @brief 设置4个用户退出
 *
 * @author 王志强

 */
void tutk_all_user_stop(void)
{
	int i;

	for (i = 0; i < 4; i++) {
		set_session_exit_state(i);
	}
}

/**
 * @brief 检测4个用户退出
 *
 * @author 王志强

 */
void tutk_check_user_stop(void)
{
	int j, k, n = 0;

check:
	for (j = 0; j < TUTK_SESSION_NUM; j++) {
		for (k = 0; k < TUTK_CHANNEL_NUM; k++) {
			if (tutk_session[j].ch[k].rdt_id == RDT_USED_INVALID) {
				n++;
			}
		}
	}

	if(n != 12){
		n = 0;
		goto check;
	}

	for (j = 0; j < TUTK_SESSION_NUM; j++) {
		tutk_session[j].session_used = SESSION_USED_INVALID;
	}
}

/**
 * @brief 设置4个用户音视频线程停止取贞
 *
 * @author 王志强

 */
void tutk_all_avchannel_stop(void)
{
	int i, j;

	for (i = 0; i < 4; i++) {
			tutk_session[i].video_usb_insert = USB_INSERT;
			tutk_session[i].audio_usb_insert = USB_INSERT;
	}
}

/**
 * @brief 设置4个用户音视频线程以取贞
 *
 * @author 王志强

 */
void tutk_all_avchannel_start(void)
{
	int i, j;

	for (i = 0; i < 4; i++) {
			tutk_session[i].video_usb_insert = 0;
			tutk_session[i].video_stop = 0;
			tutk_session[i].audio_usb_insert = 0;
			tutk_session[i].audio_stop = 0;
	}
}

/**
 * @brief 检测4个用户音视频线程已停止取贞
 *
 * @author 王志强

 */
void tutk_check_avchannel_stop(void)
{
	int j, k, n = 0;

check:
	for (j = 0; j < TUTK_SESSION_NUM; j++) {
		if (tutk_session[j].ch[1].rdt_used == RDT_USED_USING &&
				tutk_session[j].video_stop == AV_CHANNEL_STOP) {
			DEBUGPRINT(DEBUG_INFO, "%d...............video_stop\n", j);
			n++;
		}
		if (tutk_session[j].ch[2].rdt_used == RDT_USED_USING &&
				tutk_session[j].audio_stop == AV_CHANNEL_STOP) {
			DEBUGPRINT(DEBUG_INFO, "%d...............audio_stop\n", j);
			n++;
		}
	}
	DEBUGPRINT(DEBUG_INFO, "tutk_check_avchannel_stop......%d..%d\n", n,
			tutk_msg.tutk_audio_user_num+tutk_msg.tutk_video_user_num);

	if(n != (tutk_msg.tutk_video_user_num+tutk_msg.tutk_audio_user_num)){
		n = 0;
		goto check;
	}
}

/**
 * @brief 检测视频发送缓冲区，判断是否能继续发送数据
 * @author 王志强
 * ＠param[in] n : 会话数组的下标
 * @return 0: 成功
 * 			－1：失败
 * 			n(n > 0):断开

 * */
#define VIDEO_SEND_LIMIT_VAL	0
int tutk_check_v_buf_size(int n)
{
	int i;
	struct st_RDT_Status rdt_status;
	unsigned int sum = 0;
	int ret;

	i = n;
	if (tutk_session[i].session_id > SESSION_ID_INVALID) {
		if (tutk_session[i].ch[1].rdt_id > RDT_ID_INVALID) {
			if ((ret = RDT_Status_Check(tutk_session[i].ch[1].rdt_id, &rdt_status)) < 0) {
				DEBUGPRINT(DEBUG_INFO,"tutk_check_av_buf_size  session %d return %d\n", i, ret);
				return i + 1;
			}
			sum += rdt_status.BufSizeInSendQueue;
		}
	}

	if(sum == VIDEO_SEND_LIMIT_VAL){
		return 0;
	}else{
		return -1;
	}
}

#if 0
/*
 * func:
 * 
 * return 0: sucess
 * return -1: fail
 * return n(n > 0): disconnect	
*/
int tutk_check_av_buf_size(int n)
{
	int i, j;
	struct st_RDT_Status rdt_status;
	unsigned int sum = 0;
	int ret;

	i = n;
	if (tutk_session[i].session_id > SESSION_ID_INVALID) {
		for (j = 1; j < TUTK_CHANNEL_NUM; j++) {
			if (tutk_session[i].ch[j].rdt_id > RDT_ID_INVALID) {
				if ((ret = RDT_Status_Check(tutk_session[i].ch[j].rdt_id, &rdt_status)) < 0) {
					DEBUGPRINT(DEBUG_INFO,"tutk_check_av_buf_size  session %d return %d\n", i, ret);
					return i + 1;
				}
				sum += rdt_status.BufSizeInSendQueue;
			}
		}
	}

	if(sum < VIDEO_SEND_LIMIT_VAL){
		return 0;
	}else{
		//printf("tutk_check_av_buf_size: ******************** %d > %d\n", sum, VIDEO_SEND_LIMIT_VAL);
		return -1;
	}
}
#endif

/**
 * @brief 检测音频发送缓冲区，判断是否能继续发送数据
 * @author 王志强
 * ＠param[in] n : 会话数组的下标
 * @return 0: 成功
 * 			－1：失败
 * 			n(n > 0):断开

 * */
int tutk_check_a_buf_size(int n)
{
	int i, j;
	struct st_RDT_Status rdt_status;
	unsigned int sum = 0;
	int ret;

	i = n;
	if (tutk_session[i].session_id > SESSION_ID_INVALID) {
		for (j = 2; j < TUTK_CHANNEL_NUM; j++) {
			if (tutk_session[i].ch[j].rdt_id > RDT_ID_INVALID) {
				if ((ret = RDT_Status_Check(tutk_session[i].ch[j].rdt_id, &rdt_status)) < 0) {
						DEBUGPRINT(DEBUG_INFO,"tutk_check_a_buf_size  session %d return %d\n",i, ret);
						return i + 1;
				}
				sum += rdt_status.BufSizeInSendQueue;
			}
		}
	}

	if(sum < tutk_msg.audio_val){
		return 0;
	}else{
		//printf("tutk_check_a_buf_size: ################# %d > %d\n", sum, AUDIO_SEND_LIMIT_VAL);
		return -1;
	}
}

/**
 * @brief 设置音频通道方向
 * @author 王志强
 * ＠param[in] n : 会话数组的下标
 * ＠param[in] val : 方向值
 * @return 0: 成功   －1：失败
 *
 */
int set_audio_channel_dir(int n, int val)
{
	int i;
	if(val == tutk_session[n].audio_direction){
		return 0;
	}

	if (val == TUTK_MOBILE_TO_CAMERA) {//speaker
		if(NewsUtils_getAppInfo(INFO_SPEAKER, 0, 0) == INFO_SPEAKER_OFF){
			NewsUtils_setAppInfo(INFO_SPEAKER, INFO_SPEAKER_ON, tutk_msg.upnp_rev_process_id);
			tutk_session[n].audio_dir_change = TUTK_AUDIO_DIR_CH;
			tutk_session[n].audio_direction = val;
			return 0;
		}
	}else if(val == TUTK_CAMERA_TO_MOBILE){//audio
		if(tutk_session[n].audio_direction == TUTK_MOBILE_TO_CAMERA){
			NewsUtils_setAppInfo(INFO_SPEAKER, INFO_SPEAKER_OFF, tutk_msg.upnp_rev_process_id);
			tutk_session[n].audio_dir_change = TUTK_AUDIO_DIR_CH;
			tutk_session[n].audio_direction = val;
		}
		return 0;
	}
	return -1;
}


/********************************************************连接过程*********************************************************/
/**
 * @brief 用户认证时，判断连接类型的权限
 * @author 王志强
 * ＠param[in] s : 某个会话的指针
 * @return 权限值: 成功   －1：失败
 *
 */
int check_conn_type(struct tutk_iotc_session *s)
{
	int i;
	char tmp[20];

	if(s->session_mode == TUTK_P2P){
		strcpy(tmp, "2"); //p2p
	}else if(s->session_mode == TUTK_RLY){
		strcpy(tmp, "3");//rly
	}else if(s->session_mode == TUTK_LAN){
		strcpy(tmp, "1");
	}

	for(i = 0; i < 3; i++){
		//DEBUGPRINT(DEBUG_INFO, "check_conn_type:  %d, %d, %d, %d\n", i, tmp[0], user_pm[i].Cam_type[0],
			//	sizeof(s->cam_key));
		if(tmp[0] == user_pm[i].Cam_type[0]){
			s->cam_mode = user_pm[i].Cam_mode;
			memcpy(s->cam_key, user_pm[i].Cam_key, sizeof(s->cam_key));

			return user_pm[i].Cam_mode;
		}
	}

	DEBUGPRINT(DEBUG_INFO, "fail...........................check_conn_type\n");
	tutk_send_https_check_msg();//发送更新用户权限命令
	return -1;
#if 0
	int cnt = 0;
	while(1){
		usleep(500);
		cnt++;
		if(cnt >= 360){
			break;
		}
		for(i = 0; i < 3; i++){
			if(strncmp(tmp, user_pm[i].Cam_type, 1) == 0 && strncmp(user_pm[i].Cam_mode, "1", 1) == 0){//判断类型和权限
				s->cam_mode = user_pm[i].Cam_mode;
				memcpy(s->cam_key, user_pm[i].Cam_key, sizeof(s->cam_key));
				return user_pm[i].Cam_mode;
			}
		}
	}
#endif


}

/**
 * @brief 用户认证时，生成随计数
 * @author 王志强
 * ＠param[in] r : 随计数缓冲区的首地址
 * ＠param[in]len : 随计数缓冲区长度
 * ＠param[in] s : 某个会话的指针
 *
 */
void get_rand(unsigned int *r, int len, struct tutk_iotc_session *s)
{
	int i;

	for(i = 0;i < len;++i) {
		r[i] = rand();
		DEBUGPRINT(DEBUG_INFO, "r[i]: %d \n", r[i]);
	}
	memcpy(s->verify, r, 16);
	utils_btea(r, 4, s->cam_key);
}

/**
 * @brief 用户认证接收发送数据的过程
 * @author 王志强
 * ＠param[in]rdt_id : rdt通道的ID
 * ＠param[in] s : 某个会话的指针
 * @return 0: 成功   －1：失败
 *
 */
int tutk_user_auth(int rdt_id, struct tutk_iotc_session *s)
//int tutk_user_auth(int rdt_id, int session_n)
{
	RequestHead head;
	int ret;
	unsigned int r[4];
	unsigned int app_cr[4];
	char cr[17];

	ret = tutk_recv_head_data(rdt_id, &head);
	if (ret < 0) {
		printf("tutk_user_auth: read head failed %d\n", ret);
		return -1;
	}

	DEBUGPRINT(DEBUG_INFO, "tutk_user_auth:%d\n", head.cmd);
	if(head.cmd == NEWS_USER_JOIN_REQ){
		ret = check_conn_type(s);
		get_rand(r, sizeof(r)/sizeof(int), s);
		memcpy(&cr[1], r, 16);
		if(ret != '1'){
			cr[0] = 3; //没有权限
			send_msg_to_network(rdt_id, NEWS_USER_VERIFY_REQ, cr, sizeof(cr), 2);
			DEBUGPRINT(DEBUG_INFO, "tutk_user_auth: Don't access\n");
			return -1;
		}

		cr[0] = 1; //许可认证
		send_msg_to_network(rdt_id, NEWS_USER_VERIFY_REQ, cr, sizeof(cr), 2);

		ret = tutk_recv_head_data(rdt_id, &head);
		if(ret < 0){
			DEBUGPRINT(DEBUG_INFO, "tutk_user_auth: tutk_recv_head_data fail\n");
			return -1;
		}
		//DEBUGPRINT(DEBUG_INFO, "tutk_user_auth:%d %d\n", head.cmd, head.len);
		if(head.len == 16 && head.cmd == NEWS_USER_VERIFY_REP){
			ret = tutk_recv_data(rdt_id, app_cr, head.len);
			if(ret < 0){
				DEBUGPRINT(DEBUG_INFO, "tutk_user_auth: NEWS_USER_VERIFY_REP fail\n");
				return -1;
			}
			ret = memcmp(s->verify, app_cr, 16);
			if(ret == 0){
				cr[0] = 1;
			}else{
				cr[0] = 0;
			}
			send_msg_to_network(rdt_id, NEWS_USER_JOIN_REP, cr, 1, 2);
			return 0;
		}
	}
	DEBUGPRINT(DEBUG_INFO, "tutk_user_auth: fail\n");
	return -1;
}

/**
 * @brief 检测连接用户的名字，如果已连接断开之前的连接，否则寻找未用的位置
 * @author 王志强
 * ＠param[in] s : 某个会话的指针
 * @return 数组下标: 成功   －1：失败
 *
 * */
int check_user_name(struct tutk_iotc_session *s)
{
	int i;

	for (i = 0; i < TUTK_SESSION_NUM; i++) {
		if(tutk_session[i].user_name == s->user_name &&
				tutk_session[i].session_used == SESSION_USED_USING){
			DEBUGPRINT(DEBUG_INFO, "TUTK %d find the same name\n");
			set_session_exit_state(i);
			tutk_check_session_exit(i);
			return i;
		}
	}

	if(tutk_msg.client_cnt > TUTK_SESSION_NUM){
		DEBUGPRINT(DEBUG_INFO, "TUTK user full.................\n");
		return -1;
	}else{
		for (i = 0; i < TUTK_SESSION_NUM; i++) {
			pthread_mutex_lock(&tutk_session[i].tutk_mutex);
			if(tutk_session[i].session_used == SESSION_USED_INVALID){
				tutk_session[i].session_used = SESSION_USED_USING;
				pthread_mutex_unlock(&tutk_session[i].tutk_mutex);
				return i;
			}
			pthread_mutex_unlock(&tutk_session[i].tutk_mutex);
		}
	}
}

#if 0
/*
 * return -1: pthread exit
 * return > 0:success
 * return (0~3):is array index
 * return 4: is video or audio
 * */
int tutk_channel_define(int rdt_id, struct tutk_iotc_session **s, int *channel, int *n)
{
	RequestHead head;
	char buf[100];
	struct tutk_iotc_session *sp = *s;
	int len;
	int ret;

	ret = tutk_recv_head_data(rdt_id, &head);
	if (ret < 0 && ret != RDT_ER_TIMEOUT) {
		printf("tutk_channel_define: read head failed %d\n", ret);
		return -1;
	}

	if(head.len > 4 && head.cmd == NEWS_CHANNEL_DECLARATION_REQ){
		ret = tutk_recv_data(rdt_id, buf, head.len);
		if(ret < 0){
			printf("tutk_channel_define: tutk_recv_data failed %d\n", ret);
			return -1;
		}
		//s->rdt_id[j] = rdt_id;
		*channel = *(buf+8);
		if(*channel == CONTROL_CHANNEL){
			sp->user_name = *(int *)(buf+4); //先保存下来，未用
			ret = check_user_name(sp);
			if(ret < 0){
				memset(buf, 0, sizeof(buf));
				len = 4;
				buf[len] = *(buf+8);
				len += 1;
				buf[len] = 3;
				len += 2;
				send_msg_to_network(rdt_id, NEWS_CHANNEL_DECLARATION_REP, buf, len, 2);
				sp->session_used = SESSION_USED_INVALID;
				DEBUGPRINT(DEBUG_INFO, "tutk_channel_define: fail\n");
				return -1;
			}
			memcpy(&tutk_session[ret], sp, sizeof(struct tutk_iotc_session));
			*s = &tutk_session[ret];
			*n = ret;
		}
		memset(buf, 0, sizeof(buf));
		head.cmd = NEWS_CHANNEL_DECLARATION_REP;
		head.len = 10;
		memcpy(buf, &head, sizeof(head));
		len = sizeof(head)+4;
		buf[len] = channel;
		len += 1;
		buf[len] = 1;
		len += 1;
		buf[len] = 0;
		len += 4;
		tutk_send_vast(rdt_id, buf, len);

		return 0;
	}

	return -1;
}
#endif

/**
 * @brief 创建通道线程
 * @author 王志强
 * ＠param[in]channel : 通道标识
 * ＠param[in] n : 会话数组的下标
 * @return 0 : 成功
 *
 * */
int create_channel_thread(int channel, int n)
{
	struct tutk_iotc_session *s = &tutk_session[n];
	pthread_attr_t thread_attr;
	pthread_t tid;

	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);

	switch (channel) {
		case CONTROL_CHANNEL:
			tutk_log_s("ctrlchannel success");
			s->ch[0].rdt_used = RDT_USED_USING;
			if (pthread_create(&tid, &thread_attr, ctrlchannel_rdt_thread, (void*)(n*4+0)) != 0) {
				printf("ctrlchannel_RDT_thread\n");
			}
			//s->ch.tid[0] = tid;
			break;

		case VIDEO_CHANNEL:
			tutk_log_s("videochannel success");
			DEBUGPRINT(DEBUG_INFO, "VIDEO_CHANNEL......................%d\n", tutk_msg.tutk_video_user_num);
			if(tutk_msg.tutk_video_user_num > 4){
				break;
			}
			tutk_msg.tutk_video_user_num++;
			s->ch[1].rdt_used = RDT_USED_USING;
			if (pthread_create(&tid, &thread_attr, videochannel_rdt_thread, (void*)(n*4+1)) != 0) {
				printf("videochannel_RDT_thread\n");
			}
			//s->ch.tid[1] = tid;
			break;

		case AUDIO_CHANNEL:
			tutk_log_s("audiochannel success");
			DEBUGPRINT(DEBUG_INFO, "AUDIO_CHANNEL............................%d\n", tutk_msg.tutk_audio_user_num);
			if(tutk_msg.tutk_audio_user_num > 4){
				break;
			}
			tutk_msg.tutk_audio_user_num++;
			s->ch[2].rdt_used = RDT_USED_USING;
			if (pthread_create(&tid, &thread_attr, audiochannel_rdt_thread,(void*)(n*4+2)) != 0) {
				printf("audiochannel_RDT_thread\n");
			}
			//s->ch.tid[2] = tid;
			break;

		default:
			break;
	}

	return 0;
}

/**
 * @brief 创建通道线程
 * @author 王志强
 * ＠param[in] arg : 监听线程临时变量的首地址
 *
 * */
#define SESSION_WAIT_TIMEMS 	30000
void *tutk_session_thread(void *arg)
{
	struct tutk_iotc_session s;
	struct tutk_iotc_session *sp = &s;
	int sid = (int)arg;
	int n = -1;
	int j, ret;
	int rdt_id;
	/*
	 * 是否成功连接的标志。
	 * 有一个通道建立成功就代表已经建立成功了，
	 * 就需要会话线程等待通道线程退出
	 * */
#define CONNECT_FLAG_SUCCESS		1
#define CONNECT_FLAG_DEFINE_FAIL	2
	int connect_flag = CONNECT_FLAG_DEFINE_FAIL;
	char buf[100];
	RequestHead *head = buf;
	char *text = head+1;
	int channel;
	int len;
	int num = 0;

	DEBUGPRINT(DEBUG_INFO, "tutk_session_thread.........................%d\n", sp->session_id);
	tutk_session_init(sp);
	sp->session_used = SESSION_USED_USING;
	sp->session_id = sid;

	struct st_SInfo sinfo;
	ret = IOTC_Session_Check(sid, &sinfo);
	if (ret < 0) {
		DEBUGPRINT(DEBUG_ERROR, "IOTC_Session_Check: %d", ret);
		//return -1;
	}

	char *mode[3] = { "P2P", "RLY", "LAN" };
	DEBUGPRINT(DEBUG_INFO, "Client from %s:%d Mode=%s %d\n", sinfo.RemoteIP,
			sinfo.RemotePort, mode[(int) sinfo.Mode], sid);
	switch (sinfo.Mode) {
		case 0:
			sp->session_mode = TUTK_P2P;
			break;
		case 1:
			sp->session_mode = TUTK_RLY;
			break;
		case 2:
			sp->session_mode = TUTK_LAN;
			break;
	}

	while (sp->session_used == SESSION_USED_USING) {
		for (j = 0; j < TUTK_CHANNEL_NUM; j++) {
			if (sp->ch[j].rdt_id < 0 && sp->session_id > SESSION_ID_INVALID) {
				rdt_id = RDT_Create(sp->session_id, SESSION_WAIT_TIMEMS, j);
				if (rdt_id < 0) {
					if(connect_flag != CONNECT_FLAG_SUCCESS){
						connect_flag = CONNECT_FLAG_DEFINE_FAIL;
					}
					sp->session_used = SESSION_USED_EXIT;
					printf("%d RDT_Create failed[%d]!!\n", sp->session_id, rdt_id);
					break;
				} else {
					sp->ch[j].rdt_id = rdt_id;
					ret = tutk_user_auth(rdt_id, sp);
					if(ret != 0){
						if(connect_flag != CONNECT_FLAG_SUCCESS){
							connect_flag = CONNECT_FLAG_DEFINE_FAIL;
						}
						sp->session_used = SESSION_USED_EXIT;
						break;
					}

					ret = tutk_recv_data(rdt_id, buf, sizeof(RequestHead)+9);
					if (ret < 0 && ret != RDT_ER_TIMEOUT) {
						printf("tutk_channel_define: read head failed %d\n", ret);
						if(connect_flag != CONNECT_FLAG_SUCCESS){
							connect_flag = CONNECT_FLAG_DEFINE_FAIL;
						}
						sp->session_used = SESSION_USED_INVALID;
						break;
					}

					if(head->cmd == NEWS_CHANNEL_DECLARATION_REQ){
						channel = *(text+8);
						if(channel == CONTROL_CHANNEL){
							sp->user_name = *(int *)(text+4); //保存下来
							ret = check_user_name(sp);
							if(ret < 0){
								memset(buf, 0, sizeof(buf));
								len = 4;
								buf[len] = *(buf+8);
								len += 1;
								buf[len] = 3;//连接已满
								len += 5;
								send_msg_to_network(rdt_id, NEWS_CHANNEL_DECLARATION_REP, buf, len, 2);
								if(connect_flag != CONNECT_FLAG_SUCCESS){
									connect_flag = CONNECT_FLAG_DEFINE_FAIL;
								}
								sp->session_used = SESSION_USED_INVALID;
								DEBUGPRINT(DEBUG_INFO, "tutk_channel_define: fail\n");
								break;
							}
							memcpy(&tutk_session[ret], sp, sizeof(struct tutk_iotc_session));
							sp = &tutk_session[ret];
							n = ret;
						}
						memset(buf, 0, sizeof(buf));
						len = 4;
						buf[len] = channel;
						len += 1;
						buf[len] = 1;
						len += 5;
						send_msg_to_network(rdt_id, NEWS_CHANNEL_DECLARATION_REP, buf, len, 2);

						create_channel_thread(channel, n);
						connect_flag = CONNECT_FLAG_SUCCESS;
						num++;
					}
				}
			}
		}
		usleep(100000);
		if(j == 3){
			break;
		}
	}

	if(connect_flag != CONNECT_FLAG_SUCCESS){
		tutk_log_s("connect fail");
		goto tutk_session_exit;
	}

	while (sp->session_used == SESSION_USED_USING) {
		usleep(300000);
	}

	if(num > TUTK_CHANNEL_NUM){
		num = TUTK_CHANNEL_NUM;
	}

	for (j = 0; j < TUTK_CHANNEL_NUM; j++) {
		if(sp->ch[j].rdt_used == RDT_USED_WAIT_EXIT){
			continue;
		}
		sp->ch[j].rdt_used = RDT_USED_EXIT;
	}

	int i = 0;
check:
	for (j = 0; j < TUTK_CHANNEL_NUM; j++) {
		printf("*****************tutk_session_thread****%d*****%d**********%d****** %d\n", n, j, num, tutk_session[n].ch[j].rdt_used);
		if(tutk_session[n].ch[j].rdt_used == RDT_USED_WAIT_EXIT){

			i++;
		}
	}

	if(i != num){
		i = 0;
		usleep(1000000);
		goto check;
	}

	if(sp->audio_direction == TUTK_MOBILE_TO_CAMERA){
		NewsUtils_setAppInfo(INFO_SPEAKER, INFO_SPEAKER_OFF, tutk_msg.upnp_rev_process_id);
	}

#ifdef TUTK_DEBUG_LOG
	TUTK_DEBUG("TUTK RDT: client exit, name:%d\nIP:%s:%d\n", sp->user_name, sinfo.RemoteIP, sinfo.RemotePort);
#endif
tutk_session_exit:
	IOTC_Session_Close(sp->session_id);
	for (j = 0; j < TUTK_CHANNEL_NUM; j++) {
		if(sp->ch[j].rdt_id > -1){
			RDT_Destroy(sp->ch[j].rdt_id);
		}
	}

	tutk_session_init(sp);
	if(n > -1){
		if(num == 2){
			tutk_msg.tutk_video_user_num--;
		}else if(num > 2){
			tutk_msg.tutk_video_user_num--;
			tutk_msg.tutk_audio_user_num--;
		}
	}
	if(tutk_msg.client_cnt > 0){
		tutk_msg.client_cnt--;
	}

	tutk_log_s("tutk_session_thread exit");
	DEBUGPRINT(DEBUG_INFO,"tutk_session_thread===============exit========= %d\n", n);
	pthread_exit(NULL );
}

