/*
 * NewsAdapter.h
 *
 *  Created on: Mar 30, 2013
 *      Author: yangguozheng wangzhiqiang
 */

#ifndef NEWSADAPTER_H_
#define NEWSADAPTER_H_

#include "NewsChannel.h"
typedef void (*newsFun)(void *self);
extern void NewsAdapter_cmd_table_fun(void *self);
extern int NewsAdapter_cmd_compar(const void *left, const void *right);
extern void *NewsAdapter_cmd_bsearch(NewsCmdItem *cmd, NewsCmdItem *cmd_table, size_t count);
int send_msg_to_network(int sockfd, int cmd, char *data, int len, int path);

#endif /* NEWSADAPTER_H_ */
