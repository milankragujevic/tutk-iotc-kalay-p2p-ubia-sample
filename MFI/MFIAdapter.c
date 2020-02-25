/** @file [MFIAdapter.c]
 *  @brief MFI线程的外部交互C文件
 *	@author: Summer.Wang
 *	@data: 2013-9-24
 *	@modify record: 即日创建了这套文件注释系统
 */

#include "common/utils.h"
#include "MFIAdapter.h"
#include "common/common.h"
#include "common/common_func.h"
#include "common/msg_queue.h"
#include <string.h>
#include <stdio.h>
#include "MFI.h"

/**
 * @brief 向外交互的消息函数
 * @author Summer.Wang
 * @return Null
 * @param[out] <self><控制及查询模块属性>
 * @note 获取加密信息失败后，调用这个函数
 */
void MFI_achieve_fucking_agreement_for_HTTPS(msg_container *self)
{
	MsgData *msgsnd = (MsgData *)self->msgdata_snd;
	char *data = (char *)(msgsnd+1);

	msgsnd->len = 1;
	msgsnd->type = MFI_MSG_TYPE;
	msgsnd->cmd = 6;
	*data = 3;
	if(msg_queue_snd(self->rmsgid, self->msgdata_snd, sizeof(struct MsgData)-sizeof(long int) + 1) == -1)
	{
		return;
	}
}

/**
 * @brief 向外交互的消息函数
 * @author Summer.Wang
 * @return Null
 * @param[out] <self><控制及查询模块属性>
 * @note 发送网络设置
 */
void MFI_process_send_SSID(msg_container *self)
{

	printf("SSID receive\n");
	send_message_to_set_net((netset_info_t *)(((MsgData *)self->msgdata_rcv)+1));
}

/**
 * @brief 向外交互的消息函数
 * @author Summer.Wang
 * @return Null
 * @param[out] <self><控制及查询模块属性>
 * @note 发送给IO线程停止ioctl
 */
void MFI_process_suspend_ioctl(msg_container *self)
{
	char acBuf[256] = {0};
    int lThreadMsgId =-1;
	MsgData *pstMsgSnd = (MsgData *)acBuf;
	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->type = IO_CTL_MSG_TYPE;
	pstMsgSnd->cmd = 8;
	pstMsgSnd->len = 0;
	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
}

/**
 * @brief 向外交互的消息函数
 * @author Summer.Wang
 * @return Null
 * @param[out] <self><控制及查询模块属性>
 * @note 发送给IO线程开始ioctl
 */
void MFI_process_start_ioctl(msg_container *self)
{
	char acBuf[256] = {0};
    int lThreadMsgId =-1;

	MsgData *pstMsgSnd = (MsgData *)acBuf;
	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->type = IO_CTL_MSG_TYPE;
	pstMsgSnd->cmd = 9;
	pstMsgSnd->len = 0;
	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
}

/**
 * @brief 向外交互的消息函数
 * @author Summer.Wang
 * @return Null
 * @param[out] <self><控制及查询模块属性>
 * @note 通知IO线程获取CP
 */
void MFI_process_cp(msg_container *self)
{
	char acBuf[256] = {0};
    int lThreadMsgId =-1;
    MsgData *msgrcv = (MsgData *)self->msgdata_rcv;
	MsgData *pstMsgSnd = (MsgData *)acBuf;

	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->type = IO_CTL_MSG_TYPE;
	pstMsgSnd->cmd = 5;
	pstMsgSnd->len = 21;
	*(char *)(pstMsgSnd + 1) = 2;
	memcpy((char *)(pstMsgSnd + 1)+1, (char *)(msgrcv + 1), msgrcv->len);
	printf("send to ioctl MSG_IOCTL_T_I2C get cp\n");
	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
}

/**
 * @brief 向外交互的消息函数
 * @author Summer.Wang
 * @return Null
 * @param[out] <self><控制及查询模块属性>
 * @note 通知IO线程切换到camera
 */
void MFI_process_inform_usb_switch_to_cam(msg_container *self)
{
	char acBuf[256] = {0};
    int lThreadMsgId =-1;
    MsgData *msgrcv = (MsgData *)self->msgdata_rcv;

	MsgData *pstMsgSnd = (MsgData *)acBuf;
	//printf("please give me deviceID\n");
	//printf("type = %d \n", msgrcv->type);
	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->type = IO_CTL_MSG_TYPE;
	pstMsgSnd->cmd = 1;
	pstMsgSnd->len = 1;
	*(char *)(pstMsgSnd + 1) = 1;
	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
}

/**
 * @brief 向外交互的消息函数
 * @author Summer.Wang
 * @return Null
 * @param[out] <self><控制及查询模块属性>
 * @note 通知IO线程iic
 */
void MFI_process_test(msg_container *self)
{
    char acBuf[256] = {0};
    int lThreadMsgId =-1;
	MsgData *msgrcv = (MsgData *)self->msgdata_rcv;

    MsgData *pstMsgSnd = (MsgData *)acBuf;

	get_process_msg_queue_id(&lThreadMsgId,  NULL);
	pstMsgSnd->type = IO_CTL_MSG_TYPE;
	pstMsgSnd->cmd = 5;
	pstMsgSnd->len = 1;
	*(char *)(pstMsgSnd + 1) = 1;

	msg_queue_snd(lThreadMsgId, pstMsgSnd, sizeof(MsgData) - sizeof(long) + pstMsgSnd->len);
}

/**
 * @brief 向外交互的消息函数
 * @author Summer.Wang
 * @return Null
 * @param[out] <self><控制及查询模块属性>
 * @note 预留接口，通知HTTPS线程没有R'
 */
void MFI_process_inform_https_no_r(msg_container *self){
	MsgData *msgsnd = (MsgData *)self->msgdata_snd;
//	HttpsSeverVerifycam(self);
}

/** 协议实现列表 */
cmd_item MFI_cmd_table[] = {
		{MSG_MFI_P_TEST_ALIVE,                       MFI_process_test},
		{MSG_MFI_P_GET_CP,                           MFI_process_cp},
		{MSG_MFI_P_INFORM_SWITCH_USB_TO_CAM,         MFI_process_inform_usb_switch_to_cam},
		{MSG_MFI_P_NO_R,							 MFI_process_inform_https_no_r},
		{MSG_MFI_P_SEND_SSID,                        MFI_process_send_SSID},
		{MSG_MFI_P_SUSPEND_IOCTL,                    MFI_process_suspend_ioctl},
		{MSG_MFI_P_START_IOCTL,                      MFI_process_start_ioctl},
};//"T":from process,used by thread;"P":from thread,used by process.

/**
 * @brief 协议查询入口
 * @author Summer.Wang
 * ＠param[in|out] <self><适配器运行环境>
 * @return Null
 */
void MFI_cmd_table_fun(msg_container *self) {
	MsgData *msgrcv = (MsgData *)self->msgdata_rcv;
	cmd_item icmd;
	icmd.cmd = msgrcv->cmd;
	msg_fun fun = utils_cmd_bsearch(&icmd, MFI_cmd_table, sizeof(MFI_cmd_table)/sizeof(cmd_item));
	if(fun != NULL) {
		fun(self);
	}
}

