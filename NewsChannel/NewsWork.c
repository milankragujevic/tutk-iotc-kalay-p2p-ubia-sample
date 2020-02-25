/*
 * NewsWork.c
 *
 *  Created on: May 20, 2013
 *      Author: root
 */
#include "NewsWork.h"
#include "playback/speaker.h"
#include "NewsChannel.h"
#include "common/logfile.h"
#include <string.h>

void *NewsWork_run(void *arg);

void *NewsWork_run(void *arg) {
	int fd = (int)arg;
	char buf[512];
	RequestHead *rHead;
	int retval = 0;
	int len = 0;
	while(1) {
		if(len < sizeof(RequestHead)) {
			retval = recv(fd, buf + len,sizeof(RequestHead) - len, 0);
			if(retval <= 0) {
				close(fd);
				break;
			}
			len += retval;
			if(len == sizeof(RequestHead)) {
				if(0 != strncmp("IBBM", buf, strlen("IBBM"))) {
					len = 0;
					continue;
				}
			}
			DEBUGPRINT(DEBUG_INFO, "len < sizeof(RequestHead): %d\n", len);
		}
		else if(len < 177 + sizeof(RequestHead)){
			retval = recv(fd, buf + len, 177 + sizeof(RequestHead) - len, 0);
			if(retval <= 0) {
				close(fd);
				break;
			}
			len += retval;
			DEBUGPRINT(DEBUG_INFO, "len < 177 + sizeof(RequestHead): %d\n", len);
		}
		else {
			PlaySpeaker(buf + sizeof(RequestHead) + 17);
			len = 0;
			DEBUGPRINT(DEBUG_INFO, "else: %d\n", len);
		}
	}
	return 0;
}
