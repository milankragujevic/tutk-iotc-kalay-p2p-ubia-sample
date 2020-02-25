#include <stdio.h>
#include "common/utils.h"
#include "SerialPorts/SerialAdapter.h"
#include "SerialPorts.h"
#include "common/common.h"
#include "common/logfile.h"
#include "common/common_func.h"
#include "common/msg_queue.h"
#include "VideoAlarm/VideoAlarm.h"
#include "NewsChannel/NewsChannelAdapter.h"
#include "HTTPServer/httpsserver.h"

#define AngToStep 10
unsigned char serialportadapter_battery_data[4];

void serialportadapter_move_rel(msg_container *self);
void serialportadapter_move_abs(msg_container *self);
void serialportadapter_move_rel_speed(msg_container *self);
void serialportadapter_move_abs_speed(msg_container *self);
void serialportadapter_cruise(msg_container *self);
void serialportadapter_speed(msg_container *self);
void serialportadapter_calibrate(msg_container *self);
void serialportadapter_query_position(msg_container *self);
void serialportadapter_query_battery_vol(msg_container *self);
void serialportadapter_query_charge(msg_container *self);
void serialportadapter_send_position(msg_container *self);
void serialportadapter_send_battery(msg_container *self);
void serialportadapter_send_motorerror(msg_container *self);
void serialportadapter_send_motormove(msg_container *self);
void serialportadapter_battery_alarm(msg_container *self);
/*
 *函数名： serialports_cmd_table_fun
 *功能：判断串口模块从子进程收到的命令，调用相应的函数处理
 *参数：
 *	self：指向串口模块msg_container结构的指针
 *返回：无
 */
void serialports_cmd_table_fun(msg_container *self) {
	MsgData *msgrcv = (MsgData *)self->msgdata_rcv;

	switch(msgrcv->cmd)
	{
	case MSG_SP_T_MOVE_REL:
		serialportadapter_move_rel(self);
		break;
	case MSG_SP_T_MOVE_ABS:
		serialportadapter_move_abs(self);
		break;
	case MSG_SP_T_MOVE_REL_SPEED:
		serialportadapter_move_rel_speed(self);
		break;
	case MSG_SP_T_MOVE_ABS_SPEED:
		serialportadapter_move_abs_speed(self);
		break;
	case MSG_SP_T_CRUISE:
		serialportadapter_cruise(self);
		break;
	case MSG_SP_T_SPEED:
		serialportadapter_speed(self);
		break;
	case MSG_SP_T_CALIBRATE:
		serialportadapter_calibrate(self);
		break;
	case MSG_SP_T_QUERY_POSITION:
		serialportadapter_query_position(self);
		break;
	case MSG_SP_T_QUERY_BATTERY_VOL:
		serialportadapter_query_battery_vol(self);
		break;
	case MSG_SP_T_QUERY_CHARGE:
		serialportadapter_query_charge(self);
		break;
	case MSG_SP_T_BATTERY_ALARM:
		serialportadapter_battery_alarm(self);
		break;
	case MSG_SP_P_POSITION:
		serialportadapter_send_position(self);
		break;
	case MSG_SP_P_BATTERY:
		serialportadapter_send_battery(self);
		break;
	case MSG_SP_P_MOTORERROR:
		serialportadapter_send_motorerror(self);
		break;
	case MSG_SP_P_MOTORMOVE:

		serialportadapter_send_motormove(self);
		break;
	default:
		break;
	}
}
/*
 *函数名： serialportadapter_move_rel
 *功能：处理相对运动命令
 *参数：
 *	self：指向串口模块msg_container结构的指针
 *返回：无
 */
void serialportadapter_move_rel(msg_container *self)
{
	MsgData *msgrcv;
	int pan, tilt;


	printf("serial received move rel\n");
	if (serialport_mach_type == MACH_TYPE_M3S  && serialport_confirm_flag == 1) {
		msgrcv = (MsgData *) self->msgdata_rcv;

		pan = *(short *) (self->msgdata_rcv + sizeof(MsgData));
		tilt = *(short *) (self->msgdata_rcv + sizeof(MsgData) + 2);
		if (pan >= 180) {
			pan = 1920;
		} else if (pan < -180) {
			pan = -1920;
		} else {
			pan = pan * AngToStep;
		}
		tilt = tilt * AngToStep;

		serialport_move_abs_rel(pan, tilt, SERIALPORT_MOVE_REL);
	}
}

/*
 *函数名： serialportadapter_move_abs
 *功能：处理绝对运动命令
 *参数：
 *	self：指向串口模块msg_container结构的指针
 *返回：无
 */
void serialportadapter_move_abs(msg_container *self)
{
	MsgData *msgrcv;
	int pan, tilt;

	if (serialport_mach_type == MACH_TYPE_M3S  && serialport_confirm_flag == 1) {
		msgrcv = (MsgData *) self->msgdata_rcv;
		pan = *(short *) (self->msgdata_rcv + sizeof(MsgData));
		tilt = *(short *) (self->msgdata_rcv + sizeof(MsgData) + 2);

		if (pan >= 180) {
			pan = 1920;
		} else if (pan < -180) {
			pan = -1920;
		} else {
			pan = pan * AngToStep;
		}
		tilt = tilt * AngToStep;

		serialport_move_abs_rel(pan, tilt, SERIALPORT_MOVE_ABS);
	}
}

/*
 *函数名： serialportadapter_move_rel_speed
 *功能：处理以一定速度相对运动命令
 *参数：
 *	self：指向串口模块msg_container结构的指针
 *返回：无
 */
void serialportadapter_move_rel_speed(msg_container *self)
{
	MsgData *msgrcv;
	int pan, tilt;
	unsigned char speed;

	if (serialport_mach_type == MACH_TYPE_M3S  && serialport_confirm_flag == 1) {
		msgrcv = (MsgData *) self->msgdata_rcv;
		pan = *(short *) (self->msgdata_rcv + sizeof(MsgData));
		tilt = *(short *) (self->msgdata_rcv + sizeof(MsgData) + 3);
		speed = (self->msgdata_rcv)[sizeof(MsgData) + 2];
		printf("pan = %d, tilt = %d, speed= %d\n", pan, tilt, speed);
		if (pan >= 180) {
			pan = 1920;
		} else if (pan < -180) {
			pan = -1920;
		} else {
			pan = pan * AngToStep;
		}
		tilt = tilt * AngToStep;

		serialport_speed(speed);
		serialport_move_abs_rel(pan, tilt, SERIALPORT_MOVE_REL);
	}
}

/*
 *函数名： serialportadapter_move_abs_speed
 *功能：处理以一定速度绝对运动命令
 *参数：
 *	self：指向串口模块msg_container结构的指针
 *返回：无
 */
void serialportadapter_move_abs_speed(msg_container *self)
{
	MsgData *msgrcv;
	int pan, tilt;
	unsigned char speed;

	if (serialport_mach_type == MACH_TYPE_M3S  && serialport_confirm_flag == 1) {
		msgrcv = (MsgData *) self->msgdata_rcv;
		pan = *(short *) (self->msgdata_rcv + sizeof(MsgData));
		tilt = *(short *) (self->msgdata_rcv + sizeof(MsgData) + 2);
		speed = *(unsigned char *) (self->msgdata_rcv + sizeof(MsgData) + 4);
		if (pan >= 180) {
			pan = 1920;
		} else if (pan < -180) {
			pan = -1920;
		} else {
			pan = pan * AngToStep;
		}
		tilt = tilt * AngToStep;
		serialport_speed(speed);
		serialport_move_abs_rel(pan, tilt, SERIALPORT_MOVE_ABS);
	}
}

/*
 *函数名： serialportadapter_cruise
 *功能：处理巡航命令
 *参数：
 *	self：指向串口模块msg_container结构的指针
 *返回：无
 */
void serialportadapter_cruise(msg_container *self)
{
	MsgData *msgrcv;
	unsigned char time, type;

	if (serialport_mach_type == MACH_TYPE_M3S && serialport_confirm_flag == 1) {
		msgrcv = (MsgData *) self->msgdata_rcv;
		time = (self->msgdata_rcv)[sizeof(MsgData)];
		type = (self->msgdata_rcv)[sizeof(MsgData) + 1];

		serialport_cruise(time, type);
	}
}

/*
 *函数名： serialportadapter_speed
 *功能：处理设置速度命令
 *参数：
 *	self：指向串口模块msg_container结构的指针
 *返回：无
 */
void serialportadapter_speed(msg_container *self)
{
	MsgData *msgrcv;

	if (serialport_mach_type == MACH_TYPE_M3S  && serialport_confirm_flag == 1) {
		msgrcv = (MsgData *) self->msgdata_rcv;
		serialport_speed((self->msgdata_rcv)[sizeof(MsgData)]);
	}
}

/*函数名： serialportadapter_calibrate
*功能：处理原点校准命令
*参数：
*	self：指向串口模块msg_container结构的指针
*返回：无
*/
void serialportadapter_calibrate(msg_container *self)
{
	if (serialport_mach_type == MACH_TYPE_M3S  && serialport_confirm_flag == 1) {
		serialport_calibrate();
	}
}
/* 函数名：serialportadapter_battery_alarm
 * 功能：打开和关闭电池报警
 * 参数：
	self：指向串口模块msg_container结构的指针
	字节1：报警打开关闭
	  1：开启
	  2：关闭
 * 返回值：无
 */
void serialportadapter_battery_alarm(msg_container *self)
{
	unsigned char data;

	data = (self->msgdata_rcv)[sizeof(MsgData)];
	switch(data)
	{
	case 1:
		serialport_battery_alarm = 1;
		break;
	case 2:
		serialport_battery_alarm = 0;
		break;
	default:
		break;
	}
}
/*函数名： serialportadapter_query_position
*功能：处理查询云台位置命令
*参数：
*	self：指向串口模块msg_container结构的指针
*返回：无
*/
void serialportadapter_query_position(msg_container *self)
{
	if (serialport_mach_type == MACH_TYPE_M3S  && serialport_confirm_flag == 1) {
	serialport_query_position();
	}
}

/*函数名： serialportadapter_query_battery_vol
*功能：处理查询电池电压命令
*参数：
*	self：指向串口模块msg_container结构的指针
*返回：无
*/
void serialportadapter_query_battery_vol(msg_container *self)
{
	if (serialport_mach_type == MACH_TYPE_M2  && serialport_confirm_flag == 1) {
	serialport_query_battery_vol();
	}
}

/*函数名： serialportadapter_query_charge
*功能：处理查询电池是否充电命令
*参数：
*	self：指向串口模块msg_container结构的指针
*返回：无
*/
void serialportadapter_query_charge(msg_container *self)
{
	if (serialport_mach_type == MACH_TYPE_M2  && serialport_confirm_flag == 1) {
		serialport_query_charge();
	}
}

/*函数名： serialportadapter_send_position
*功能：处理串口模块发给子进程的查询云台位置的命令
*参数：
*	self：指向串口模块msg_container结构的指针
*返回：无
*/
void serialportadapter_send_position(msg_container *self)
{
	short pan, tilt;

	pan = *(short *) (self->msgdata_rcv + sizeof(MsgData));
	tilt = *(short *) (self->msgdata_rcv + sizeof(MsgData) + 2);
	printf("child process:position return pan=%d, tilt=%d\n", pan, tilt);
}
/*函数名： serialportadapter_send_position
*功能：处理串口模块发给子进程的电池状态的命令，发给IO线程控制灯颜色
*    正在充电：黄
*    充满电：绿
*    无充电且低电：红
*    无充电非低电：绿
*参数：
*	self：指向串口模块msg_container结构的指针
*返回：无
*/
void serialportadapter_send_battery(msg_container *self)
{
	MsgData *msgsnd;
	if(serialportadapter_battery_data[0] != self->msgdata_rcv[sizeof(MsgData)]
		|| serialportadapter_battery_data[1] != self->msgdata_rcv[sizeof(MsgData) + 1])
	{
		msgsnd = (MsgData *)self->msgdata_snd;
		msgsnd->type = HTTP_SERVER_MSG_TYPE;
		msgsnd->len = 6;
		if(self->msgdata_rcv[sizeof(MsgData)] == 0) {
			msgsnd->sign[0] = 2;
		}
		else {
			msgsnd->sign[0] = 1;
		}
		msgsnd->sign[1] = self->msgdata_rcv[sizeof(MsgData) + 1] + 1;
		msgsnd->sign[2] = self->msgdata_rcv[sizeof(MsgData) + 2];
		msgsnd->sign[3] = 0;


		energyStatus_BATTERY = msgsnd->sign[0];

		chargeStatus_BATTERY = msgsnd->sign[1];


		DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
		DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++        battery   %d    %d         +++++++++++++++++\n",energyStatus_BATTERY,chargeStatus_BATTERY);
		DEBUGPRINT(DEBUG_INFO, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
		static unsigned int count_a[3][3] = {
				{0, 0, 0},
				{0, 0, 0},
				{0, 0, 0},
		};
		char a0 = self->msgdata_rcv[sizeof(MsgData)];
		char a1 = self->msgdata_rcv[sizeof(MsgData) + 1];
		if(a0 >= 0 && a0 < 3 && a1 >= 0 && a1 < 3) {
			count_a[a0][a1] += 1;
			FILE *file_a = fopen("/usr/share/batteryAlarm", "w");
			if(NULL != file_a) {
				fprintf(file_a, "%d, %d, %d\n", count_a[0][0], count_a[0][1], count_a[0][2]);
				fprintf(file_a, "%d, %d, %d\n", count_a[1][0], count_a[1][1], count_a[1][2]);
				fprintf(file_a, "%d, %d, %d\n", count_a[2][0], count_a[2][1], count_a[2][2]);
				fclose(file_a);
			}
		}
		msgsnd->cmd = MSG_HTTPSPOWER_T_STATE;
#ifdef _ENABLE_BATTERY_UPLOAD
		if(msgsnd->sign[1] != 2) {
			msg_queue_snd(self->rmsgid, self->msgdata_snd, sizeof(MsgData) - sizeof(long) + msgsnd->len);
		}
#endif
		log_printf("battery alarm a0:%d a1:%d\n", a0, a1);
	}
	serialportadapter_battery_data[0] = self->msgdata_rcv[sizeof(MsgData)];
	serialportadapter_battery_data[1] = self->msgdata_rcv[sizeof(MsgData) + 1];
	serialportadapter_battery_data[2] = self->msgdata_rcv[sizeof(MsgData) + 2];
	serialportadapter_battery_data[3] = 0;

	/* 控制灯的颜色, 报警*/
	switch(serialportadapter_battery_data[1]) {
	case 0:
		if(serialportadapter_battery_data[0] == 1) {

			battary_power_state_report(BATTERY_POWER_STATE_LOW);
			if(serialport_battery_alarm == 1) {
				msgsnd = (MsgData *)self->msgdata_snd;
				msgsnd->type = NEWS_CHANNEL_MSG_TYPE;
				msgsnd->len = 6;
				msgsnd->sign[0] = 0;
				msgsnd->sign[1] = 0;
				msgsnd->sign[2] = 0;
				msgsnd->sign[3] = 0;
				msgsnd->cmd = MSG_NEWS_CHANNEL_T_NEWS_BROADCAST;
				*(int *)(self->msgdata_snd + sizeof(MsgData)) = 0;
				*(unsigned char *)(self->msgdata_snd + sizeof(MsgData) + 4) = 3;
				*(unsigned char *)(self->msgdata_snd + sizeof(MsgData) + 5) = 1;
				msg_queue_snd(self->rmsgid, self->msgdata_snd, sizeof(MsgData) - sizeof(long) + msgsnd->len);
			}
		}
		else {
			battary_power_state_report(BATTERY_POWER_STATE_NORMAL);
		}
		break;
	case 1:
		battary_power_state_report(BATTERY_POWER_STATE_CHARGE);
		break;
	case 2:
		battary_power_state_report(BATTERY_POWER_STATE_NORMAL);
		break;
	default:
		break;
	}


}

/*函数名： serialportadapter_send_motorerror
*功能：处理串口模块发给子进程的云台错误的命令
*参数：
*	self：指向串口模块msg_container结构的指针
*返回：无
*/
void serialportadapter_send_motorerror(msg_container *self)
{

	printf("child process:send motor error return\n:");

}
/*函数名： serialportadapter_send_motorstop
*功能：处理串口模块发给子进程的电机停止的命令
*参数：
*	self：指向串口模块msg_container结构的指针
*返回：无
*/
void serialportadapter_send_motormove(msg_container *self)
{
	MsgData *msgsnd;


//	msgrcv = (MsgData *)self->msgdata_rcv;
	msgsnd = (MsgData *)self->msgdata_snd;
	msgsnd->type = VIDEO_ALARM_MSG_TYPE;
	msgsnd->len = 4;
	msgsnd->sign[0] = 0;
	msgsnd->sign[1] = 0;
	msgsnd->sign[2] = 0;
	msgsnd->sign[3] = 0;
	msgsnd->cmd = MSG_VA_T_MOTOR;

	switch (self->msgdata_rcv[sizeof(MsgData)])
	{
	case 0:
		*(int *)(self->msgdata_snd + sizeof(MsgData)) = 0;
		msg_queue_snd(self->rmsgid, self->msgdata_snd, sizeof(MsgData) - sizeof(long) + msgsnd->len);
		break;
	case 1:
		*(int *)(self->msgdata_snd + sizeof(MsgData)) = 1;
		msg_queue_snd(self->rmsgid, self->msgdata_snd, sizeof(MsgData) - sizeof(long) + msgsnd->len);
		break;
	default:
		break;
	}

}

