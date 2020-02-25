/*
 * NewsProtocol.h
 *
 *  Created on: Apr 24, 2013
 *      Author: root
 */

#ifndef NEWSPROTOCOL_H_
#define NEWSPROTOCOL_H_

#if 0
/*网络协议v1.1，奇数为输出，偶数为输入*/
enum {
    /*用户登陆 100 - 101*/
    UDP_SEARCH_REQ                      = 100, //搜索摄像头请求
    UDP_SEARCH_REP                      = 101, //搜索摄像头响应
    /*用户登陆 140 - 149*/
    NEWS_USER_JOIN_REQ                  = 140, //用户加入
    NEWS_USER_VERIFY_REQ                = 141, //用户验证
    NEWS_USER_VERIFY_REP                = 142, //用户验证回应
    NEWS_USER_JOIN_REP                  = 143, //用户加入回应
    /*通道 150 - 159*/
    NEWS_VIDEO_START_REQ                = 150, //视频请求
    NEWS_VIDEO_START_REP                = 151, //视频响应
    NEWS_AUDIO_START_REQ                = 152, //视频请求
    NEWS_AUDIO_START_REP                = 153, //视频响应
    NEWS_SPEAKER_START_REQ              = 154, //放音请求
    NEWS_SPEAKER_START_REP              = 155, //放音回应
    /*心跳包*/
    NEWS_HEART_BEAT_IN                  = 156, //心跳包
    NEWS_HEART_BEAT_OUT                 = 157, //心跳包
    /*网络灯 160 -169*/
    NEWS_LED_NET_STATUS_REQ             = 160, //网络灯状态查询
    NEWS_LED_NET_STATUS_REP             = 161, //网络灯状态查询回应
    NEWS_LED_NET_SET_REQ                = 162, //网络灯状态设置
    NEWS_LED_NET_SET_REP                = 163, //网络灯状态设置回应
    /*电源灯 170 -179*/
    NEWS_LED_POWER_STATUS_REQ           = 170, //电源灯状态查询
    NEWS_LED_POWER_STATUS_REP           = 173, //电源灯状态查询回应
    NEWS_LED_POWER_SET_REQ              = 170, //电源灯状态设置
    NEWS_LED_POWER_SET_REP              = 173, //电源灯状态设置回应
    /*夜视灯 180 -199*/
    NEWS_LED_NIGHT_STATUS_REQ           = 180, //夜视灯状态查询
    NEWS_LED_NIGHT_STATUS_REP           = 183, //夜视灯状态查询回应
    NEWS_LED_NIGHT_SET_REQ              = 180, //夜视灯状态设置
    NEWS_LED_NIGHT_SET_REP              = 183, //夜视灯状态设置回应
    /*时钟 220 -239*/
    NEWS_CLOCK_REQ                      = 220, //时钟状态请求
    NEWS_CLOCK_REP                      = 221, //时钟状态请求回应
    NEWS_CLOCK_SET_REQ                  = 222, //时钟设置请求
    NEWS_CLOCK_SET_REP                  = 223, //时钟设置回应
    /*系统命令 240 -259*/
    NEWS_SYS_REBOOT_REQ                 = 240, //系统重启请求
    NEWS_SYS_REBOOT_REP                 = 241, //系统重启请求回应
    NEWS_SYS_UPGRADE_REQ                = 242, //系统升级请求
    NEWS_SYS_UPGRADE_REP                = 243, //系统升级请求回应
    NEWS_SYS_UPGRADE_PROCESS_IN         = 244, //系统升级过程中输入包
    NEWS_SYS_UPGRADE_PROCESS_OUT        = 245, //系统升级过程中输出包
    NEWS_SYS_UPGRADE_OVER               = 247, //系统升级完成
    NEWS_SYS_UPGRADE_PROCESS_CANCEL_REQ = 248, //取消系统升级请求
    NEWS_SYS_UPGRADE_PROCESS_CANCEL_REP = 249, //取消系统升级回应
    /*恢复出厂设置 260 - 269*/
    NEWS_RESTORE_FACTORY_REQ            = 260, //恢复出厂设置请求
    NEWS_RESTORE_FACTORY_REP            = 261, //回复出场设置回应
    /*音频传输 300 - 399*/
    NEWS_AUDIO_TRANSMIT_IN              = 304, //音频输入包（放音）
    NEWS_AUDIO_TRANSMIT_OUT             = 305, //音频输出包
    /*视频传输 400 - */
    NEWS_VIDEO_TRANSMIT_OUT             = 400, //视频输出包
    /*视频设置 401 - 499*/
    NEWS_VIDEO_SET_REQ                  = 401, //视频设置请求
    NEWS_VIDEO_SET_REP                  = 402, //视频设置回应
    NEWS_VIDEO_QUERY_REQ                = 403, //视频参数查询请求
    NEWS_VIDEO_QUERY_REP                = 404, //视频参数查询回应

    /*云台（电机） 500 - 599*/
    NEWS_CAMERA_MOVE_ABS                = 500, //云台绝对旋转请求
    NEWS_CAMERA_POSITION                = 501, //云台居中请求
    NEWS_CAMERA_CRUISE                  = 502, //云台巡航请求
    NEWS_CAMERA_MOVE_REL                = 503, //云台相对旋转请求
    NEWS_CAMERA_MOVE_RES                = 504, //云台旋转相应
    /*报警 600 - 699*/
    NEWS_ALARM_QUERY_REQ                = 600, //报警参数查询请求
    NEWS_ALARM_QUERY_REP                = 601, //报警参数查询响应
    NEWS_ALARM_CONFIG_REQ               = 602, //报警参数设置请求
    NEWS_ARARM_CONFIG_REP               = 603, //报警参数设置响应
    NEWS_ALARM_EVENT_NOTIFY             = 605, //报警事件通知
    /*电量 1000 - */
    NEWS_ELICTRIC_QUANTITY_REQ          = 1000, //电量查询请求
    NEWS_ELICTRIC_QUANTITY_REP          = 1001, //电量查询响应
};
#endif
#endif /* NEWSPROTOCOL_H_ */
