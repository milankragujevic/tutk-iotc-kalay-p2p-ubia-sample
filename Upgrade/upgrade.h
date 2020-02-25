/*
 * UPGRADE.h
 *
 *  Created on: Mar 12, 2013
 *      Author: root
 */

#ifndef UPGRADE_H_
#define UPGRADE_H_

#include "common/msg_queue.h"
#include "common/common.h"

/*cloud configuration*/
//#define CLOUD_SERVER_PORT 30000
//#define CLOUD_HTTPS_SERVER_PORT 8443
////1: 移动报警
//#define ALRAM_4_MOVE 1
////2: 声音报警
//#define ALARM_4_AUDIO 2


typedef struct {
	int MSGID_REV_PROCESS;
	int MSGID_SEND_PROCESS;
	int MSGID_REV_DO_HTTPS;
	int MSGID_SEND_DO_HTTPS;
	Child_Process_Msg * s_rev_process;
	Child_Process_Msg * s_send_process;
	Child_Process_Msg * s_rev_do_https;
	Child_Process_Msg * s_send_do_https;
}UpgradeStruct;

enum Upgrade_Msg_CMD{

	MSG_UPGRADE_T_START = 9,
	MSG_UPGRADE_P_START = 10,

	MSG_UPGRADE_T_GET_UPDATE_INFO = 1,//Camera 验证（只当USB线连上Cam时进行）
	MSG_UPGRADE_P_GET_UPDATE_INFO = 2,//Camera 验证（只当USB线连上Cam时进行）

	MSG_UPGRADE_T_DOWN_LOAD_BLOCK = 3,//Camera 注册
	MSG_UPGRADE_P_DOWN_LOAD_BLOCK = 4,//Camera 1注册

	MSG_UPGRADE_T_UPGRADE_STATE = 5,//Camera上传文件请求（报警，日志）
	MSG_UPGRADE_P_UPGRADE_STATE = 6,//Camera上传文件请求（报警，日志）

	MSG_UPGRADE_T_STOP = 7,
	MSG_UPGRADE_P_STOP = 8,
};

enum Upgrade_Key{
	UPGRADE_SERVER_MSG_KEY = 2002,
	DO_UPGRADE_SERVER_MSG_KEY,
};

enum Upgrade_Type{
	UPGRADE_2_DO_MSG_TYPE = 1,
	DO_2_UPGRADE_MSG_TYPE,
};

int createUpgradeThread(void);
//int SendMsgFunction(int msgID,Child_Process_Msg*snd_msg);
//int RevMsgFunction(int msgID,Child_Process_Msg*rev_msg,const int msgtype);

#endif /* UPGRADE_H_ */
