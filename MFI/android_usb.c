/**	
   @file android_usb.c
   @brief 应用程序主文件
   @note
   author: yangguozheng
   date: 2014-07-10
   mcdify record: creat this file.
   author: yangguozheng
   date: 2014-07-10
   modify record: add a function
*/


#include <stdio.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <stdint.h>
#include <fcntl.h>

#include "f_accessory.h"
#include "rt5350_ioctl.h"

#ifdef _ANDROID_USB_IMP
#include "common/thread.h"
#include "common/common.h"
#include "common/msg_queue.h"
#include "common/common_func.h"
#include "common/common_config.h"
#include "common/logfile.h"
#include "NewsChannel/NewsUtils.h"
#include "common/utils.h"
#include "common/logserver.h"
#else
#define log_printf(format, arg...)
#endif
#include "MFI.h"

#define printf_i(format, arg...) fprintf(stdout, "%s %d "format, __FUNCTION__, __LINE__, ##arg)

#define printf(format, arg...) fprintf(stdout, "%s %d "format, __FUNCTION__, __LINE__, ##arg); \
	log_printf("%s %d "format, __FUNCTION__, __LINE__, ##arg)

#define perror(format, arg...) fprintf(stdout, "%s %d errno %d errstr %s"format, __FUNCTION__, __LINE__, errno, strerror(errno), ##arg); \
	log_printf("%s %d errno %d errstr %s"format, __FUNCTION__, __LINE__, errno, strerror(errno), ##arg)

#define perror_i(format, arg...) fprintf(stdout, "%s %d errno %d errstr %s"format, __FUNCTION__, __LINE__, errno, strerror(errno), ##arg)


#define BUF_SIZE   512

#define ANDROID_USB_DATA_HEAD   "AUSB"

#define USB_DIR_OUT			0		/* to device */
#define USB_DIR_IN			0x80		/* to host */

/** @brief 模块全局属性 */
AndroidUsb_t androidUsb_t = {0, 0, 0, 0, NULL, 0, {0, 0}, {0}, {0}, 0};

/** @brief USB头部数据结构 */
typedef struct UsbDataHead_t {
	char head[4];  //<头部
	int cmd;       //<命令
	int len;       //<长度
}__attribute__ ((packed)) UsbDataHead_t;

enum {
	USB_COMM_ANDROID_BEGIN = 1, //<通信开始
};

enum {
	ANDROID_USB_COMMUNICATION_BEGIN = 0x01, //<通信开始
	ANDROID_USB_SET_WIFI = 0x03,                   //<设置wifi
	ANDROID_USB_GET_NETWORK_INFO = 0x05,           //<获取网络信息
	ANDROID_USB_GET_REGISTER_INFO = 0x07,          //<获取注册信息
	ANDROID_USB_SET_STATIC_IP = 0x09,              //<设置静态ip
	ANDROID_USB_GET_CAMERA_STATUS_INFO = 0x0B,     //<获取摄像头状态信息
	ANDROID_USB_SWITCH_TO_FACTORY_SETTING = 0x0D,  //<回出厂设置
	ANDROID_USB_GET_CAMERA_STATUS = 0x0F,          //<获取摄像头状态
	ANDROID_USB_WRITE_MAC_KEYNE = 0x11,            //<mac及公钥写入
	ANDROID_USB_GET_CAMERA_INFO = 0x13,            //<获取摄像头信息
	ANDROID_USB_SET_SERVICE_ADDR = 0x15,           //<设置服务器地址
	ANDROID_USB_GET_MAC_KEYNE = 0x20,              //<获取mac及公钥
};


/** 定义支持的手机型号 */
enum
{
ANDROID_VENDOR_ID_ACER                   = 0x0502,
ANDROID_VENDOR_ID_ASUS                   = 0x0b05,
ANDROID_VENDOR_ID_Dell                   = 0x413c,
ANDROID_VENDOR_ID_FOXCONN                = 0x0489,
ANDROID_VENDOR_ID_FUJITSU                = 0x04c5,
ANDROID_VENDOR_ID_FUJITSU_TOSHIBA        = 0x04c5,
ANDROID_VENDOR_ID_GARMIN_ASUS            = 0x091e,
ANDROID_VENDOR_ID_GOOGLE                 = 0x18d1,
ANDROID_VENDOR_ID_HAIER                  = 0x201E,
ANDROID_VENDOR_ID_HISENSE                = 0x109b,
ANDROID_VENDOR_ID_HTC                    = 0x0bb4,
ANDROID_VENDOR_ID_HUAWEI                 = 0x12d1,
ANDROID_VENDOR_ID_K_TOUCH                = 0x24e3,
ANDROID_VENDOR_ID_KT_TECH                = 0x2116,
ANDROID_VENDOR_ID_KYOCERA                = 0x0482,
ANDROID_VENDOR_ID_LENOVO                 = 0x17ef,
ANDROID_VENDOR_ID_LG                     = 0x1004,
ANDROID_VENDOR_ID_MOTOROLA               = 0x22b8,
ANDROID_VENDOR_ID_MTK                    = 0x0e8d,
ANDROID_VENDOR_ID_NEC                    = 0x0409,
ANDROID_VENDOR_ID_NOOK                   = 0x2080,
ANDROID_VENDOR_ID_NVIDIA                 = 0x0955,
ANDROID_VENDOR_ID_OTGV                   = 0x2257,
ANDROID_VENDOR_ID_PANTECH                = 0x10a9,
ANDROID_VENDOR_ID_PEGATRON               = 0x1d4d,
ANDROID_VENDOR_ID_PHILIPS                = 0x0471,
ANDROID_VENDOR_ID_PMC_SIERRA             = 0x04da,
ANDROID_VENDOR_ID_QUAL_COMM              = 0x05c6,
ANDROID_VENDOR_ID_SK_TELESYS             = 0x1f53,
ANDROID_VENDOR_ID_SAMSUNG                = 0x04e8,
ANDROID_VENDOR_ID_SHARP                  = 0x04dd,
ANDROID_VENDOR_ID_SONY                   = 0x054c,
ANDROID_VENDOR_ID_SONY_ERICSSON          = 0x0fce,
ANDROID_VENDOR_ID_TELEEPOCH              = 0x2340,
ANDROID_VENDOR_ID_TOSHIBA                = 0x0930,
ANDROID_VENDOR_ID_ZTE                    = 0x19d2,
//ANDROID_VENDOR_ID_HONGMI                 = 0xe980,
};

//建立支持的数组
unsigned int aunSupportDevice[] =
{
ANDROID_VENDOR_ID_ACER,
ANDROID_VENDOR_ID_ASUS,
ANDROID_VENDOR_ID_Dell,
ANDROID_VENDOR_ID_FOXCONN,
ANDROID_VENDOR_ID_FUJITSU,
ANDROID_VENDOR_ID_FUJITSU_TOSHIBA,
ANDROID_VENDOR_ID_GARMIN_ASUS,
ANDROID_VENDOR_ID_GOOGLE,
ANDROID_VENDOR_ID_HAIER,
ANDROID_VENDOR_ID_HISENSE,
ANDROID_VENDOR_ID_HTC,
ANDROID_VENDOR_ID_HUAWEI,
ANDROID_VENDOR_ID_K_TOUCH,
ANDROID_VENDOR_ID_KT_TECH,
ANDROID_VENDOR_ID_KYOCERA,
ANDROID_VENDOR_ID_LENOVO,
ANDROID_VENDOR_ID_LG,
ANDROID_VENDOR_ID_MOTOROLA,
ANDROID_VENDOR_ID_MTK,
ANDROID_VENDOR_ID_NEC,
ANDROID_VENDOR_ID_NOOK,
ANDROID_VENDOR_ID_NVIDIA,
ANDROID_VENDOR_ID_OTGV,
ANDROID_VENDOR_ID_PANTECH,
ANDROID_VENDOR_ID_PEGATRON,
ANDROID_VENDOR_ID_PHILIPS,
ANDROID_VENDOR_ID_PMC_SIERRA,
ANDROID_VENDOR_ID_QUAL_COMM,
ANDROID_VENDOR_ID_SK_TELESYS,
ANDROID_VENDOR_ID_SAMSUNG,
ANDROID_VENDOR_ID_SHARP,
ANDROID_VENDOR_ID_SONY,
ANDROID_VENDOR_ID_SONY_ERICSSON,
ANDROID_VENDOR_ID_TELEEPOCH,
ANDROID_VENDOR_ID_TOSHIBA,
ANDROID_VENDOR_ID_ZTE,
//ANDROID_VENDOR_ID_HONGMI,
};

#define TRUE  1
#define FALSE 0

//查看是否是有效的安卓设备
int if_valid_android_device(unsigned int nVendorId)
{
int i = 0;

    //printf("nVendorIdnVendorIdnVendorId = %ld\n", nVendorId);
    for(i = 0; i < sizeof(aunSupportDevice)/sizeof(unsigned int); i++)
    {
    //printf("aunSupportDevice[%d] = %d\n", i, aunSupportDevice[i]);
    if (nVendorId == aunSupportDevice[i])
    {
            return TRUE;
    }
    }

    return FALSE;
}

/** @brief 协议实现函数格式 */
typedef int (*android_usb_fun)(void *);

/** @brief 协议实现条目格式，用于存储命令号及对应函数 */
typedef struct AndroidUsbItem_t {
	int cmd;    //<命令
	void *data; //<函数
}AndroidUsbItem_t;


/** @brief 函数声明 */
void ioctl_usb_switch_control ();

void ioctl_usb_find_android_device ();

int android_usb_open (void *data);

int android_usb_close (void *data);

int android_usb_communication_begin(void *data);

int android_usb_set_wifi (void *data);

int android_usb_get_network_info (void *data);

int android_usb_get_register_info (void *data);

int android_usb_set_static_ip (void *data);

int android_usb_get_camera_status_info (void *data);

int android_usb_switch_to_factory_setting (void *data);

int android_usb_get_camera_status (void *data);

int android_usb_write_mac_and_keyne (void *data);

int android_usb_get_camera_info (void *data);

int android_usb_set_service_addr (void *data);

int android_usb_get_mac_keyne (void *data);

int android_usb_before_deal (void *data);

int android_usb_after_deal (void *data);

void android_usb_pack_head(UsbDataHead_t *data, int cmd, int len);

/** @brief 协议列表 */
AndroidUsbItem_t androidUsbTable_t[] = {
		{ANDROID_USB_COMMUNICATION_BEGIN, android_usb_communication_begin},
		{ANDROID_USB_SET_WIFI, android_usb_set_wifi},
		{ANDROID_USB_GET_NETWORK_INFO, android_usb_get_network_info},
		{ANDROID_USB_GET_REGISTER_INFO, android_usb_get_register_info},
		{ANDROID_USB_SET_STATIC_IP, android_usb_set_static_ip},
		{ANDROID_USB_GET_CAMERA_STATUS_INFO, android_usb_get_camera_status_info},
		{ANDROID_USB_SWITCH_TO_FACTORY_SETTING, android_usb_switch_to_factory_setting},
		{ANDROID_USB_GET_CAMERA_STATUS, android_usb_get_camera_status},
		{ANDROID_USB_WRITE_MAC_KEYNE, android_usb_write_mac_and_keyne},
		{ANDROID_USB_GET_CAMERA_INFO, android_usb_get_camera_info},
		{ANDROID_USB_SET_SERVICE_ADDR, android_usb_set_service_addr},
		{ANDROID_USB_GET_MAC_KEYNE, android_usb_get_mac_keyne}
};

#if 0
void ioctl_usb_switch_control () {

}
#endif

#include "usb.h"

void milli_sleep(int millis) {
	struct timespec tm;

	tm.tv_sec = 0;
	tm.tv_nsec = millis * 1000000;
	nanosleep(&tm, NULL);
}

void print_endpoint(struct usb_endpoint_descriptor *endpoint) {
	printf("      bEndpointAddress: %02xh\n", endpoint->bEndpointAddress);
	printf("      bmAttributes:     %02xh\n", endpoint->bmAttributes);
	printf("      wMaxPacketSize:   %d\n", endpoint->wMaxPacketSize);
	printf("      bInterval:        %d\n", endpoint->bInterval);
	printf("      bRefresh:         %d\n", endpoint->bRefresh);
	printf("      bSynchAddress:    %d\n", endpoint->bSynchAddress);
}

void print_altsetting(struct usb_interface_descriptor *interface) {
	int i;

	printf("    bInterfaceNumber:   %d\n", interface->bInterfaceNumber);
	printf("    bAlternateSetting:  %d\n", interface->bAlternateSetting);
	printf("    bNumEndpoints:      %d\n", interface->bNumEndpoints);
	printf("    bInterfaceClass:    %d\n", interface->bInterfaceClass);
	printf("    bInterfaceSubClass: %d\n", interface->bInterfaceSubClass);
	printf("    bInterfaceProtocol: %d\n", interface->bInterfaceProtocol);
	printf("    iInterface:         %d\n", interface->iInterface);

	for (i = 0; i < interface->bNumEndpoints; i++)
		print_endpoint(&interface->endpoint[i]);
}

void print_interface(struct usb_interface *interface) {
	int i;

	for (i = 0; i < interface->num_altsetting; i++)
		print_altsetting(&interface->altsetting[i]);
}

void print_configuration(struct usb_config_descriptor *config) {
	int i;

	printf("  wTotalLength:         %d\n", config->wTotalLength);
	printf("  bNumInterfaces:       %d\n", config->bNumInterfaces);
	printf("  bConfigurationValue:  %d\n", config->bConfigurationValue);
	printf("  iConfiguration:       %d\n", config->iConfiguration);
	printf("  bmAttributes:         %02xh\n", config->bmAttributes);
	printf("  MaxPower:             %d\n", config->MaxPower);

	for (i = 0; i < config->bNumInterfaces; i++)
		print_interface(&config->interface[i]);
}

void send_string(struct usb_dev_handle *device, int index, const char* string) {

	int ret = usb_control_msg(device, USB_DIR_OUT | USB_TYPE_VENDOR,
			ACCESSORY_SEND_STRING, 0, index, (void *) string,
			strlen(string) + 1, 0);

	// some devices can't handle back-to-back requests, so delay a bit
	milli_sleep(10);

}

int print_device(struct usb_device *dev, int level) {
	usb_dev_handle *udev;
	char description[256];
	char string[256];
	int ret, i;

	udev = usb_open(dev);
	if (udev) {
		//RT5350 COME IN
		if (dev->descriptor.iManufacturer) {
			ret = usb_get_string_simple(udev, dev->descriptor.iManufacturer,
					string, sizeof(string));
			if (ret > 0) {
				snprintf(description, sizeof(description), "%s - ", string);
			} else {
				//RT5350 COME IN
				//	printf("__LINE__ :%d description:%s\n",__LINE__,description);
				snprintf(description, sizeof(description), "%04X - ",
						dev->descriptor.idVendor);
			}
		} else {
			snprintf(description, sizeof(description), "%04X - ",
					dev->descriptor.idVendor);
		}
		if (dev->descriptor.iProduct) {
			//RT5350 COME IN
			//	printf("__LINE__ :%d description:%s\n",__LINE__,description);
			ret = usb_get_string_simple(udev, dev->descriptor.iProduct, string,
					sizeof(string));
			if (ret > 0) {
				snprintf(description + strlen(description),
						sizeof(description) - strlen(description), "%s",
						string);
			} else {
				//RT5350 COME IN
				//	printf("__LINE__ :%d description:%s\n",__LINE__,description);
				snprintf(description + strlen(description),
						sizeof(description) - strlen(description), "%04X",
						dev->descriptor.idProduct);
			}
		} else {
			snprintf(description + strlen(description),
					sizeof(description) - strlen(description), "%04X",
					dev->descriptor.idProduct);

		}
	} else {
		snprintf(description, sizeof(description), "%04X - %04X",
				dev->descriptor.idVendor, dev->descriptor.idProduct);
	}
//	printf("__LINE__ :%d description:%s\n",__LINE__,description);
	printf("%.*sDev #%d: %s\n", level * 2, "                    ", dev->devnum,
			description);

	int verbose = 0;

	if (udev && verbose) {
		//	printf("__LINE__ :%d \n",__LINE__);
		if (dev->descriptor.iSerialNumber) {
			ret = usb_get_string_simple(udev, dev->descriptor.iSerialNumber,
					string, sizeof(string));
			if (ret > 0) {
				//		printf("__LINE__ :%d description:%s\n",__LINE__,description);
				printf("%.*s  - Serial Number: %s\n", level * 2,
						"                    ", string);
			}
		} else {
			//	printf("__LINE__ :%d \n",__LINE__);
		}
	}

	if (udev)
		usb_close(udev);

	if (verbose) {
		if (!dev->config) {
			printf("  Couldn't retrieve descriptors\n");
			return 0;
		}

		for (i = 0; i < dev->descriptor.bNumConfigurations; i++)
			print_configuration(&dev->config[i]);
	} else {
		for (i = 0; i < dev->num_children; i++)
			print_device(dev->children[i], level + 1);
	}

	return 0;
}

/** @brief 设备支持列表 */
struct usb_device * scan_android_device(struct usb_device *dev){
	for(;dev;dev = dev->next){
		printf("idVendor :%4X\n",dev->descriptor.idVendor);
		if (dev->descriptor.idVendor == 0x18D1 || dev->descriptor.idVendor == 0x22B8
				|| dev->descriptor.idVendor == 0x04e8) {
			printf("Normal Android Device \n");
			return dev;
		}else if( dev->descriptor.idVendor == 0x2717){
			printf("MIUI Android Device \n");
			return dev;
		}else if( dev->descriptor.idVendor == 0x0bb4){
			printf("HTC Android Device \n");
			return dev;
		}else if( dev->descriptor.idVendor == 0x22b8){
			printf("motorola Android Device \n");
			return dev;
		}else if( dev->descriptor.idVendor == 0x05ac){
			printf("05ac:12ab Apple Computer, Inc. \n");
			return dev;
		}else if( dev->descriptor.idVendor == 0x04e8){
			printf("04e8:6860 Samsung Electronics Co., Ltd\n");
			return dev;
		}else if( dev->descriptor.idVendor == 0x12d1){
			printf("04e8:6860 huawei Electronics Co., Ltd\n");
			return dev;
		}else if( dev->descriptor.idVendor == 0x109b){
			printf("04e8:6860 huawei Electronics Co., Ltd\n");
			return dev;
		} else if( dev->descriptor.idVendor == 0x18D1){
			printf("04e8:6860 huawei Electronics Co., Ltd\n");
			return dev;
		}
		else if(TRUE == if_valid_android_device(dev->descriptor.idVendor)){
			return dev;
		}
	}

	return NULL;
}


int scan_android_usb_bus(){
	int nRet = 0;
	/*初始化 usb_bus 结构*/
	struct usb_bus *bus;
	struct usb_device *dev;
	struct usb_device *android_dev;
	AndroidUsb_t *self = &androidUsb_t;
	usb_set_debug(3);
	usb_init();
	/*int SensorId = 0;*/
	/*查找bus 在这里将所有bus的结果都读出来，读到一个全局的结构体里面*/
	int bus_num = usb_find_busses();
	/*查找devices 在这里将所有bus的结果都读出来，读到一个全局的结构体里面*/
	int dev_num = usb_find_devices();
	for (bus = usb_busses; bus; bus = bus->next) {
		print_device(bus->devices, 0);
		if(NULL == scan_android_device(bus->devices) /*&& bus->devices >0*/ ){
			/*SensorId = bus->devices->descriptor.idVendor;*/
			self->device = USB_DEVICE_ANDROID;
		} else {
			self->device = 0;
		}
	}
	nRet = self->device;
	return nRet;
}
/**
   @brief android USB 设备识别
   @author yangguozheng 
   @return {0:有Android USB设备，-1：没有Android USB 设备}
   @note usb切换时调用，若无mfi设备则调用此函数
 */
int android_usb_identity() {
	/*初始化 usb_bus 结构*/
	struct usb_bus *bus;
	struct usb_device *dev;
	struct usb_device *android_dev;
	static int start_flag = 0;
	/*详细结果显示模式*/
	if(0 == start_flag) {
		usb_set_debug(3);
		usb_init();
//		start_flag = 1;
		printf("initial of android usb when plug in\n");
	}
	AndroidUsb_t *self = &androidUsb_t;

	/*查找bus 在这里将所有bus的结果都读出来，读到一个全局的结构体里面*/
	int bus_num = usb_find_busses();
	/*查找devices 在这里将所有bus的结果都读出来，读到一个全局的结构体里面*/
	int dev_num = usb_find_devices();
	usb_dev_handle *udev;

	printf("bus_num = %d  dev_num = %d \n", bus_num, dev_num);

	char camera_type[10] = {0};
	int m;

	get_config_item_value(CONFIG_CAMERA_TYPE, camera_type, &m);
	if(m <= 0 || m > sizeof(camera_type) - 1) {
//		memcpy(camera_type, "other", strlen("other"));
		strcpy(camera_type, "other");
	}

	printf("usb begin model type %s\n", camera_type);


	for (bus = usb_busses; bus; bus = bus->next) {
		printf_i("1\n");
//		for(;bus->devices->descriptor.idVendor != 0;bus->devices->next){
//			scan_android_device(bus->devices);
//		}
		print_device(bus->devices, 0);

		if((android_dev = scan_android_device(bus->devices)) == NULL){
			printf_i("2\n");
		}else{
			printf_i("1\n");
			udev = usb_open(android_dev);
			if(udev){

				printf("This Device is Android Accessory \n");
				printf("Found possible android device - attempting to switch to accessory mode\n");
				uint16_t protocol;

				//发送消息到缺省控制管道。
				int send_len = usb_control_msg(udev, USB_DIR_IN | USB_TYPE_VENDOR,
											ACCESSORY_GET_PROTOCOL, 0, 0, (char *)&protocol, sizeof(protocol), 0);
				if (send_len == 2) {
					printf("device supports protocol version %d\n", protocol);
				}else{
					fprintf(stderr, "failed to read protocol version\n");
				}

				send_string(udev, ACCESSORY_STRING_MANUFACTURER,"Google, Inc.");
//				send_string(udev, ACCESSORY_STRING_MODEL, "M3S");camera_type
				send_string(udev, ACCESSORY_STRING_MODEL, camera_type);
				send_string(udev, ACCESSORY_STRING_DESCRIPTION,"Accessory Chat");
				send_string(udev, ACCESSORY_STRING_VERSION, "1.0");
//				send_string(udev, ACCESSORY_STRING_URI,"http://www.android.com");
//				send_string(udev, ACCESSORY_STRING_SERIAL, "1234567890");

				send_len = usb_control_msg(udev, USB_DIR_OUT | USB_TYPE_VENDOR,
											ACCESSORY_START, 0, 0, 0, 0, 0);
	//			flag_stat =  RET_FIND_POSSIBLE_DEV;
				//初始化USB接口要求
//				usb_claim_interface(udev, android_dev->config->bNumInterfaces);
				usb_close(udev);
				//标识识修改，插入的是Android USB 设备
				self->device = USB_DEVICE_ANDROID;
				printf("android usb find ok, self->device = USB_DEVICE_ANDROID\n");
//				break;
			}
			printf_i("1.2\n");
		}
		printf_i("2.2\n");
	}
	printf_i("3\n");
	android_dev = NULL;
	if(self->device == USB_DEVICE_ANDROID) {
		return 0;
	}
//	while(1){
//		printf(".\n");
//		sleep(1);
//	}
	return -1;
}

/**
   @brief Android USB 写函数封装
   @author yangguozheng 
   @param[in] sDevice 设备句柄
   @param[in] endpoint_out 输出端点
   @param[in] buf 输入数据缓冲
   @param[in] len 数据长度
   @return 长度
   @note
 */
int android_usb_write(struct usb_dev_handle *sDevice, int endpoint_out, char *buf, int len) {
	int w_len = usb_bulk_write(sDevice,endpoint_out, buf, len,1000);
	return w_len;
}

/**
   @brief Android USB 读函数封装
   @author yangguozheng 
   @param[in] sDevice 设备句柄
   @param[in] endpoint_out 输出端点
   @param[out] buf 输入数据缓冲
   @param[in] len 数据长度
   @return 长度
   @note
 */
int android_usb_read(struct usb_dev_handle *sDevice, int endpoint_in, char *buf, int len) {
	int w_len = usb_bulk_read(sDevice,endpoint_in, buf, len,1000);
	return w_len;
}

/**
   @brief Android USB 读函数封装，根据协议进行封装的简单形式
   @author yangguozheng 
   @return 长度
   @note
 */
int android_usb_read_data() {
	AndroidUsb_t *self = &androidUsb_t;
	int ret = android_usb_read(self->sDevice, self->endpoint[0], self->bufrcv, BUF_SIZE);
	return ret;
}

/**
   @brief Android USB 读处理函数封装，根据协议进行封装的简单形式，用于select读取，外部调用
   @author yangguozheng
   @return 长度
   @note
 */
int android_usb_read_data_and_deal() {
	AndroidUsb_t *self = &androidUsb_t;
	struct usb_dev_handle *sDevice = self->sDevice;
	int in_point = self->endpoint[0];
	if(self->device != USB_DEVICE_ANDROID || NULL == sDevice) {
//		printf("android not plug in device %d point %4X\n", sDevice, in_point);
		return -1;
	}
	bzero(self->bufrcv, BUF_SIZE);
	bzero(self->bufsnd, BUF_SIZE);
	int ret = android_usb_read(sDevice, in_point, self->bufrcv, BUF_SIZE);
	if(ret > 0) {
		printf("android_usb_read ret %d %d %4X\n", ret, sDevice, in_point);
		ret = android_usb_msg_deal(self);
		return ret;
	}
	if(-19 == ret) {
		android_usb_close(NULL);
	}
	perror_i("can not read data %d\n", ret);
	return -2;
}

/**
   @brief Android USB 写函数封装，根据协议进行封装的简单形式，用于协议处理后的回复
   @author yangguozheng 
   @return 长度
   @note
 */
int android_usb_write_data() {
	AndroidUsb_t *self = &androidUsb_t;
	struct usb_dev_handle *sDevice = self->sDevice;
	UsbDataHead_t *s_head = (UsbDataHead_t *)self->bufsnd;
	if(2 == self->debugLever) {
		return 5;
	}
	int ret = android_usb_write(sDevice, self->endpoint[1], self->bufsnd, sizeof(UsbDataHead_t) + s_head->len);
	return ret;
}

void ioctl_usb_find_android_device () {

}

int android_usb_msg_deal (void *data);

/**
   @brief 对比辅助函数
   @author yangguozheng 
   @param[in] x0 对比左值
   @param[in] x1 对比右值
   @return 长度
   @note
 */
int android_usb_item_cmp(AndroidUsbItem_t *x0, AndroidUsbItem_t *x1) {
	return x0->cmd -x1->cmd;
}

/**
   @brief 协议查找处理函数
   @author yangguozheng 
   @param[in | out] data 上下文数据
   @return {0：成功，-1：失败}
   @note
 */
int android_usb_msg_deal (void *data) {
	AndroidUsb_t *self = (AndroidUsb_t *)data;
	UsbDataHead_t *usbHead = (UsbDataHead_t *)self->bufrcv;
	AndroidUsbItem_t key = {usbHead->cmd, 0};
	AndroidUsbItem_t *result;
	android_usb_fun fun;
//	printf_i("%02X\n", key.cmd);
	result = (AndroidUsbItem_t *)bsearch(&key, androidUsbTable_t, sizeof(androidUsbTable_t)/sizeof(AndroidUsbItem_t), sizeof(AndroidUsbItem_t), android_usb_item_cmp);
	if(NULL != result) {
		fun = (android_usb_fun)result->data;
		if(NULL != fun) {
			if(0 == android_usb_before_deal(data)) {
				printf("before_deal ok %d\n", key.cmd);
				fun(data); //< 未进行错误处理
				android_usb_after_deal(data);
			} else {
				printf("before_deal error %d\n", key.cmd);
			}
		} else {
			printf("fun null key.cmd %d\n", key.cmd);
		}
	} else {
		printf("bsearch null %d\n", key.cmd);
	}
	return 0;
}

/**
   @brief Android USB 打开函数
   @author yangguozheng 
   @param[in | out] data 上下文数据
   @return {0：成功，-1：失败}
   @note 在mfi消息接收处调用
 */
int android_usb_open (void *data) {
	AndroidUsb_t *self = &androidUsb_t;
	if(self->device != USB_DEVICE_ANDROID && self->device == NULL) {
		printf("self->device != USB_DEVICE_ANDROID\n");
		return -1;
	}
	int nRet;
	/*初始化 usb_bus 结构*/
	int time = 30;
	struct usb_bus *bus;
	struct usb_device *dev;
	struct usb_device *android_dev;
	usb_dev_handle *udev;
	usb_dev_handle *udev_android;
	struct usb_dev_handle *sDevice = NULL;
	int a0 = 30;
	printf_i("\n");
#ifdef _ANDROID_USB_TEST2
	self->io_status = ANDROID_USB_IO_ON;
#endif
	while(1){
		if(0 > a0) {
			self->device = 0;
			self->endpoint[0] = 0;
			self->endpoint[1] = 0;
			printf("0 > a0, time out\n");
			return -1;
		}

//		if(self->io_status != ANDROID_USB_IO_ON) {
//			continue;
//		}
		--a0;
		printf_i("\n");
		/*查找bus 在这里将所有bus的结果都读出来，读到一个全局的结构体里面*/
		int bus_num = usb_find_busses();
		/*查找devices 在这里将所有bus的结果都读出来，读到一个全局的结构体里面*/
		int dev_num = usb_find_devices();

		for (bus = usb_busses; bus; bus = bus->next) {
			for(dev= bus->devices;dev;dev = dev->next){
				if (bus->devices->descriptor.idVendor == 0x18D1 || bus->devices->descriptor.idVendor == 0x22B8
						|| bus->devices->descriptor.idVendor == 0x04e8) {
					printf("Normal Android Device VID :0x0%x \n",bus->devices->descriptor.idVendor );

					if(bus->devices->descriptor.idVendor == 0x18D1){
						printf("-------------------------------------------------------------\n");
						android_dev = bus->devices;
						udev_android = usb_open(android_dev);
						if(udev_android){
							if (!sDevice && ((bus->devices->descriptor.idProduct) == 0x2D00 || (bus->devices->descriptor.idProduct) == 0x2D01)) {
								printf(" android accessory is open \n");
								printf("Found android device in accessory mode\n");
								sDevice = udev_android;
								printf("open android usb ok, android_dev %d\n", android_dev);
								goto found;
							}else{
								printf("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
							}
						}else{
							printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
						}
					}
				}else if( bus->devices->descriptor.idVendor == 0x2717){
					printf("MIUI Android Device \n");
				}else{
		//			printf(".\n");
				}
			}
		}
		usleep(1000000/2);
	}

	found:
	printf_i("\n");
//	exit(1);
	printf_i("\n");
	//print_altsetting(android_dev->config->interface->altsetting);
	int i = 0;
	int endpoint[2];
	if(NULL != android_dev) {
	printf_i("android_dev %d\n", android_dev);
	}
	for (i = 0; i < android_dev->config->interface->altsetting->bNumEndpoints; i++){
		printf_i("\n");
		endpoint[i] = android_dev->config->interface->altsetting->endpoint[i].bEndpointAddress;
//		print_endpoint(&interface->endpoint[i]);
		printf_i("\n");
	}
	printf_i("\n");
	printf("write thread endpoint :%4X \n",endpoint[0]);
	printf("write thread endpoint :%4X \n",endpoint[1]);
//	while(1){
//		usbCommunication(endpoint[0],endpoint[1]/*android_dev->config->interface->altsetting->endpoint->bEndpointAddress*/);
//
//	}
	self->endpoint[0] = endpoint[0];
	self->endpoint[1] = endpoint[1];
	self->sDevice = udev_android;
	printf_i("\n");
	printf("android usb find ok %d %d %d\n", udev_android, sDevice, self->sDevice);
	printf_i("\n");
	return 0;
}

/**
   @brief Android USB 关闭函数
   @author yangguozheng 
   @param[in | out] data 上下文数据
   @return {0：成功，-1：失败}
   @note 在mfi消息接收处（关闭）调用
 */
int android_usb_close (void *data){
	AndroidUsb_t *self = &androidUsb_t;
	if(NULL != self->sDevice) {
		usb_close(self->sDevice);
		self->sDevice = NULL;
		self->device = 0;
		self->endpoint[0] = 0;
		self->endpoint[1] = 0;
		printf("close android usb -+-+-+-+\n");
	}
	return 0;
}

int android_usb_communication_begin(void *data){
	printf("\n");
	AndroidUsb_t *self = (AndroidUsb_t *)data;
	self->communicate_status = USB_COMM_ANDROID_BEGIN; //<标识修改

	UsbDataHead_t *s_head = (UsbDataHead_t *)self->bufsnd;
	char *s_text = (char *)(s_head + 1);
	android_usb_pack_head(s_head, ANDROID_USB_COMMUNICATION_BEGIN + 1, sizeof(int));
	*(int *)s_text = 0;
	printf("ok ok ok\n");
	return 0;
}

/** @brief 错误归类 */
enum {
	ANDROID_USB_DATA_NO_ERROR = 0,
	ANDROID_USB_SSID_LEN_ERROR = 1,
	ANDROID_USB_SSID_STRLEN_ERROR = 2,
	ANDROID_USB_PASS_LEN_ERROR,
	ANDROID_USB_PASS_STRLEN_ERROR,
	ANDROID_USB_LINK_TYPE_ERROR,
};

typedef struct WifiInfo_t
{
	int  lNetType;                         /* 网络类型 有线  或是 无线  */
	int  lIpType;                          /* ip类型 动态或是静态         */
	int  nEnrType;                         /* wifi 类型  */
	char acIpAddr[16];                     /* ip地址     */
    char acSubNetMask[16];                 /* 子网掩码     */
    char acGateWay[16];                    /* 网关            */
    char acWiredDNS[16];                   /* 有线网 DNS */
    char acWifiDNS[16];                    /* 无线网 DNS */
    char strSSID[128];                     /* ssid */
    char strPasswd[128];                   /* 密码   */
    /* 若有需求再增加  但总长度不要超过1024 */
}WiFiInfo_t;

int android_usb_set_wifi (void *data){
	printf("\n");
	AndroidUsb_t *self = (AndroidUsb_t *)data;
	int ret = ANDROID_USB_DATA_NO_ERROR;


	/*unpack receive data*/
	UsbDataHead_t *r_head = (UsbDataHead_t *)self->bufrcv;
	char *r_text = (char *)(r_head + 1);
	int ssid_len = 0;
	int pass_len = 0;
	int link_type = 0;
	char tmpbuf[BUF_SIZE] = {0};
	int n = 0;
	WiFiInfo_t wifiInfo = {0, 0, 0, {0}, {0}, {0}, {0}, {0}, {0}, {0}};
	if(ssid_len = *(int *)r_text, ssid_len > 128 || ssid_len <= 0) {
		ret = ANDROID_USB_SSID_LEN_ERROR;
		printf("ssid len error len: %d\n", ssid_len);
	} else if(strnlen(r_text + sizeof(int), ssid_len) < ssid_len) {
		ret = ANDROID_USB_SSID_STRLEN_ERROR;
		printf("ssid strlen error len: %d\n", strnlen(r_text + sizeof(int), ssid_len));
	} else if(pass_len = *(int *)(r_text + sizeof(int) + ssid_len), pass_len > 128 || pass_len < 0) {
		ret = ANDROID_USB_PASS_LEN_ERROR;
		printf("pass len error len: %d\n", pass_len);
	} else if(strnlen(r_text + sizeof(int) + sizeof(int) + ssid_len, pass_len) < pass_len) {
		ret = ANDROID_USB_PASS_STRLEN_ERROR;
		printf("pass strlen error len: %d\n", strnlen(r_text + sizeof(int) + sizeof(int) + ssid_len, pass_len));
	} else if(link_type = *(int *)(r_text + sizeof(int) + sizeof(int) + ssid_len + pass_len), link_type != 0 && link_type != 1 && link_type != 4 && link_type != 5){
		ret = ANDROID_USB_LINK_TYPE_ERROR;
		printf("link_type error len: %d\n", link_type);
	} else {
//		printf("wifi data ok\n");
		printf("ok ok ok\n");
		//set wifi function
#ifdef _ANDROID_USB_IMP
//		bzero(tmpbuf, sizeof(tmpbuf));
		if(0 == link_type) {
			set_config_value_to_ram(CONFIG_WIFI_ENRTYPE, "0", 1);
		} else if(1 == link_type) {
			set_config_value_to_ram(CONFIG_WIFI_ENRTYPE, "1", 1);
		} else if(4 == link_type) {
			set_config_value_to_ram(CONFIG_WIFI_ENRTYPE, "4", 1);
		} else if(5 == link_type) {
			set_config_value_to_ram(CONFIG_WIFI_ENRTYPE, "5", 1);
			printf("connect type other %d\n", link_type);
		}
		wifiInfo.nEnrType = link_type;
		wifiInfo.lNetType = NET_TYPE_WIRELESS;
		/* 获取ip类型 */
		 get_config_item_value(CONFIG_WIFI_MODE, tmpbuf, &n);

		 if (strcmp(tmpbuf, "S"))
		 {
			 wifiInfo.lIpType = IP_TYPE_DYNAMIC;
		 }
		 else
		 {
			 wifiInfo.lIpType = IP_TYPE_STATIC;
		 }

		bzero(wifiInfo.strSSID, sizeof(wifiInfo.strSSID));
		memcpy(wifiInfo.strSSID, r_text + sizeof(int), ssid_len);

		bzero(wifiInfo.strPasswd, sizeof(wifiInfo.strPasswd));
		memcpy(wifiInfo.strPasswd, r_text + sizeof(int) + ssid_len + sizeof(int), pass_len);

		ret = set_config_value_to_ram(CONFIG_WIFI_SSID,    r_text + sizeof(int), ssid_len);
		ret += set_config_value_to_ram(CONFIG_WIFI_PASSWD,  r_text + sizeof(int) + ssid_len + sizeof(int), pass_len);
		ret += set_config_value_to_ram(CONFIG_WIFI_ACTIVE,  "y", 1);
		/* updata flash info */
		updata_config_to_flash();
		send_msg_to_child_process(MFI_MSG_TYPE, MSG_MFI_P_SEND_SSID, &wifiInfo, sizeof(WiFiInfo_t));

//		log_printf("android wifi info: %s %s %s %s %s %d %d %d %s %s\n", wifiInfo.acGateWay, wifiInfo.acIpAddr, wifiInfo.acSubNetMask, wifiInfo.acWifiDNS, wifiInfo.acWiredDNS
//						 , wifiInfo.lIpType, wifiInfo.lNetType, wifiInfo.nEnrType, wifiInfo.strPasswd, wifiInfo.strSSID);
#endif
	}

	/*pack send data*/
	UsbDataHead_t *s_head = (UsbDataHead_t *)self->bufsnd;
	char *s_text = (char *)(s_head + 1);
	android_usb_pack_head(s_head, ANDROID_USB_SET_WIFI + 1, sizeof(int));
	if(ANDROID_USB_DATA_NO_ERROR == ret) {
		*(int *)s_text = 0; //success
		setNetStatusInfo(NET_INFO_CONN_WIFI);
	} else {
		*(int *)s_text = 1; //fail
#ifdef _ANDROID_USB_IMP
		setNetStatusInfo(NET_INFO_NO_SSID_PASSWD);
#endif
	}
	return 0;
}

int android_usb_get_network_info (void *data){
	printf("\n");
	AndroidUsb_t *self = (AndroidUsb_t *)data;

	UsbDataHead_t *s_head = (UsbDataHead_t *)self->bufsnd;
	char *s_text = (char *)(s_head + 1);
	android_usb_pack_head(s_head, ANDROID_USB_GET_NETWORK_INFO + 1, sizeof(int));
	//ret = get net info function
//	printf("net info\n");
	*(int *)s_text = 6;
	printf("ok ok ok\n");
#ifdef _ANDROID_USB_IMP
	*(int *)s_text = getNetStatusInfo();
#endif
	return 0;
}

int android_usb_get_register_info (void *data){
	printf("\n");
	AndroidUsb_t *self = (AndroidUsb_t *)data;
	int ret = 0;

	UsbDataHead_t *s_head = (UsbDataHead_t *)self->bufsnd;
	char *s_text = (char *)(s_head + 1);
	android_usb_pack_head(s_head, ANDROID_USB_GET_REGISTER_INFO + 1, 12 + 16 + 20);
	//memset(s_text, 'F', 12 + 16 + 20);
	memcpy(s_text, "080808080808", 12);
	memcpy(s_text + 12, "FFFFFFFFFFFFFFFF", 16);
	memcpy(s_text + 12 + 16, "FFFFFFFFFFFFFFFFFFFF", 20);
	printf("ok ok ok\n");
#ifdef _ANDROID_USB_IMP
	int n = 0;
	char *mac = s_text;
	ret = get_mac_addr_value(mac, &n);
	memcpy(mac, mac + 12, 12);
	char *rl = s_text + 12;
	ret += get_config_item_value(CONFIG_CAMERA_ENCRYTIONID, rl, &n);
//	if(12 != strnlen(rl, 12)) {
//		bzero(rl, 12);
//	}
	char *uid = s_text + 12 + 16;
	ret += get_config_item_value(CONFIG_P2P_UID, uid, &n);
	//ret = get register info function
	//printf("register info mac rl uid\n");
#endif
	return ret;
}

#define IP_MAX_LEN  (4 * 3 + 3)

enum {
	ANDROID_USB_STATIC_IP_LEN_ERROR = 1,
	ANDROID_USB_STATIC_SUBMASK_LEN_ERROR,
	ANDROID_USB_STATIC_GATEWAY_LEN_ERROR,
	ANDROID_USB_STATIC_DNS_LEN_ERROR,
};

int android_usb_set_static_ip (void *data){
	printf("\n");
	AndroidUsb_t *self = (AndroidUsb_t *)data;
	int ret = ANDROID_USB_DATA_NO_ERROR;

	/*unpack receive data*/
	UsbDataHead_t *r_head = (UsbDataHead_t *)self->bufrcv;
	char *r_text = (char *)(r_head + 1);
	int ip_len = 0;
	int submask_len = 0;
	int gateway_len = 0;
	int dns_len = 0;
	if(ip_len = strnlen(r_text, IP_MAX_LEN), ip_len <= 0) {
		ret = ANDROID_USB_STATIC_IP_LEN_ERROR;
		printf("ip len error len: %d\n", ip_len);
	} else if(submask_len = strnlen(r_text + ip_len + 1, IP_MAX_LEN), submask_len <= 0) {
		ret = ANDROID_USB_STATIC_SUBMASK_LEN_ERROR;
		printf("submask len error len: %d\n", submask_len);
	} else if(gateway_len = strnlen(r_text + ip_len + 1 + submask_len + 1, IP_MAX_LEN), gateway_len <= 0) {
		ret = ANDROID_USB_STATIC_GATEWAY_LEN_ERROR;
		printf("gateway len error len: %d\n", gateway_len);
	} else if(dns_len = strnlen(r_text + ip_len + 1 + submask_len + 1 + gateway_len + 1, IP_MAX_LEN),  dns_len <= 0) {
		ret = ANDROID_USB_STATIC_DNS_LEN_ERROR;
		printf("dns len error len: %d\n", dns_len);
	} else {
//		printf("static ip data ok\n");
		printf("ok ok ok\n");
		//set static ip function
#ifdef _ANDROID_USB_IMP
		ret = set_config_value_to_ram(CONFIG_WIFI_IP, r_text, ip_len);
		ret += set_config_value_to_ram(CONFIG_WIFI_SUBNET, r_text + ip_len + 1, submask_len);
		ret += set_config_value_to_ram(CONFIG_WIFI_GATEWAY, r_text + ip_len + 1 + submask_len + 1, gateway_len);
		ret += set_config_value_to_ram(CONFIG_WIFI_DNS, r_text + ip_len + 1 + submask_len + 1 + gateway_len + 1, dns_len);
#endif
	}

	/*pack send data*/
	UsbDataHead_t *s_head = (UsbDataHead_t *)self->bufsnd;
	char *s_text = (char *)(s_head + 1);
	android_usb_pack_head(s_head, ANDROID_USB_SET_STATIC_IP + 1, sizeof(int));
	if(ANDROID_USB_DATA_NO_ERROR == ret) {
		*(int *)s_text = 0; //success
	} else {
		*(int *)s_text = 1; //fail
	}
	return 0;
}

int android_usb_get_camera_status_info (void *data){
	printf("\n");
	AndroidUsb_t *self = (AndroidUsb_t *)data;

	UsbDataHead_t *s_head = (UsbDataHead_t *)self->bufsnd;
	char *s_text = (char *)(s_head + 1);

	int n = 0;
	char cameraInfo[] = "1.2.3.273\0"
			"camera\0"
			"0.0.0.1\0"
			"m3s\0"
			"192.168.1.109\0";
//	memcpy(s_text, cameraInfo, sizeof(cameraInfo));
//	n = sizeof(cameraInfo);
	printf("not ok ok ok next ok will be ok\n");

	/*
	 *
//volatile char Cam_firmware[SYSTME_VERSION_LEN];
//
///** 摄像头类型 */
//volatile char Cam_camera_type[SYSTME_VERSION_LEN];
//
///** 硬件版本号 */
//volatile char Cam_hardware[SYSTME_VERSION_LEN];
//
///** 摄像头外型 */
//volatile char Cam_camera_model[SYSTME_VERSION_LEN];

//	 */
#ifdef _ANDROID_USB_IMP
	int m = 0;
	n = 0;
	char *firmware_version = s_text + n;
	//get firmware version
//	get_config_item_value(CONFIG_SYSTEM_FIRMWARE_VERSION, firmware_version, &m);
	strncpy(firmware_version, Cam_firmware, strlen(Cam_firmware) - 1);
	printf("test firmware_version %s\n", firmware_version);
	n += strlen(s_text + n) + 1;
	char *camera_type = s_text + n;
	//get camera type
//	get_config_item_value(CONFIG_CAMERA_TYPE, camera_type, &m);
	strncpy(camera_type, Cam_camera_type, strlen(Cam_camera_type) - 1);
	n += strlen(s_text + n) + 1;
	char *cameraTypeP = camera_type;
	for(;cameraTypeP != camera_type + n;++cameraTypeP) {
		*cameraTypeP = toupper(*cameraTypeP);
	}

	printf("test camera type %s\n", camera_type);

	char *hardware_version = s_text + n;
	//get hardware version
//	get_config_item_value(CONFIG_APP_FIRMWARE_VERSION, hardware_version, &m);
	strncpy(hardware_version, Cam_hardware, strlen(Cam_hardware) - 1);
	printf("test hardware_version %s\n", hardware_version);
	n += strlen(s_text + n) + 1;
	char *camera_camodel = s_text + n;
	//get camera camodel
//	get_config_item_value(CONFIG_CONF_VERSION, camera_camodel, &m);
	strncpy(camera_camodel, Cam_camera_model, strlen(Cam_camera_model) - 1);
	printf("test camera_camodel %s\n", camera_camodel);
	n += strlen(s_text + n) + 1;
	char *camera_ip = s_text + n;
	//get camera ip
//	get_config_item_value(CONFIG_WIFI_IP, camera_ip, &m);
	get_cur_ip_addr(camera_ip);
	printf("camera wifi ip is %s\n", camera_ip);
	n += strlen(s_text + n) + 1;

	if(n > BUF_SIZE) {
		n = BUF_SIZE;
	}
#endif
//	char ip[30] = {'\0'};
//	get_cur_ip_addr(ip);

	//volatile char Cam_firmware[SYSTME_VERSION_LEN];
	//
	///** 摄像头类型 */
	//volatile char Cam_camera_type[SYSTME_VERSION_LEN];
	//
	///** 硬件版本号 */
	//volatile char Cam_hardware[SYSTME_VERSION_LEN];
	//
	///** 摄像头外型 */
	//volatile char Cam_camera_model[SYSTME_VERSION_LEN];
//	sprintf(s_text, "%s\0%s\0%s\0%s\0%s\0", Cam_firmware, Cam_camera_type, Cam_hardware, Cam_camera_model, ip);
//	n = strlen(Cam_firmware) + 1 + strlen(Cam_camera_type) + 1 + strlen(Cam_hardware) + 1 + strlen(Cam_camera_model) + 1 + strlen(ip) + 1;
	android_usb_pack_head(s_head, ANDROID_USB_GET_CAMERA_STATUS_INFO + 1, n);
	//ret = get register info function
	//printf("register info mac rl uid\n");
	printf("ok ok ok\n");
	return 0;
}

int android_usb_switch_to_factory_setting (void *data){
	printf("\n");
	AndroidUsb_t *self = (AndroidUsb_t *)data;
	int ret = 0;

	UsbDataHead_t *s_head = (UsbDataHead_t *)self->bufsnd;
	char *s_text = (char *)(s_head + 1);

	system("echo -e Default'\n'WiFi_EnrType='\n'WiFi_SSID='\n'WiFi_Passwd='\n'WiFi_KeyID='\n'Camera_EncrytionID='\n' > /tmp/ssid;ralink_init renew 2860 /tmp/ssid;rm /tmp/ssid");
	//ret = set to factory setting function
	android_usb_pack_head(s_head, ANDROID_USB_SWITCH_TO_FACTORY_SETTING + 1, sizeof(int));
	if(0 == ret) {
		*(int *)s_text = 0; //success
		printf("ok ok ok\n");
//		printf("success\n");
	} else {
		*(int *)s_text = 1; //fail
		printf("fail\n");
	}
	//printf("register info mac rl uid\n");
	return 0;
}

enum {
	ANDROID_USB_STATUS_ERROR = 1,
	ANDROID_USB_SET_STATUS_ERROR,
};

int android_usb_get_camera_status (void *data){
	printf("\n");
	AndroidUsb_t *self = (AndroidUsb_t *)data;
	int ret = ANDROID_USB_DATA_NO_ERROR;

	/*unpack receive data*/
	UsbDataHead_t *r_head = (UsbDataHead_t *)self->bufrcv;
	char *r_text = (char *)(r_head + 1);
	int camera_status = 0;
	if(camera_status = *(int *)r_text, camera_status != 0 && camera_status !=1) {
		ret = ANDROID_USB_STATUS_ERROR;
		printf("camera status error status: %d\n", camera_status);
	} else {
//		printf("camera status data ok\n");
		printf("ok ok ok\n");
		//ret = set camera status function
		//camera_status = set camera status function
#ifdef _ANDROID_USB_IMP
		setCameraMode(camera_status?0:1);
#endif
	}

	/*pack send data*/
	UsbDataHead_t *s_head = (UsbDataHead_t *)self->bufsnd;
	char *s_text = (char *)(s_head + 1);
	android_usb_pack_head(s_head, ANDROID_USB_GET_CAMERA_STATUS + 1, sizeof(int));
	if(ANDROID_USB_DATA_NO_ERROR == ret) {
		*(int *)s_text = camera_status; //success
	} else {
#ifdef _ANDROID_USB_IMP
		*(int *)s_text = (appInfo.cameraMode?0:1); //fail
#endif
	}
	return 0;
}

enum {
	ANDROID_USB_MAC_ERROR = 1,
	ANDROID_USB_W_MAC_ERROR,
	ANDROID_USB_P_KEY_ERROR,
};

int android_usb_write_mac_and_keyne (void *data){
	printf("\n");
	AndroidUsb_t *self = (AndroidUsb_t *)data;
	int ret = ANDROID_USB_DATA_NO_ERROR;

	/*unpack receive data*/
	UsbDataHead_t *r_head = (UsbDataHead_t *)self->bufrcv;
	char *r_text = (char *)(r_head + 1);
	char *mac = r_text;
	char *w_mac = r_text + 12;
	char *p_key = r_text + 12 + 12;

	int mac_len = 0;
	int w_mac_len = 0;
	int p_key_len = 0;

	if(mac_len = strnlen(mac, 12), mac_len < 12) {
		ret = ANDROID_USB_MAC_ERROR;
		printf("camera mac len error len: %d\n", mac_len);
	} else if(w_mac_len = strnlen(w_mac, 12), w_mac_len < 12) {
		ret = ANDROID_USB_W_MAC_ERROR;
		printf("camera wireless mac len error len: %d\n", w_mac_len);
		//ret = set camera status function
		//camera_status = set camera status function
	} else if(p_key_len = strnlen(p_key, 172), p_key_len < 172) {
		ret = ANDROID_USB_P_KEY_ERROR;
		printf("camera public key len error len: %d\n", p_key_len);
	} else {
//		printf("camera mac key data ok\n");
		printf("ok ok ok\n");
#ifdef _ANDROID_USB_IMP
		ret += set_pb_key_value(p_key, p_key_len);
		ret += set_mac_addr_value(mac);
#endif
	}

	/*pack send data*/
	UsbDataHead_t *s_head = (UsbDataHead_t *)self->bufsnd;
	char *s_text = (char *)(s_head + 1);
	android_usb_pack_head(s_head, ANDROID_USB_WRITE_MAC_KEYNE + 1, sizeof(int));
	if(ANDROID_USB_DATA_NO_ERROR == ret) {
		*(int *)s_text = 0; //success
	} else {
		*(int *)s_text = 1; //fail
	}
	return 0;
}

int android_usb_get_camera_info (void *data){
	printf("\n");
	AndroidUsb_t *self = (AndroidUsb_t *)data;

	UsbDataHead_t *s_head = (UsbDataHead_t *)self->bufsnd;
	char *s_text = (char *)(s_head + 1);

	int n = 0;
	int m = 0;
	char cameraInfo[] = "www.baidu.com\0"
			"0\0"
			"1\0"
			"\0";
	n = sizeof(cameraInfo);
	memcpy(s_text, cameraInfo, n);
#ifdef _ANDROID_USB_IMP
	char *server_addr = s_text + n;
	//get server address
	get_config_item_value(CONFIG_SERVER_URL, server_addr, &m);
	n += strlen(s_text + n) + 1;
	char *server_conn_status = s_text + n;
	//get server connect status
	*server_conn_status = '1';
	//get_config_item_value(CONFIG_SYSTEM_FIRMWARE_VERSION, server_addr, &m);
	n += strlen(s_text + n) + 1;
	char *tutk_status = s_text + n;
	//get tutk status
	*tutk_status = '1';
	n += strlen(s_text + n) + 1;
	//get_config_item_value(CONFIG_SYSTEM_FIRMWARE_VERSION, server_addr, &m);

	n += strlen(s_text + n) + 1;
#endif
	if(n > BUF_SIZE) {
		n = BUF_SIZE;
	}
	printf("ok ok ok\n");
	android_usb_pack_head(s_head, ANDROID_USB_GET_CAMERA_INFO + 1, n);
	//ret = get camera info function
	//printf("camera info mac rl uid\n");
	return 0;
}

enum {
	ANDROID_USB_SERVER_ADDR_LEN_ERROR = 1,
};

int android_usb_set_service_addr (void *data){
	printf("\n");
	AndroidUsb_t *self = (AndroidUsb_t *)data;
	int ret = ANDROID_USB_DATA_NO_ERROR;

	/*unpack receive data*/
	UsbDataHead_t *r_head = (UsbDataHead_t *)self->bufrcv;
	char *r_text = (char *)(r_head + 1);
	char *server_addr = r_text;
	int server_addr_len = 0;

	if(server_addr_len = strnlen(server_addr, BUF_SIZE),  server_addr_len <= 0) {
		ret = ANDROID_USB_SERVER_ADDR_LEN_ERROR;
		printf("camera server address len error len: %d\n", server_addr_len);
	} else {
//		printf("camera server address data ok\n");
		printf("ok ok ok\n");
#ifdef _ANDROID_USB_IMP
		ret = set_config_value_to_flash(CONFIG_SERVER_URL, server_addr, server_addr_len);
#endif
	}

	/*pack send data*/
	UsbDataHead_t *s_head = (UsbDataHead_t *)self->bufsnd;
	char *s_text = (char *)(s_head + 1);
	android_usb_pack_head(s_head, ANDROID_USB_SET_SERVICE_ADDR + 1, sizeof(int));
	if(ANDROID_USB_DATA_NO_ERROR == ret) {
		*(int *)s_text = 0; //success
	} else {
		*(int *)s_text = 1; //fail
	}
	return 0;
}

int android_usb_get_mac_keyne (void *data){
	printf("\n");
	AndroidUsb_t *self = (AndroidUsb_t *)data;

	int ret = 0;

	UsbDataHead_t *s_head = (UsbDataHead_t *)self->bufsnd;
	char *s_text = (char *)(s_head + 1);

	char cameraInfo[] = "080808080807"
			"080808080808"
			"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
			"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
			"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
			"FFFFFFFFFFFFFFFFFFFFFFFFFF";
	memcpy(s_text, cameraInfo, sizeof(cameraInfo));
	char *mac = s_text;
	//get mac function
#ifdef _ANDROID_USB_IMP
	read_mac_addr_from_flash(mac);
#endif
	if(strnlen(mac, 12) < 12) {
		ret = ANDROID_USB_MAC_ERROR;
	}
	char *w_mac = s_text + 12;
	//get wireless mac function
	if(strnlen(w_mac, 12) < 12) {
		ret = ANDROID_USB_W_MAC_ERROR;
	}
	char *p_key = s_text + 12 + 12;
	//get public key function
#ifdef _ANDROID_USB_IMP
	read_pb_key_addr_from_flash(p_key, 172);
#endif
	if(strnlen(p_key, 172) < 172) {
		ret = ANDROID_USB_P_KEY_ERROR;
	}

	if(0 != ret) {
//		printf("mac key get ok\n");
		printf("ok ok ok\n");
	}
	android_usb_pack_head(s_head, ANDROID_USB_GET_MAC_KEYNE + 1, 12 + 12 + 172);
	//printf("camera get mac w_mac p_key\n");
	return 0;
}

enum {
	ANDROID_USB_DATA_HEAD_ERROR = 1,
	ANDROID_USB_DATA_LEN_ERROR,
};

int android_usb_before_deal (void *data){
	printf("-->\n");
	AndroidUsb_t *self = (AndroidUsb_t *)data;
	int ret = 0;

	/*unpack receive data*/
	UsbDataHead_t *r_head = (UsbDataHead_t *)self->bufrcv;
	char *r_text = (char *)(r_head + 1);
	if(0 != strncmp(ANDROID_USB_DATA_HEAD, r_head->head, strlen(ANDROID_USB_DATA_HEAD))) {
		ret = ANDROID_USB_DATA_HEAD_ERROR;
		printf("data head error\n");
	} else if(r_head->len > BUF_SIZE || r_head->len < 0) {
		ret = ANDROID_USB_DATA_LEN_ERROR;
		printf("data len error\n");
	} else {
		printf("data head ok\n");
	}

	int *content = (int *)(r_head + 1);

	if(0 == ret) {
		printf("head: %c%c%c%c, cmd: %d, len: %d\n"
				, r_head->head[0], r_head->head[1], r_head->head[2], r_head->head[3]
		        , r_head->cmd
		        , r_head->len);
#ifdef _ANDROID_USB_IMP
		char dataRcv[256 + 1] = {0};
		utils_istoas(dataRcv, r_head, 128);
		printf("%s\n", dataRcv);
#endif
		return 0;
	} else {
		return -1;
	}
}

int android_usb_after_deal (void *data){
	printf("<--\n");
	AndroidUsb_t *self = (AndroidUsb_t *)data;
	int ret = 0;

	/*unpack receive data*/
	UsbDataHead_t *s_head = (UsbDataHead_t *)self->bufsnd;
	char *s_text = (char *)(s_head + 1);
	if(0 != strncmp(ANDROID_USB_DATA_HEAD, s_head->head, strlen(ANDROID_USB_DATA_HEAD))) {
		ret = ANDROID_USB_DATA_HEAD_ERROR;
		printf("data head error\n");
	} else if(s_head->len > BUF_SIZE || s_head->len < 0) {
		ret = ANDROID_USB_DATA_LEN_ERROR;
		printf("data len error\n");
	} else {
		printf("data head ok\n");
		android_usb_write_data();
	}

	int *content = (int *)(s_head + 1);

	if(0 == ret) {
		printf("head: %c%c%c%c, cmd: %d, len: %d\n"
				, s_head->head[0], s_head->head[1], s_head->head[2], s_head->head[3]
				, s_head->cmd
				, s_head->len);
#ifdef _ANDROID_USB_IMP
		char dataSnd[256 + 1] = {0};
		utils_istoas(dataSnd, s_head, 128);
		printf("%s\n", dataSnd);
#endif
		return 0;
	} else {
		return -1;
	}
	return 0;
}

void android_usb_pack_head(UsbDataHead_t *data, int cmd, int len) {
	strcpy(data->head, ANDROID_USB_DATA_HEAD);
	data->cmd = cmd;
	if(len < BUF_SIZE - sizeof(UsbDataHead_t)) {
		data->len = len;
	} else {
		data->len = BUF_SIZE - sizeof(UsbDataHead_t);
	}
}

#ifdef _ANDROID_USB_IMP
int android_usb_send_msg(int qid, long type, int cmd) {
	MsgData msg = {type, cmd, {0}, 0};
	msg_queue_snd(qid, &msg, sizeof(MsgData) - sizeof(long));
	return 0;
}
#endif

#define RT350_IO_DEVICE_PATH         "/dev/rt_dev"

int android_usb_setdebug_lever(int opt) {
	AndroidUsb_t *self = &androidUsb_t;
	self->debugLever = opt;
	return 0;
}

#ifdef _ANDROID_USB_TEST

int android_usb_test() {
	printf("\n");
	AndroidUsb_t *self = (AndroidUsb_t *)&androidUsb_t;
	UsbDataHead_t *usbHead = (UsbDataHead_t *)self->bufrcv;

	android_usb_pack_head(usbHead, 0, 0);

	usbHead->cmd = 0x01;
	android_usb_msg_deal(self);

	usbHead->cmd = 0x03;
	android_usb_msg_deal(self);

	usbHead->cmd = 0x05;
	android_usb_msg_deal(self);

	usbHead->cmd = 0x07;
	android_usb_msg_deal(self);

	usbHead->cmd = 0x09;
	android_usb_msg_deal(self);

	usbHead->cmd = 0x0B;
	android_usb_msg_deal(self);

	usbHead->cmd = 0x0D;
	android_usb_msg_deal(self);

	usbHead->cmd = 0x0F;
	android_usb_msg_deal(self);

	usbHead->cmd = 0x11;
	android_usb_msg_deal(self);

	usbHead->cmd = 0x13;
	android_usb_msg_deal(self);

	usbHead->cmd = 0x15;
	android_usb_msg_deal(self);

	usbHead->cmd = 0x17;
	android_usb_msg_deal(self);

	usbHead->cmd = 0x19;
	android_usb_msg_deal(self);

	usbHead->cmd = 0x1A;
	android_usb_msg_deal(self);

	usbHead->cmd = 0x20;
	android_usb_msg_deal(self);
	return 0;
}

int android_usb_test2() {
	printf("\n");
	AndroidUsb_t *self = (AndroidUsb_t *)&androidUsb_t;
	UsbDataHead_t *usbHead = (UsbDataHead_t *)self->bufrcv;

	android_usb_pack_head(usbHead, 0, 0);

#ifdef _ANDROID_USB_TEST2
	self->io_fd = open(RT350_IO_DEVICE_PATH, O_RDWR, O_NONBLOCK);
	if(-1 == self->io_fd) {
		perror("open");
//		return -1;
	}
	printf("\n");
	poweroff_usb();
	printf("\n");
	usb_switch_to_mfi();
	printf("\n");
	poweron_usb();

	android_usb_identity();
	printf("\n");
	android_usb_open(NULL);
	printf("\n");
	int ret = 0;
	printf("\n");
	while(1) {
		android_usb_read_data_and_deal();
	}
#endif

	usbHead->cmd = 0x01;
	android_usb_msg_deal(self);

	usbHead->cmd = 0x02;
	android_usb_msg_deal(self);

	usbHead->cmd = 0x03;
	android_usb_msg_deal(self);

	usbHead->cmd = 0x04;
	android_usb_msg_deal(self);

	usbHead->cmd = 0x05;
	android_usb_msg_deal(self);

	usbHead->cmd = 0x06;
	android_usb_msg_deal(self);

	usbHead->cmd = 0x07;
	android_usb_msg_deal(self);

	usbHead->cmd = 0x08;
	android_usb_msg_deal(self);

	usbHead->cmd = 0x09;
	android_usb_msg_deal(self);

	usbHead->cmd = 0x0A;
	android_usb_msg_deal(self);

	usbHead->cmd = 0x0B;
	android_usb_msg_deal(self);

	usbHead->cmd = 0x0C;
	android_usb_msg_deal(self);

	usbHead->cmd = 0x0D;
	android_usb_msg_deal(self);
#ifdef _ANDROID_USB_TEST2
	android_usb_close(NULL);
#endif
	return 0;
}

//#define EXIT_FAILURE -1
//#define EXIT_SUCCESS 0

/**
  *  @brief      关闭电源开关（切断）
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <成功：EXIT_SUCCESS   失败：EXIT_FAILURE>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int poweroff_usb(void)
{
	AndroidUsb_t *self = (AndroidUsb_t *)&androidUsb_t;
	int lData = -1;

   	if (-1 == ioctl(self->io_fd, CMD_POWEROFF_USB, &lData))
	{
		printf("rt_dev ioctl failed55 fd:= %d error is %d, descrip: %s\n",
				self->io_fd, errno, strerror(errno));
		return EXIT_FAILURE;
	}
    else
    {
	    return EXIT_SUCCESS;
    }
}

/**
  *  @brief      闭合电源开关
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <成功：EXIT_SUCCESS   失败：EXIT_FAILURE>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int poweron_usb(void)
{
	int lData = -1;
	AndroidUsb_t *self = (AndroidUsb_t *)&androidUsb_t;

	if (-1 == ioctl(self->io_fd, CMD_POWERON_USB, &lData))
	{
		printf("rt_dev ioctl failed66 fd:= %d error is %d, descrip: %s\n",
				self->io_fd, errno, strerror(errno));
		return EXIT_FAILURE;
	}
	else
	{
		return EXIT_SUCCESS;
	}
}

/**
  *  @brief      usb切换处到MFI
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <成功：EXIT_SUCCESS   失败：EXIT_FAILURE>
  *  @remark     2014-02-28 增加该注释
  *  @note
*/
int usb_switch_to_mfi(void)
{
	int lRet = EXIT_FAILURE;
    int lData = -1;
    AndroidUsb_t *self = (AndroidUsb_t *)&androidUsb_t;

	if (-1 == ioctl(self->io_fd, CMD_SWITCH_TO_MFI, &lData))
	{
	    printf("rt_dev ioctl failed33  fd: %d  error is %d, descrip: %s\n",
	    		self->io_fd, errno, strerror(errno));
        return EXIT_FAILURE;
	}
	else
	{
		return EXIT_SUCCESS;
	}
}

/**
  *  @brief      usb切换处到cam
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <成功：EXIT_SUCCESS   失败：EXIT_FAILURE>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
int usb_switch_to_cam(void)
{
	int lData = -1;
	AndroidUsb_t *self = (AndroidUsb_t *)&androidUsb_t;
	if (-1 == ioctl(self->io_fd, CMD_SWITCH_TO_CAM, &lData))
	{
		printf("rt_dev ioctl failed44 fd:= %d error is %d, descrip: %s\n",
				self->io_fd, errno, strerror(errno));
		return EXIT_FAILURE;
	}
	else
	{
		return EXIT_SUCCESS;
	}
}

int main(int argc, char *argv[]) {
	if(argc == 2) {
		if(0 == strcmp("test1", argv[1])) {
			android_usb_setdebug_lever(2);
			android_usb_test();
		} else if(0 == strcmp("test2", argv[1])) {
			android_usb_test2();
		}
	}
	//android_usb_test2();
	return 0;
}
#endif
