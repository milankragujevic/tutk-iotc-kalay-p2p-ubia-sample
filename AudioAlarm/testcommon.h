/*
 * testcommon.h
 *
 *  Created on: Mar 26, 2013
 *      Author: root
 */

#ifndef TESTCOMMON_H_
#define TESTCOMMON_H_

#include "common/common.h"
#define SUCCESS 0
#define FAIL   -1

typedef struct {
	int MSGID_REV_PROCESS;
	int MSGID_SEND_PROCESS;
	Child_Process_Msg * s_rev_process;
	Child_Process_Msg * s_send_process;
}ArgsStruct;

extern int InitArgs(ArgsStruct *args);
extern int RevMSGProcess(ArgsStruct * msg,const int msgtype);
extern int SendMSG_2_Process(ArgsStruct * msg);
extern int RevMsgFunction(int msgID,Child_Process_Msg*rev_msg,const int msgtype);
extern int SendMsgFunction(int msgID,Child_Process_Msg*snd_msg);

#endif /* TESTCOMMON_H_ */
