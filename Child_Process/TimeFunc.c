/*
 * TimeFunc.c
 *
 *  Created on: Feb 28, 2013
 *      Author: root
 */
#include "TimeFunc.h"
#include "common/logfile.h"
#include "common/msg_queue.h"

void First_Thread_FUNC(int *msgID){
//	DEBUGPRINT(DEBUG_INFO,"Do Send MSG come here=====%i\n",*msgID);
	struct MsgData *msg = (struct MsgData *)child_Recv;
	msg->type = VIDEO_MSG_TYPE;
	msg->cmd = 0;
	msg->len = 0;
/*
	if(msg_queue_snd(*msgID,child_Recv,sizeof(struct MsgData)-sizeof(long int)) == -1){

	}else{
		DEBUGPRINT(DEBUG_INFO,"Do Send MSG Success!!!!!!!=====\n");
	}*/

}
