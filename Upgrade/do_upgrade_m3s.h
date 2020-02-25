/*
 * do_https_m3s.h
 *
 *  Created on: Mar 12, 2013
 *      Author: root
 */

#ifndef DO_UPGRADE_M3S_H_
#define DO_UPGRADE_M3S_H_

#include "upgrade.h"
////Camera 验证（只当USB线连上Cam时进行）		-->1
//#define ADDR_VERIFYCAM						"/camapi/getverifyinfo.htm"
////Camera注册									-->2
//#define ADDR_LOGIN_CAM						"/camapi/register.htm"
////Camera上传文件请求（报警，日志）				-->5
//#define ADDR_UPLOAD_FILE_CAM				"/camapi/upload.htm"
////Camera向云服务器推送报警信息					-->6
//#define ADDR_PUSH_ALARM_2_CLOUND_CAM		"/camapi/alarmnotice.htm"

//Camera升级信息查询							-->3
#define ADDR_UPGRADE_VERSION				"/camapi/getupdateinfo.htm"
//升级文件下载请求							-->4
#define ADDR_UPGRADE_M3S					"/camapi/downloadblock.htm"
//Camera升级状态回馈信息						-->7
#define ADDR_UPGRADE_RETURN_MSG				"/camapi/updatestate.htm"


extern volatile int R_UpgradeHost  ;
extern volatile int R_UpgradeThread;

extern void * DO_Upgrade_Thread() ;

#endif /* DO_HTTPS_M3S_H_ */
