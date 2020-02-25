/*
 * cmbtable.h
 *
 *  Created on: Jan 14, 2013
 *      Author: root
 */

#ifndef CMBTABLE_H_
#define CMBTABLE_H_

#define NULL 0
#define	CMD_VERSION	1

typedef void (*PTRDOCMDFUNC)(char* message,int* msgID);

typedef struct
{
	const short int nVersionOne;
	const short int nCmdOne;
	const PTRDOCMDFUNC ptrDoCmdOne;
}CMDTableItemList;

typedef int (*PTRDOCMDFUNCSELONE)(int nCmd);

typedef struct
{
	const short int nVersion;
	const short int nCmd;
	const PTRDOCMDFUNCSELONE ptrDoCmd;
}CMDTableItem;

extern PTRDOCMDFUNCSELONE SearchCmdFunction(int nCmd);

#endif /* CMBTABLE_H_ */
