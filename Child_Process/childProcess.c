/*
 * childProcess.c

 *
 *  Created on: Jan 7, 2013
 *      Author: root
 */
#include "common/common.h"
#include "common/msg_queue.h"
#include "common/logfile.h"
#include "Forwarding/Forwarding_cmbtable.h"
#include "cmbtable.h"
#include "TimeFunc.h"

char child_Recv[MAX_MSG_LEN];

CMDTableItemTIMEList cmdTablelistTime[] =
{
	{5,1,First_Thread_FUNC,0},
};

void child_processThread(){

	char led_init = 1;
	int i=0;

	PTRDOCMDFUNC ptrDoCmdFunc = 0;
	PTRDOCMDFUNCSELONE ptDoCmdFuncOne = 0;
	PTRDOCMDTIME ptrDoCmdTime = 0;
	int iIndexHigh = sizeof(cmdTablelistTime)/sizeof(CMDTableItemTIMEList);

	msg_queue_remove((key_t)CHILD_PROCESS_MSG_KEY);
	msg_queue_remove((key_t)CHILD_THREAD_MSG_KEY);

	int mainMsgId = msg_queue_create((key_t)CHILD_PROCESS_MSG_KEY);
	if(mainMsgId < 0){
		DEBUGPRINT(DEBUG_INFO, "Create fail\n");
		exit(1);
	}

	int MsgId = msg_queue_create((key_t)CHILD_THREAD_MSG_KEY);
	if(MsgId < 0){
		DEBUGPRINT(DEBUG_INFO, "Create fail\n");
		exit(1);
	}

	while(1) {
		if(
			(rAudio == 1)&&
//			(rAudioAlarm == 1)&&
			(rAudioSend == 1)&&
//		    (rDdnsServer == 1)&&
//			(rFlashRW == 1)&&
//			(rForwarding == 1)
//			(rHTTPServer == 1)&&
//			(rMFI == 1)&&
//			(rNetWorkSet == 1)&&
//			(rNewsChannel == 1)&&
//			(rNTP == 1)&&
			(rplayback == 1)
//			(rrevplayback == 1)&&
//			(rSerialPorts == 1)&&
//			(rTooling == 1)&&
//			(rUdpserver == 1)&&
//			(rUpgrade == 1)&&
//			(rUPNP == 1)&&
//			(rVideo == 1)
//			(rVideoAlarm == 1)&&
//			(rVideoSend == 1)
			)
		{
		rChildProcess = 1;

		break;
		}
		thread_usleep(0);
	}
	thread_usleep(0);


	int isCount = 0;

	struct MsgData *msg = (struct MsgData *)child_Recv;
	DEBUGPRINT(DEBUG_INFO,"=========come here Child Process After Sync \n");
	while(1)
	{

		if(isCount > 100){
			isCount = 0;
			for(i=0;i<iIndexHigh;i++){
				if(cmdTablelistTime[i].isActive == 1){
					if(cmdTablelistTime[i].num == cmdTablelistTime[i].minute){
						cmdTablelistTime[i].num = 0;
						ptrDoCmdTime = cmdTablelistTime[i].ptrDoCmd;
						ptrDoCmdTime(&MsgId);
					}else{
						cmdTablelistTime[i].num++;
					}
				}
			}
		}
		isCount++;


		if(msg_queue_rcv(mainMsgId, (void *)child_Recv, MAX_MSG_LEN, CHILD_PROCESS_MSG_TYPE) == -1) {
			thread_usleep(10000);
		}
		else {
			DEBUGPRINT(DEBUG_INFO,"Reply!");
/*
			ptDoCmdFuncOne = SearchCmdFunction(msg->type);
			if(ptDoCmdFuncOne == NULL){

				printf( "Don't find function pointer of command:%d\n", msg->type);

			}else{
				ptrDoCmdFunc = ptDoCmdFuncOne(msg->cmd);
				if( NULL == ptrDoCmdFunc )
				{
					printf( "Don't find function pointer of command:%d\n", msg->cmd);
				}else{
					ptrDoCmdFunc(child_Recv,&MsgId);
				}
			}
			memset(child_Recv,0,MAX_MSG_LEN);*/
		}
	}
}

