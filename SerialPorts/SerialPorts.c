/*
 * SerialPorts.c
 *
 *  Created on: Mar 11, 2013
 *      Author: root
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <semaphore.h>
#include <fcntl.h>
#include <termios.h>
#include "common/thread.h"
#include "common/common.h"
#include "common/msg_queue.h"
#include "common/utils.h"
#include "SerialPorts.h"
#include "SerialAdapter.h"
#include "common/common_config.h"


#define REQ 0x01
#define RETE 0x0e
#define SUC 0x0b
#define R1_ 0x08
#define R2 0xa
#define MX ((z>>5^y<<2) +(y>>3^z<<4))^((sum^y) + (ka[(p&3)^e]^z))
#define DELTA 0x9e3779b9
#define SERIALPORT_STOP_DELAY 20
#define SERIALPORT_DELAY_TIME 50000
#define SERIALPORT_SECOND_COUNT (1000000 / SERIALPORT_DELAY_TIME)
/*---------全局变量结构体---------*/
struct serialport_t{
	msg_container MsgBowl;//extends class msg_container
};
/* 指向动态分配的串行端口容器 */
struct serialport_t *serialport_list;
/* 串行端口设备的文件描述符 */
volatile int serialport_fd;
const unsigned int ka[4]={0xe5d9f39a,0x3686790d,0x7376a4be,0x5c2a76c1};
/* 和单片机的确认标志 1：确认成功 0：没有确认或者确认不成功 */
volatile int serialport_confirm_flag;
/* 机器类型 */
volatile int serialport_mach_type;
/* 用于电机停止时计算时间 */
int serialport_sendstop_count;
/* 接收串口数据的缓冲区 */
unsigned char serialport_rcv_buf[128];
/* 用于串口命令解析 */
int serialport_parse_position;
/* 串口缓冲区的当然接收数据的位置 */
int serialport_rcv_position;
/* 电机的停止表示 1：电机停止 0：电机没有停止*/
volatile int serialport_stop_flag;
/* 用于消息循环，记录串行端口线程的睡眠时间  */
int serialport_sleep_time;
/* 电机停止时需向子进程发消息，用于计算发送消息的时间 */
int serialport_sendstop_count;
/* 用于计算秒数 */
int serialport_second_count;
/* 电池报警标志0：不报警 1：报警 */
int serialport_battery_alarm;
/* 云台模式 0:用户模式 1:工装模式高速巡航 2:工装模式低速巡航 */
int serialport_mode;
/* 记录云台工装态时巡航的时间 */
int serialport_cruise_time;
/*-----------内部函数声明-----------*/
void *serialport_thread(void *arg);
void serialports_msg_receive_process(msg_container* self);
int serialport_config(int fd);
int serialport_changeID(unsigned char buf[20],unsigned int Rand[4]);
int serialport_request(unsigned char dataname);
int serialport_confirm_read(unsigned char *buf);
int * serialport_encode(unsigned int *v, int n, unsigned int const ka[4]);
int serialport_send_R2R1(unsigned char *buf, char dataname);
void serialport_random(int Rand[4]);
ssize_t serialport_read(int fd, void *buf, size_t bytes, unsigned int timout);
unsigned char serialport_check_sum(unsigned char * buf, int len);
int serialport_find_head(void);
void serialport_process_cmd(unsigned char *buf);
void serialport_run_confirm(void);
ssize_t serialport_select_read(int fd, void *buf, size_t bytes, unsigned int timout);
void serialport_receive(void);
int serialport_send_msg(unsigned char cmd, unsigned char sign[4], unsigned char len, void * buf);
void serialport_move_data(void);

/*-----------线程控制函数-----------*/
int SerialPorts_Thread_Creat() {
	int result;
	result = thread_start(serialport_thread);
	return result;
}
int SerialPorts_Thread_Stop() {
	return 0;
}

/*-----------线程初始化函数-----------*/

/*
 *函数名： serialport_module_init
 *功能：串行端口模块初始化。初始化全局变量，获得消息队列ID，运行和单片机的认证。
 *参数：无
 *返回：无
 */
void serialport_module_init(msg_container* self)
{
	unsigned char buf[8];
	int len;

	serialport_confirm_flag = 0;
	serialport_mach_type = MACH_TYPE_NONE;
	serialport_rcv_position = 0;
	serialport_parse_position = 0;
	serialport_sendstop_count = 0;
	serialport_second_count = 0;
	serialport_battery_alarm = 1;
	serialport_mode = 0;
	serialportadapter_battery_data[0] = 0;
	serialportadapter_battery_data[1] = 0;
	serialportadapter_battery_data[2] = 0;
	serialportadapter_battery_data[3] = (unsigned char)0xff;
	if(get_config_item_value(CONFIG_CAMERA_TYPE, buf, &len) == EXIT_SUCCESS) {
		if(strncmp(buf, "M3S", 4) == 0) {
			serialport_mach_type = MACH_TYPE_M3S;
		}

		if(strncmp(buf, "M2", 4) == 0) {
			serialport_mach_type = MACH_TYPE_M2;
			}
	}

	if(get_config_item_value(CONFIG_ALARM_BATTERY, buf, &len) == EXIT_SUCCESS) {
		if(strncmp(buf, "y", 4) == 0) {
			serialport_battery_alarm = 1;
		}
		if(strncmp(buf, "n", 4) == 0) {
			serialport_battery_alarm = 0;
			}
	}


	self->lmsgid = msg_queue_get((key_t)CHILD_THREAD_MSG_KEY);
	if( self->lmsgid == -1) {
		exit(0);
	}
	self->rmsgid = msg_queue_get((key_t)CHILD_PROCESS_MSG_KEY);
	if( self->rmsgid == -1) {
		exit(0);
	}

	serialport_run_confirm();
	appInitMotor(0);

}


/*
 *函数名： serialports_msg_receive_process
 *功能：接受子进程发来的消息并调用处理函数，接受串口的数据并调用处理函数。
 *	控制线程睡眠，控制向子进程发送停止消息的时间。
 *参数：
 *	self：指向串行端口msg_container结构的指针。
 *返回：无
 */
void serialports_msg_receive_process(msg_container* self)
{
	unsigned char snd[4];

	if(msg_queue_rcv(self->lmsgid, self->msgdata_rcv, MAX_MSG_LEN, SERIAL_PORTS_MSG_TYPE) == -1)
	{
		serialport_sleep_time = SERIALPORT_DELAY_TIME;
	}
	else
	{
		serialports_cmd_table_fun(self);
		serialport_sleep_time = 0;
	}

	serialport_receive();
	if(serialport_sendstop_count > 0 && serialport_sleep_time > 0) {
		serialport_sendstop_count--;
		if(serialport_sendstop_count == 0)
		{
			snd[0] = 0;
			serialport_send_msg(MSG_SP_P_MOTORMOVE, NULL, 1, snd);
			serialport_stop_flag = 1;
		}
	}
	/* 计算慢速和快速巡航的时间 */

	if(serialport_mode > 0 && serialport_sleep_time > 0){
		serialport_cruise_time++;
		/* serialport_mode == 1 正快速巡航 */
		if(serialport_mode == 1 && serialport_cruise_time > 200) {
			serialport_cruise(0x00, 0x0);
			usleep(1000);
			serialport_speed(10);
			usleep(1000);
			serialport_cruise(0x0f, 0xff);
			serialport_mode = 2;
			serialport_cruise_time = 0;
		}
		/* serialport_mode == 2 正慢速巡航 */
		if(serialport_mode == 2 && serialport_cruise_time > 1200) {
			printf("start fast cruise\n");
			serialport_cruise(0x0, 0x0);
			usleep(1000);
			serialport_speed(1);
			usleep(1000);
			serialport_cruise(0x0f, 0xff);
			serialport_mode = 1;
			serialport_cruise_time = 0;
		}

		if(serialport_mode == 3) {
			serialport_cruise_time ==0;
		}
	}

	if(serialport_second_count < SERIALPORT_SECOND_COUNT && serialport_sleep_time > 0) {
		serialport_second_count++;
	}
	else {
		serialport_second_count = 0;
		if(serialportadapter_battery_data[3] < 255) {
			serialportadapter_battery_data[3]++;
		}
	}
	usleep(serialport_sleep_time);
}

/*
 *函数名： serialport_thread
 *功能：串行端口模块的主线程
 *参数：
 *	arg：暂时不使用。
 *返回：函数返回0
 */
void *serialport_thread(void *arg)
{
	serialport_list = (struct serialport_t*)malloc(sizeof(struct serialport_t));
	struct serialport_t* self = serialport_list;
	serialport_module_init((msg_container*)self);
	thread_synchronization(&rSerialPorts, &rChildProcess);
	while(1)
	{
		serialports_msg_receive_process((msg_container*)self);
	}
	return 0;
}

/*
 *函数名： serialport_run_confirm
 *功能：调用serialport_confirm函数和单片机认证。最多认证5次。
 *	并根据是否成功设置全局变量serialport_confirm_flag。
 *参数：
 *	arg：暂时不使用。
 *返回：函数返回0
 *全局变量serialport_confirm_flag：
 *	0：认证不成功。
 *	1：认证成功。
 */
void serialport_run_confirm(void)
{
	int i;
	int ret;

	if(serialport_fd == -1)
	{
		serialport_confirm_flag = 0;
		return;
	}

	for(i = 0; i < 5; i++) {
		ret = serialport_confirm();
		if(ret != -1) {
			serialport_confirm_flag = 1;
			break;
		}
	}
}
/*
 *函数名： serialport_send_msg
 *功能：向子进程发送消息
 *参数：
 *	cmd：命令号
 *	sign：流水号
 *	len：命令长度
 *	buf：命令数据
 *返回：
 *	0：成功
 *	-1：失败
 */
int serialport_send_msg(unsigned char cmd, unsigned char *sign, unsigned char len, void * buf)
{
	msg_container* self = (msg_container*)serialport_list;
	MsgData *msgsnd;

	msgsnd = (MsgData *)self->msgdata_snd;
	msgsnd->cmd = cmd;
	msgsnd->type = SERIAL_PORTS_MSG_TYPE;
	if(sign == NULL) {
		msgsnd->sign[0] = 0;
		msgsnd->sign[1] = 0;
		msgsnd->sign[2] = 0;
		msgsnd->sign[3] = 0;
	}
	else {
		memcpy(msgsnd->sign, sign, 4);
	}
	msgsnd->len = len;
	if (msgsnd->len > 0 && buf != NULL ) {
		memcpy(self->msgdata_snd + sizeof(MsgData), buf, len);
	}
	msg_queue_snd(self->rmsgid, self->msgdata_snd, sizeof(MsgData) - 4 + msgsnd->len);
	return 0;
}

/*
 *函数名： serialport_check_sum
 *功能：计算校验和
 *参数：
 *	buf：数据开始指针
 *	len：数据长度
 *返回：校验和
 */
unsigned char serialport_check_sum(unsigned char * buf, int len)
{
	int i;
	unsigned char sum;

	sum = 0;
	for(i = 0; i < len; i++) {
		sum += buf[i];
	}
	return sum;
}

/*
 *函数名： serialport_find_head
 *功能：根据命令的同步字节0xef和同步字节后的长度字节查找命令头
 *参数：无
 *返回：
 *	0：没有找到合法的命令头
 *	1：找到合法的命令头，命令没有传送完
 *	2：找到合法的命令头，命令传送完
 */
int serialport_find_head(void)
{
	int ret;
	int len;

	ret = 0;
	while(serialport_parse_position < serialport_rcv_position)
	{
		if(serialport_rcv_buf[serialport_parse_position] == 0xef)
		{
			if(serialport_parse_position == serialport_rcv_position - 1)
			{
				ret = 1;
				break;
			}
			else
			{
				len = serialport_rcv_buf[serialport_parse_position + 1];
				if(len <= 8)
				{
					if(serialport_parse_position + len + 3 < serialport_rcv_position)
					{
						ret = 2;
						break;
					}
					else
					{
						ret = 1;
						break;
					}
				}
			}
		}

		serialport_parse_position++;
	}

	return ret;
}

/*
 *函数名： serialport_process_cmd
 *功能：处理接受到的串口命令
 *参数：
 *	buf：命令的开始指针
 *返回：无
 */
void serialport_process_cmd(unsigned char *buf)
{
	unsigned char snd[4];

	switch(buf[1]) {	/* the len of command */
	case 0:
		if ((buf[2] & 0xf0) != 0) {	/* 处理电机错误消息 */
			switch (buf[2] & 0xf0) {
			case 0x10:	/* 撞上限 */
				if((buf[2] & 0xf) != 5) {
					snd[0] = buf[2];
					serialport_sendstop_count = SERIALPORT_STOP_DELAY;
					serialport_send_msg(MSG_SP_P_MOTORERROR, NULL, 1, snd);
				}
				break;
			case 0x20: /* 撞下限 */
				if((buf[2] & 0xf) != 5) {
					snd[0] = buf[2];
					serialport_sendstop_count = SERIALPORT_STOP_DELAY;
					serialport_send_msg(MSG_SP_P_MOTORERROR, NULL, 1, snd);
				}
				break;
			case 0x30:	/* 未移动 */
			case 0x40:	/* 撞左限 */
			case 0x80:	/* 撞右限 */
			case 0x50:	/* 撞上限和撞左限 */
			case 0x90:	/* 撞上限和撞右限 */
			case 0x60:	/* 撞下限和撞左限 */
			case 0xa0:	/* 撞下限和撞右限 */
				snd[0] = buf[2];
				serialport_sendstop_count = SERIALPORT_STOP_DELAY;
				serialport_send_msg(MSG_SP_P_MOTORERROR, NULL, 1, snd);
				break;

			}
		} else {
			switch (buf[2]) {	/* 处理运动停止，巡航停止，原点校准停止 */
			case 2:	/* 相对转动停止 */
			case 3:	/* 绝对转动停止 */
				serialport_sendstop_count = SERIALPORT_STOP_DELAY;
				break;
			case 4:	/* 巡航停止 */
				serialport_sendstop_count = SERIALPORT_STOP_DELAY;
				break;
			case 7: /* 原点校准停止 */
				serialport_sendstop_count = SERIALPORT_STOP_DELAY;
				break;
			default:
				break;
			}
		}
		break;
	case 3: /* 电池查询 */
		if(buf[2] == 0x10) {
			snd[0] = buf[3];
			snd[1] = buf[4];
			snd[2] = buf[5];
			serialport_send_msg(MSG_SP_P_BATTERY, NULL, 3, snd);
		}
		break;
	case 4:	/* 云台位置查询 */
		if(buf[2] == 6) {
			snd[0] = buf[3];
			snd[1] = buf[4];
			snd[2] = buf[5];
			snd[3] = buf[6];
			serialport_send_msg(MSG_SP_P_POSITION, NULL, 4, snd);
		}
		break;
	default:
		break;
	}
}

/*
 *函数名： serialport_move_data
 *功能：从serialport_parse_position位置把后面接收到的
 *	数据移动到缓冲区开始
 *参数：无
 *返回：无
 */
void serialport_move_data(void)
{
	int i;

	for(i = serialport_parse_position; i < serialport_rcv_position; i++)
	{
		serialport_rcv_buf[i - serialport_parse_position] = serialport_rcv_buf[i];
	}

	serialport_rcv_position -= serialport_parse_position;
	serialport_parse_position = 0;
}

/*
 *函数名： serialport_receive
 *功能：接收串口数据并处理
 *参数：无
 *返回：无
 */
void serialport_receive(void)
{
	int ret;
	int rcv_len, cmd_len;
	unsigned char chk;
	unsigned char buf[16];

	if (serialport_confirm_flag != 1) {
		return;
	}

	rcv_len = serialport_select_read(serialport_fd, buf, 16, 0);

	if(rcv_len <= 0) {
		return;
	}


	if (serialport_rcv_position + rcv_len > 127) {
		serialport_rcv_position = 0;
		serialport_parse_position = 0;
	}

	memcpy(serialport_rcv_buf + serialport_rcv_position, buf, rcv_len);

	serialport_rcv_position += rcv_len;

	do {
		ret = serialport_find_head();
		switch (ret) {
		case 0:
			serialport_parse_position = 0;
			serialport_rcv_position = 0;
			break;
		case 1:
			break;
		case 2:
			cmd_len = serialport_rcv_buf[serialport_parse_position + 1];
			chk = serialport_rcv_buf[serialport_parse_position + 3 + cmd_len];

			if (chk == serialport_check_sum(
					serialport_rcv_buf + serialport_parse_position, cmd_len + 3)) {
				serialport_process_cmd(serialport_rcv_buf + serialport_parse_position);
			}
			serialport_parse_position += cmd_len + 4;
			serialport_move_data();
			break;
		default:

			break;
		}
	} while (ret == 2);
}

/*
 *函数名： serialport_open
 *功能：打开串口设备，设备的文件描述符存于serialport_fd
 *参数：
 *	name：设备的文件名
 *返回：
 *	0：设备打开成功
 *	-1：设备打开失败
 */
int serialport_open(const char *name)
{
	serialport_fd = open(name, O_RDWR | O_NOCTTY | O_NDELAY);
	if (serialport_fd == -1) {
		return -1;
	} else {
		fcntl(serialport_fd, F_SETFL, 0);
		serialport_config(serialport_fd);
		return 0;
	}

}
/*
 *函数名： serialport_get_battery_state
 *功能：返回电池的状态
 *参数：无
 *返回：电池的状态，四字节地址由低到高分别表示
 *	字节1：是否低电压
 *	  1：低电压
 *	  0：非低电压
 *	字节2：充电状态
 *	  0：无充电
 *	  1：正在充电
 *	  2：已充满
 *	字节3：电量百分比
 *	字节4：主芯片收到数据的时间，单位秒。注：单片机每两秒发一次电池状态的数据。
 *	  0~254：收到数据的时间
 *	  255：还没有收到数据或收到数据时间大于等于255秒
 */
unsigned int serialport_get_battery_state(void)
{
	return *(int *)serialportadapter_battery_data;
}
/*
 *函数名： serialport_config
 *功能：配置串口
 *参数：
 *	fd：设备的文件描述符
 *返回：
 *	0：成功
 *	-1：失败
 */
int serialport_config(int fd)
{
	struct termios operation;
	if (tcgetattr(fd, &operation) != 0) {
		perror("set tty");
		return -1;
	}
	cfmakeraw(&operation);/*raw mode*/
	operation.c_cflag |= CLOCAL | CREAD; /*local mode and can read*/
	operation.c_cflag &= ~CSIZE;/*NO mask*/
	operation.c_cflag |= CS8;/*8 bit data */
	operation.c_cflag &= ~PARENB;/*no parity check */
	cfsetispeed(&operation, B57600);
	cfsetospeed(&operation, B57600);
	operation.c_cflag &= ~CSTOPB; // 1 stop bit;
	operation.c_cc[VTIME] = 0;
	operation.c_cc[VMIN] = 1;

	tcflush(fd, TCIFLUSH);

	if ((tcsetattr(fd, TCSANOW, &operation)) != 0) {
		perror("tty set error");
		return -1;
	}

	return 0;
}

/*
 *函数名： serialport_changeID
 *功能：转换buf中的输入数据，结果存于Rand中
 *参数：
 *	buf：输入数据
 *	Rand：输出数据
 *返回：函数返回0
 */
int serialport_changeID(unsigned char buf[20],unsigned int Rand[4])
{
	int i;
	int nbuf = 3;
	memset(Rand, 0, 4);
	for (i = 0; i < 4; i++) {
		Rand[i] = buf[nbuf];
		Rand[i] |= buf[nbuf + 1] << 8;
		Rand[i] |= buf[nbuf + 2] << 16;
		Rand[i] |= buf[nbuf + 3] << 24;
		nbuf += 4;
	}
	return 0;
}

/*
 *函数名： serialport_request
 *功能：向单片机发送认证请求命令
 *参数：
 *	dataname：命令
 *返回：
 *	0：成功
 *	-1：失败
 */
int serialport_request(unsigned char dataname)
{
	ssize_t ret;
	int count;
	unsigned char snd_buf[4] = { 0 };

	snd_buf[0] = 0xfe;
	snd_buf[1] = 0x00;
	snd_buf[2] = dataname;
	for (count = 0; count < 3; count++)
	{
		snd_buf[3] += snd_buf[count];
	}
	ret = write(serialport_fd, snd_buf, 4);
	if (ret == -1)
	{
		return -1;
	}

	return 0;
}

/*
 *函数名： serialport_confirm_read
 *功能：用于主芯片和单片机认证时从串口读取数据。
 *参数：
 *	buf：接收数据的缓冲区地址
 *返回：
 *	0：有数据
 *	-1：没有数据
 */
int serialport_confirm_read(unsigned char *buf) {
	ssize_t ret;
	int count = 0;

	while (1) {
		ret = serialport_read(serialport_fd, buf, 20, 500);
		if (ret >= 0) {
			break;
		}
		count++;
		if (count == 200) {
			return -1;
		}
	}

	return 0;
}

/*
 *函数名： serialport_encode
 *功能：加密数据。
 *参数：
 *	v：输入和输出的数据
 *	n：数据长度
 *	ka：密钥
 *返回：返回加密后数据的开始地址
 */
int *serialport_encode(unsigned int *v, int n, unsigned int const ka[4])
{
	unsigned int y, z, sum;
	unsigned int p, rounds, e;

	if (n > 1) {
		rounds = 6 + 52 / n;
		sum = 0;
		z = v[n - 1];
		do {
			sum += DELTA;
			e = (sum >> 2) & 3;
			for (p = 0; p < n - 1; p++)
				y = v[p + 1], z = v[p] += MX;
			y = v[0];
			z = v[n - 1] += MX;
		} while (--rounds);
	}
	return v;
}
/*
 *函数名： serialport_send_R2R1
 *功能：向单片机发送R2R1命令。
 *参数：
 *	buf：输入数据
 *	dataname：命令
 *返回：
 *	0：成功
 *	-1：失败
 */
int serialport_send_R2R1(unsigned char *buf, char dataname)
{
	ssize_t ret;
	int i;
	unsigned char snd_buf[20] = { 0 };
	snd_buf[0] = 0xfe;
	snd_buf[1] = 0x10;
	snd_buf[2] = dataname;
	for (i = 0; i < 16; i++) {
		snd_buf[3 + i] = buf[3 + i];

	}
	for (i = 0; i < 19; i++) {
		snd_buf[19] += snd_buf[i];

	}
	ret = write(serialport_fd, snd_buf, 20);
	if (ret == -1) {
		return -1;
	}
	return 0;

}

/*
 *函数名： serialport_random
 *功能：求随即数。
 *参数：
 *	Rand：输出数据
 *返回：无
 */
void serialport_random(int Rand[4])/*求随机数*/
{
	int i;
    for(i = 0; i < 4; i++)
   	{
     	Rand[i] =rand();
   	}
}
/*
 *函数名： serialport_select_read
 *功能：用select方式从串口设备读取数据
 *参数：
 *	fd：串口设备文件描述符
 *	buf：接收数据的缓冲区
 *	bytes：读取的最大字节数
 *	timout：超时值
 *返回：
 *	-1：读取失败
 *	>0：读取的字节数
 */
ssize_t serialport_select_read(int fd, void *buf, size_t bytes, unsigned int timout)
{
	int nfds;

   	fd_set readfds;
	ssize_t ret;
	struct timeval tv;

	tv.tv_sec =0;
   	tv.tv_usec =timout;
   	FD_ZERO(&readfds);
   	FD_SET(fd, &readfds);
   	nfds = select(fd+1, &readfds, NULL, NULL, &tv);
    if (nfds <= 0)
	{
		return -1;
  	}else
   	{
		if(-1==(ret=read(fd, buf, bytes)))
		{
			perror("read failure\n");
			return -1;
		}
	 }
   	 return ret;

}
/*
 *函数名： serialport_read
 *功能：调用serialport_select_read，从串口读取给定长度的数据。
 *参数：
 *	fd：串口设备文件描述符
 *	buf：接收数据的缓冲区
 *	bytes：读取的最大字节数
 *	timout：超时值
 *返回：
 *	-1：读取失败
 *	>0：读取的字节数
 */
ssize_t serialport_read(int fd, void *buf, size_t bytes, unsigned int timout)
{
	size_t left;
	ssize_t read;

	left = bytes;
	while (left > 0) {
		if ((read = serialport_select_read(fd, buf, left, timout)) < 0) {
			if (left == bytes) {
				return -1;
			} else {
				break;
			}
		} else if (read == 0) {
			break;
		}
		left -= read;
		buf += read;
	}
	return (bytes - left);
}
/*
 *函数名： serialport_confirm
 *功能：执行同单片机的认证
 *参数：无
 *返回：
 *	-1：读取失败
 *	0：认证成功
 */
int serialport_confirm(void)
{
	int i;
	int ret;
	unsigned char rec_buf[20] = { 0 };	/*接收缓存*/
	unsigned int R1[4] = { 0 };	/*R1*/
	unsigned int k[4] = { 0 };	/*k*/
	unsigned int ID[4] = { 0 };	/*ID*/

	/********************************************************************
	 *	5350->NEC
	 ********************************************************************/
	serialport_request(REQ);/*发送认证请求*/
	ret = serialport_confirm_read(rec_buf);
	if (-1 == ret) {
		return -1;
	}
	serialport_changeID(rec_buf, ID);
	usleep(10);
	/* **************************************************************
	 * 5350 decode k
	 ***************************************************************/
	serialport_encode(ID, 4, ka); /*根据得到的ID，和本地的ka来求k值*/
	for (i = 0; i < 4; i++) {
		k[i] = ID[i];
	}
	/****************************************************************
	 *	5350->NEC
	 *
	 ****************************************************************/
	serialport_request(RETE);/*告诉NEC单片机已经接收到了ID*/
	usleep(10);
	ret = serialport_confirm_read(rec_buf); /*接收NEC发来的经过加密的R2'*/
	if (-1 == ret) {
		return -1;
	}

	/****************************************************************

	 *****************************************************************/
	usleep(10);
	serialport_encode((unsigned int *) (&rec_buf[3]), 4, k); /*根据k，R2'求出R2*/
	serialport_send_R2R1(rec_buf, R2);/*把求出来的R2发送给NEC*/
	usleep(10);
	ret = serialport_confirm_read(rec_buf); /*接收NEC返回，0x0c代表解密失败）*/
	if (-1 == ret || 0x0c == rec_buf[2]) {
		return -1;
	}

	/*********************************************************************

	 *********************************************************************/

	serialport_random(R1);/*生成随机数*/
	memcpy(&rec_buf[3], R1, 16);
	serialport_encode((unsigned int *) (&rec_buf[3]), 4, k);/*加密R1成R1'*/
	serialport_send_R2R1(rec_buf, R1_);/*发送R1'*/
	usleep(10);
	ret = serialport_confirm_read(rec_buf);/*接收NEC解密后R1*/
	if (-1 == ret) {
		return -1;
	} else {
		ret = memcmp(R1, &rec_buf[3], 16);/*对比R1*/
		if (ret == 0) {
			serialport_request(SUC);
			printf("file=%s,func=%s, line=%d: confirm return = %d\n",
									__FILE__, __FUNCTION__, __LINE__, ret);
			return 0;
		} else {
			printf("file=%s,func=%s, line=%d: confirm return = %d\n",
									__FILE__, __FUNCTION__, __LINE__, ret);
			return -1;
		}
	}

}
/*
 *函数名： serialport_foctory_mode
 *功能：进入工厂模式
 *参数：mode
     0:进入工装模式并巡航
     1：进入工装模式不巡航
 *返回：函数返回0
 */
int serialport_foctory_mode(int mode)
{
	if (serialport_mach_type == MACH_TYPE_M3S  && serialport_confirm_flag == 1) {
		switch(mode) {
		case 0:
			serialport_mode = 1;
			serialport_speed(1);
			serialport_cruise(0x0f, 0xff);
			break;
		case 1:
			serialport_mode = 3;
			serialport_cruise(0x0, 0xff);
			break;
		default:
			break;
		}

	}
	return 0;
}

/*
 *函数名： serialport_foctory_mode
 *功能：进入工厂模式
 *参数：无
 *返回：函数返回0
 */
int serialport_user_mode(void)
{
	if (serialport_mach_type == MACH_TYPE_M3S  && serialport_confirm_flag == 1) {
		serialport_mode = 0;
		serialport_speed(5);
		serialport_calibrate();
	}

	return 0;
}
/*
 *函数名： serialport_speed
 *功能：设置电机的速度
 *参数：
 *	speed：电机速度
 *返回：
 *	0：成功
 *	1：失败
 */

int serialport_speed(unsigned char speed)
{
	unsigned char buf[5];
	int ret;

	buf[0]=0xFE;
	buf[1] = 0x01;
	buf[2] = 0x05;
	buf[3] = speed;
	buf[4] = serialport_check_sum(buf, 4);
	ret = write(serialport_fd, buf,5);
	if(ret == -1)
	{
		perror("set speed failed !!!\n");
		return -1;
	}
	return 0;
}
/*
 *函数名： serialport_cruise
 *功能：云台巡航
 *参数：
 *	time：巡航次数
 *		0：停止
 *		1~254：巡航次数
 *		255：无限巡航
 *	type：巡航模式
 *		0x0c：水平
 *		0x03：垂直
 *		0x0f：波浪
 *返回：
 *	0：成功
 *	1：失败
 */
int serialport_cruise(unsigned char time,char type) /*巡航*/
{
	ssize_t ret;
	unsigned char buf[6];

	buf[0]=0xFE;
	buf[1] = 0x02;
	buf[2] = 0x04;
	buf[3] =time;
	buf[4] =type;
	buf[5] = serialport_check_sum(buf, 5);
	ret = write(serialport_fd, buf, 6);
	if(ret == -1)
	{
		perror("motor cruise failure !!!\n");
		return -1;
	}

	serialport_sendstop_count = 0;
	serialport_stop_flag = 0;
	buf[0] = 1;
	serialport_send_msg(MSG_SP_P_MOTORMOVE, NULL, 1, buf);
	return 0;
}

/*
 *函数名： serialport_calibrate
 *功能：云台原点校准
 *参数：无
 *返回：
 *	0：成功
 *	1：失败
 */
int serialport_calibrate(void)
{
	ssize_t ret;
	unsigned char buf[4];

	buf[0] = 0xFE;
	buf[1] = 0x00;
	buf[2] = 0x07;
	buf[3] = serialport_check_sum(buf, 3);
	ret = write(serialport_fd, buf, 4);
	if (ret == -1) {
		perror("motor calibrate failure !!!\n");
		return -1;
	}

	serialport_sendstop_count = 0;
	serialport_stop_flag = 0;
	buf[0] = 1;
	serialport_send_msg(MSG_SP_P_MOTORMOVE, NULL, 1, buf);
	return 0;
}

/*
 *函数名： serialport_query_position
 *功能：查询云台位置
 *参数：无
 *返回：
 *	0：成功
 *	1：失败
 */
int serialport_query_position(void)
{
	ssize_t ret;
	unsigned char buf[4];

	buf[0] = 0xFE;
	buf[1] = 0x00;
	buf[2] = 0x06;
	buf[3] = serialport_check_sum(buf, 3);
	ret = write(serialport_fd, buf, 4);
	if (ret == -1) {
		perror("serialport_cruise failure !!!\n");
		return -1;
	}
	return 0;
}

/*
 *函数名： serialport_move_abs_rel
 *功能：云台以相对或绝对位置移动
 *参数：
 *	pan：水平位置
 *	tilt：垂直位置
 *	type：相对或绝对位置标志
 *		2：相对位置
 *		3：绝对位置
 *返回：
 *	0：成功
 *	1：失败
 */
int serialport_move_abs_rel(short pan, short title, unsigned char type)
{
	ssize_t ret;
	char buf[8];

	buf[0] = 0xfe;
	buf[1] = 0x04;
	buf[2] = type;
	buf[3] = pan;
	buf[4] = pan >> 8;
	buf[5] = title;
	buf[6] = title >> 8;
	buf[7] = serialport_check_sum(buf, 7);
	ret = write(serialport_fd, buf, 8);
	if(ret == -1)
	{
		perror("move Control failure !!!\n");
		return -1;
	}

	serialport_sendstop_count = 0;
	serialport_stop_flag = 0;
	buf[0] = 1;
	serialport_send_msg(MSG_SP_P_MOTORMOVE, NULL, 1, buf);
	return 0;
}

/*
 *函数名： serialport_query_battery_vol
 *功能：查询电池电压
 *参数：无
 *返回：
 *	0：成功
 *	1：失败
 */
int serialport_query_battery_vol(void)
{
	ssize_t ret;
	char buf[8];

	buf[0] = 0xfe;
	buf[1] = 0x00;
	buf[2] = 0x10;
	buf[3] = serialport_check_sum(buf, 3);
	ret = write(serialport_fd, buf, 4);
	if (ret == -1) {
		perror("query battery voltage failed !!!\n");
		return -1;
	}

	return 0;
}
/*
 *函数名： serialport_query_charge
 *功能：查询电池是否充电
 *参数：无
 *返回：
 *	0：成功
 *	1：失败
 */
int serialport_query_charge(void)
{
	ssize_t ret;
	char buf[8];

	buf[0] = 0xfe;
	buf[1] = 0x00;
	buf[2] = 0x11;
	buf[3] = serialport_check_sum(buf, 3);
	ret = write(serialport_fd, buf, 4);
	if (ret == -1) {
		perror("query battery charge failed !!!\n");
		return -1;
	}

	return 0;
}
int serialport_sendAuthenticationFailure()
{
	return serialport_request(0x0c);
}
