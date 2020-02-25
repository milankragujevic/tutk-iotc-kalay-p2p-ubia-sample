/*
 * do_https_m3s.h
 *
 *  Created on: Mar 12, 2013
 *      Author: root
 */

#ifndef DO_HTTPS_M3S_H_
#define DO_HTTPS_M3S_H_

#include "httpsserver.h"
#if 0
//Camera 验证（只当USB线连上Cam时进行）		-->1
#define ADDR_VERIFYCAM						"/camapi/getverifyinfo.htm"
//Camera注册									-->2
#define ADDR_LOGIN_CAM						"/camapi/register.htm"

#define ADDR_GETP2PID						"/camapi/getp2pid.htm"

#define ADDR_UPLOADING_CAM_INFO				"/camapi/uploadingcaminfo.htm"
//Camera上传文件请求（报警，日志）				-->5
#define ADDR_UPLOAD_FILE_CAM				"/camapi/upload.htm"
//Camera向云服务器推送报警信息					-->6
#define ADDR_PUSH_ALARM_2_CLOUND_CAM		"/camapi/alarmnotice.htm"
#else

//Camera 验证（只当USB线连上Cam时进行）		-->1
#define ADDR_VERIFYCAM						"/camapi/getverifyinfo.ashx"
//Camera注册									-->2
#define ADDR_LOGIN_CAM						"/camapi/register.ashx"

#define ADDR_GETP2PID						"/camapi/getp2pid.ashx"

#define ADDR_UPLOADING_CAM_INFO				"/camapi/uploadingcaminfo.ashx"
//Camera上传文件请求（报警，日志）				-->5
#define ADDR_UPLOAD_FILE_CAM				"/camapi/upload.ashx"
//Camera向云服务器推送报警信息					-->6
#define ADDR_PUSH_ALARM_2_CLOUND_CAM		"/camapi/alarmnotice.ashx"

#define ADDR_CONNCHECK						"/camapi/connchecking.htm"

#define ADDR_CAMSTATE                       "/camapi/camstate.htm"

#define ADDR_PUSH_SSID_IP_2_CLOUND_CAM		"/camapi/CamUploadSSIDAndIP.htm"

#endif

#define SC									"4d5ba53a2ed546c7b0b03bf13e66f079"
//#define SN									"4d5ba53a2ed546c7b0b03bf13e66f079"
#define SV_ADDR_VERIFYCAM					"1a55b99a33a244f18d9bb9b5f3843570"
#define SV_ADDR_LOGIN_CAM					"c7cf1d39e21d44408ec2b2a8b000e5b3"
#define SV_ADDR_GETP2PID					"9b33c73437a64098a3aec50df7b08ca9"
#define SV_ADDR_UPLOADING_CAM_INFO			"b721a36bef864cd0ab37403cbe46fcee"
#define SV_ADDR_PUSH_ALARM_2_CLOUND_CAM		"20cac16f7dc34990adc9875ecc9b03f8"
#define SV_ADDR_UPLOAD_FILE_CAM 			"56a0d1a34647451a83acb6449221cf54"
#define SV_ADDR_CONNCHECK					"6b33c23437a64095a3aec10df7b12ca7"
#define SV_ADDR_CAMSTATE                    "7f08031a888e40e2957607dc87cc9d10"

extern volatile int R_HttpsHost  ;
extern volatile int R_HttpsThread;
extern volatile int flag_setnetwork;
extern volatile int my_uid;
extern void * DO_Https_Thread() ;
extern int my_remove_file(char * path);


#endif /* DO_HTTPS_M3S_H_ */
