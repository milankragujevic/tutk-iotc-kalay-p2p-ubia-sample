/*
 * Upnp_Protocol.c
 *
 *  Created on: Mar 21, 2013
 *      Author: root
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

//#include <miniupnpc/miniwget.h>
//#include <miniupnpc/miniupnpc.h>
//#include <miniupnpc/upnpcommands.h>
//#include <miniupnpc/upnperrors.h>

#include "miniwget.h"
#include "miniupnpc.h"
#include "upnpcommands.h"
#include "upnperrors.h"

//#define DEBUG
int CheckForwarding(struct IGDdatas *data, struct UPNPUrls *urls, char *lanaddr, char *szInternalPort)
{
        int r;
        int i = 0;
        char index[6];
        char intClient[40];
        char intPort[6];
        char extPort[6];
        char protocol[4];
        char desc[80];
        char enabled[6];
        char rHost[64];
        char duration[16];
        do
        {
                snprintf(index, 6, "%d", i);
                rHost[0] = '\0'; enabled[0] = '\0';
                duration[0] = '\0'; desc[0] = '\0';
                extPort[0] = '\0'; intPort[0] = '\0'; intClient[0] = '\0';
                r = UPNP_GetGenericPortMappingEntry(urls->controlURL,
                                data->first.servicetype,
                                index,
                                extPort, intClient, intPort,
                                protocol, desc, enabled,
                                rHost, duration);
                if(0 == strncmp(lanaddr, intClient, 64) &&
                                0 == strncmp(intPort, szInternalPort, 64))
                {
                        //sscanf(duration, "%d", iTimeout);
                        return 1;
                }
                i++;
        } while (0 == r);
        return 0;
}

int SetupForwarding(struct IGDdatas *data, struct UPNPUrls *urls, char *lanaddr, char *szInternalPort)
{
	char szExternalPort[14];
	char duration[16];
	int ret;
	strncpy(szExternalPort, szInternalPort, 14);
	strncpy(duration, "0", 5);
	//printf("Trying to map port %s %s\n", szExternalPort, szInternalPort);
	ret = UPNP_AddPortMapping(urls->controlURL, data->first.servicetype,
			szExternalPort, szInternalPort, lanaddr, "iBabyMonitor", "UDP", 0,
			duration);
	if (0 == ret) {
		//sscanf(duration, "%d", iTimeout);
		printf("UPnP TCP Port mapping ok\n");
	} else {
		/* errorCode errorDescription (short) - Description (long)
		 * 402 Invalid Args - See UPnP Device Architecture section on Control.
		 * 501 Action Failed - See UPnP Device Architecture section on Control.
		 * 715 WildCardNotPermittedInSrcIP - The source IP address cannot be
		 *                                   wild-carded
		 * 716 WildCardNotPermittedInExtPort - The external port cannot be wild-carded
		 * 718 ConflictInMappingEntry - The port mapping entry specified conflicts
		 *                     with a mapping assigned previously to another client
		 * 724 SamePortValuesRequired - Internal and External port values
		 *                              must be the same
		 * 725 OnlyPermanentLeasesSupported - The NAT implementation only supports
		 *                  permanent lease times on port mappings
		 * 726 RemoteHostOnlySupportsWildcard - RemoteHost must be a wildcard
		 *                             and cannot be a specific IP address or DNS name
		 * 727 ExternalPortOnlySupportsWildcard - ExternalPort must be a wildcard and
		 *                                        cannot be a specific port value */
		printf("Add TCP port mapping returned failed,");
		printf("Error NO:%d\n", ret);
		return ret;
	}
	ret = UPNP_AddPortMapping(urls->controlURL, data->first.servicetype,
			szExternalPort, szInternalPort, lanaddr, "iBabyMonitor", "TCP", 0,
			duration);
	if(ret == 0){
		printf("UPnP UDP Port mapping ok\n");
	}else{
		printf("Add UDP port mapping returned failed,");
		printf("Error NO:%d\n", ret);
		return ret;
	}
}


int DeleteForwarding(struct IGDdatas *data, struct UPNPUrls *urls, char *lanaddr, char *szInternalPort)
{
	int r = -1;

	r = UPNP_DeletePortMapping(urls->controlURL, data->first.servicetype, szInternalPort, "TCP", 0);
	if(r == 0){
		printf("UPnP Delete success\n");
		return 0;
	}
	return r;
}

int SetUpnp(char *map_port)
{
	int iError;
	struct UPNPDev *devlist = NULL;
	struct UPNPUrls urls;
	struct IGDdatas data;
	char lanaddr[64];
	int iTimeout;
	int ret = 0;
	int delay = 1000;

	memset(lanaddr, 0, 64);

	/** discover UPnP devices on the network.*/
	devlist = upnpDiscover(delay, NULL, NULL, 0, 0, &iError);
	if (!devlist) {
		printf("discover UPnP devices failed\n");
		return -1;
	}

#ifdef DEBUG
	struct UPNPDev *device;
	for(device = devlist; device; device = device->pNext)
	{
		printf("GUO-------------------------- desc: %s\n st: %s\n\n",
				device->descURL, device->st);
	}
#endif
	ret = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr));
	if (ret == 3 || ret < 0) {
		printf("UPNP_GetValidIGD: failed %d\n", ret);
		FreeUPNPUrls(&urls);
		freeUPNPDevlist(devlist);
		return -1;
	}

	//检查是否映射
	ret = CheckForwarding(&data, &urls, lanaddr, map_port);
	if (ret == 1) {
		printf("map ok\n");
		ret = 0;
		goto free_source;
	}
	ret = SetupForwarding(&data, &urls, lanaddr, map_port);
	if (ret != 0) {

	}
free_source:
	FreeUPNPUrls(&urls);
	freeUPNPDevlist(devlist);

	return ret;
}
