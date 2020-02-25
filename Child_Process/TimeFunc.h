/*
 * TimeFunc.h
 *
 *  Created on: Feb 28, 2013
 *      Author: root
 */
#include "common/msg_queue.h"
#include "common/common.h"

#ifndef TIMEFUNC_H_
#define TIMEFUNC_H_

typedef void (*PTRDOCMDTIME)(int *msgID);

typedef struct
{
	const short int minute;
	const short int isActive;
	const PTRDOCMDTIME ptrDoCmd;
	short int num;
}CMDTableItemTIMEList;

extern char child_Recv[MAX_MSG_LEN];

extern void First_Thread_FUNC(int *msgID);

#endif /* TIMEFUNC_H_ */
