/*
 * UPNPAdapter.h
 *
 *  Created on: Mar 21, 2013
 *      Author: root
 */

#ifndef UPNPADAPTER_H_
#define UPNPADAPTER_H_

#define UPNP_USLEEP_TIME	20000 //20ms

/*
 * 与子进程通信的结构体
 * */
typedef struct UPNP_msg{
	Child_Process_Msg *upnp_rev_process_msg;
	Child_Process_Msg *upnp_snd_process_msg;
	int upnp_rev_process_id;
	int upnp_snd_process_id;
}upnp_msg_st;

/*
 * 			控制流程的结构体
 *
 * flag_cmd:从子进程接收到消息，保存到该变量中。0:stop 1:start 2:again
 * flag_snd：控制收到命令只给子进程发送1次回复。0:no send 1:send
 * flag_success: 用于映射成功后,是否计数的标志.
 * count_fail：记录映射失败的次数，当大于3给子进程回复失败消息。
 * count_cmd：用于映射成功后，计数30min，再次映射。
 * 				30min = 1800 000ms
 * 				90000 = 1800 000 / 20000
 * 	map_port：存放子进程发送的映射端口号
 * */
typedef struct UPNP_flag{
	int flag_process;
	int flag_snd;
	int flag_success;
	int count_fail;
	int count_cmd;
	char map_port[8];
}upnp_flag_st;

//flag_process
enum{
	UPNP_STOP_CMD,
	UPNP_START_CMD,
};

//flag_success
enum{
	UPNP_DELAY_STOP,
	UPNP_DELAY_START,
};

#define T_30MIN_DELAY_COUNT		90000 //30min
//#define T_30MIN_DELAY_COUNT		1000

typedef void (*upnp_msg_fun)(upnp_msg_st *self);

void UpnpCommandFun(upnp_msg_st *msg, upnp_flag_st *upnp_flag);
int SetUpnp(char *map_port);

int upnp_send_msg(upnp_msg_st *msg);
int upnp_recv_msg(upnp_msg_st *msg);

#endif /* UPNPADAPTER_H_ */
