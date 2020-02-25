/*
 * ToolingAdapter.h
 *
 *  Created on: Mar 14, 2013
 *      Author: root
 */

#ifndef SERIAL_ADAPTER_H_
#define SERIAL_ADAPTER_H_
#include "common/utils.h"



/*msg commmad 宏定义*/
enum {
	MSG_SP_T_MOVE_REL = 35,
	MSG_SP_T_MOVE_ABS,
	MSG_SP_T_MOVE_REL_SPEED,
	MSG_SP_T_MOVE_ABS_SPEED,
	MSG_SP_T_CRUISE,
	MSG_SP_T_SPEED,
	MSG_SP_T_CALIBRATE,
	MSG_SP_T_QUERY_POSITION,
	MSG_SP_T_QUERY_BATTERY_VOL,
	MSG_SP_T_QUERY_CHARGE,
	MSG_SP_T_BATTERY_ALARM,
	MSG_SP_P_POSITION,
	MSG_SP_P_BATTERY,
	MSG_SP_P_MOTORERROR,
	MSG_SP_P_MOTORMOVE
};
extern void serialports_cmd_table_fun(msg_container *self);
unsigned char serialportadapter_battery_data[4];
#endif /* SERIAL_ADAPTER_H_ */

