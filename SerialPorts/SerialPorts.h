/*
 * SerialPorts.h
 *
 *  Created on: Mar 13, 2013
 *      Author: root
 */

#ifndef SERIALPORTS_H_
#define SERIALPORTS_H_

#define MACH_TYPE_NONE 0
#define MACH_TYPE_M2 1
#define MACH_TYPE_M3S 2

#define SERIALPORT_MOVE_ABS 0x03
#define SERIALPORT_MOVE_REL 0x02

extern int serial_port_fd;
extern volatile int serialport_confirm_flag;
extern volatile int serialport_stop_flag;
extern volatile int serialport_mach_type;
extern int serialport_battery_alarm;
extern int serialport_open(const char *name);
extern int serialport_confirm(void);
extern int serialport_foctory_mode(int mode);
extern int serialport_user_mode(void);
extern int serialport_speed(unsigned char speed);
extern int serialport_cruise(unsigned char time,char type);
extern int serialport_calibrate(void);
extern int serialport_query_position(void);
extern int serialport_move_abs_rel(short pan, short title, unsigned char type);
extern int serialport_query_battery_vol(void);
extern int serialport_query_charge(void);
extern unsigned int serialport_get_battery_state(void);
extern int serialport_sendAuthenticationFailure();
extern int SerialPorts_Thread_Creat();
#endif /* SERIALPORTS_H_ */

