/*
 * adapter_httpserver.h
 *
 *  Created on: Apr 8, 2013
 *      Author: root
 */

#ifndef ADAPTER_HTTPSERVER_H_
#define ADAPTER_HTTPSERVER_H_

#include "common/utils.h"

extern volatile int rGetIP;
extern volatile int rGetRSUCCESS;
extern volatile int rGetRlSUCCESS;
extern void reHttpsSeverGetUid(msg_container *self);
extern void HttpsSeverGetUid(msg_container self);
extern void HttpsServerUploadCamInfo(msg_container self);
extern void reHttpsServerUploadCamInfo(msg_container *self);
extern void HttpsSeverVerifycamTest(msg_container self);
extern void HttpServer_cmd_table_fun(msg_container *self);
extern void HttpServerStart(msg_container self);
extern void HttpServerEnd(msg_container self);
extern void HttpsSeverVerifycam(msg_container *self);
extern void HttpsSeverLoginCamTest(msg_container self);
extern void HttpsSeverVerifycamTest(msg_container self);
extern void retHttpsSeverVerifycam(msg_container *self);
//void retHttpsSeverVerifycam(msg_container *self);
extern void HttpsSeverLoginCam(msg_container * self);
extern void retHttpsSeverLoginCam(msg_container *self);
extern void HttpsSeverPushAlarm(msg_container self);
extern void retHttpsSeverPushAlarm(msg_container *self);
extern void HttpsSeverUploadFileCam(msg_container self);
extern void retHttpsSeverUploadFileCam(msg_container *self);
extern void retHttpServerEnd(msg_container *self);
extern void retHttpServerStart(msg_container *self);
extern void UID_is_Err();
#endif /* ADAPTER_HTTPSERVER_H_ */
