/*
 * cmbtable.c
 *
 *  Created on: Jan 14, 2013
 *      Author: root
 */
#include "cmbtable.h"
#include "Forwarding/Forwarding_cmbtable.h"
#include "UPNP/UPNP.h"

#define NULL 0

CMDTableItem cmdTablelist[] =
{
	{CMD_VERSION,1,SearchCmdFunctionUPNP},
	{CMD_VERSION,6,SearchCmdFunctionForwarding},
};

//二分查找法查询iCmd对应的处理函数
PTRDOCMDFUNCSELONE SearchCmdFunction(int nCmd)
{
	int iIndexLow = 0;
	int iIndexMid = 0;
	int iIndexHigh = sizeof(cmdTablelist)/sizeof(CMDTableItem);
	int iIndex = -1;

	while(iIndexLow <= iIndexHigh)
	{
		iIndexMid =(iIndexLow + iIndexHigh)/2;

        if(nCmd == cmdTablelist[iIndexMid].nCmd)
        {
        	iIndex = iIndexMid;
        	break;
        }
        else if(nCmd < cmdTablelist[iIndexMid].nCmd)
        {
        	iIndexHigh = iIndexMid - 1;
        }
        else
        {
        	iIndexLow = iIndexMid + 1;
        }
	}
	return iIndex > -1 ? cmdTablelist[iIndex].ptrDoCmd : NULL;
}
