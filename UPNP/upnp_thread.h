/*
 * UPNP..h
 *
 *  Created on: Jan 7, 2013
 *      Author: root
 */

#ifndef UPNP__H_
#define UPNP__H_
#include "common/common.h"
#include "common/utils.h"

enum{
	MSG_UPNP_T_UPNPSTART,
	MSG_UPNP_T_UPNPSTOP,
	MSG_UPNP_P_UPNPSUCESS,
	MSG_UPNP_P_UPNPFAILED,
	MSG_UPNP_T_UPNPAGAIN,
};

void upnp_cmd_table_fun(msg_container *msg);

int CreateUpnpThread(void);

#endif /* UPNP__H_ */
