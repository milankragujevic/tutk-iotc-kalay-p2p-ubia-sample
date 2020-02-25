/*
 * httpsserver.h
 *
 *  Created on: Mar 12, 2013
 *      Author: root
 */

#ifndef HTTPSSERVER_H_
#define HTTPSSERVER_H_

#include "common/msg_queue.h"
#include "common/common.h"

/*cloud configuration*/
#if M3S_New == 0 //USA
#define www "api.ibabycloud.com"
#else
#define www "54.248.88.59" //JAPAN
#endif
#define CLOUD_SERVER_PORT 30000
#define CLOUD_HTTPS_SERVER_PORT 8443
//#define CLOUD_NAME "54.241.27.115"
//1: 移动报警
#define ALRAM_4_MOVE 1
//2: 声音报警
#define ALARM_4_AUDIO 2
//1: 图片
//2: 声音
//3：视频
//4：错误日志
//5：配置信息
enum FileType{
	TYPE_PICTURE = 1,
	TYPE_AUDIO = 2,
	TYPE_VIDEO = 3,
	TYPE_ERROR_NOTE,
	TYPE_CONF_MSG,
};
//初始化为0
typedef struct{
	char  Cam_mode;
	char Cam_key[16];
	char Cam_type[16];
}conncheck;

typedef struct {
	int MSGID_REV_PROCESS;
	int MSGID_SEND_PROCESS;
	int MSGID_REV_DO_HTTPS;
	int MSGID_SEND_DO_HTTPS;
	Child_Process_Msg * s_rev_process;
	Child_Process_Msg * s_send_process;
	Child_Process_Msg * s_rev_do_https;
	Child_Process_Msg * s_send_do_https;
}HttpsStruct;

enum HttpsSever_Msg_CMD{

	MSG_HTTPSSERVER_T_START = 13,
	MSG_HTTPSSERVER_P_START = 14,

	MSG_HTTPSSEVER_T_VERIFYCAM = 1,//Camera 验证（只当USB线连上Cam时进行）
	MSG_HTTPSSEVER_P_VERIFYCAM = 2,//Camera 验证（只当USB线连上Cam时进行）

	MSG_HTTPSSEVER_T_LOGIN_CAM = 3,//Camera 注册
	MSG_HTTPSSEVER_P_LOGIN_CAM = 4,//Camera 1注册

	MSG_HTTPSSEVER_T_GET_UID = 9,//Camera向云服务器申请UID
	MSG_HTTPSSEVER_P_GET_UID = 10,//Camera向云服务器申请UID	返回

	MSG_HTTPSSERVER_T_UPLOADING_CAM_INFO = 15,///camapi/uploadingcaminfo.htm
	MSG_HTTPSSERVER_P_UPLOADING_CAM_INFO = 16,///camapi/uploadingcaminfo.htm

	MSG_HTTPSSERVER_T_UPDATE_STATE = 17,//CameraType return from Progress
	MSG_HTTPSSERVER_P_UPDATE_STATE = 18,//CameraType return from HttpsThread


	MSG_HTTPSSERVER_T_AUTHENTICATION_POWER = 19,
	MSG_HTTPSSERVER_P_AUTHENTICATION_POWER = 20,

	MSG_HTTPSSEVER_T_PUSH_ALARM_2_CLOUND_CAM = 7,//Camera向云服务器推送报警信息
	MSG_HTTPSSEVER_p_PUSH_ALARM_2_CLOUND_CAM = 8,//Camera向云服务器推送报警信息

	MSG_HTTPSSEVER_T_UPLOAD_FILE_CAM = 5,//Camera上传文件请求（报警，日志）
	MSG_HTTPSSEVER_P_UPLOAD_FILE_CAM = 6,//Camera上传文件请求（报警，日志）

	MSG_HTTPSPOWER_T_STATE=21,
	MSG_HTTPSPOWER_P_STATE=22,

	MSG_HTTPSSERVER_T_STOP = 11,
	MSG_HTTPSSERVER_P_STOP = 12,



};

enum HttpsServer_Key{
	HTTP_SERVER_MSG_KEY = 2000,
	DO_HTTP_SERVER_MSG_KEY,
};

enum HttpsServer_Type{
	HTTP_2_DO_MSG_TYPE = 1,
	DO_2_HTTP_MSG_TYPE,
};
extern volatile  conncheck   user_pm[3];
int createHttpsServerThread(void);
//int SendMsgFunction(int msgID,Child_Process_Msg*snd_msg);
//int RevMsgFunction(int msgID,Child_Process_Msg*rev_msg,const int msgtype);
int my_remove_file(char * path);
#endif /* HTTPSSERVER_H_ */
