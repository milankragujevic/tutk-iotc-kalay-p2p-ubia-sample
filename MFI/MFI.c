/** @file [MFI.c]
 *  @brief 与苹果设备USB的认证过程和与APPS的USB通讯协议的主要功能函数C文件
 *	@author: Summer.Wang
 *	@data: 2013-9-24
 *	@modify record: 即日创建了这套文件注释系统
 */
#include "MFI.h"
#include "common/thread.h"
#include "common/common.h"
#include "common/msg_queue.h"
#include "common/common_func.h"
#include "common/common_config.h"
#include "common/logfile.h"
#include "NewsChannel/NewsUtils.h"
#include <sys/ioctl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

/**USB设备宏定义*/
#define HOT_PLUG_USB
#ifdef HOT_PLUG_USB
#define DEV "/dev/mfiusb0"
#else
#define DEV "/dev/appleusb"
#endif

/**IIC宏定义*/
#define I2CDEV "/dev/i2cM0"
#define USBCDEV "/dev/usbc"

//#define OUT 68
#define IN 200										///< 用于从USBread时读入buffer

#define MFI_OPEN_INVEVAL                500000		///< 定义打开mfi间隔
#define MFI_THREAD_SLEEP_TIME           100000		///< 定义mfi线程sleep时间

volatile int g_nMfiLoopTimes = 0;					///< 定义循环次数
volatile int g_nOpenAppleRetryTimes = 0;			///< 尝试打开USB设备的次数
long g_lAppleDeviceState = 0;						///< USB设备状态，苹果设置状态  打开：1  关闭 ：0

int fd_apple = -1;									///< USB设备的文件描述符
int fd_i2c = -1;									///< IIC设备的文件描述符

char MFI_AppsOk = 0;								///< 预留接口，设置wifi成功
fd_set	rfds;										///< 封装的select函数内read文件描述符
struct timeval wait_time;							///< 封装的select函数的等待时间全局变量
int retval,i;										///< 全局变量，方便数据赋值和使用
static char maxmum;									///< indicator the postion about the CER
static char nowmum;									///< marker for postion
unsigned char buf_out[200];							///< USB发送数据的缓存
unsigned char buf_in[200];							///< USB接收数据的缓存
unsigned char buf_ID[185];							///< USB数据处理的缓存
unsigned char buf_variable[130];					///< USB临时存储的缓存
unsigned char ssid_passwd[96];						///< 存储SSID和密码的缓存
unsigned char MFI_DeviceID[4];						///< 从IIC获取的device ID的缓存
unsigned char checksum;								///< 全局变量，方便存储和比较和校验
int wifi_request_flag = 0;							///< 表征wifi状态
int wifi_share_flag = 0; 							///< maker to infor whether the user share wifi
int session_flag = 1;   							///< used for first openDataSessionForProtocol
int globle_session_flag = 0;						///< session创立状态的标志
int flg_get_Rl = 0;									///< 是否获取到加密R'的标志
int iPodAck = 0;									///< 苹果设备应答标志
unsigned short int global_transID;					///< 完成苹果底层通讯的顺序ID的全局变量
unsigned char tempID;								///< used to get global_transID's high byte
unsigned char sessionID[2];							///< session ID
unsigned char mark[100];							///< 应用层的顺序ID存储
unsigned char mark_ft[100];							///< 上一次的顺序ID存储，防止包冗余
unsigned char camera_ID[190];						///< 摄像头加密信息R和mac的存储
char flag_send = 0;									///< 用于是否对苹果设备进行回复

enum
{
	MFI_STATE_INVALID = 0,							///< MFI线程空闲
    MFI_WIFI_CONNECT_SUCCESS = 1,					///< wifi设置成功
    MFI_WIFI_CONNECT_FAILURE,						///< wifi连接失败
    MFI_WIFI_GET_SSID_SUCCESS,						///< 共享wifi获取SSID和密码
    MFI_APP_LUNCH_OK,								///< 与APPS成功可以开始通讯
    MFI_GET_R_FLAG,									///< 应该获取R'
    MFI_READY_OPEN_APPLE_DEV,						///< 第一次没有成功打开USB设备
};

/* mfi线程状态 */
int g_nMifStatus = MFI_STATE_INVALID;				///< 表征线程运行状态，与上面的枚举共同使用

typedef struct WifiInfo
{
	int  lNetType;                         /* 网络类型 有线  或是 无线  */
	int  lIpType;                          /* ip类型 动态或是静态         */
	int  nEnrType;                         /* wifi 类型  */
	char acIpAddr[IPV4_ATTRIBUTE_LEN];     /* ip地址     */
    char acSubNetMask[IPV4_ATTRIBUTE_LEN]; /* 子网掩码     */
    char acGateWay[IPV4_ATTRIBUTE_LEN];    /* 网关            */
    char acWiredDNS[IPV4_ATTRIBUTE_LEN];   /* 有线网 DNS */
    char acWifiDNS[IPV4_ATTRIBUTE_LEN];    /* 无线网 DNS */
    char strSSID[WIFI_SSID_LEN];           /* ssid */
    char strPasswd[WIFI_CODE_LEN];         /* 密码   */
    /* 若有需求再增加  但总长度不要超过1024 */
}WiFiInfo_T;
WiFiInfo_T wifi_info;								///< 存储wifi信息结构体

char getData_MFI[MAX_MSG_LEN];						///< 接收消息队列数据的容器
int localMsgId_MFI,remoteMsgId_MFI;					///< 两消息队列的ID号

/**
 * @brief 封装的读取IIC数据的基础函数
 * @author Summer.Wang
 * @return <失败返回-1><成功返回read读取的字节数>
 * @param[out] <data><数据指针指向待存入的容器 >
 */
int certificatedata(unsigned char *data)
{
	retval = read(fd_i2c, data, data[1]);
	if(retval < 0)
	{
		printf("read i2c error\n");
		return -1;
	}
	return retval;
}

/**
 * @brief 封装的select读取USB数据函数
 * @author Summer.Wang
 * @return <失败返回-1><成功返回0>
 * @note select等待时间设置为500ms
 */
int my_select(void)
{
	wait_time.tv_sec = 0;
	wait_time.tv_usec = 500000;
	FD_ZERO(&rfds);
	FD_SET(fd_apple, &rfds);
	select(fd_apple + 1, &rfds, NULL, NULL, &wait_time);
	if(FD_ISSET(fd_apple, &rfds))
		return 0;

	return -1;
}

/**
 * @brief 启动Identification流程命令
 * @author Summer.Wang
 * @return <失败返回-1><成功返回0>
 * @param[out] <buf_out><发送的USB数据>
 * @param[in|out] <buf_in><USB数据读入和查询>
 * @remark 与苹果底层通讯的流程节点封装
 */
int startIDPS()
{
	buf_out[0] = 0x05;
	buf_out[1] = 0x00;
	buf_out[2] = 0x55;
	buf_out[3] = 0x04;
	buf_out[4] = 0x00;
	buf_out[5] = 0x38;
	buf_out[6] = 0x00;
	buf_out[7] = 0x00;
	buf_out[8] = 0xc4;

	retval = write(fd_apple, buf_out, 9);
	if (retval < 0)
	{
		return -1;
	}

	retval = my_select();
	if (retval < 0)
	{
		return -1;
	}

	retval = read(fd_apple, buf_in, IN);
	if (retval < 0)
	{
		return -1;
	}
	else
	{
		if((buf_in[8] == 0x00) && (buf_in[9] == 0x38))
		{

			return 0;
		}
	}
	return -1;
}

/**
 * @brief 设置通讯过程中最大的数据负载
 * @author Summer.Wang
 * @return <失败返回-1><成功返回0>
 * @param[out] <buf_out><发送的USB数据>
 * @param[in|out] <buf_in><USB数据读入和查询>
 * @remark 与苹果底层通讯的流程节点封装
 */
int RequestTranMax()
{
	unsigned int value = 0;
	buf_out[0] = 0x05;
	buf_out[1] = 0x00;
	buf_out[2] = 0x55;
	buf_out[3] = 0x04;
	buf_out[4] = 0x00;
	buf_out[5] = 0x11;
	buf_out[6] = 0x00;
	buf_out[7] = 0x01;
	buf_out[8] = 0xEA;

	retval = write(fd_apple, buf_out, 9);
	if (retval < 0)
	{
		return -1;
	}

	retval = my_select();
	if(retval < 0)
	{
		return -1;
	}

	retval = read(fd_apple, buf_in, IN);
	if (retval < 0)
	{
		return -1;
	}
	else
	{
		if((buf_in[7] != 0x04) && (0x12 == buf_in[5]))
		{
			value |= buf_in[8]<<16;
			value |= buf_in[9];
			return 0;
		}
	}
	return -1;
}

/**
 * @brief 设置附件的选项字节
 * @author Summer.Wang
 * @return <失败返回-1><成功返回0>
 * @param[out] <buf_out><发送的USB数据>
 * @param[in|out] <buf_in><USB数据读入和查询>
 * @remark 与苹果底层通讯的流程节点封装
 * @note 查看是否支持USB
 */
int GetiPodOptionsForLingo()
{
	buf_out[0] = 0x06;
	buf_out[1] = 0x00;
	buf_out[2] = 0x55;
	buf_out[3] = 0x05;
	buf_out[4] = 0x00;
	buf_out[5] = 0x4B;
	buf_out[6] = 0x00;
	buf_out[7] = 0x02;
	buf_out[8] = 0x00;
	buf_out[9] = 0xAE;

	retval = write(fd_apple, buf_out, 10);
	if (retval < 0)
	{
		return -1;
	}

	retval = my_select();
	if(retval < 0)
	{
		return -1;
	}

	retval = read(fd_apple, buf_in, IN);
	if (retval < 0)
	{
		return -1;
	}
	else
	{
		if((buf_in[5] == 0x4C) && (0x02 == buf_in[7]))
		{
			return 0;
		}
	}
	return -1;
}

/**
 * @brief 发送FID字符
 * @author Summer.Wang
 * @return <失败返回-1><成功返回0>
 * @param[out] <buf_out><发送的USB数据>
 * @param[in|out] <buf_in><USB数据读入和查询>
 * @remark 与苹果底层通讯的流程节点封装
 * @note 发送从CP读取的Device ID
 */
int sendDeviceID()
{
	buf_out[0] = 0x09;
	buf_out[1] = 0x00;
	buf_out[2] = 0x55;
	buf_out[3] = 0x12;
	buf_out[4] = 0x00;
	buf_out[5] = 0x39;
	buf_out[6] = 0x00;
	buf_out[7] = 0x03;
	buf_out[8] = 0x01;
	buf_out[9] = 0x0C;
	buf_out[10] = 0x00;
	buf_out[11] = 0x00;
	buf_out[12] = 0x01;
	buf_out[13] = 0x00;
	buf_out[14] = 0x00;
	buf_out[15] = 0x00;
	buf_out[16] = 0x00;
	buf_out[17] = 0x02;
	buf_out[18] = 0x00;
	buf_out[19] = 0x00;
	buf_out[20] = 0x02;
	buf_out[21] = 0x00;
	buf_out[18] = MFI_DeviceID[0];
	buf_out[19] = MFI_DeviceID[1];
	buf_out[20] = MFI_DeviceID[2];
	buf_out[21] = MFI_DeviceID[3];

	checksum = 0;
	for(i=3; i<22; i++)
		checksum += buf_out[i];
	checksum = ~checksum + 1;
	buf_out[22] = checksum;

	retval = write(fd_apple, buf_out, 23);
	if (retval < 0)
	{
		return -1;
	}

	retval = my_select();
	if(retval < 0)
	{
		return -1;
	}

	retval = read(fd_apple, buf_in, IN);
	if (retval < 0)
	{
		return -1;
	}
	else
	{
		if ((buf_in[12] == 0x00) && (0x3a == buf_in[5]) && (0x03 == buf_in[7]))
		{
			return 0;
		}
	}
	return -1;
}

/**
 * @brief 发送FID字符
 * @author Summer.Wang
 * @return <失败返回-1><成功返回0>
 * @param[out] <buf_out><发送的USB数据>
 * @param[in|out] <buf_in><USB数据读入和查询>
 * @remark 与苹果底层通讯的流程节点封装
 * @note 通知苹果设备该附件支持与应用程序通讯
 */
int AccessoryCapsToken()
{
	buf_out[0] = 0x08;
	buf_out[1] = 0x00;
	buf_out[2] = 0x55;
	buf_out[3] = 0x10;
	buf_out[4] = 0x00;
	buf_out[5] = 0x39;
	buf_out[6] = 0x00;
	buf_out[7] = 0x04;
	buf_out[8] = 0x01;
	buf_out[9] = 0x0A;
	buf_out[10] = 0x00;
	buf_out[11] = 0x01;
	buf_out[12] = 0x00;
	buf_out[13] = 0x00;
	buf_out[14] = 0x00;
	buf_out[15] = 0x00;
	buf_out[16] = 0x00;
	buf_out[17] = 0x00;
	buf_out[18] = 0x02;
	buf_out[19] = 0x00;
	buf_out[20] = 0xA5;

	retval = write(fd_apple, buf_out, 0x15);
	if (retval < 0)
	{
		return -1;
	}

	retval = my_select();
	if(retval < 0)
	{
		return -1;
	}

	retval = read(fd_apple, buf_in, IN);
	if (retval < 0)
	{
		return -1;
	}
	else
	{
		if((buf_in[12] == 0x00) && (0x3a == buf_in[5]) && (0x04 == buf_in[7]))
		{
			return 0;
		}
	}
	return -1;
}

/**
 * @brief 发送FID字符
 * @author Summer.Wang
 * @return <失败返回-1><成功返回0>
 * @param[out] <buf_out><发送的USB数据>
 * @param[in|out] <buf_in><USB数据读入和查询>
 * @remark 与苹果底层通讯的流程节点封装
 * @note 通知苹果设备该附件信息类型，该条信息发送附件名字
 */
int sendAccessoryName()
{
	buf_out[0] = 0x09;
	buf_out[1] = 0x00;
	buf_out[2] = 0x55;
	buf_out[3] = 0x11;
	buf_out[4] = 0x00;
	buf_out[5] = 0x39;
	buf_out[6] = 0x00;
	buf_out[7] = 0x05;
	buf_out[8] = 0x01;
	buf_out[9] = 0x0b;
	buf_out[10] = 0x00;
	buf_out[11] = 0x02;
	buf_out[12] = 0x01;
	buf_out[13] = 'M';
	buf_out[14] = 'o';
	buf_out[15] = 'n';
	buf_out[16] = 'i';
	buf_out[17] = 't';
	buf_out[18] = 'o';
	buf_out[19] = 'r';
	buf_out[20] = 0x00;
	checksum = 0;
	for(i=3; i<21; i++)
		checksum += buf_out[i];
	checksum = ~checksum + 1;
	buf_out[21] = checksum;

	retval = write(fd_apple, buf_out, 22);
	if (retval < 0)
	{
		return -1;
	}

	retval = my_select();
	if (retval < 0)
	{
		return -1;
	}

	retval = read(fd_apple, buf_in, IN);
	if (retval < 0)
	{
		return -1;
	}
	else
	{
		if((buf_in[12] == 0x00) && (0x3a == buf_in[5])){
			return 0;
		}
	}
	return -1;
}

/**
 * @brief 发送FID字符
 * @author Summer.Wang
 * @return <失败返回-1><成功返回0>
 * @param[out] <buf_out><发送的USB数据>
 * @param[in|out] <buf_in><USB数据读入和查询>
 * @remark 与苹果底层通讯的流程节点封装
 * @note 通知苹果设备该附件信息类型，该条信息发送附件固件版本号
 */
int sendAccessoryFirmwareVersion() // 1 0 2
{
	buf_out[0] = 0x08;
	buf_out[1] = 0x00;
	buf_out[2] = 0x55;
	buf_out[3] = 0x0c;
	buf_out[4] = 0x00;
	buf_out[5] = 0x39;
	buf_out[6] = 0x00;
	buf_out[7] = 0x06;
	buf_out[8] = 0x01;
	buf_out[9] = 0x06;
	buf_out[10] = 0x00;
	buf_out[11] = 0x02;
	buf_out[12] = 0x04;
	buf_out[13] = 0x01;
	buf_out[14] = 0x00;
	buf_out[15] = 0x00;
	checksum = 0;
	for(i=3; i<16; i++)
		checksum += buf_out[i];
	checksum = ~checksum + 1;
	buf_out[16] = checksum;

	retval = write(fd_apple, buf_out, 17);
	if (retval < 0)
	{
		return -1;
	}

	retval = my_select();
	if (retval < 0)
	{
		return -1;
	}

	retval = read(fd_apple, buf_in, IN);
	if (retval < 0)
	{
		return -1;
	}
	else
	{
		if((buf_in[12] == 0x00) && (0x3a == buf_in[5]) &&(0x06 == buf_in[7]))
		{
			return 0;
		}
	}
	return -1;
}

/**
 * @brief 发送FID字符
 * @author Summer.Wang
 * @return <失败返回-1><成功返回0>
 * @param[out] <buf_out><发送的USB数据>
 * @param[in|out] <buf_in><USB数据读入和查询>
 * @remark 与苹果底层通讯的流程节点封装
 * @note 通知苹果设备该附件信息类型，该条信息发送附件硬件版本号
 */
int sendAccessoryHardwareVersion()//1 0 2
{
	buf_out[0] = 0x08;
	buf_out[1] = 0x00;
	buf_out[2] = 0x55;
	buf_out[3] = 0x0c;
	buf_out[4] = 0x00;
	buf_out[5] = 0x39;
	buf_out[6] = 0x00;
	buf_out[7] = 0x07;
	buf_out[8] = 0x01;
	buf_out[9] = 0x06;
	buf_out[10] = 0x00;
	buf_out[11] = 0x02;
	buf_out[12] = 0x05;
	buf_out[13] = 0x01;
	buf_out[14] = 0x00;
	buf_out[15] = 0x00;
	checksum = 0;
	for(i=3; i<16; i++)
		checksum += buf_out[i];
	checksum = ~checksum + 1;
	buf_out[16] = checksum;
	retval = write(fd_apple, buf_out, 17);
	if (retval < 0)
	{
		return -1;
	}

	retval = my_select();
	if(retval < 0)
	{
		return -1;
	}

	retval = read(fd_apple, buf_in, IN);
	if (retval < 0)
	{
		return -1;
	}
	else
	{
		if((buf_in[12] == 0x00) && (0x3a == buf_in[5]) && (0x07 == buf_in[7])){
			return 0;
		}
	}
	return -1;
}

/**
 * @brief 发送FID字符
 * @author Summer.Wang
 * @return <失败返回-1><成功返回0>
 * @param[out] <buf_out><发送的USB数据>
 * @param[in|out] <buf_in><USB数据读入和查询>
 * @remark 与苹果底层通讯的流程节点封装
 * @note 通知苹果设备该附件信息类型，该条信息发送附件制造商
 */
int sendAccessorymanufacturer()
{
	buf_out[0] = 0x09;
	buf_out[1] = 0x00;
	buf_out[2] = 0x55;
	buf_out[3] = 0x0f;
	buf_out[4] = 0x00;
	buf_out[5] = 0x39;
	buf_out[6] = 0x00;
	buf_out[7] = 0x08;
	buf_out[8] = 0x01;
	buf_out[9] = 0x09;
	buf_out[10] = 0x00;
	buf_out[11] = 0x02;
	buf_out[12] = 0x06;
	buf_out[13] = 'i';
	buf_out[14] = 'B';
	buf_out[15] = 'a';
	buf_out[16] = 'b';
	buf_out[17] = 'y';
	buf_out[18] = 0x00;

	checksum = 0;
	for(i=3; i<19; i++)
		checksum += buf_out[i];

	checksum = ~checksum + 1;
	buf_out[19] = checksum;
	retval = write(fd_apple, buf_out, 20);
	if (retval < 0)
	{
		return -1;
	}

	retval = my_select();
	if(retval < 0)
	{
		return -1;
	}

	retval = read(fd_apple, buf_in, IN);
	if (retval < 0)
	{
		return -1;
	}
	else
	{
		if((buf_in[12] == 0x00) && (0x3a == buf_in[5]) && (0x08 == buf_in[7])){
			return 0;
		}
	}
	return -1;
}

/**
 * @brief 发送FID字符
 * @author Summer.Wang
 * @return <失败返回-1><成功返回0>
 * @param[out] <buf_out><发送的USB数据>
 * @param[in|out] <buf_in><USB数据读入和查询>
 * @remark 与苹果底层通讯的流程节点封装
 * @note 通知苹果设备该附件信息类型，该条信息发送附件型号
 */
int sendAccessoryModel()
{
	int len;
	char buf_tmp[4];

	buf_out[0] = 0x09;
	buf_out[1] = 0x00;
	buf_out[2] = 0x55;
	buf_out[3] = 0x0d;
	buf_out[4] = 0x00;
	buf_out[5] = 0x39;
	buf_out[6] = 0x00;
	buf_out[7] = 0x09;
	buf_out[8] = 0x01;
	buf_out[9] = 0x07;
	buf_out[10] = 0x00;
	buf_out[11] = 0x02;
	buf_out[12] = 0x07;
	buf_out[13] = 'M';
	buf_out[14] = '3';
	buf_out[15] = 's';
	buf_out[16] = 0x00;

	if(get_config_item_value(CONFIG_CAMERA_TYPE, buf_tmp, &len) == 0) {
		if(strncmp(buf_tmp, "M3S", 4) == 0) {
		}

		if(strncmp(buf_tmp, "M2", 4) == 0) {
				buf_out[13] = 'M';
				buf_out[14] = '2';
				buf_out[15] = 0x00;
			}
	}

	checksum = 0;
	for(i=3; i<17; i++)
		checksum += buf_out[i];
	checksum = ~checksum + 1;
	buf_out[17] = checksum;
	retval = write(fd_apple, buf_out, 18);
	if (retval < 0)
	{
		return -1;
	}

	retval = my_select();
	if(retval < 0)
	{
		return -1;
	}

	retval = read(fd_apple, buf_in, IN);
	if (retval < 0)
	{
		return -1;
	}
	else
	{
		if((buf_in[12] == 0x00) && (0x3a == buf_in[5]) && (0x09 == buf_in[7])){
			return 0;
		}
	}
	return -1;
}

/**
 * @brief 发送FID字符
 * @author Summer.Wang
 * @return <失败返回-1><成功返回0>
 * @param[out] <buf_out><发送的USB数据>
 * @param[in|out] <buf_in><USB数据读入和查询>
 * @remark 与苹果底层通讯的流程节点封装
 * @note 通知苹果设备该附件信息类型，该条信息设置附件最大传输包（0x40,64Bytes）
 */
int sendAccessoryInComeSize()
{
	buf_out[0] = 0x08;
	buf_out[1] = 0x00;
	buf_out[2] = 0x55;
	buf_out[3] = 0x0B;
	buf_out[4] = 0x00;
	buf_out[5] = 0x39;
	buf_out[6] = 0x00;
	buf_out[7] = 0x0A;
	buf_out[8] = 0x01;
	buf_out[9] = 0x05;
	buf_out[10] = 0x00;
	buf_out[11] = 0x02;
	buf_out[12] = 0x09;
	buf_out[13] = 0x00;
	buf_out[14] = 0x40;

	checksum = 0;
	for(i=3; i<15; i++)
		checksum += buf_out[i];
	checksum = ~checksum + 1;
	buf_out[15] = checksum;

	retval = write(fd_apple, buf_out, 16);
	if (retval < 0)
	{
		return -1;
	}

	retval = my_select();
	if(retval < 0)
	{
		return -1;
	}

	retval = read(fd_apple, buf_in, IN);
	if (retval < 0)
	{
		return -1;
	}
	else
	{
		if((buf_in[12] == 0x00) && (0x3a == buf_in[5]) && (0x0a == buf_in[7])){
			return 0;
		}
	}
	return -1;
}

/**
 * @brief 发送FID字符
 * @author Summer.Wang
 * @return <失败返回-1><成功返回0>
 * @param[out] <buf_out><发送的USB数据>
 * @param[in|out] <buf_in><USB数据读入和查询>
 * @remark 与苹果底层通讯的流程节点封装
 * @note 通知苹果设备该附件信息类型，该条信息发送附件可支持的苹果设备版本号（支持iphone3 iphone4 iphone4s）
 */
int sendAccessoryRfCertification()
{
	buf_out[0] = 0x08;
	buf_out[1] = 0x00;
	buf_out[2] = 0x55;
	buf_out[3] = 0x0D;
	buf_out[4] = 0x00;
	buf_out[5] = 0x39;
	buf_out[6] = 0x00;
	buf_out[7] = 0x0b;
	buf_out[8] = 0x01;
	buf_out[9] = 0x07;
	buf_out[10] = 0x00;
	buf_out[11] = 0x02;
	buf_out[12] = 0x0c;
	buf_out[13] = 0x00;
	buf_out[14] = 0x00;
	buf_out[15] = 0x00;
	buf_out[16] = 0x1b;
	checksum = 0;
	for(i=3; i<17; i++)
		checksum += buf_out[i];
	checksum = ~checksum + 1;
	buf_out[17] = checksum;

	retval = write(fd_apple, buf_out, 18);
	if (retval < 0)
	{
		return -1;
	}

	retval = my_select();
	if(retval < 0)
	{
		return -1;
	}

	retval = read(fd_apple, buf_in, IN);
	if (retval < 0)
	{
		return -1;
	}
	else
	{
		if((buf_in[12] == 0x00) && (0x3a == buf_in[5]) && (0x0b == buf_in[7])){
			return 0;
		}
	}
	return -1;
}

/**
 * @brief 发送FID字符
 * @author Summer.Wang
 * @return <失败返回-1><成功返回0>
 * @param[out] <buf_out><发送的USB数据>
 * @param[in|out] <buf_in><USB数据读入和查询>
 * @remark 与苹果底层通讯的流程节点封装
 * @note 发送EA protocol
 */
int sendAccessoryEAprotocolIndex()
{
	int len;
	char buf_tmp[4];

	buf_out[0] = 0x09;
	buf_out[1] = 0x00;
	buf_out[2] = 0x55;
	buf_out[3] = 0x1b;
	buf_out[4] = 0x00;
	buf_out[5] = 0x39;
	buf_out[6] = 0x00;
	buf_out[7] = 0x0C;
	buf_out[8] = 0x01;
	buf_out[9] = 0x15;
	buf_out[10] = 0x00;
	buf_out[11] = 0x04;
	buf_out[12] = 0x01;
	buf_out[13] = 'c';
	buf_out[14] = 'o';
	buf_out[15] = 'm';
	buf_out[16] = '.';
	buf_out[17] = 'i';
	buf_out[18] = 'B';
	buf_out[19] = 'a';
	buf_out[20] = 'b';
	buf_out[21] = 'y';
	buf_out[22] = 'l';
	buf_out[23] = 'a';
	buf_out[24] = 'b';
	buf_out[25] = 's';
	buf_out[26] = '.';
	buf_out[27] = 'm';
	buf_out[28] = '3';
	buf_out[29] = 's';
	buf_out[30] = 0x00;

	if(get_config_item_value(CONFIG_CAMERA_TYPE, buf_tmp, &len) == 0) {
		if(strncmp(buf_tmp, "M3S", 4) == 0) {
		}

		if(strncmp(buf_tmp, "M2", 4) == 0) {
			buf_out[28] = '2';
			buf_out[29] = 0x00;
		}
	}


	checksum = 0;
	for(i=3; i<31; i++)
		checksum += buf_out[i];
	buf_out[31] = ~checksum + 1;
	retval = write(fd_apple, buf_out, 32);
	if (retval < 0)
	{
		return -1;
	}

	retval = my_select();
	if (retval < 0)
	{
		return -1;
	}

	retval = read(fd_apple, buf_in, IN);
	if (retval < 0)
	{
		return -1;
	}
	else
	{
		if((buf_in[12] == 0x00) && (0x3a == buf_in[5]) && (0x0c == buf_in[7])){
			return 0;
		}
	}
	return -1;
}

/**
 * @brief 发送FID字符
 * @author Summer.Wang
 * @return <失败返回-1><成功返回0>
 * @param[out] <buf_out><发送的USB数据>
 * @param[in|out] <buf_in><USB数据读入和查询>
 * @remark 与苹果底层通讯的流程节点封装
 * @note 发送Bundle seed ID
 */
int sendAccessoryBundleseedIDPrefToken()
{
	buf_out[0] = 0x09;
	buf_out[1] = 0x00;
	buf_out[2] = 0x55;
	buf_out[3] = 0x13;
	buf_out[4] = 0x00;
	buf_out[5] = 0x39;
	buf_out[6] = 0x00;
	buf_out[7] = 0x0D;
	buf_out[8] = 0x01;
	buf_out[9] = 0x0d;
	buf_out[10] = 0x00;
	buf_out[11] = 0x05;
/*#ifdef FOR_APPLE_TEST
	//用于测试版的
	buf_out[12] = '7';//'J';
	buf_out[13] = 'J';//'R'; //7JJWGUK7G2
	buf_out[14] = 'J';//'G';
	buf_out[15] = 'W';//'B';
	buf_out[16] = 'G';//'H';
	buf_out[17] = 'U';//'C';
	buf_out[18] = 'K';//'4';
	buf_out[19] = '7';//'J';
	buf_out[20] = 'G';//'8';
	buf_out[21] = '2';//'7';
	buf_out[22] = 0x00;
#else*/
	//用于上线版的
	buf_out[12] = 'J';
	buf_out[13] = 'R'; // JRGBHC4J87
	buf_out[14] = 'G';
	buf_out[15] = 'B';
	buf_out[16] = 'H';
	buf_out[17] = 'C';
	buf_out[18] = '4';
	buf_out[19] = 'J';
	buf_out[20] = '8';
	buf_out[21] = '7';
	buf_out[22] = 0x00;
//#endif

	checksum = 0;
	for(i=3; i<23; i++)
		checksum += buf_out[i];
	checksum = ~checksum + 1;
	buf_out[23] = checksum;
	retval = write(fd_apple, buf_out, 24);
	if (retval < 0)
	{
		return -1;
	}

	retval = my_select();
	if (retval < 0)
	{
		return -1;
	}

	retval = read(fd_apple, buf_in, IN);
	if (retval < 0)
	{
		return -1;
	}
	else
	{
		if((buf_in[12] == 0x00) && (0x3a == buf_in[5]) && (0x0d == buf_in[7])){
			return 0;
		}
	}
	return -1;
}

/**
 * @brief 结束Identification流程命令
 * @author Summer.Wang
 * @return <失败返回-1><成功返回0>
 * @param[out] <buf_out><发送的USB数据>
 * @param[in|out] <buf_in><USB数据读入和查询>
 * @remark 与苹果底层通讯的流程节点封装
 */
int endIDPS()
{
	buf_out[0] = 0x06;
	buf_out[1] = 0x00;
	buf_out[2] = 0x55;
	buf_out[3] = 0x05;
	buf_out[4] = 0x00;
	buf_out[5] = 0x3B;
	buf_out[6] = 0x00;
	buf_out[7] = 0x0E;
	buf_out[8] = 0x00;

	checksum = 0;
	for(i=3; i<9; i++)
		checksum += buf_out[i];
	checksum = ~checksum + 1;
	buf_out[9] = checksum;

	retval = write(fd_apple, buf_out, 10);
	if (retval < 0)
	{
		return -1;
	}

	retval = my_select();
	if (retval < 0)
	{
		return -1;
	}

	retval = read(fd_apple, buf_in, IN);
	if (retval < 0)
	{
		return -1;
	}
	else
	{

		if((buf_in[8] == 0x00) && (0x3c == buf_in[5]) && (0x0e == buf_in[7])){
			return 0;
		}
	}
	return -1;
}

/**
 * @brief 发送CP存储的认证证书
 * @author Summer.Wang
 * @return <失败返回-1><成功返回0>
 * @param[out] <buf_out><发送的USB数据>
 * @param[in|out] <buf_in><USB数据读入和查询>
 * @remark 与苹果底层通讯的流程节点封装
 */
int sendAccessoryAuthenticationInfo()
{
	unsigned char transID_low;
	unsigned char transID_hig;
	unsigned int lengthipod = 0;
	unsigned char left = 0;
	unsigned char  buf_temp[140];

	retval = my_select();
	if (retval < 0)
	{
		goto error_send_authentication;
	}

	retval = read(fd_apple, buf_in, IN);
	if(retval < 0)
	{
		goto error_send_authentication;//
	}

	if(0x14 == buf_in[5]) //apple device send Get Accessory AuthenticationInfo
	{
		transID_hig = buf_in[6];
		transID_low = buf_in[7];

#if 0  /*add */
		buf_in[0] = 0x30;
		retval = read(fd_i2c, buf_in, 2);  	//get size
#endif
		buf_in[0] = 3;
		buf_in[1] = 142;
		if(retval < 0)
		{
			printf("error ocoured when read the size of cpInfo\n");
			goto error_send_authentication;
		}

		lengthipod = buf_in[0];
		lengthipod = (lengthipod<<8) + buf_in[1];

		maxmum = (char)(lengthipod/128 +1);
		left = (char)(lengthipod % 128);

		/* send message to stop io thread */
		send_msg_to_child_process(MFI_MSG_TYPE, MSG_MFI_P_SUSPEND_IOCTL, NULL, 0);
		printf("send msg suspend ioctl\n");
        thread_usleep(100000);

		for(nowmum=0; nowmum<maxmum; nowmum++)  //send cer's content
		{
			memset(buf_out, 0, 200);
			buf_temp[0] = 0x31+nowmum;
			if(i == (maxmum-1))
			{
				buf_temp[1] = left;
			}
			else
				buf_temp[1] = 128;

			/* open */
			fd_i2c = open(I2CDEV, O_RDWR);
			retval = certificatedata(buf_temp);
			close(fd_i2c);

			if(retval < 0)
			{
				printf("error to read CP chip\n");
				goto error_send_authentication;
			}

			if(nowmum == (maxmum-1))
			{
				//the last times
				memset(buf_out, 0, 0x40);
				buf_out[0] = 0x09;
				buf_out[1] = 0x00;
				buf_out[2] = 0x55;
				buf_out[3] = left + 8;//0x16;
				buf_out[4] = 0x00;
				buf_out[5] = 0x15;
				buf_out[6] = transID_hig;
				buf_out[7] = transID_low;
				buf_out[8] = 0x02;
				buf_out[9] = 0x00;
				buf_out[10] = nowmum;
				buf_out[11] = nowmum;

				for(i=0; i<left; i++)
					buf_out[12+i] = buf_temp[i];
				checksum = 0;
				for(i=3; i<=(11+left); i++)
					checksum += buf_out[i];
				buf_out[11+left+1] = (~checksum) +1;

				retval = write(fd_apple, buf_out, 0x40);		//check apple's response
				if (retval < 0)
				{
					printf("error to write usb packet\n");
					goto error_send_authentication;
				}

				retval = my_select();
				if (retval < 0)
				{
					printf("error_at_select\n");
					goto error_send_authentication;
				}

				retval = read(fd_apple, buf_in, IN);
				if(retval < 0)
				{
					printf("error at read usb packet\n");
					goto error_send_authentication;
				}

				if((0x16 == buf_in[5]) && (0x00 == buf_in[8]))
				{
					printf("Conguraturation,Apple Device supported the Info\n");
					//return 0;
				}
				else
				{
					printf("error, Apple Device do not supported the Info\n");
					goto error_send_authentication;
				}

			}
			else 	//not end yet
			{
				buf_out[0] = 0x09;
				buf_out[1] = 0x02;
				buf_out[2] = 0x55;
				buf_out[3] = 0x88;
				buf_out[4] = 0x00;
				buf_out[5] = 0x15;
				buf_out[6] = transID_hig;
				buf_out[7] = transID_low;
				buf_out[8] = 0x02;
				buf_out[9] = 0x00;
				buf_out[10] = nowmum;
				buf_out[11] = maxmum-1;

				checksum = 0;
				for(i=3; i<=11; i++)
					checksum += buf_out[i];
				for(i=0; i<128; i++)
					checksum += buf_temp[i];
				checksum = (~checksum) + 1;

				// 52 + 62 + 14
				for(i=0; i<52; i++)
					buf_out[i+12] = buf_temp[i];
				retval = write(fd_apple, buf_out, 0x40);
				if (retval < 0)
				{
					printf("error to write usb packet\n");
					goto error_send_authentication;
				}

				buf_out[0] = 0x09;
				buf_out[1] = 0x03;
				for(i=0; i<62; i++)
					buf_out[2+i] = buf_temp[52+i];
				retval = write(fd_apple, buf_out, 0x40);
				if (retval < 0)
				{
					printf("error to write usb packet\n");
					goto error_send_authentication;
				}


				memset(buf_out, 0, 0x15);
				buf_out[0] = 0x81;
				buf_out[1] = 0x01;
				for(i=0; i<14; i++)
					buf_out[2+i] = buf_temp[114+i];
				buf_out[16] = checksum;
				retval = write(fd_apple, buf_out, 0x15);
				if (retval < 0)
				{
					printf("error to write usb packet\n");
					goto error_send_authentication;
				}

				retval = my_select();
				if (retval < 0)
				{
					printf("error_at_select\n");
					goto error_send_authentication;
				}

				retval = read(fd_apple, buf_in, IN);
				if (retval < 0)
				{
					printf("error to read usb packet\n");
					goto error_send_authentication;
				}
				else
				{
					if((buf_in[8] == 0x00) && (0x15 == buf_in[9]))
					{
						printf("one packet of certificatedataInfo was sent successfully\n");
					}
					else
					{
						printf("error at send packet certificatedataInfo\n");
						goto error_send_authentication;
					}
				}
			}
		}
		send_msg_to_child_process(MFI_MSG_TYPE, MSG_MFI_P_START_IOCTL, NULL, 0);
		/* wait */
		thread_usleep(100000);
		printf("send msg start ioctl\n");
	}
	return 0;
error_send_authentication:
    send_msg_to_child_process(MFI_MSG_TYPE, MSG_MFI_P_START_IOCTL, NULL, 0);
    printf("send msg start ioctl error\n");
	return -1;
}

/**
 * @brief 读取数字签名发送给CP芯片
 * @author Summer.Wang
 * @return <失败返回-1><成功返回0>
 * @param[out] <buf_out><发送的USB数据>
 * @param[in|out] <buf_in><USB数据读入和查询>
 * @remark 与苹果底层通讯的流程节点封装
 */
int DevAuthenticationSignature()
{
	unsigned char challengedata[21];
	unsigned char retransID_h;
	unsigned char retransID_l;
	unsigned char databuffer[200];

	if (NULL == databuffer)
	{
		return -1;
	}

	retval = my_select();
	if (retval < 0)
	{
		printf("error_at_select\n");
		return -1;
	}

	retval = read(fd_apple, buf_in, IN);
	if(retval < 0)
	{
		printf("error to read usb packet\n");
		return -1;//read GetDevAuthenticationSignature cmd_info failed
	}

	if(0x17 != buf_in[5])
	{
		printf("doesnot_recv_CMDID_0x17\n");
		return -1;		//if not GetDevAuthenticationSignature ,should return -1
	}

	retransID_h = buf_in[6];
	retransID_l = buf_in[7];

	for(i=1; i<=20; i++)
	{
		challengedata[i] = buf_in[i+7];	//read in the msg from apple which should be written into cp chip
	}

	send_msg_to_child_process(MFI_MSG_TYPE, MSG_MFI_P_SUSPEND_IOCTL, NULL, 0);
	usleep(100000);
	fd_i2c = open(I2CDEV, O_RDWR);

	buf_in[0] = 0x20;
	buf_in[1] = 0x00;
	buf_in[2] = 0x14;

	retval = write(fd_i2c, buf_in, 2);
	if(retval < 0)
	{
		printf("error_at_write_i2c_address_0x20\n");
		return -1;
	}

	challengedata[0] = 0x21;
	retval = write(fd_i2c, challengedata, 0x14);
	if(retval < 0)
	{
		printf("error_at_write_i2c_address_0x21\n");
		return -1;
	}

	buf_in[0] = 0x10;
	buf_in[1] = 0x01;
	retval = write(fd_i2c, buf_in, 0x01);
	if(retval < 0)
	{
		printf("error_at_write_i2c_address_0x10\n");
		return -1;
	}

	buf_in[0] = 0x00;
	while(0x00 == buf_in[0])
	{
		buf_in[0] = 0x10;
		retval = read(fd_i2c, buf_in, 0x01);
	}

	buf_in[0] = 0x11;
	retval = read(fd_i2c, buf_in, 0x02);
	while(retval < 0)
	{
		retval = read(fd_i2c, buf_in, 0x02);
	}
	databuffer[0] = 0x12;

	retval = read(fd_i2c, databuffer, buf_in[1]);
	while(retval < 0)
	{
		retval = read(fd_i2c, databuffer, buf_in[1]);
	}
	close(fd_i2c);
	send_msg_to_child_process(MFI_MSG_TYPE, MSG_MFI_P_START_IOCTL, NULL, 0);
	thread_usleep(100000);

	for(i=buf_in[1]-1; i>=0; i--)
	databuffer[i+8] = databuffer[i];
	databuffer[0] = 0x09;
	databuffer[1] = 0x02;
	databuffer[2] = 0x55;
	databuffer[3] = buf_in[1] + 4; //0x84;
	databuffer[4] = 0x00;
	databuffer[5] = 0x18;
	databuffer[6] = retransID_h;
	databuffer[7] = retransID_l;

	checksum = 0;
	for(i=buf_in[1]+7; i>2; i--)	//buf_in[1] -1  + 8 stored the last byte
		checksum += databuffer[i];
	checksum = (~checksum) +1;
	databuffer[(buf_in[1]+8)] = checksum;

	retval = write(fd_apple, databuffer, 9+buf_in[1]);
	if(retval < 0)
	{
		printf("error_at_write_usb\n");
		return -1;
	}

	retval = my_select();
	if (retval < 0)
	{
		printf("error_at_select\n");
		return -1;
	}

	retval = read(fd_apple, buf_in, IN);
	if (retval < 0)
	{
		printf("error to read usb packet\n");
		return -1;
	}

	if((0x19 == buf_in[5]) && (0x00 == buf_in[8]))
	{
		return 0;
	}

	return -1;
}

/**
 * @brief 设置苹果设备到不充电的状态
 * @author Summer.Wang
 * @return <失败返回-1><成功返回0>
 * @param[out] <buf_out><发送的USB数据>
 * @param[in|out] <buf_in><USB数据读入和查询>
 * @remark 与苹果底层通讯的流程节点封装
 */
int SetInternalBatteryChargingState()
{
	buf_out[0] = 0x06;
	buf_out[1] = 0x00;
	buf_out[2] = 0x55;
	buf_out[3] = 0x05;
	buf_out[4] = 0x00;
	buf_out[5] = 0x56;
	buf_out[6] = 0x00;
	buf_out[7] = 0x10;
	buf_out[8] = 0x00;
	buf_out[9] = 0x95;	//checksum

	global_transID = 0x11;

	retval = write(fd_apple, buf_out, 10);
	if(retval < 0)
		return -1;

	retval = my_select();
	if (retval < 0)
	{
		printf("error_at_select\n");
		return -1;
	}

	retval = read(fd_apple, buf_in, IN);
	if(retval < 0)
	{
		printf("error to read usb packet\n");
		return -1;
	}

	if((0x00 == buf_in[8]) && (0x56 == buf_in[9]))
	{
		return 0;
	}

	printf("error to let apple Device to stop charging\n");
	return -1;
}

/**
 * @brief 发送bundle ID,请求开始应用程序
 * @author Summer.Wang
 * @return <失败返回-1><成功返回0>
 * @param[out] <buf_out><发送的USB数据>
 * @param[in|out] <buf_in><USB数据读入和查询>
 * @remark 与苹果底层通讯的流程节点封装
 */
int RequestApplicationLaunch()
{
#ifdef FOR_APPLE_TEST
	int flags = 0;
	buf_out[0] = 0x06;
	buf_out[1] = 0x00;
	buf_out[2] = 0x55;
	buf_out[3] = 31;
	buf_out[4] = 0x00;
	buf_out[5] = 0x64;
	buf_out[6] = (global_transID>>8 | tempID);
	buf_out[7] = (unsigned char)global_transID;
	buf_out[8] = 0x00;
	buf_out[9] = 0x02;
	buf_out[10] = 0x00;
	buf_out[11] = 'c';
	buf_out[12] = 'o';
	buf_out[13] = 'm';
	buf_out[14] = '.';
	buf_out[15] = 'i';
	buf_out[16] = 'B';
	buf_out[17] = 'a';
	buf_out[18] = 'b';
	buf_out[19] = 'y';
	buf_out[20] = 'l';
	buf_out[21] = 'a';
	buf_out[22] = 'b';
	buf_out[23] = 's';
	buf_out[24] = '.';
	buf_out[25] = 'i';
	buf_out[26] = 'B';
	buf_out[27] = 'a';
	buf_out[28] = 'b';
	buf_out[29] = 'y';
	buf_out[30] = 'T';
	buf_out[31] = 'e';
	buf_out[32] = 's';
	buf_out[33] = 't';
	buf_out[34] = '\0';

ApplicationLaunchAgain:
printf("ApplicationLaunchAgain\n");
	if (1 == flags)
	{
		buf_out[9] = 0x01;
		buf_out[6] = (global_transID>>8 | tempID);
		buf_out[7] = (unsigned char)global_transID;	//transID
	}
	checksum = 0;
	for(i=3; i<35; i++)
		checksum += buf_out[i];
	checksum = ~checksum + 1;
	buf_out[35] = checksum;
	global_transID++;

	retval = write(fd_apple, buf_out, 36);
	if(retval < 0)
	{
		printf("error to write usb packet\n");
		return -1;
	}
#else
	int flags = 0;
	buf_out[0] = 0x06;
	buf_out[1] = 0x00;
	buf_out[2] = 0x55;
	buf_out[3] = 30;
	buf_out[4] = 0x00;
	buf_out[5] = 0x64;
	buf_out[6] = (global_transID>>8 | tempID);
	buf_out[7] = (unsigned char)global_transID;
	buf_out[8] = 0x00;
	buf_out[9] = 0x02;
	buf_out[10] = 0x00;
	buf_out[11] = 'c';
	buf_out[12] = 'o';
	buf_out[13] = 'm';
	buf_out[14] = '.';
	buf_out[15] = 'i';
	buf_out[16] = 'B';
	buf_out[17] = 'a';
	buf_out[18] = 'b';
	buf_out[19] = 'y';
	buf_out[20] = 'l';
	buf_out[21] = 'a';
	buf_out[22] = 'b';
	buf_out[23] = 's';
	buf_out[24] = '.';
	buf_out[25] = 'i';
	buf_out[26] = 'B';
	buf_out[27] = 'a';
	buf_out[28] = 'b';
	buf_out[29] = 'y';
	buf_out[30] = 'N';
	buf_out[31] = 'e';
	buf_out[32] = 'w';
	buf_out[33] = '\0';

	if (1 == flags)
	{
		buf_out[9] = 0x01;
		buf_out[6] = (global_transID>>8 | tempID);
		buf_out[7] = (unsigned char)global_transID;	//transID
	}
	checksum = 0;
	for(i=3; i<34; i++)
		checksum += buf_out[i];
	checksum = ~checksum + 1;
	buf_out[34] = checksum;
	global_transID++;

	retval = write(fd_apple, buf_out, 35);
	if(retval < 0)
	{
		printf("error to write usb packet\n");
		return -1;
	}
#endif
	printf("Request to launch the application\n");

	return 0;

}

/**
 * @brief 申请wifi共享
 * @author Summer.Wang
 * @return <失败返回-1><成功返回0>
 * @param[out] <buf_out><发送的USB数据>
 * @param[in|out] <buf_in><USB数据读入和查询>
 * @remark 与苹果底层通讯的流程节点封装
 */
int requestWiFiConnectionInfo()
{

	buf_out[0] = 0x05;
	buf_out[1] = 0x00;
	buf_out[2] = 0x55;
	buf_out[3] = 0x04;
	buf_out[4] = 0x00;
	buf_out[5] = 0x69;
	buf_out[6] = (global_transID>>8 | tempID);
	buf_out[7] = (unsigned char)global_transID;	//transID

	global_transID++;

	checksum = 0;
	for(i=3; i<8; i++)
		checksum += buf_out[i];
	checksum = ~checksum + 1;
	buf_out[8] = checksum;

	retval = write(fd_apple, buf_out, 9);
	if (retval < 0)
	{
		printf("error to write usb packet\n");
		return -1;
	}

	retval = my_select();
	if (retval < 0)
	{
		printf("error_at_select\n");
		return -1;
	}

	retval = read(fd_apple, buf_in, IN);
	if(retval < 0)
	{
		printf("failed to read buf_in\n");
		return -1;
	}

	if(0x69 == buf_in[9])
	{
		if (0x00 == buf_in[8])
		{
			return 0;
		}
		else //many reasons for failed
		{
			return 1;
		}
	}

	printf("error to request wifi information\n");
	return -1;
}

/**
 * @brief 与苹果底层的通讯流程实现
 * @author Summer.Wang
 * @return <失败返回-1><成功返回0>
 * @param[in] <buf_out><USB数据发送>
 * @param[in] <buf_in><USB数据读入和查询>
 */
int CommandUsb()
{
	retval = startIDPS();

	if(retval == 0)
	{
		retval = RequestTranMax();
	}
	else
	{
		printf("startIDPS\n");
		goto monitor_error;//startIDPS failed
	}

	if(retval == 0)
	{
		retval = GetiPodOptionsForLingo();
	}
	else
	{
		printf("RequestTranMax\n");
		goto monitor_error;
	}

	if(retval == 0)
	{
		retval = sendDeviceID();
	}
	else
	{
		printf("GetiPodOptionsForLingo\n");
		goto monitor_error;//GetiPodOptionsForLingo failed
	}

	if(retval == 0)
	{
		retval = AccessoryCapsToken();
	}
	else
	{
		printf("sendDeviceID\n");
		goto monitor_error;//sendDeviceID failed
	}

	if(retval == 0)
	{
		retval = sendAccessoryName();
	}
	else
	{
		printf("AccessoryCapsToken_was_failed\n");
		goto monitor_error;//AccessoryCapsToken failed;
	}

	if(retval == 0)
	{
		retval = sendAccessoryFirmwareVersion();
	}
	else
	{
		printf("sendAccessoryName_was_failed\n");
		goto monitor_error;//sendAccessoryName failed;
	}

	if(0 == retval)
	{
		retval = sendAccessoryHardwareVersion();
	}
	else
	{
		printf("sendAccessoryFirmwareVersion_was_failed\n");
		goto monitor_error;//sendAccessoryFirmwareVersion failed;
	}

	if(0 == retval)
	{
		retval = sendAccessorymanufacturer();
	}
	else
	{
		printf("sendAccessoryHardwareVersion_was_failed\n");
		goto monitor_error;//sendAccessoryHardwareVersion failed;
	}

	if(0 == retval)
	{
		retval = sendAccessoryModel();
	}
	else
	{
		printf("sendAccessorymanufacturer_was_failed\n");
		goto monitor_error;//sendAccessorymanufacturer failed;
	}

	if(0 == retval)
	{
		retval = sendAccessoryInComeSize();
	}
	else
	{
		printf("sendAccessoryModel_was_failed\n");
		goto monitor_error;//failed sendAccessoryModel;
	}

	if(0 == retval)
	{
		retval = sendAccessoryRfCertification();
	}
	else
	{
		printf("sendAccessoryInComeSize_was_failed\n");
		goto monitor_error;//sendAccessoryInComeSize failed;
	}

	if(0 == retval)
	{
		retval = sendAccessoryEAprotocolIndex();
	}
	else
	{
		printf("sendAccessoryRfCertification_was-failed\n");
		goto monitor_error;//sendAccessoryRfCertification failed;
	}

	if(0 == retval)
	{
		retval = sendAccessoryBundleseedIDPrefToken();
	}
	else
	{
		printf("sendAccessoryEAprotocolIndex_was_failed\n");
		goto monitor_error;//sendAccessoryEAprotocolIndex failed;
	}

	if(0 == retval)
	{
		retval = endIDPS();
	}
	else
	{
		printf("sendAccessoryBundleseedIDPrefToken_was_failed\n");
		goto monitor_error;//sendAccessoryBundleseedIDPrefToken failed;
	}

	if(0 == retval)
	{
		//sendAccessoryAuthenticationInfo
		retval = sendAccessoryAuthenticationInfo();
	}
	else
	{
		printf("IDPS have monitor_error\n");
		goto monitor_error;
	}

	if(0 == retval)
	{
		printf("before DevAuthenticationSignature\n");
		retval = DevAuthenticationSignature();
	}
	else
	{
		printf("monitor_error_at_sendAccessoryAuthenticationInfo\n");
		goto monitor_error;
	}

	if(0 == retval)
	{
		printf("after DevAuthenticationSignature\n");
		retval = SetInternalBatteryChargingState();
	}
	else
	{
		printf("monitor_error_at_DevAuthenticationSignature\n");
		goto monitor_error;
	}

	if (0 == retval)
	{
		//retval = requestWiFiConnectionInfo();
		if (0 == requestWiFiConnectionInfo())
		{
			g_nMifStatus = MFI_WIFI_CONNECT_SUCCESS;
			MFI_AppsOk = 1;
		}
		else
		{
            if (1 == retval)
            {
		        wifi_request_flag = 0x02;//command failed
            }

			g_nMifStatus = MFI_WIFI_CONNECT_FAILURE;
		}
	}
	else
	{
		printf("SetInternalBatteryChargingState_was_failed\n");
		goto monitor_error;	//SetInternalBatteryChargingState failed
	}
monitor_error:

return -1;

}

/**
 * @brief 接收wifi帐户与密码并处理
 * @author Summer.Wang
 * @return <失败返回-1><成功返回0>
 * @param[out] <buf_out><发送的USB数据>
 * @param[in|out] <buf_in><USB数据读入和查询>
 * @remark 与苹果底层通讯的流程节点封装
 */
int wifi_msg_deal()
{
	char acTmpBuf[64] = {0};
	int nBufLen = 0;

	switch(buf_in[8])
	{
		case 0x00:
			wifi_share_flag = 1;
			wifi_request_flag = 0x01;
			break;
		case 0x01:
			wifi_share_flag = 2;
			wifi_request_flag = 0x02;
			printf("Connection information unavailable\n");
			break;
		case 0x02:
			wifi_share_flag = 2;
			wifi_request_flag = 0x03;
			printf("User declined to share wifi infor\n");
			break;
		case 0x03:
			wifi_share_flag = 2;
			wifi_request_flag = 0x02;
			printf("Command failed\n");
			break;
		default:
			break;
	}
	if (0x01 == wifi_request_flag)
	{
		if (0x00 == buf_in[9])
		{
			/* just need SSID */
			for(i=10; i<42; i++)
			{
				if(buf_in[i] != 0x00){
					wifi_info.strSSID[i-10] = buf_in[i];
				}
				else
				{
					wifi_info.strSSID[i-10] = '\0';
					break;
				}
			}
			wifi_info.nEnrType = buf_in[9];
		}
		else
		{
			wifi_info.nEnrType = buf_in[9];

			for(i=0; i<96; i++)//retval为read返回值
			{
				ssid_passwd[i] = buf_in[i];
			}
#if 0
			retval = my_select();
			if(retval == 0)
			{
				retval = read(fd_apple, buf_in, IN);
				if(retval < 0)
				{
					printf("error_to_read\n");
					return -1;
				}

				checksum = retval;
				retval = 0;
				for(; retval<checksum; i++,retval++)
					ssid_passwd[i] = buf_in[retval];
			}
#endif

			for(i=10; i<42; i++)
			{
				if(ssid_passwd[i] != 0x00)
					wifi_info.strSSID[i-10] = ssid_passwd[i];
				else
				{
					wifi_info.strSSID[i-10] = '\0';
					break;
				}
			}
			printf("fill in SSID\n");

			for(i=42; i<96; i++)
			{
				if(ssid_passwd[i] != 0x00)
					wifi_info.strPasswd[i-42] = ssid_passwd[i];
				else
				{
					wifi_info.strPasswd[i-42] = '\0';
					break;
				}
			}
			printf("fill in Password\n");

		}
		 wifi_info.lNetType = NET_TYPE_WIRELESS;
		 /* 获取ip类型 */
		 get_config_item_value(CONFIG_WIFI_MODE, acTmpBuf, &nBufLen);

		 if (strcmp(acTmpBuf, "S"))
		 {
			 wifi_info.lIpType = IP_TYPE_DYNAMIC;
		 }
		 else
		 {
			 wifi_info.lIpType = IP_TYPE_STATIC;
		 }
		 memset(acTmpBuf, 0x00, sizeof(acTmpBuf));
		 snprintf(acTmpBuf, sizeof(acTmpBuf), "%d", wifi_info.nEnrType);
		 /* write flash info , remove it to time func */

		 set_config_value_to_ram(CONFIG_WIFI_ENRTYPE, acTmpBuf, strlen(acTmpBuf));
		 set_config_value_to_ram(CONFIG_WIFI_SSID,    wifi_info.strSSID,strlen(wifi_info.strSSID));
		 set_config_value_to_ram(CONFIG_WIFI_PASSWD,  wifi_info.strPasswd,strlen(wifi_info.strPasswd));
		 set_config_value_to_ram(CONFIG_WIFI_ACTIVE,  "y", 1);
		 /* updata flash info */
		 updata_config_to_flash();

		 send_msg_to_child_process(MFI_MSG_TYPE, MSG_MFI_P_SEND_SSID, &wifi_info, sizeof(wifi_info));
		 log_printf("wifi info: %s %s %s %s %s %d %d %d %s %s\n", wifi_info.acGateWay, wifi_info.acIpAddr, wifi_info.acSubNetMask, wifi_info.acWifiDNS, wifi_info.acWiredDNS
				 , wifi_info.lIpType, wifi_info.lNetType, wifi_info.nEnrType, wifi_info.strPasswd, wifi_info.strSSID);
	}

	return 0;
}

/**
 * @brief 线程同步函数
 * @author Summer.Wang
 * @return Null
 * @param[out] <rMFI><代表线程的全局变量>
 * @param[in]  <rChildProcess><代表子进程的全局变量>
 * @note 线程控制函数
 */
void threadSynchronization_MFI() {
	rMFI = 1;
	while(rChildProcess == 0) {
		thread_usleep(0);
	}
	thread_usleep(0);
}

/**
 * @brief 消息队列初始化函数
 * @author Summer.Wang
 * @return Null
 * @note 线程控制函数
 */
void msgModuleInit_MFI() {
	localMsgId_MFI  = msg_queue_get((key_t)CHILD_THREAD_MSG_KEY);
	if(localMsgId_MFI == -1) {
		exit(0);
	}
	remoteMsgId_MFI = msg_queue_get((key_t)CHILD_PROCESS_MSG_KEY);
	if(remoteMsgId_MFI == -1) {
		exit(0);
	}
}

/**
 * @brief 消息队列发送到子进程的函数
 * @author Summer.Wang
 * @return Null
 * @note 线程控制函数
 */
void USB_Msg_Send(int command)
{
	struct MsgData *msg = (struct MsgData *)getData_MFI;

	msg->len = 0;
	msg->type = MFI_MSG_TYPE;
	msg->cmd = command;
	if(msg_queue_snd(remoteMsgId_MFI, getData_MFI, sizeof(struct MsgData) - sizeof(long int))== -1)
	{
		return;
	}
}

/**
 * @brief 消息队列接收和数据处理函数
 * @author Summer.Wang
 * @return Null
 * @note 线程控制函数
 */
void msgModuleHandler_MFI() {
	struct MsgData *msg = (struct MsgData *)getData_MFI;
	int nRet;
	int n;
	AndroidUsb_t *androidUsb_p = &androidUsb_t;

	if(msg_queue_rcv(localMsgId_MFI, getData_MFI, MAX_MSG_LEN, MFI_MSG_TYPE) == -1) {
	}
	else {
		switch(msg->cmd) {
			case 1:
//				extern int android_usb_read_data_and_deal();
//
//				extern int android_usb_identity();
//
//				extern int android_usb_open (void *data); //null
//
//				extern int android_usb_close (void *data); //null
				if(androidUsb_p->device == USB_DEVICE_ANDROID) {
					android_usb_open (NULL);
					break;
				}
				write_log_info_to_file("mfi rcv from io  usb in\n");
				common_write_log("USB: mfi rcv from io  usb in");
				/* 获取device id */
		    	MFI_DeviceID[0] = 4;
		    	MFI_DeviceID[1] = 4;

		    	if (-1 == (fd_i2c = open(I2CDEV, O_RDWR)))
		    	{
		    		MFI_DeviceID[0] = 0;
		    		MFI_DeviceID[1] = 0;
		    		MFI_DeviceID[2] = 2;
		    		MFI_DeviceID[3] = 0;
		    	}
		    	else if(-1 == (nRet = read(fd_i2c, MFI_DeviceID, MFI_DeviceID[1])))
		    	{
		    		MFI_DeviceID[0] = 0;
		    		MFI_DeviceID[1] = 0;
		    		MFI_DeviceID[2] = 2;
		    		MFI_DeviceID[3] = 0;
		    		close(fd_i2c);
		    	}
		    	else
		    	{
		    		close(fd_i2c);
		    	}
		    	/* 在这里如果第一次没有成功打开苹果设备 那么则要置变量 为MFI_READY_OPEN_APPLE_DEV */
		    	if (0 == g_lAppleDeviceState)
		    	{
		    		if((fd_apple = open(DEV, O_RDWR)) < 0)
		    		{
		    		 	g_nMifStatus = MFI_READY_OPEN_APPLE_DEV;
		    		 	g_nOpenAppleRetryTimes++;
		    		}
		    		else
		    		{
		    		    g_lAppleDeviceState = 1;
		    		    CommandUsb();
		    		}
		    	}
		    	else
		    	{
		    		CommandUsb();
		    	}
				break;

			case 2:
				if(androidUsb_p->device == USB_DEVICE_ANDROID) {
					android_usb_close (NULL);
//					break;
				} else {
					if (1 == g_lAppleDeviceState)
					{
						close(fd_apple);
					}
					else
					{
					}
					g_lAppleDeviceState = 0;
				}
                msg->type = MFI_MSG_TYPE;
                msg->cmd =  MSG_MFI_P_INFORM_SWITCH_USB_TO_CAM;
                if(msg_queue_snd(remoteMsgId_MFI, getData_MFI, sizeof(struct MsgData) - sizeof(long int))== -1)
                {
                };
                write_log_info_to_file("snd msg to io switch usb to cam~~~\n");
                common_write_log("USB: mfi switch usb to cam");
                if(androidUsb_p->device == USB_DEVICE_ANDROID) {

                } else {
					MFI_AppsOk = 0;
					g_nMifStatus = MFI_STATE_INVALID;
					flg_get_Rl = 0;
					globle_session_flag = 0;
                }
			break;

			case 3:                        //receive Device ID
				memcpy(MFI_DeviceID, (char *)(msg + 1), 4);
				CommandUsb();
			break;

			case 5:
				if((g_lAppleDeviceState == 1)&&(globle_session_flag == 1)){
					buf_out[12] = 0x0A;
					buf_out[13] = *((char *)(msg + 1));//0x01;
					if(buf_out[13] == 4){
						buf_out[12] = 0x04;
						buf_out[13] = 0x02;
					}
					buf_out[3] = 0x0a;
					checksum = 0;
					for(i=3; i<=(3+ buf_out[3]); i++)
						checksum += buf_out[i];
					checksum = ~checksum + 1;
					buf_out[buf_out[3] + 4] = checksum;
					global_transID++;

					retval = write(fd_apple, buf_out, buf_out[3]+5);
					if (retval < 0)
					{
						return;
					}
				}
				setNetStatusInfo(NET_INFO_CONN_WIFI);
				break;

			case 6:
				if((g_lAppleDeviceState == 1)&&(globle_session_flag == 1)){
					buf_out[12] = 0x04;
					buf_out[13] = *((char *)(msg + 1));//0x01;
					buf_out[3] = 0x0a;
					checksum = 0;
					for(i=3; i<=(3+ buf_out[3]); i++)
						checksum += buf_out[i];
					checksum = ~checksum + 1;
					buf_out[buf_out[3] + 4] = checksum;
					global_transID++;

					retval = write(fd_apple, buf_out, buf_out[3]+5);
					if (retval < 0)
					{
						return;
					}
				}
				setNetStatusInfo(NET_INFO_DHCP);
				break;
			case 7:
				setNetStatusInfo(NET_INFO_DHCP_FAIL);
				break;
			case 8:
				setNetStatusInfo(NET_INFO_REQ_RL);
				break;
			case 9:
				setNetStatusInfo(NET_INFO_REQ_UID);
				break;
			case 10:
				//setNetStatusInfo(NET_INFO_TUTK_LOGIN_OK);
				break;
			case 11:
				//setNetStatusInfo(NET_INFO_TUTK_LOGIN_FAIL);
				break;
			case 12:
				setNetStatusInfo(NET_INFO_UID_OK);
		}

	}
}

/**
 * @brief 应用层USB数据处理和协议实现函数
 * @author Summer.Wang
 * @return <失败返回-1><成功返回0>
 * @param[out] <buf_out><发送的USB数据>
 * @param[in|out] <buf_in><USB数据读入和查询>
 * @note 实现了从打开session开始的与APPS的所有协议，包括工装
 */
int USB_data_process()
{
	int n = 0;
	char buf_temp[8];
	char buf_temp_1[4];
	flag_send = 0;

	/*下面switch为USB数据过来后的一些基础配置
	 *3F：打开一个session的协议头
	 *40：关闭一个session的协议头，没做处理？
	 *43：实现各用户层协议交互的协议头
	 *02：苹果端应答的协议头*/
	switch(buf_in[5])
	{
		/*存储session号*/
		case 0x3f:	//openDataSessionForProtocol
			sessionID[0] = buf_in[8];
			sessionID[1] = buf_in[9];
			globle_session_flag = 1;
		//	break;
		case 0x40:	//CloseDataSession
			globle_session_flag = 0;
            session_flag = 1;
		//	break;
		/*附件端发送应答数据*/
		case 0x43:	//iPodDataTransfer
			buf_out[0] = 0x09;
			buf_out[1] = 0x02;
			buf_out[2] = 0x55;
			buf_out[3] = 0x06;	//length
			buf_out[4] = 0x00;
			buf_out[5] = 0x41;
			buf_out[6] = buf_in[6]; //retransID_h;
			buf_out[7] = buf_in[7]; //retransID_l;
			buf_out[8] = 0x00;	//ack status
			buf_out[9] = buf_in[5];
			checksum = 0;
			for(i=3; i<10; i++)
				checksum += buf_out[i];
			checksum = ~checksum + 1;
			buf_out[10] = checksum;

			retval = write(fd_apple, buf_out, 11);
			if (retval < 0)
			{
				return -1;
			}
			break;
		/*苹果端应答*/
		case 0x02:	//iPodAck
			iPodAck = 1;
			break;
		default:
			break;
	}
	/*--end:所有协议头预处理--*/

	/*处理打开session的命令*/
	if(0x3F == buf_in[5])
	{
		if(1 == session_flag)/*如果初次打开session*/
		{
			session_flag = 0;
		    memset(mark, 0xff, 100);
		    memset(mark_ft, 0xff, 100);

//			if(0x01 != wifi_request_flag)/*如果没有设置wifi*/
//			{
//				/*此处为与苹果设置wifi的交互*/
//				printf("here requestWiFiConnectionInfo again >>>>>>>>>>>>\n");
//				retval = requestWiFiConnectionInfo();
//				if (retval < 0)
//				{
//					printf("error to request wifi info\n");
//					return 0;
//				}
//				if (0 == retval)        /* share success */
//				{
//					printf("here wifi_msg_deal >>>>>>>>>>>>\n");
//					for(i=0; i<3; i++){
//						retval = wifi_msg_deal();
//						if(retval == 0) break;
//						thread_usleep(200000);
//					}
//				}
//			}
//			else
//			{
//				thread_usleep(200000);
//			}
			/*以下为0x0C协议实现*/
			/* send ok to request Camera info */
			buf_out[0] = 0x09;
			buf_out[1] = 0x02;
			buf_out[2] = 0x55;
			buf_out[3] = 0x09;	//length
			buf_out[4] = 0x00;
			buf_out[5] = 0x42;	//AccessoryDataTransfer
			buf_out[6] = (global_transID>>8 | tempID); 	//transID_h;
			buf_out[7] = (unsigned char)global_transID; //transID_l;
			buf_out[8] = sessionID[0];	//sessionID[0]
			buf_out[9] = sessionID[1];	//sessionID[1]
			buf_out[10] = 0x42;//'b';
			buf_out[11] = 0x4d;//'m';
			buf_out[12] = 0x0c;

			checksum = 0;
			for (i=3; i<13; i++)
			{
				checksum += buf_out[i];
			}
			checksum = ~checksum + 1;
			buf_out[13] = checksum;

			global_transID++;
			retval = write(fd_apple, buf_out, 14);
			if (retval < 0)
			{
				return -1;
			}
			usleep(100000);
		}
	}
	/*--end:打开session协议头--*/

	/*处理用户层数据传输的协议头*/
	else if (0x43 == buf_in[5])	//iPodDataTransfer
	{
		/*命令头为BM*/
		if((0x42 == buf_in[10])&&(0x4d == buf_in[11]))
		{
			/*发送缓存的初始化*/
			buf_out[0] = 0x09;
			buf_out[1] = 0x02;
			buf_out[2] = 0x55;
			buf_out[3] = 0x00;	//length
			buf_out[4] = 0x00;
			buf_out[5] = 0x42;	//AccessoryDataTransfer
			buf_out[6] = (global_transID>>8 | tempID); 	//transID_h;
			buf_out[7] = (unsigned char)global_transID; //transID_l;
			buf_out[8] = sessionID[0];	//sessionID[0]
			buf_out[9] = sessionID[1];	//sessionID[1]
			buf_out[10] = 0x42;//'b';
			buf_out[11] = 0x4d;//'m';

			/*消息序号相同，则忽略*/
			if (buf_in[12] == mark[buf_in[12]])
			{
				return 0;
			}
			mark[buf_in[12]] = buf_in[12];//储存消息序号
			/*处理命令字*/
			switch(buf_in[13])
			{
				case 0x01://索要camera注册信息
					get_config_item_value(CONFIG_CAMERA_ENCRYTIONID,camera_ID, &n);
					n = strlen(camera_ID);
					if(n != 172){//没获取到R‘，
		     			flg_get_Rl = MFI_GET_R_FLAG;
						flag_send = 1;//不发送回复
//						USB_Msg_Send(MSG_MFI_P_NO_R);
					}
					else
					{
						n = 0;
//						memcpy(buf_ID,"220C43305048220C43305048",25);
//						set_mac_addr_value(buf_ID);
						get_mac_addr_value(buf_ID, &n);
						DEBUGPRINT(DEBUG_INFO,"the MAC is %s\n",buf_ID);

						/* exist */
						buf_out[12] = 0x02;
						checksum = 0;
						for(i=13; i<25; i++)//后十二个字节为无线mac
						{
							buf_out[i] = buf_ID[i-1];
							checksum += buf_out[i];
						}
						for(i=25; i<197; i++)//R‘有172个字节
						{
							buf_out[i] = camera_ID[i-25];
							checksum += buf_out[i];
						}
						checksum = ~checksum + 1;
						buf_out[197] = checksum;
						buf_out[3] = 194;
					}
					break;

				case 0x05://判断ack应答，如果是出错了，再次发送加密信息一次
					if(0x02 == buf_in[14])
					{
						/* send camera information again */
						buf_out[12] = 0x02;
						checksum = 0;
						for(i=13; i<25; i++)
						{
							buf_out[i] = buf_ID[i-1];
							checksum += buf_out[i];
						}
						for(i=25; i<197; i++)
						{
							buf_out[i] = camera_ID[i-25];
							checksum += buf_out[i];
						}
						checksum = ~checksum + 1;
						buf_out[197] = checksum;
						buf_out[3] = 195;//此长度和上面的不一致？待测试
					}
					else if(0x01 == buf_in[14])
					{
						flag_send = 1;
					}
					break;

				case 0x07:/*  此处 提取发送过来的时区信息 ，未作处理*/
					buf_out[12] = 0x08;
					buf_out[13] = 0x01;
					buf_out[3] = 0x0a;
					break;

				case 0x0D://用户设置静态IP等

					checksum = 0;
					for(i=14; i<30; i++)
						checksum += buf_in[i];
					checksum = ~checksum + 1;

					if (buf_in[30] == checksum)
					{
						utils_istoas(buf_temp,buf_in+14,4);
						set_config_value_to_ram(CONFIG_WIFI_IP,buf_temp,8);

						utils_istoas(buf_temp,buf_in+18,4);
						set_config_value_to_ram(CONFIG_WIFI_SUBNET,buf_temp,8);

						utils_istoas(buf_temp,buf_in+22,4);
						set_config_value_to_ram(CONFIG_WIFI_GATEWAY,buf_temp,8);

						utils_istoas(buf_temp,buf_in+26,4);
						set_config_value_to_ram(CONFIG_WIFI_DNS,buf_temp,8);

						buf_out[12] = 0x0E;
						buf_out[13] = 0x01;
						buf_out[3] = 0x0a;
					}
					else
					{
						buf_out[12] = 0x0E;
						buf_out[13] = 0x03;
						buf_out[3] = 0x0a;
					}
					break;

				case 0x0F://获取固件版本号
					n = 0;
					/**---------给所有读出flash判度是否为空----------**/
					get_config_item_value(CONFIG_SYSTEM_FIRMWARE_VERSION,buf_temp, &n);
					utils_astois(buf_temp_1,buf_temp,4);

					buf_out[12] = 0x10;
					buf_out[13] = buf_temp_1[0];
					buf_out[14] = buf_temp_1[1];
					buf_out[15] = buf_temp_1[2];
					buf_out[16] = buf_temp_1[3];
					checksum = 0;
					for(i=0; i<4; i++)
						checksum += buf_temp_1[i];
					checksum = ~checksum + 1;
					buf_out[17] = checksum;
					buf_out[3] = 0x0E;
					break;

				case 0x11://获取摄像头型号
					/**---------给所有读出flash判度是否为空----------**/
//					memcpy(buf_temp,"M3S",4);
//					set_config_value_to_ram(CONFIG_CAMERA_TYPE,buf_temp,8);
					get_config_item_value(CONFIG_CAMERA_TYPE,buf_temp, &n);

					buf_out[12] = 0x12;
					if(strncmp(buf_temp, "M3S", 4) == 0) {
						buf_out[3] = 0x0E;
						buf_out[13] = 3;
						buf_out[14] = 'M';
						buf_out[15] = '3';
						buf_out[16] = 'S';
						checksum = 0;
						for(i=13; i<17; i++){
							checksum += buf_out[i];
						}
						checksum = ~checksum + 1;
						buf_out[17] = checksum;
					}
					else if(strncmp(buf_temp, "M2", 3) == 0) {
						buf_out[3] = 0x0D;
						buf_out[13] = 2;
						buf_out[14] = 'M';
						buf_out[15] = '2';
						checksum = 0;
						for(i=13; i<16; i++){
							checksum += buf_out[i];
						}
						checksum = ~checksum + 1;
						buf_out[16] = checksum;
					}
					else
						flag_send = 1;
					break;

				case 0x13://索取应用程序版本号
					n = 0;
					/**---------给所有读出flash判度是否为空----------**/
					get_config_item_value(CONFIG_APP_FIRMWARE_VERSION,buf_temp, &n);
					utils_astois(buf_temp_1,buf_temp,4);

					buf_out[12] = 0x14;
					buf_out[13] = buf_temp_1[0];
					buf_out[14] = buf_temp_1[1];
					buf_out[15] = buf_temp_1[2];
					buf_out[16] = buf_temp_1[3];
					checksum = 0;
					for(i=0; i<4; i++)
						checksum += buf_temp_1[i];
					checksum = ~checksum + 1;
					buf_out[17] = checksum;
					buf_out[3] = 0x0E;
					break;

				case 0x15://索取摄像头配置文件版本号
					n = 0;
					/**---------给所有读出flash判度是否为空----------**/
					get_config_item_value(CONFIG_CONF_VERSION,buf_temp, &n);
					utils_astois(buf_temp_1,buf_temp,4);

					buf_out[12] = 0x16;
					buf_out[13] = buf_temp_1[0];
					buf_out[14] = buf_temp_1[1];
					buf_out[15] = buf_temp_1[2];
					buf_out[16] = buf_temp_1[3];
					checksum = 0;
					for(i=0; i<4; i++)
						checksum += buf_temp_1[i];
					checksum = ~checksum + 1;
					buf_out[17] = checksum;
					buf_out[3] = 0x0E;
					break;

				default:
					break;
			}
			/*每次循环下面这个标志位会被置低，以使能USB发送*/
			if (0 == flag_send)
			{
				checksum = 0;
				for(i=3; i<=(3+ buf_out[3]); i++)
					checksum += buf_out[i];
				checksum = ~checksum + 1;
				buf_out[buf_out[3] + 4] = checksum;
				global_transID++;

				retval = write(fd_apple, buf_out, buf_out[3]+5);
				if (retval < 0)
				{
					DEBUGPRINT(DEBUG_INFO,"error to write usb packet\n");
					return -1;
				}
			}
		}
#define __FACTORY_TEST
#ifdef __FACTORY_TEST
		/*命令头为RS*/
		else if ((0x52 == buf_in[10]) && (0x53 == buf_in[11]))
		{
			unsigned char *pointor = buf_out+10;
			system("echo -e Default'\n'WiFi_EnrType='\n'WiFi_SSID='\n'WiFi_Passwd='\n'WiFi_KeyID='\n'Camera_EncrytionID='\n' > /tmp/ssid;ralink_init renew 2860 /tmp/ssid;rm /tmp/ssid");

			/*恢复出厂设置,待测试*/
//			set_config_value_to_flash(CONFIG_CAMERA_ENCRYTIONID,point_tmp,2);
//			set_config_value_to_flash(CONFIG_WIFI_ENRTYPE,point_tmp,2);
//			set_config_value_to_flash(CONFIG_WIFI_SSID,point_tmp,2);
//			set_config_value_to_flash(CONFIG_WIFI_PASSWD,point_tmp,2);

			buf_out[0] = 0x09;
			buf_out[1] = 0x02;
			buf_out[2] = 0x55;
			buf_out[3] = 0x00;	//length
			buf_out[4] = 0x00;
			buf_out[5] = 0x42;	//AccessoryDataTransfer
			buf_out[6] = (global_transID>>8 | tempID); 	//transID_h;
			buf_out[7] = (unsigned char)global_transID; //transID_l;
			buf_out[8] = sessionID[0];	//sessionID[0]
			buf_out[9] = sessionID[1];	//sessionID[1]
			strcpy(pointor, "RSresetbabymonitor");
			pointor = NULL;
			buf_out[28] = 0x01;
			buf_out[3] = 25;
			checksum = 0;
			for(i=3; i<=(3+ buf_out[3]); i++)
				checksum += buf_out[i];
			checksum = ~checksum + 1;
			buf_out[buf_out[3] + 4] = checksum;
			retval = write(fd_apple, buf_out, buf_out[3]+5);
			global_transID++;
			if (retval < 0)
			{
				return -1;
			}

		}
		/*命令头为FT*/
		else if ((0x46 == buf_in[10]) && (0x54 == buf_in[11]))
		{
			buf_out[0] = 0x09;
			buf_out[1] = 0x02;
			buf_out[2] = 0x55;
			buf_out[3] = 0x00;	//length
			buf_out[4] = 0x00;
			buf_out[5] = 0x42;	//AccessoryDataTransfer
			buf_out[6] = (global_transID>>8 | tempID); 	//transID_h;
			buf_out[7] = (unsigned char)global_transID; //transID_l;
			buf_out[8] = sessionID[0];	//sessionID[0]
			buf_out[9] = sessionID[1];	//sessionID[1]
			buf_out[10] = 0x46;	//'F';
			buf_out[11] = 0x54;	//'T';

			if (buf_in[12] == mark_ft[buf_in[12]])
			{
				return 0;
			}
			mark_ft[buf_in[12]] = buf_in[12];

			switch(buf_in[13])
			{
				case 0x01:	//ask fall into test mode,写入camera型号
					retval = buf_in[14];
					retval += 15;
					checksum = 0;
					for(i=15; i<retval; i++)
					{
						buf_variable[i-15] = buf_in[i];
						checksum += buf_in[i];
					}
					checksum = ~checksum + 1;
	//				buf_variable[i-15] = '\0';
					if (checksum == buf_in[i])
					{
						/* set Camera_Type */
						retval = set_config_value_to_flash(CONFIG_CAMERA_TYPE,buf_variable,strlen(buf_variable));
						if (0 == retval)//success
						{
							buf_out[12] = 0x02;
							buf_out[13] = 0x01;
							buf_out[3] = 0x0a;
						}
						else
						{
							buf_out[12] = 0x02;
							buf_out[13] = 0x02;
							buf_out[3] = 0x0a;
						}
					}
					else
					{
						buf_out[12] = 0x02;
						buf_out[13] = 0x03;
						buf_out[3] = 0x0a;
					}
				break;

				/*=============无线mac发送后有线mac立即跟随着传过来===========*/
				case 0x03: 	//wireless mac，写入flash的线程提供的接口是将无线mac和有线mac一起写入的
					checksum = 0;
					for (i=14; i<26; i++)
					{
						checksum += buf_in[i];
						buf_variable[i-2] = buf_in[i];//后十二个为无线
					}
					checksum = ~checksum + 1;
					if (checksum == buf_in[26])
					{
						buf_out[12] = 0x04;
						buf_out[13] = 0x01;
						buf_out[3] = 0x0a;
					}
					else
					{
						buf_out[12] = 0x04;
						buf_out[13] = 0x03;
						buf_out[3] = 0x0a;
					}
				break;

				case 0x05:	//ethernet mac
					checksum = 0;
					for (i=14; i<26; i++)
					{
						checksum += buf_in[i];
						buf_variable[i-14] = buf_in[i];//前十二个为有线
					}
					buf_variable[24] = '\0';
					checksum = ~checksum + 1;
					if (checksum == buf_in[26])
					{
						/* write wireless mac address*/
						/* write eth mac address*/
						retval = set_mac_addr_value(buf_variable);//测试看是否需要加入再读出后比较校验
						if (retval == 1)
						{
							/* failed */
							buf_out[12] = 0x06;
							buf_out[13] = 0x02;
							buf_out[3] = 0x0a;
						}
						else
						{
							buf_out[12] = 0x06;
							buf_out[13] = 0x01;
							buf_out[3] = 0x0a;
						}
					}
					else
					{
						buf_out[12] = 0x06;
						buf_out[13] = 0x03;
						buf_out[3] = 0x0a;
					}
				break;

				case 0x07:	//IP + SUBMASK + gateway
					checksum = 0;
					for (i=14; i<26; i++)
					{
						checksum += buf_in[i];
					}
					checksum = ~checksum + 1;
					if (checksum == buf_in[26])
					{
						/*write mode,ip address, submask, gateway */
						retval = set_config_value_to_flash(CONFIG_WIFI_MODE,"S",1);
						//retval = SUCCESS;
						if (retval == 0)
						{
							buf_variable[0] = 0xff;
						}
						else
						{
							buf_variable[0] = 0xee;
						}

						retval = set_config_value_to_flash(CONFIG_WIRED_MODE,"S",1);
						//retval = SUCCESS;
						if (retval == 0)
						{
							buf_variable[1] = 0xff;
						}
						else
						{
							buf_variable[1] = 0xee;
						}

						sprintf(buf_ID, "%d.%d.%d.%d", (int)buf_in[14], (int)buf_in[15], (int)buf_in[16], (int)buf_in[17]);
						retval = set_config_value_to_flash(CONFIG_WIFI_IP,buf_ID,strlen(buf_ID));
						//retval = SUCCESS;
						if (retval == 0)
						{
							buf_variable[2] = 0xff;
						}
						else
						{
							buf_variable[2] = 0xee;
						}

						retval = set_config_value_to_flash(CONFIG_WIRED_IP,buf_ID,strlen(buf_ID));
						//retval = SUCCESS;
						if (retval == 0)
						{
							buf_variable[3] = 0xff;
						}
						else
						{
							buf_variable[3] = 0xee;
						}

						sprintf(buf_ID, "%d.%d.%d.%d", (int)buf_in[18], (int)buf_in[19], (int)buf_in[20], (int)buf_in[21]);
						retval = set_config_value_to_flash(CONFIG_WIFI_SUBNET,buf_ID,strlen(buf_ID));
						//retval = SUCCESS;
						if (retval == 0)
						{
							buf_variable[4] = 0xff;
						}
						else
						{
							buf_variable[4] = 0xee;
						}

						retval = set_config_value_to_flash(CONFIG_WIRED_SUBNET,buf_ID,strlen(buf_ID));
						//retval = SUCCESS;
						if (retval == 0)
						{
							buf_variable[5] = 0xff;
						}
						else
						{
							buf_variable[5] = 0xee;
						}

						sprintf(buf_ID, "%d.%d.%d.%d", (int)buf_in[22], (int)buf_in[23], (int)buf_in[24], (int)buf_in[25]);
						retval = set_config_value_to_flash(CONFIG_WIFI_GATEWAY,buf_ID,strlen(buf_ID));
						//retval = SUCCESS;
						if (retval == 0)
						{
							buf_variable[6] = 0xff;
						}
						else
						{
							buf_variable[6] = 0xee;
						}

						retval = set_config_value_to_flash(CONFIG_WIRED_GATEWAY,buf_ID,strlen(buf_ID));
						//retval = SUCCESS;
						if (retval == 0)
						{
							buf_variable[7] = 0xff;
						}
						else
						{
							buf_variable[7] = 0xee;
						}

						retval = set_config_value_to_flash(CONFIG_WIFI_DNS,buf_ID,strlen(buf_ID));
						//retval = SUCCESS;
						if (retval == 0)
						{
							buf_variable[8] = 0xff;
						}
						else
						{
							buf_variable[8] = 0xee;
						}

						retval = set_config_value_to_flash(CONFIG_WIRED_DNS,buf_ID,strlen(buf_ID));
						//retval = SUCCESS;
						if (retval == 0)
						{
							buf_variable[9] = 0xff;
						}
						else
						{
							buf_variable[9] = 0xee;
						}

						for (i=0; i<10; i++)
						{
							if (buf_variable[i] != 0xff)
							{
								retval = 0xff;
								break;
							}
						}

						if (retval != 0xff)
						{
							/* success */
							buf_out[13] = 0x01;
							/*=============将有线及无线参数写入后，是否需要设置网络使之生效？===========*/
						}
						else
						{
							/* failed */
							buf_out[13] = 0x02;
						}
						buf_out[12] = 0x08;
						buf_out[3] = 0x0a;

					}
					else
					{
						buf_out[12] = 0x08;
						buf_out[13] = 0x03;
						buf_out[3] = 0x0a;
					}
				break;

				case 0x09:	//factory Mode
					/* clear the temp ip address ,SUBMASK, gateway
					 * reset the mode of wan*/
#if 0
					retval = set_config_value_to_flash(CONFIG_WIFI_MODE,"D",1);
					//retval = SUCCESS;
					if (retval == 0)
					{
						buf_variable[0] = 0xff;
					}
					else
					{
						buf_variable[0] = 0xee;
					}

					retval = set_config_value_to_flash(CONFIG_WIFI_IP,"192.168.1.57",strlen("192.168.1.57"));
					//retval = SUCCESS;
					if (retval == 0)
					{
						buf_variable[1] = 0xff;
					}
					else
					{
						buf_variable[1] = 0xee;
					}

					retval = set_config_value_to_flash(CONFIG_WIFI_SUBNET,"255.255.255.0",strlen("255.255.255.0"));
					//retval = SUCCESS;
					if (retval == 0)
					{
						buf_variable[2] = 0xff;
					}
					else
					{
						buf_variable[2] = 0xee;
					}

					retval = set_config_value_to_flash(CONFIG_WIFI_GATEWAY,"192.168.1.1",strlen("192.168.1.1"));
					//retval = SUCCESS;
					if (retval == 0)
					{
						buf_variable[3] = 0xff;
					}
					else
					{
						buf_variable[3] = 0xee;
					}

					retval = set_config_value_to_flash(CONFIG_WIFI_DNS,"192.168.1.1",strlen("192.168.1.1"));
					if (retval == 0)
					{
						buf_variable[4] = 0xff;
					}
					else
					{
						buf_variable[4] = 0xee;
					}

					retval = set_config_value_to_flash(CONFIG_WIRED_MODE,"D",1);
					//retval = SUCCESS;
					if (retval == 0)
					{
						buf_variable[5] = 0xff;
					}
					else
					{
						buf_variable[5] = 0xee;
					}

					retval = set_config_value_to_flash(CONFIG_WIRED_IP,"192.168.0.57",strlen("192.168.0.57"));
					//retval = SUCCESS;
					if (retval == 0)
					{
						buf_variable[6] = 0xff;
					}
					else
					{
						buf_variable[6] = 0xee;
					}

					retval = set_config_value_to_flash(CONFIG_WIRED_SUBNET,"255.255.255.0",strlen("255.255.255.0"));
					//retval = SUCCESS;
					if (retval == 0)
					{
						buf_variable[7] = 0xff;
					}
					else
					{
						buf_variable[7] = 0xee;
					}

					retval = set_config_value_to_flash(CONFIG_WIRED_GATEWAY,"192.168.0.1",strlen("192.168.0.1"));
					//retval = SUCCESS;
					if (retval == 0)
					{
						buf_variable[8] = 0xff;
					}
					else
					{
						buf_variable[8] = 0xee;
					}

					retval = set_config_value_to_flash(CONFIG_WIRED_DNS,"192.168.0.1",strlen("192.168.0.1"));
					if (retval == 0)
					{
						buf_variable[9] = 0xff;
					}
					else
					{
						buf_variable[9] = 0xee;
					}

					for (i=0; i<10; i++)
					{
						if (buf_variable[i] != 0xff)
						{
							retval = 0xff;
							break;
						}
					}
#endif
					system("echo -e Default'\n'\
Wifi_Mode=D'\n'\
Wifi_IP=192.168.1.57'\n'\
Wifi_Subnet=255.255.255.0'\n'\
Wifi_DNS=192.168.1.1'\n'\
Wifi_Gateway=192.168.1.1'\n'\
Wired_Mode=D'\n'\
Wired_IP=192.168.0.57'\n'\
Wired_Subnet=255.255.255.0'\n'\
Wired_Gateway=192.168.0.1'\n'\
Wired_DNS=192.168.0.1'\n' > /tmp/ssid;ralink_init renew 2860 /tmp/ssid;rm /tmp/ssid");
//					system("echo -e Default'\n'\
//Wifi_Mode=S'\n'\
//Wifi_IP=192.168.1.58'\n'\
//Wifi_Subnet=255.255.255.255'\n'\
//Wifi_DNS=192.168.0.1'\n'\
//Wifi_Gateway=192.168.0.1'\n'\
//Wired_Mode=S'\n'\
//Wired_IP=192.168.0.58'\n'\
//Wired_Subnet=255.255.255.255'\n'\
//Wired_Gateway=192.168.1.1'\n'\
//Wired_DNS=192.168.1.1'\n' > /tmp/ssid;ralink_init renew 2860 /tmp/ssid;rm /tmp/ssid");
//					system("echo -e Default'\n'Wifi_Mode=D'\n'Wifi_IP=192.168.1.57'\n'Wifi_Subnet=255.255.255.0'\n'Wifi_DNS=192.168.1.1'\n'Wifi_Gateway=192.168.1.1'\n'Wired_Mode=D'\n'Wired_IP=192.168.0.57'\n'Wired_Subnet=255.255.255.0'\n'Wired_Gateway=192.168.0.1'\n'Wired_DNS=192.168.0.1'\n' > /tmp/ssid;ralink_init renew 2860 /tmp/ssid;rm /tmp/ssid");

					retval = 0xee;//默认成功
					buf_out[12] = 0x0a;
					if (retval != 0xff)
					{
						/* success */
						buf_out[13] = 0x01;
					}
					else
					{
						/* failed */
						buf_out[13] = 0x02;
						//buf_out[13] = 0x01;
					}
					buf_out[3] = 0x0a;
				break;

				/*=============keyN后KeyE立即跟随着发送过来=============*/
				case 0x0b:	//RSA_KeyN
				if (0x04 != buf_in[14])
				{
					/* three packet size 45 */
					checksum = 0;
					for (i=15; i<60; i++)
					{
						checksum += buf_in[i];
					}
					checksum = ~checksum + 1;
					if (checksum == buf_in[i])
					{
						/* checksum ok */
						for (i=15; i<60; i++)
						{
							buf_ID[(buf_in[14]-1)*45 + i -15] = buf_in[i];
						}
						buf_out[12] = 0x0c;
						buf_out[13] = buf_in[14];
						buf_out[14] = 0x01;
						buf_out[3] = 0x0b;
					}
					else
					{
						/* checksum error */
						buf_out[12] = 0x0c;
						buf_out[13] = buf_in[14];
						buf_out[14] = 0x03;
						buf_out[3] = 0x0b;
					}
				}
				else
				{
					/* last packet 37 */
					checksum = 0;
					for (i=15; i<52; i++)
					{
						checksum += buf_in[i];
					}
					checksum = ~checksum + 1;
					if (checksum == buf_in[i])
					{
						/* checksum ok */
						for (i=15; i<52; i++)
						{
							buf_ID[(buf_in[14]-1)*45 + i -15] = buf_in[i];
						}
//						buf_ID[(buf_in[14]-1)*45 + i -15] = '\0';//新的接口不用自己加0

						//retval = SUCCESS;
						buf_out[12] = 0x0c;
						buf_out[13] = 0x04;
						buf_out[14] = 0x01;
						buf_out[3] = 0x0b;
					}
					else
					{
						/* checksum error */
						buf_out[12] = 0x0c;
						buf_out[13] = buf_in[14];
						buf_out[14] = 0x03;
						buf_out[3] = 0x0b;
					}
				}
				break;

				case 0x0d:	//RSA_KeyE
					retval = 4;
					retval += 14;
					checksum = 0;
					for(i=14; i<retval; i++)
					{
						buf_variable[i-14] = buf_in[i];
						checksum += buf_in[i];
					}
					checksum = ~checksum + 1;
					if (checksum == buf_in[i])
					{
						/* set RSA_KeyE */
//						get_pb_key_value(buf_ID,&n);
						memcpy((buf_ID+172),buf_variable,4);
						retval = set_pb_key_value(buf_ID,176);
						//retval = SUCCESS;
						if (0 == retval)
						{
							buf_out[12] = 0x0e;
							buf_out[13] = 0x01;
							buf_out[3] = 0x0a;
						}
						else
						{
							buf_out[12] = 0x0e;
							buf_out[13] = 0x02;
							buf_out[3] = 0x0a;
						}
					}
					else
					{
						buf_out[12] = 0x0e;
						buf_out[13] = 0x03;
						buf_out[3] = 0x0a;
					}
				break;

				case 0x0f:	//new server IP or url
					if (0x01 == buf_in[14])
					{
						/* IP */
						checksum = 0;
						for (i=15; i<19; i++)
						{
							checksum += buf_in[i];
						}
						checksum = ~checksum + 1;
						sprintf(buf_variable, "%d.%d.%d.%d", (int)buf_in[15], (int)buf_in[16], (int)buf_in[17], (int)buf_in[18]);
						if (checksum == buf_in[19])
						{
							/* new server ip should be writen and check */
							retval = set_config_value_to_flash(CONFIG_SERVER_URL,buf_variable,strlen(buf_variable));
							//retval = SUCCESS;
							if (retval == 0)
							{
								buf_out[13] = 0x01;
							}
							else
							{
								buf_out[13] = 0x02;
							}
							buf_out[12] = 0x10;
							buf_out[3] = 0x0a;
						}
						else
						{
							buf_out[12] = 0x10;
							buf_out[13] = 0x03;
							buf_out[3] = 0x0a;
						}
					}
					else
					{
						/* url */
						retval = buf_in[15];
						retval += 16;
						checksum = 0;
						for(i=16; i<retval; i++)
						{
							buf_variable[i-16] = buf_in[i];
							checksum += buf_in[i];
						}
						checksum = ~checksum + 1;
						buf_variable[i-16] = '\0';//此处为求长度多加了一个\0，测试看是否可行
						if (checksum == buf_in[i])
						{
							retval = set_config_value_to_flash(CONFIG_SERVER_URL,buf_variable,strlen(buf_variable));
							//retval = SUCCESS;
							if (0 == retval)
							{
								buf_out[12] = 0x10;
								buf_out[13] = 0x01;
								buf_out[3] = 0x0a;
							}
							else
							{
								buf_out[12] = 0x10;
								buf_out[13] = 0x02;
								buf_out[3] = 0x0a;
							}
						}
						else
						{
							buf_out[12] = 0x10;
							buf_out[13] = 0x03;
							buf_out[3] = 0x0a;
						}
					}
				break;

				case 0x11:{
					buf_out[12] = 0x12;//命令字
//                	buf_out[13] = 0x03;
					utils_getWifiItem(buf_out + 13, sizeof(buf_out) - 13);//数据字在里面填写了
					buf_out[3] = 12 + buf_out[15];//长度填写
					break;
				}

				case 0x13:{
					if(buf_in[14] == 0x01) {
						setCameraMode(INFO_FACTORY_MODE);
					}
					else {
						setCameraMode(INFO_USER_MODE);
					}
					buf_out[12] = 0x14;
					buf_out[13] = 0x01;
					buf_out[3] = 0x0a;
					break;
				}
			}

			checksum = 0;
			for(i=3; i<=(3+ buf_out[3]); i++)
				checksum += buf_out[i];
			checksum = ~checksum + 1;
			buf_out[buf_out[3] + 4] = checksum;
			global_transID++;

			retval = write(fd_apple, buf_out, buf_out[3]+5);
			if (retval < 0)
			{
				return -1;
			}
		}
#endif

	}
	/*--end:用户层数据传输协议头--*/

	/*--将WIFI设置恢复也作为事件被触发--*/
	else if(0x6a == buf_in[5])
	{
		wifi_msg_deal();//处理WIFI信息
		RequestApplicationLaunch();//请求打开APPS
	}
	return 0;
}

/**
 * @brief 应用层USB数据select读入
 * @author Summer.Wang
 * @return <失败返回-1><成功返回0>
 * @param[out] <buf_in><USB数据读入>
 */
int msgApple_WS()
{
	/*-------USB数据读入-------*/
	int retval = my_select();

	if(retval < 0)
	{
		return -1;
	}

	memset(buf_in, 0, sizeof(buf_in));
	retval = read(fd_apple, buf_in, IN);
	if(retval < 0)	return -1;
	else{
		USB_data_process();
	}

#if 0
	wait_time.tv_sec = 0;
	wait_time.tv_usec = 500000;
	FD_ZERO(&rfds);
	FD_SET(fd_apple, &rfds);//保证在本模块执行前已打开USB设备
	switch(select(fd_apple + 1, &rfds, NULL, NULL, &wait_time))
	{
		case -1:
			break;
		case 0:
			break;
		default:
			if(FD_ISSET(fd_apple, &rfds)){

				int result = read(fd_apple, buf_in, IN);
				if(result < 0)	return -1;
				else USB_data_process();
				return 0;
			}
			break;
	}
#endif
	return 0;
}

/**
 * @brief MFI线程入口函数
 * @author Summer.Wang
 * @return Null
 * @note 线程主循环
 */
void * MFI_Thread(void)
{
	MFI_AppsOk = 0;
	int n = 0;
	msgModuleInit_MFI();
    threadSynchronization_MFI();
    while (1)
    {
    	msgModuleHandler_MFI();
    	android_usb_read_data_and_deal();
		if (g_nMifStatus == MFI_READY_OPEN_APPLE_DEV)
		{
			if (g_nMfiLoopTimes == MFI_OPEN_INVEVAL/MFI_THREAD_SLEEP_TIME)
			{
				 g_nMfiLoopTimes = 0;
				 /* 尝试打开苹果设备 */
				 if((fd_apple = open(DEV, O_RDWR)) < 0)
				 {
					 g_nMifStatus = MFI_READY_OPEN_APPLE_DEV;
					 g_nOpenAppleRetryTimes++;

					 if (10 == g_nOpenAppleRetryTimes)
					 {
						 /* 打开苹果设备失败 */
						 g_nMifStatus = MFI_STATE_INVALID;
						 g_nOpenAppleRetryTimes = 0;
					 }
					 else
					 {
						 g_nOpenAppleRetryTimes ++;
					 }
				 }
				 else
				 {
					 g_lAppleDeviceState = 1;
					 CommandUsb();
				 }
			}
			else
			{
				g_nMfiLoopTimes++;
			}
		}
		if(g_nMifStatus == MFI_WIFI_CONNECT_SUCCESS ||
		   g_nMifStatus	== MFI_WIFI_CONNECT_FAILURE )
		{
			msgApple_WS();
		}
#if 0
		if (g_nMifStatus == MFI_WIFI_CONNECT_SUCCESS)
		{
			retval = wifi_msg_deal();
			if (0 == retval)
			{
				g_nMifStatus = MFI_WIFI_GET_SSID_SUCCESS;
			}
		}
		if ( MFI_WIFI_CONNECT_FAILURE == g_nMifStatus ||
			 MFI_WIFI_GET_SSID_SUCCESS == g_nMifStatus)
		{
			g_nMifStatus = MFI_STATE_INVALID;
			RequestApplicationLaunch();
		}
		if (g_nMifStatus == MFI_APP_LUNCH_OK)
		{
			msgApple_WS();
		}
#endif
//		if(androidUsb_t.device == USB_DEVICE_ANDROID) {
//			get_config_item_value(CONFIG_CAMERA_ENCRYTIONID,camera_ID, &n);
//			n = strlen(camera_ID);
//			if(n != 172){//没获取到R‘
//				flag_send = 1;//不发送回复
//			}
//			else
//			{
//				setNetStatusInfo(NET_INFO_REQ_RL);
//			}
//		}
		if(flg_get_Rl == MFI_GET_R_FLAG)//自在APPS触发获取R‘查询没有时进入该部分，反复进行查询
		{
			buf_out[0] = 0x09;
			buf_out[1] = 0x02;
			buf_out[2] = 0x55;
			buf_out[3] = 0x00;	//length
			buf_out[4] = 0x00;
			buf_out[5] = 0x42;	//AccessoryDataTransfer
			buf_out[6] = (global_transID>>8 | tempID); 	//transID_h;
			buf_out[7] = (unsigned char)global_transID; //transID_l;
			buf_out[8] = sessionID[0];	//sessionID[0]
			buf_out[9] = sessionID[1];	//sessionID[1]
			buf_out[10] = 0x42;//'b';
			buf_out[11] = 0x4d;//'m';

			flag_send = 0;
			get_config_item_value(CONFIG_CAMERA_ENCRYTIONID,camera_ID, &n);
			n = strlen(camera_ID);
			if(n != 172){//没获取到R‘
				flag_send = 1;//不发送回复
			}
			else
			{
				setNetStatusInfo(NET_INFO_REQ_RL);
				flg_get_Rl = 0;
				n = 0;
				get_mac_addr_value(buf_ID, &n);

				/* exist */
				buf_out[12] = 0x02;
				checksum = 0;
				for(i=13; i<25; i++)//后十二个字节为无线mac
				{
					buf_out[i] = buf_ID[i-1];
					checksum += buf_out[i];
				}
				for(i=25; i<197; i++)//R‘有172个字节
				{
					buf_out[i] = camera_ID[i-25];
					checksum += buf_out[i];
				}
				checksum = ~checksum + 1;
				buf_out[197] = checksum;
				buf_out[3] = 194;
			}
			if (0 == flag_send)
			{
				checksum = 0;
				for(i=3; i<=(3+ buf_out[3]); i++)
					checksum += buf_out[i];
				checksum = ~checksum + 1;
				buf_out[buf_out[3] + 4] = checksum;
				global_transID++;

				retval = write(fd_apple, buf_out, buf_out[3]+5);
				if (retval < 0)
				{

				}
			}
		}

     	/* mfi线程sleep时间 100ms */
     	thread_usleep(MFI_THREAD_SLEEP_TIME);
    }

}

/**
 * @brief MFI线程创建函数
 * @author Summer.Wang
 * @return Null
 * @note 线程控制函数
 */
int Create_MFI_Thread(void)
{
	int Create_MFI;
	Create_MFI = thread_start(MFI_Thread);
	return Create_MFI;
}

